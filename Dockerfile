# syntax=docker/dockerfile:1
# Build and runtime based on Debian Trixie
FROM debian:trixie

ENV DEBIAN_FRONTEND=noninteractive

# Install build and runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential cmake pkg-config git \
    libprotobuf-dev protobuf-compiler \
    libabsl-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libgumbo-dev \
    libpqxx-dev libpq-dev \
    libssl-dev \
    libmodbus-dev \
    libcpprest-dev \
    prometheus-cpp-dev \
    libhueplusplus-dev \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for safety
RUN useradd -m -u 10001 appuser

# Copy source and build wastlernet
WORKDIR /app
COPY . /app

# Build only the wastlernet target in Release mode
RUN cmake -S /app -B /app/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /app/build --target wastlernet -j \
    && install -D /app/build/wastlernet /usr/local/bin/wastlernet

# Default configuration mount point
VOLUME ["/config"]
ENV CONFIG_PATH=/config/wastlernet.textpb

# Networking: allow using host.docker.internal on Linux via --add-host at run time
# EXPOSE is optional; the app exposes Prometheus metrics at 32154 and REST is set in config
EXPOSE 32154

# Entrypoint script
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh \
    && chown appuser:appuser /usr/local/bin/entrypoint.sh /usr/local/bin/wastlernet

USER appuser

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD [""]
