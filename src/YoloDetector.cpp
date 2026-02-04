#include "YoloDetector.hpp"
#include <fstream>
#include <iostream>

YoloDetector::YoloDetector() {
    loadClassNames();
}

void YoloDetector::loadClassNames() {
    // COCO class names
    classNames = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"
    };
}

bool YoloDetector::loadModel(const std::string& modelPath) {
    try {
        net = cv::dnn::readNetFromONNX(modelPath);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU); // Or CUDA if available
        std::cout << "Model loaded successfully: " << modelPath << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Detection> YoloDetector::detect(const cv::Mat& frame, float confThreshold, float nmsThreshold) {
    std::vector<Detection> detections;
    if (frame.empty()) return detections;

    // Preprocess
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(INPUT_WIDTH, INPUT_HEIGHT), cv::Scalar(), true, false);
    net.setInput(blob);

    // Inference
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    // Post-processing (parsing YOLOv8 output)
    // YOLOv8 output shape: [1, 84, 8400] -> [1, 4 + 80 classes, proposals]
    
    // We need to verify if the output is [1, 84, 8400] or something else depending on export
    // Assuming standard export:
    if (outputs.empty()) return detections;

    cv::Mat output = outputs[0];
    // Check dimensions. Usually 3D: [Batch, Channels, Anchors]
    // We want to iterate over anchors.
    
    // If output is 3 dimensions:
    int dimensions = output.dims;
    int rows = output.size[1]; // 84 (4 box coords + 80 classes)
    int cols = output.size[2]; // 8400 anchors

    // Access data pointer
    // Note: Some ONNX exports transpose this to [1, 8400, 84]. 
    // Standard Ultralytics export is [1, 84, 8400].
    
    // For easier processing, let's transpose if needed so we have one row per detection
    if (dimensions == 3 && output.size[1] < output.size[2]) {
        // Current: [1, 84, 8400]. Reshape to remove batch dim, then transpose
        output = output.reshape(1, rows); // [84, 8400]
        cv::transpose(output, output);    // [8400, 84]
    }
    
    rows = output.rows; 
    
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    float x_scale = frame.cols / INPUT_WIDTH;
    float y_scale = frame.rows / INPUT_HEIGHT;

    float* data = (float*)output.data;
    
    // Stride is 84 usually
    int stride = output.cols; 

    for (int i = 0; i < rows; ++i) {
        float* rowPtr = data + i * stride;
        
        // Find max confidence in class scores (index 4 to 83)
        float maxClassScore = 0.0f;
        int classId = -1;
        
        for (int j = 4; j < stride; ++j) {
            if (rowPtr[j] > maxClassScore) {
                maxClassScore = rowPtr[j];
                classId = j - 4;
            }
        }

        if (maxClassScore >= confThreshold) {
            // YOLOv8 bbox is cx, cy, w, h
            float cx = rowPtr[0];
            float cy = rowPtr[1];
            float w = rowPtr[2];
            float h = rowPtr[3];

            int left = int((cx - 0.5 * w) * x_scale);
            int top = int((cy - 0.5 * h) * y_scale);
            int width = int(w * x_scale);
            int height = int(h * y_scale);

            boxes.push_back(cv::Rect(left, top, width, height));
            confidences.push_back(maxClassScore);
            classIds.push_back(classId);
        }
    }

    // NMS
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

    for (int idx : indices) {
        Detection det;
        det.class_id = classIds[idx];
        det.confidence = confidences[idx];
        det.box = boxes[idx];
        det.className = (det.class_id >= 0 && det.class_id < classNames.size()) ? classNames[det.class_id] : "Unknown";
        detections.push_back(det);
    }

    return detections;
}

void YoloDetector::drawDetections(cv::Mat& frame, const std::vector<Detection>& detections) {
    for (const auto& det : detections) {
        cv::rectangle(frame, det.box, cv::Scalar(0, 255, 0), 2);
        
        std::string label = det.className + ": " + cv::format("%.2f", det.confidence);
        int baseLine;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        
        int top = std::max(det.box.y, labelSize.height);
        cv::rectangle(frame, cv::Point(det.box.x, top - labelSize.height),
                      cv::Point(det.box.x + labelSize.width, top + baseLine), cv::Scalar(0, 255, 0), cv::FILLED);
        cv::putText(frame, label, cv::Point(det.box.x, top), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }
}
