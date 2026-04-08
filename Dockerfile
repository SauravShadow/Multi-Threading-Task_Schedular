# =============================================================================
# Stage 1: BUILD — compile the C++ binary using GCC + CMake
# =============================================================================
FROM debian:bookworm-slim AS builder

# Install build tools: GCC (C++20), CMake, Git (needed by FetchContent)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy all source files
COPY . .

# Configure and build in Release mode
# FetchContent will download httplib and nlohmann_json automatically
RUN mkdir -p build && \
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release -j$(nproc)

# =============================================================================
# Stage 2: RUNTIME — slim image with just the binary + frontend
# =============================================================================
FROM debian:bookworm-slim AS runtime

WORKDIR /app

# Copy only the compiled binary from the builder stage
COPY --from=builder /app/build/task_schedular ./task_schedular

# Copy the frontend static files (served by the C++ server at ./frontend/public)
COPY --from=builder /app/frontend ./frontend

# Port the C++ server listens on
EXPOSE 8081

# Run the binary from /app so that ./frontend/public resolves correctly
CMD ["./task_schedular"]
