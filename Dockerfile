# --- Build Phase: C++ ---
FROM gcc:latest AS cpp-builder
RUN apt-get update && apt-get install -y cmake
WORKDIR /app
COPY . .
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# --- Runtime Phase: Node.js + C++ Binary ---
FROM node:18-slim

# Install minimal libraries for C++ execution
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary and config
COPY --from=cpp-builder /app/build/loadbalancer .
COPY --from=cpp-builder /app/config ./config

# Copy web files and install dependencies
COPY web-demo ./web-demo
WORKDIR /app/web-demo
RUN npm install

# Expose the web port
EXPOSE 3000

# Start the web wrapper
CMD ["npm", "start"]
