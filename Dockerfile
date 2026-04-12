# ── Stage 1: Build ──────────────────────────────────────────────────────────
FROM gcc:13 AS builder

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
FROM gcr.io/distroless/cc-debian12

LABEL maintainer="maintainer@example.com" \
      version="1.0.0" \
      description="C comprehensive template — example binaries"

COPY --from=builder /build/out/examples/example_* /usr/local/bin/

USER nonroot:nonroot

ENTRYPOINT ["example_cli"]
