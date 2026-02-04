#include "HLSRecorder.hpp"
#include <iostream>

HLSRecorder::HLSRecorder() {}

HLSRecorder::~HLSRecorder() {
    finish();
}

bool HLSRecorder::init(const std::string& outputFilename, AVCodecParameters* inputCodecParams, AVRational inputTimeBase) {
    std::cout << "[DEBUG-HLS] init called." << std::endl;
    this->inTimebase = inputTimeBase;

    if (!inputCodecParams) {
        std::cerr << "[DEBUG-HLS] inputCodecParams is NULL!" << std::endl;
        return false;
    }

    // Allocate output context for HLS
    // Note: outputFilename should be something like "hls_out/stream.m3u8"
    int ret = avformat_alloc_output_context2(&outFmtCtx, nullptr, "hls", outputFilename.c_str());
    if (!outFmtCtx) {
        std::cerr << "Could not create output context for HLS." << std::endl;
        return false;
    }

    outStream = avformat_new_stream(outFmtCtx, nullptr);
    if (!outStream) {
        std::cerr << "Failed allocating output stream." << std::endl;
        return false;
    }

    ret = avcodec_parameters_copy(outStream->codecpar, inputCodecParams);
    if (ret < 0) {
        std::cerr << "Failed to copy codec parameters." << std::endl;
        return false;
    }
    
    // Reset codec tag for compatibility
    outStream->codecpar->codec_tag = 0;

    // HLS Options
    av_opt_set(outFmtCtx->priv_data, "hls_time", "2", 0); // 2 second segments
    av_opt_set(outFmtCtx->priv_data, "hls_list_size", "5", 0); // keep 5 segments in playlist
    // optional: start number, etc.

    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFmtCtx->pb, outputFilename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            std::cerr << "Could not open output file '" << outputFilename << "'" << std::endl;
            return false;
        }
    }

    ret = avformat_write_header(outFmtCtx, nullptr);
    if (ret < 0) {
        std::cerr << "Error occurred when opening output file." << std::endl;
        return false;
    }

    initialized = true;
    std::cout << "HLS Recorder initialized. Output: " << outputFilename << std::endl;
    return true;
}

void HLSRecorder::writePacket(AVPacket* packet) {
    if (!initialized || !packet) return;

    // Rescale timestamps
    av_packet_rescale_ts(packet, inTimebase, outStream->time_base);
    packet->stream_index = outStream->index;
    
    // Ensure monotonic Pts
    if (lastPts != -1 && packet->pts <= lastPts) {
         // simple fix for non-monotonic packets
         packet->pts = lastPts + 1;
         packet->dts = packet->pts;
    }
    lastPts = packet->pts;

    int ret = av_interleaved_write_frame(outFmtCtx, packet);
    if (ret < 0) {
        // char errBuf[AV_ERROR_MAX_STRING_SIZE];
        // av_strerror(ret, errBuf, sizeof(errBuf));
        // std::cerr << "Error writing packet: " << errBuf << std::endl;
    }
}

void HLSRecorder::finish() {
    if (initialized && outFmtCtx) {
        av_write_trailer(outFmtCtx);
        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&outFmtCtx->pb);
        
        avformat_free_context(outFmtCtx);
        outFmtCtx = nullptr;
        initialized = false;
        std::cout << "HLS Recording finished." << std::endl;
    }
}
