#pragma once

#include <string>
#include <vector>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

class HLSRecorder {
public:
    HLSRecorder();
    ~HLSRecorder();

    bool init(const std::string& outputFilename, AVCodecParameters* inputCodecParams, AVRational inputTimebase);
    void writePacket(AVPacket* packet);
    void finish();

private:
    AVFormatContext* outFmtCtx = nullptr;
    AVStream* outStream = nullptr;
    AVRational inTimebase;
    bool initialized = false;
    int64_t lastPts = -1;
};
