# Analytics Pipeline - RTSP Video Analytics System

A high-performance C++ video analytics pipeline that processes RTSP streams in real-time, performs object detection using YOLOv8, generates HLS streams, and provides a web interface for monitoring detections.

## 🚀 Features

- **Real-time RTSP Stream Processing**: Ingest and process live RTSP video streams
- **YOLOv8 Object Detection**: Parallel object detection with configurable confidence thresholds
- **HLS Stream Generation**: Generate HTTP Live Streaming (HLS) output for web playback
- **PostgreSQL Database**: Store detection metadata with timestamps and confidence scores
- **Web Dashboard**: FastAPI-based web interface to view live streams and detection history
- **Multi-threaded Architecture**: Separate threads for streaming, detection, and HLS recording
- **Automatic Frame Saving**: Save frames with detected objects for later review

## 📋 Prerequisites

### System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libopencv-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libpq-dev \
    python3 \
    python3-pip \
    python3-venv \
    docker.io
```

### Required Software

- **C++ Compiler**: GCC 7+ or Clang with C++17 support
- **CMake**: Version 3.14 or higher
- **OpenCV**: Version 4.x
- **FFmpeg**: libavcodec, libavformat, libavutil, libswscale
- **PostgreSQL**: Version 15 (via Docker)
- **Python**: Version 3.8+
- **Docker**: For PostgreSQL container

## 🏗️ Project Structure

```
Analytics_Pipeline/
├── src/                      # C++ source files
│   ├── main.cpp             # Main pipeline orchestrator
│   ├── RTSPStreamer.cpp     # RTSP stream handler
│   ├── YoloDetector.cpp     # YOLOv8 detection engine
│   ├── HLSRecorder.cpp      # HLS stream generator
│   ├── DatabaseHandler.cpp  # PostgreSQL interface
│   └── SafeQueue.hpp        # Thread-safe queue template
├── web/                      # Web interface
│   ├── api.py               # FastAPI backend
│   ├── templates/           # HTML templates
│   │   └── index.html       # Dashboard UI
│   └── venv/                # Python virtual environment
├── build/                    # CMake build directory
├── detected_frames/          # Saved detection frames
├── hls_output/              # HLS stream segments
├── CMakeLists.txt           # CMake configuration
├── run_all.sh               # Startup script
├── yolov8n.onnx            # YOLOv8 model file
└── test.mp4                # Sample video file
```

## 🔧 Installation

### 1. Clone the Repository

```bash
cd /home/mantra/Analytics_Pipeline
```

### 2. Build the C++ Pipeline

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Return to project root
cd ..
```

### 3. Setup Python Environment

```bash
cd web
python3 -m venv venv
source venv/bin/activate
pip install fastapi uvicorn asyncpg jinja2
cd ..
```

### 4. Setup PostgreSQL Database

```bash
# Start PostgreSQL container
sudo docker run -d \
    --name postgres \
    -p 5432:5432 \
    -e POSTGRES_USER=admin \
    -e POSTGRES_PASSWORD=password \
    -e POSTGRES_DB=analytics_db \
    postgres:15

# Wait for PostgreSQL to be ready
sleep 5
```

The database schema will be automatically created on first run by the C++ application.

## 🚀 Usage

### Quick Start (Automated)

Use the provided startup script to launch all components:

```bash
./run_all.sh
```

This script will:
1. Setup the Python virtual environment
2. Start PostgreSQL container
3. Launch the web API on port 9090
4. Start the C++ pipeline

### Manual Start

#### 1. Start PostgreSQL

```bash
sudo docker start postgres
```

#### 2. Start Web API

```bash
source web/venv/bin/activate
python3 web/api.py &
```

#### 3. Run the Pipeline

```bash
./build/rtsp_pipeline <rtsp_url> yolov8n.onnx
```

**Example:**
```bash
./build/rtsp_pipeline rtsp://127.0.0.1:8554/laptop yolov8n.onnx
```

### Access the Dashboard

Open your browser and navigate to:
```
http://localhost:9090
```

The dashboard displays:
- Live HLS video stream
- Real-time detection feed
- Detection history with thumbnails
- Object class and confidence scores

