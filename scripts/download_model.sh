#!/bin/bash
# Download YOLOv8n ONNX model

MODEL_URL="https://github.com/ultralytics/assets/releases/download/v8.1.0/yolov8n.onnx"
OUTPUT_FILE="../yolov8n.onnx"

echo "Downloading YOLOv8n ONNX model..."
wget -O "$OUTPUT_FILE" "$MODEL_URL"

if [ $? -eq 0 ]; then
    echo "Download successful: $OUTPUT_FILE"
else
    echo "Download failed!"
    exit 1
fi
