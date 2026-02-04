#include "RTSPStreamer.hpp"
#include <iostream>

RTSPStreamer::RTSPStreamer() : shouldStop(false) {
    avformat_network_init();
}

RTSPStreamer::~RTSPStreamer() {
    stop();
    if (fmtCtx) {
        avformat_close_input(&fmtCtx);
    }
}

bool RTSPStreamer::open(const std::string& url) {
    std::cout << "[DEBUG-RTSP] open called with: " << url << std::endl;
    rtspUrl = url;
    // Set options for low latency
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0); // Prefer TCP for reliability
    av_dict_set(&opts, "buffer_size", "1024000", 0);
    av_dict_set(&opts, "max_delay", "500000", 0); // 0.5 sec

    std::cout << "[DEBUG-RTSP] calling avformat_open_input..." << std::endl;
    int ret = avformat_open_input(&fmtCtx, url.c_str(), nullptr, &opts);
    av_dict_free(&opts);

    if (ret < 0) {
        std::cerr << "Could not open RTSP stream: " << url << std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        std::cerr << "Could not find stream info." << std::endl;
        return false;
    }

    videoStreamIndex = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "No video stream found." << std::endl;
        return false;
    }

    std::cout << "RTSP Stream opened. Video Stream Index: " << videoStreamIndex << std::endl;
    return true;
}

AVCodecParameters* RTSPStreamer::getCodecParameters() {
    if (fmtCtx && videoStreamIndex >= 0) {
        return fmtCtx->streams[videoStreamIndex]->codecpar;
    }
    return nullptr;
}

AVRational RTSPStreamer::getTimeBase() {
    if (fmtCtx && videoStreamIndex >= 0) {
        return fmtCtx->streams[videoStreamIndex]->time_base;
    }
    return {1, 90000}; // Default fallback
}

void RTSPStreamer::start(SafeQueue<AVPacket*>& hlsQueue, SafeQueue<AVPacket*>& detectQueue) {
    shouldStop = false;
    streamThread = std::thread(&RTSPStreamer::recordLoop, this, std::ref(hlsQueue), std::ref(detectQueue));
}

void RTSPStreamer::stop() {
    shouldStop = true;
    if (streamThread.joinable()) {
        streamThread.join();
    }
}

void RTSPStreamer::recordLoop(SafeQueue<AVPacket*>& hlsQueue, SafeQueue<AVPacket*>& detectQueue) {
    AVPacket* packet = av_packet_alloc();
    while (!shouldStop) {
        int ret = av_read_frame(fmtCtx, packet);
        if (ret < 0) {
            std::cerr << "Error reading frame or EOF." << std::endl;
            // Attempt reconnect? For now just exit.
            break;
        }

        if (packet->stream_index == videoStreamIndex) {
            // We need to clone the packet for each consumer because they will own it and free it.
            // Packet 1 for HLS
            AVPacket* packetHLS = av_packet_clone(packet);
            if (packetHLS) {
                 hlsQueue.push(packetHLS);
            }

            // Packet 2 for Detection
            // Only push if queue isn't full to avoid lag? 
            // For now, push all, let consumer drop if too slow.
            // Actually, we'll check size.
            if (detectQueue.size() < 30) { // arbitrary limit to prevent OOM
                AVPacket* packetDetect = av_packet_clone(packet);
                if (packetDetect) {
                    detectQueue.push(packetDetect);
                }
            }
        }

        av_packet_unref(packet);
    }
    av_packet_free(&packet);
}
