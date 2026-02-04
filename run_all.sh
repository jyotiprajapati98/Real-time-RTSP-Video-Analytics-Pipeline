#!/bin/bash

# 1. Setup Python Environment for Web UI
echo "--- Setting up Web API ---"
cd web
if [ ! -d "venv" ]; then
    python3 -m venv venv
fi
source venv/bin/activate
pip install fastapi uvicorn asyncpg jinja2
cd ..

# 2. Start PostgreSQL with Docker
echo "--- Starting PostgreSQL (Docker) ---"

# Check if postgres container already exists
if sudo docker ps -a --format '{{.Names}}' | grep -q '^postgres$'; then
    echo "PostgreSQL container exists. Starting it..."
    sudo docker start postgres
else
    echo "Creating new PostgreSQL container..."
    sudo docker run -d \
        --name postgres \
        -p 5432:5432 \
        -e POSTGRES_USER=admin \
        -e POSTGRES_PASSWORD=password \
        -e POSTGRES_DB=analytics_db \
        postgres:15
fi

echo "Waiting for PostgreSQL to be ready..."
sleep 5

# Ensure directories exist
mkdir -p detected_frames hls_output

# 3. Start Web API in background
echo "--- Starting Web API (Background) ---"
source web/venv/bin/activate
python3 web/api.py > web_log.txt 2>&1 &
WEB_PID=$!
echo "Web API started on http://localhost:9090 (PID: $WEB_PID)"

# 4. Start C++ Pipeline
echo "--- Starting Pipeline ---"
echo "Stream: rtsp://127.0.0.1:8554/laptop"
echo "Press Ctrl+C to stop everything."

./build/rtsp_pipeline rtsp://127.0.0.1:8554/laptop yolov8n.onnx

# Cleanup on exit
kill $WEB_PID
