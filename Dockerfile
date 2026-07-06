# Build stage
FROM gcc:latest AS builder

RUN apt-get update && apt-get install -y cmake

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Final stage
FROM debian:stable-slim

WORKDIR /app
COPY --from=builder /app/build/loadbalancer .
COPY --from=builder /app/config ./config

# Default entrypoint
ENTRYPOINT ["./loadbalancer"]
CMD ["simulate", "config/config.json"]