## 🔌 API Endpoints

### Web API (Port 9090)

- `GET /` - Main dashboard interface
- `GET /detections` - Fetch latest 20 detections (JSON)
- `GET /images/{filename}` - Serve detected frame images
- `GET /hls/stream.m3u8` - HLS master playlist
- `GET /hls/stream{N}.ts` - HLS video segments

### Database Schema

```sql
CREATE TABLE IF NOT EXISTS detections (
    id SERIAL PRIMARY KEY,
    device_name VARCHAR(255),
    class_name VARCHAR(255),
    confidence REAL,
    timestamp VARCHAR(255),
    frame_path VARCHAR(512)
);
```

## ⚙️ Configuration

### Environment Variables

The pipeline supports the following environment variables:

```bash
# Database Configuration
export DB_HOST=localhost
export DB_PORT=5432
export POSTGRES_USER=admin
export POSTGRES_PASSWORD=password
export POSTGRES_DB=analytics_db
```

### YOLOv8 Model

The project uses YOLOv8 Nano (`yolov8n.onnx`) for object detection. You can replace it with other YOLOv8 variants:

- `yolov8s.onnx` - Small (better accuracy, slower)
- `yolov8m.onnx` - Medium
- `yolov8l.onnx` - Large
- `yolov8x.onnx` - Extra Large

Download models from the [Ultralytics YOLOv8 repository](https://github.com/ultralytics/ultralytics).

## 🏛️ Architecture

### Multi-threaded Pipeline

```
RTSP Stream → RTSPStreamer (Thread 1)
                    ↓
            ┌───────┴───────┐
            ↓               ↓
    HLS Queue          Detect Queue
            ↓               ↓
    HLS Worker      Detector Worker
    (Thread 2)       (Thread 3)
            ↓               ↓
    HLS Output      Frame Saving
                           ↓
                    PostgreSQL DB
```

### Components

1. **RTSPStreamer**: Captures RTSP stream and distributes packets to queues
2. **HLSRecorder**: Converts stream to HLS format for web playback
3. **YoloDetector**: Performs object detection on decoded frames
4. **DatabaseHandler**: Logs detections to PostgreSQL
5. **Web API**: Serves dashboard and provides REST endpoints

## 🐛 Troubleshooting

### PostgreSQL Connection Failed

```bash
# Check if container is running
sudo docker ps | grep postgres

# Restart container
sudo docker restart postgres

# Check logs
sudo docker logs postgres
```

### RTSP Stream Not Opening

- Verify the RTSP URL is accessible
- Check network connectivity
- Ensure the stream is in a compatible format (H.264 recommended)

### Build Errors

```bash
# Clean build directory
rm -rf build
mkdir build
cd build
cmake ..
make clean
make -j$(nproc)
```

### Web API Not Starting

```bash
# Check if port 9090 is available
sudo netstat -tulpn | grep 9090

# Kill existing process
pkill -f "python3 web/api.py"

# Restart API
source web/venv/bin/activate
python3 web/api.py
```

## 📊 Performance

- **Detection Speed**: ~30 FPS on modern CPUs (YOLOv8n)
- **HLS Latency**: 6-10 seconds (3 segments @ 2s each)
- **Database Throughput**: 100+ inserts/second
- **Memory Usage**: ~500MB (base) + model size

## 🔐 Security Notes

⚠️ **Important**: The default configuration uses hardcoded credentials for development purposes only.

For production deployment:
1. Use environment variables for all credentials
2. Enable PostgreSQL SSL/TLS
3. Implement authentication for the web API
4. Use reverse proxy (nginx) with HTTPS
5. Restrict database access to localhost or VPN

## 📝 License

This project is provided as-is for educational and development purposes.

## 🤝 Contributing

Contributions are welcome! Areas for improvement:
- Multi-camera support
- Advanced analytics (tracking, counting)
- GPU acceleration (CUDA)
- Kubernetes deployment
- User authentication
- Alert notifications

## 📧 Support

For issues and questions, please check the troubleshooting section or review the source code documentation.

---

**Built with**: C++17, OpenCV, FFmpeg, YOLOv8, PostgreSQL, FastAPI
