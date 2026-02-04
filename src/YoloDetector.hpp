#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>

struct Detection {
    int class_id;
    float confidence;
    cv::Rect box;
    std::string className;
};

class YoloDetector {
public:
    YoloDetector();
    ~YoloDetector() = default;

    bool loadModel(const std::string& modelPath);
    std::vector<Detection> detect(const cv::Mat& frame, float confThreshold = 0.4f, float nmsThreshold = 0.4f);

    // Helper to draw bounding boxes
    void drawDetections(cv::Mat& frame, const std::vector<Detection>& detections);

private:
    cv::dnn::Net net;
    std::vector<std::string> classNames;
    
    // Model input parameters (YOLOv8 default)
    const float INPUT_WIDTH = 640.0;
    const float INPUT_HEIGHT = 640.0;
    
    void loadClassNames();
};
