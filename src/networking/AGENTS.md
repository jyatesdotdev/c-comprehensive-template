# networking — sockets and event loop (`src/networking/`)

TCP, UDP, Unix domain sockets, and a poll(2) event loop. Links `core`.
POSIX only (Linux/macOS); a Windows port would wrap Winsock. Public socket
types are transparent one-field structs (`{ int fd; }`) so demos and the
event loop can interoperate on raw fds.

## Shared plumbing

TCP and Unix stream sockets have identical send/recv/accept semantics, so
both wrap the fd-level helpers declared in `socket_internal.h` (defined in
socket.c): `nw_send_all`, `nw_recv`, `nw_accept_intr`,
`nw_disable_sigpipe`. Add stream-level behavior there once, not per family.

## Socket rules (each encodes a real bug class)

- **Partial sends are normal**: `send` may write fewer bytes than asked;
  `nw_send_all` loops until done. Never call bare `send` and assume.
- **EINTR everywhere**: accept/send/recv/sendto/recvfrom all retry on
  EINTR. `recv` returning 0 is clean EOF — surfaced as ERR_OK with
  `*out_len == 0`, distinct from ERR_IO.
- **SIGPIPE kills processes**: writes to a closed peer are guarded by
  `MSG_NOSIGNAL` on Linux and `SO_NOSIGPIPE` on macOS (both, via the
  helper). Any new write path must go through it.
- `getaddrinfo` results are always `freeaddrinfo`'d; every early return
  between `socket()` and success closes the fd. Port 0 + `*_local_port`
  is the pattern for test-friendly ephemeral binding.
- Unix socket paths must fit `sun_path` (checked, ~104 bytes); listeners
  unlink stale socket files before bind and owners unlink after shutdown.

## event_loop.c

Single-threaded readiness dispatch. The invariants:

- Callbacks may add/remove registrations **during dispatch** — removal
  tombstones the slot (`fd = -1`) and compaction runs after the dispatch
  walk; additions only participate from the next poll. Preserve this or
  callbacks corrupt the iteration.
- POLLHUP/POLLERR are delivered as `EV_READ` so owners observe EOF/errors
  via their normal read path.
- The loop never closes fds — registration is borrowing, callers own
  lifetime.

Two server demos share one protocol on purpose: `echo_server_demo.c`
(thread-pool-per-connection) vs `evloop_server_demo.c` (one thread,
multiplexed). Keep them behaviorally identical — the comparison is the
lesson. Tests must stay loopback/local-only (CI has no external network).
