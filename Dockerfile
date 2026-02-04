# Base image with build tools
FROM ubuntu:24.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
# We include libpq-dev (Postgres) and libcurl4-openssl-dev/libssl-dev (for S3 SDK later)
# along with our current needs (OpenCV, FFmpeg, SQLite)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libopencv-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libsqlite3-dev \
    libpq-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Work directory
WORKDIR /app

# Copy source code
COPY . .

# Build
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Download Model (if not present, though we should copy it)
# We assume the model is copied with the source or mounted
# COPY yolov8n.onnx .

# Entrypoint
# We use a wrapper script or run directly. 
# For now, we assume standard usage.
CMD ["./build/rtsp_pipeline"]
