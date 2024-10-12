#pragma once

#include <vector>

#include "../utility/av_resource.h"
#include "../data/frame.h"

struct file {
    file(const char* filename);

    void seek(uint64_t milliseconds);
    frame get_next_frame();

    unique_av_format_context format_context;
    struct AVCodec* codec;
    unique_av_codec_context codec_context;

    unique_av_filter_in_out input;
    unique_av_filter_in_out output;
    unique_av_filter_graph graph;
    struct AVFilterContext* source_context;
    struct AVFilterContext* sink_context;
    int stream_index;

    unique_av_frame av_frame;
    unique_av_packet packet;
};

