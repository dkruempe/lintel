FROM ubuntu:24.04 AS builder

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    make \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install conan --break-system-packages --no-cache-dir

WORKDIR /workspace
COPY . .

RUN conan profile detect --force
RUN conan install . --output-folder=build --build=missing -s build_type=Release
RUN cmake -S . -B build/build/Release \
    -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build/build/Release --parallel $(nproc)

FROM builder AS test
WORKDIR /workspace/build/build/Release
CMD ["ctest", "--output-on-failure"]

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
COPY --from=builder /workspace/build/build/Release ./build

CMD ["./bin/main"]
