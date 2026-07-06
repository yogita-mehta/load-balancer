# --- Build Phase ---
FROM node:20-bookworm AS builder

# Install C++ build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libstdc++6

WORKDIR /app
COPY . .

# Build the C++ binary
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# --- Runtime Phase ---
FROM node:20-bookworm

WORKDIR /app

# Copy the compiled binary and config from builder
# They are guaranteed to match because they used the same base image
COPY --from=builder /app/build/loadbalancer .
COPY --from=builder /app/config ./config

# Setup web demo
COPY web-demo ./web-demo
WORKDIR /app/web-demo
RUN npm install

EXPOSE 3000

CMD ["npm", "start"]
