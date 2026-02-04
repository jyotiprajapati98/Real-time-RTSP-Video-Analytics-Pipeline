#pragma once

#include <string>
#include <thread>
#include <atomic>
#include "SafeQueue.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

class RTSPStreamer {
public:
    RTSPStreamer();
    ~RTSPStreamer();

    bool open(const std::string& url);
    void start(SafeQueue<AVPacket*>& hlsQueue, SafeQueue<AVPacket*>& detectQueue);
    void stop();

    AVCodecParameters* getCodecParameters();
    AVRational getTimeBase();

private:
    AVFormatContext* fmtCtx = nullptr;
    int videoStreamIndex = -1;
    std::string rtspUrl;
    std::atomic<bool> shouldStop;
    std::thread streamThread;

    void recordLoop(SafeQueue<AVPacket*>& hlsQueue, SafeQueue<AVPacket*>& detectQueue);
};
