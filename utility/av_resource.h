#pragma once

#include <stdexcept>

#include "resource.h"

extern "C" {
#include <libavutil/avutil.h>
}

// TODO: find a better place for check?
inline int check(int code) {
    if (code >= 0)
        return code;
    if (code == AVERROR(EAGAIN)) {
        throw std::runtime_error("Output is not available, send new input");
    } else if (code == AVERROR_EOF) {
        throw std::runtime_error("No more output");
    } else if (code == AVERROR(EINVAL)) {
        throw std::runtime_error("Not opened");
    } else if (code == AVERROR_INPUT_CHANGED) {
        throw std::runtime_error("Input changed between calls");
    }
    throw std::runtime_error("Unknown error");
}

extern "C" {
void avformat_close_input(struct AVFormatContext**);
void avcodec_free_context(struct AVCodecContext**);
void av_frame_free(struct AVFrame**);
void av_packet_free(struct AVPacket**);
void avfilter_inout_free(struct AVFilterInOut**);
void avfilter_graph_free(struct AVFilterGraph**);
}

using unique_av_format_context =
    unique_resource<struct AVFormatContext*, avformat_close_input>;
using unique_av_codec_context =
    unique_resource<AVCodecContext*, avcodec_free_context>;
using unique_av_frame =
    unique_resource<AVFrame*, av_frame_free>;
using unique_av_packet =
    unique_resource<AVPacket*, av_packet_free>;
using unique_av_filter_in_out =
    unique_resource<AVFilterInOut*, avfilter_inout_free>;
using unique_av_filter_graph =
    unique_resource<AVFilterGraph*, avfilter_graph_free>;
