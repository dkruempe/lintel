FROM ubuntu:24.04 AS base

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    clang-tidy-18 \
    cmake \
    ninja-build \
    ccache \
    make \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    && ln -s /usr/bin/clang-tidy-18 /usr/bin/clang-tidy \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install conan --break-system-packages --no-cache-dir

WORKDIR /workspace

# ── Dependency layer (invalidated only when conanfile.txt changes) ────
FROM base AS deps

COPY conanfile.txt .
RUN conan profile detect --force \
    && conan install . \
       --output-folder=build \
       --build=missing \
       -s build_type=Release \
       -c tools.cmake.cmaketoolchain:generator=Ninja

# ── Build layer ──────────────────────────────────────────────────────
FROM deps AS builder

COPY . .
RUN cmake -S . -B build/build/Release \
    -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -G Ninja \
    && cmake --build build/build/Release --parallel $(nproc)

# ── Test layer ───────────────────────────────────────────────────────
FROM builder AS test
WORKDIR /workspace/build/build/Release
CMD ["ctest", "--output-on-failure"]

# ── Runtime layer (minimal image) ────────────────────────────────────
FROM ubuntu:24.04 AS runtime

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libpq5 \
    libsqlite3-0 \
    libssl3t64 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY --from=builder /workspace/bin ./bin
COPY --from=builder /workspace/cfg ./cfg

CMD ["./bin/main"]
