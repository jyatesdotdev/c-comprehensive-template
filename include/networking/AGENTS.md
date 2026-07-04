# networking public API (`include/networking/`)

POSIX sockets and a poll event loop. General header rules:
`include/AGENTS.md`; EINTR/SIGPIPE/partial-send mechanics:
`src/networking/AGENTS.md`.

## Consumer contract

- Socket structs are transparent `{ int fd; }` handles — fd access is
  legitimate (the event loop uses it), but create/close only through the
  API so SIGPIPE suppression and state stay correct. `fd == -1` means
  closed/invalid.
- **Streams (TCP, Unix)**: `*_send_all` either sends everything or fails —
  no partial-success handling needed. `*_recv` with `*out_len == 0` and
  `ERR_OK` is clean EOF; loop until you have the bytes you expect (one
  recv ≠ one message — TCP has no message boundaries).
- **Testable servers**: listen on port 0, then `tcp_local_port` for the
  real port. Unix socket owners `unlink` the path after shutdown.
- `udp.h` — datagrams: one send = one message, but unreliable and
  unordered; oversized datagrams truncate on receive. `udp_recv_from`
  reports the sender as a `UdpEndpoint` you can pass straight back to
  `udp_send_to` to reply.
- `event_loop.h` — single-threaded: callbacks run on the loop thread and
  must not block (no sleeps, no blocking recv on other fds). Registration
  borrows the fd — the loop never closes it; on EOF your callback removes
  the fd and closes it itself. `EV_READ` also signals HUP/ERR — read to
  observe. Adds/removes from inside callbacks are safe.

Two concurrency models are demonstrated side by side: thread-per-connection
(`echo_server_demo.c`) vs event loop (`evloop_server_demo.c`) — pick one,
don't mix both on the same fd.
