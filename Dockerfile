# ── Stage 1: Build ──────────────────────────────────────────────────────────
# gcc 13.4.0 — multi-arch index digest (linux/amd64 + linux/arm64).
FROM gcc:13@sha256:3617a214e52a25bde5375dc9503b5e67f01b6c7322a30137e2790aa8e6db5d1f AS builder

RUN apt-get update && apt-get install -y --no-install-recommends cmake && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy build system files first for layer caching
COPY CMakeLists.txt ./
COPY cmake/ cmake/

# Copy source and headers
COPY include/ include/
COPY src/ src/
COPY examples/ examples/
COPY tests/ tests/

RUN cmake -B out -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF -DENABLE_RENDERING=OFF && \
    cmake --build out --parallel "$(nproc)"

# ── Stage 2: Runtime ───────────────────────────────────────────────────────
# distroless cc-debian12 — multi-arch index digest.
FROM gcr.io/distroless/cc-debian12@sha256:e5d81ddde149641e2a9ba55be4545bc125c67de07508b03ba4c22e6eb0ded5aa

LABEL maintainer="maintainer@example.com" \
      version="1.0.0" \
      description="C comprehensive template — example binaries"

# Only ship the CLI demo — echo/event-loop servers bind sockets and should not
# be the default image contents.
COPY --from=builder /build/out/examples/example_cli /usr/local/bin/

USER nonroot:nonroot

ENTRYPOINT ["example_cli"]
