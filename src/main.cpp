#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "RTSPStreamer.hpp"
#include "HLSRecorder.hpp"
#include "YoloDetector.hpp"
#include "SafeQueue.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "DatabaseHandler.hpp"

// Global queues
SafeQueue<AVPacket*> hlsQueue;
SafeQueue<AVPacket*> detectQueue;

void hlsWorker(HLSRecorder* recorder) {
    AVPacket* pkt = nullptr;
    while (true) {
        if (hlsQueue.pop(pkt)) {
            if (pkt) {
                recorder->writePacket(pkt);
                av_packet_free(&pkt);
            }
        } else {
            // Queue stopped and empty
            break;
        }
    }
}

// Decode and Detect Worker
// We need to implement decoding here since we receive packets
void detectorWorker(YoloDetector* detector, AVCodecParameters* codecParams, DatabaseHandler* dbHandler) {
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        std::cerr << "Codec not found for detection worker." << std::endl;
        return;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);
    
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec for detection worker." << std::endl;
        return;
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket* pkt = nullptr;
    
    // For swscale context
    struct SwsContext* sws_ctx = nullptr;
    
    while (true) {
        if (detectQueue.pop(pkt)) {
            if (pkt) {
                int ret = avcodec_send_packet(codecCtx, pkt);
                if (ret >= 0) {
                    while (ret >= 0) {
                        ret = avcodec_receive_frame(codecCtx, frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        else if (ret < 0) {
                             // error
                             break;
                        }

                        // Convert AVFrame (YUV420P usually) to cv::Mat (BGR)
                        if (!sws_ctx) {
                            sws_ctx = sws_getContext(frame->width, frame->height, codecCtx->pix_fmt,
                                                     frame->width, frame->height, AV_PIX_FMT_BGR24,
                                                     SWS_BILINEAR, nullptr, nullptr, nullptr);
                        }

                        cv::Mat img(frame->height, frame->width, CV_8UC3);
                        uint8_t* dest[4] = { img.data, 0, 0, 0 };
                        int destLinesize[4] = { (int)img.step[0], 0, 0, 0 };

                        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, dest, destLinesize);

                        // Run Detection
                        auto detections = detector->detect(img);
                        detector->drawDetections(img, detections);

                        if (!detections.empty()) {
                             std::cout << "Detected " << detections.size() << " objects." << std::endl;
                             
                             // Save frame
                             auto now = std::chrono::system_clock::now();
                             auto time = std::chrono::system_clock::to_time_t(now);
                             auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
                             
                             std::stringstream ss;
                             ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") 
                                << "_" << ms.count();
                             
                             std::string timestamp = ss.str();
                             // ISO format sort of
                             
                             std::string filename = "detected_frames/frame_" + timestamp + ".jpg";
                             
                             cv::imwrite(filename, img);

                             // Log to Database
                             for (const auto& det : detections) {
                                  dbHandler->logDetection("cam1", det.className, det.confidence, timestamp, filename);
                             }
                        }

                    }
                }
                av_packet_free(&pkt);
            }
        } else {
            break;
        }
    }

    if (sws_ctx) sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <rtsp_url> <model_path>" << std::endl;
        return 1;
    }

    // Create output directories
    const std::string frameDir = "detected_frames";
    const std::string hlsDir = "hls_output";
    std::string cmd = "mkdir -p " + frameDir + " " + hlsDir;
    system(cmd.c_str());

    std::string rtspUrl = argv[1];
    std::string modelPath = argv[2];

    // Initialize Components
    RTSPStreamer streamer;
    YoloDetector detector;
    HLSRecorder recorder;
    // DB Setup
    DatabaseHandler dbHandler; 
    
    auto getEnvVar = [](const std::string& key, const std::string& defaultValue) -> std::string {
        const char* val = std::getenv(key.c_str());
        return val ? std::string(val) : defaultValue;
    };

    // Connection string from Env Vars or defaults
    std::string dbHost = getEnvVar("DB_HOST", "localhost");
    std::string dbPort = getEnvVar("DB_PORT", "5432");
    std::string dbUser = getEnvVar("POSTGRES_USER", "admin");
    std::string dbPass = getEnvVar("POSTGRES_PASSWORD", "password");
    std::string dbName = getEnvVar("POSTGRES_DB", "analytics_db");

    std::string dbConn = "postgresql://" + dbUser + ":" + dbPass + "@" + dbHost + ":" + dbPort + "/" + dbName;
    
    bool dbConnected = false;
    const int maxRetries = 5;
    for (int i = 0; i < maxRetries; ++i) {
        if (dbHandler.init(dbConn)) {
            dbConnected = true;
            break;
        }
        std::cerr << "Warning: Could not connect to PostgreSQL database. Retrying " << (i + 1) << "/" << maxRetries << "..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    if (!dbConnected) {
        std::cerr << "[ERROR] Failed to connect to database after " << maxRetries << " attempts. Exiting." << std::endl;
        return 1;
    }
    std::cout << "[DEBUG] Database initialized." << std::endl;

    if (!detector.loadModel(modelPath)) {
        std::cerr << "Failed to load model." << std::endl;
        return 1;
    }

    if (!streamer.open(rtspUrl)) {
        std::cerr << "Failed to open RTSP stream." << std::endl;
        return 1;
    }
    
    // Calculate output path
    std::string hlsOutput = "hls_output/stream.m3u8";
    std::cout << "[DEBUG] Initializing HLSRecorder..." << std::endl;
    AVCodecParameters* codecParams = streamer.getCodecParameters();
    std::cout << "[DEBUG] CodecParams Ptr: " << codecParams << std::endl;
    if (codecParams) {
        std::cout << "[DEBUG] CodecID: " << codecParams->codec_id << std::endl;
    }
    AVRational tb = streamer.getTimeBase();
    std::cout << "[DEBUG] TimeBase: " << tb.num << "/" << tb.den << std::endl;

    if (!recorder.init(hlsOutput, codecParams, tb)) {
        std::cerr << "HLSRecorder init failed." << std::endl;
        return 1;
    }
    std::cout << "[DEBUG] HLSRecorder initialized." << std::endl;

    // Start threads
    std::cout << "Starting pipeline..." << std::endl;
    streamer.start(hlsQueue, detectQueue);

    std::thread hlsThread(hlsWorker, &recorder);
    std::thread detThread(detectorWorker, &detector, streamer.getCodecParameters(), &dbHandler);

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    // Stop
    streamer.stop();
    hlsQueue.stop();
    detectQueue.stop();

    if (hlsThread.joinable()) hlsThread.join();
    if (detThread.joinable()) detThread.join();

    return 0;
}
