# ---- Stage 1: Build C++ shared library ----
FROM ubuntu:22.04 AS cpp-builder

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build/cpp
COPY cpp/ .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# ---- Stage 2: Build Go binary ----
FROM golang:1.21 AS go-builder

# Install C++ runtime for CGO
RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy C++ headers and built library from stage 1
COPY --from=cpp-builder /build/cpp/src/ ./cpp/src/
COPY --from=cpp-builder /build/cpp/build/libsquid_solver.so ./cpp/build/

# Copy Go source
COPY go/ ./go/

# Build Go binary with CGO
WORKDIR /build/go
ENV CGO_ENABLED=1
ENV CGO_CFLAGS="-I/build/cpp/src"
ENV CGO_LDFLAGS="-L/build/cpp/build -lsquid_solver"

RUN go mod tidy && \
    go build -o /build/server .

# ---- Stage 3: Runtime ----
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary
COPY --from=go-builder /build/server .

# Copy C++ shared library
COPY --from=cpp-builder /build/cpp/build/libsquid_solver.so ./lib/

# Copy frontend
COPY client/ ./client/

# Set library path so the binary can find the .so at runtime
ENV LD_LIBRARY_PATH=/app/lib

# Railway sets PORT automatically
ENV PORT=8080
EXPOSE 8080

CMD ["./server"]
