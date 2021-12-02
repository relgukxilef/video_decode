#include "io.h"

#include <iostream>
#include <algorithm>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#include "../utility/out_ptr.h"

file::file(const char *filename) {
    check(avformat_open_input(
        out_ptr(format_context), filename, nullptr, nullptr
    ));
    check(avformat_find_stream_info(format_context.get(), nullptr));

    // find streams
    stream_index = check(av_find_best_stream(
        format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0
    ));

    codec = avcodec_find_decoder(
        format_context->streams[stream_index]->codecpar->codec_id
    );
    if (!codec) {
        throw std::runtime_error("No codec found");
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        throw std::bad_alloc();
    }

    check(avcodec_parameters_to_context(
        codec_context.get(), format_context->streams[stream_index]->codecpar
    ));

    check(avcodec_open2(codec_context.get(), codec, nullptr));

    graph = avfilter_graph_alloc();
    source_context = nullptr;
    sink_context = nullptr;

    AVRational time_base = format_context->streams[stream_index]->time_base;
    // filter parameters are stringly typed
    // TODO: use codec_context->sample_aspect_ratio
    // TODO: figure out what to do if sample_aspect_ratio is unknown (0)
    char filter[512];
    std::snprintf(
        filter, sizeof(filter),
        "buffer=video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=1/1,"
        "colorspace=all=bt709:trc=srgb:format=yuv420p:range=pc:"
        "iall=bt709," // TODO: don't override if specified in input
        "buffersink",
        codec_context->width, codec_context->height, codec_context->pix_fmt,
        time_base.num, time_base.den
    );

    check(avfilter_graph_parse2(
        graph.get(), filter,
        out_ptr(input), out_ptr(output)
    ));
    check(avfilter_graph_config(graph.get(), nullptr));

    source_context = avfilter_graph_get_filter(graph.get(), "Parsed_buffer_0");
    sink_context =
        avfilter_graph_get_filter(graph.get(), "Parsed_buffersink_2");

    char* dump = avfilter_graph_dump(graph.get(), "");
    std::cout << dump << std::endl;
    av_free(dump);

    av_frame = av_frame_alloc();
    packet = av_packet_alloc();
}

frame file::get_frame() {
    // 32 seconds into the video
    AVRational time_base = format_context->streams[stream_index]->time_base;
    check(av_seek_frame(
        format_context.get(), stream_index, 32 * time_base.den / time_base.num,
        0
    ));

    int result;
    do {
        do {
            check(av_read_frame(format_context.get(), packet.get()));
        } while (packet->stream_index != stream_index);

        check(avcodec_send_packet(codec_context.get(), packet.get()));
        av_packet_unref(packet.get());

        // 1 packet may contain 0, 1 or more frames
        result = avcodec_receive_frame(codec_context.get(), av_frame.get());
        // TODO: handle more frames

        if (result == 0) {
            check(av_buffersrc_add_frame(source_context, av_frame.get()));
            check(av_buffersink_get_frame(sink_context, av_frame.get()));
        }
    } while(result == AVERROR(EAGAIN) || result == AVERROR_EOF);

    check(result);

    unsigned width = av_frame->width, height = av_frame->height;

    auto
        y = std::make_unique<uint8_t[]>(width * height),
        cb = std::make_unique<uint8_t[]>(width / 2 * height / 2),
        cr = std::make_unique<uint8_t[]>(width / 2 * height / 2);

    uint8_t* av_row = av_frame->data[0];
    uint8_t* frame_row = y.get();
    for (auto row = 0u; row < height; row++) {
        std::move(av_row, av_row + width, frame_row);
        av_row += av_frame->linesize[0];
        frame_row += width;
    }
    av_row = av_frame->data[1];
    frame_row = cb.get();
    for (auto row = 0u; row < height / 2; row++) {
        std::move(av_row, av_row + width / 2, frame_row);
        av_row += av_frame->linesize[1];
        frame_row += width / 2;
    }
    av_row = av_frame->data[2];
    frame_row = cr.get();
    for (auto row = 0u; row < height / 2; row++) {
        std::move(av_row, av_row + width / 2, frame_row);
        av_row += av_frame->linesize[1];
        frame_row += width / 2;
    }

    return frame{
        .pixels = {
            .y = std::move(y),
            .cb = std::move(cb),
            .cr = std::move(cr),
        },
        .width = width,
        .height = height,
    };
}
