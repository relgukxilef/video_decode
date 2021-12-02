#include <iostream>
#include <stdexcept>
#include <memory>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

int check(int code) {
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

template<typename T, auto Deleter>
struct unique_av_resource {
    typedef T pointer;

    unique_av_resource() : value(nullptr) {}
    unique_av_resource(T value) : value(value) {}
    unique_av_resource(const unique_av_resource&) = delete;
    unique_av_resource(unique_av_resource&& o) : value(o.value) {
        o.value = nullptr;
    }
    ~unique_av_resource() {
        if (value != nullptr)
            Deleter(&value);
    }
    unique_av_resource& operator= (const unique_av_resource&) = delete;
    unique_av_resource& operator= (unique_av_resource&& o) {
        if (value != nullptr)
            Deleter(&value);
        value = o.value;
        o.value = nullptr;
        return *this;
    }
    auto operator*() {
        return *value;
    }
    T operator->() {
        return value;
    }
    operator bool() const {
        return value;
    }

    const T& get() {
        return value;
    };

private:
    T value;
};

template<class Smart, class Pointer>
struct out_ptr_t {
    out_ptr_t(Smart& smart) : smart(smart), pointer(nullptr) {}
    ~out_ptr_t() { smart = Smart(pointer); }

    operator Pointer*() { return &pointer; }

    Smart &smart;
    Pointer pointer;
};

template<class Smart>
out_ptr_t<Smart, typename Smart::pointer> out_ptr(Smart& smart) {
    return { smart };
}

using unique_av_format_context =
    unique_av_resource<AVFormatContext*, avformat_close_input>;
using unique_av_codec_context =
    unique_av_resource<AVCodecContext*, avcodec_free_context>;
using unique_av_frame =
    unique_av_resource<AVFrame*, av_frame_free>;
using unique_av_packet =
    unique_av_resource<AVPacket*, av_packet_free>;
using unique_av_filter_in_out =
    unique_av_resource<AVFilterInOut*, avfilter_inout_free>;
using unique_av_filter_graph =
    unique_av_resource<AVFilterGraph*, avfilter_graph_free>;

int main()
{
    // demuxer
    const char* file = "file:test.mkv";
    unique_av_format_context format_context;
    check(avformat_open_input(out_ptr(format_context), file, nullptr, nullptr));
    check(avformat_find_stream_info(format_context.get(), nullptr));

    // find streams
    auto stream_index = check(av_find_best_stream(
        format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0
    ));

    AVCodec* codec = avcodec_find_decoder(
        format_context->streams[stream_index]->codecpar->codec_id
    );
    if (!codec) {
        throw std::runtime_error("No codec found");
    }

    unique_av_codec_context codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        throw std::bad_alloc();
    }

    check(avcodec_parameters_to_context(
        codec_context.get(), format_context->streams[stream_index]->codecpar
    ));

    check(avcodec_open2(codec_context.get(), codec, nullptr));

    // pixel format conversion
    // filters are stringly typed
    const AVFilter* buffer = avfilter_get_by_name("buffer");
    const AVFilter* buffer_sink = avfilter_get_by_name("buffersink");
    unique_av_filter_in_out input;
    unique_av_filter_in_out output;
    unique_av_filter_graph graph = avfilter_graph_alloc();
    AVFilterContext* source_context = nullptr;
    AVFilterContext* sink_context = nullptr;

    AVRational time_base = format_context->streams[stream_index]->time_base;
    // filter parameters are also stringly typed
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

    // decode a frame
    unique_av_frame frame = av_frame_alloc();
    unique_av_packet packet = av_packet_alloc();

    int result;
    do {
        do {
            check(av_read_frame(format_context.get(), packet.get()));
        } while (packet->stream_index != stream_index);

        check(avcodec_send_packet(codec_context.get(), packet.get()));
        av_packet_unref(packet.get());

        // 1 packet may contain 0, 1 or more frames
        result = avcodec_receive_frame(codec_context.get(), frame.get());
        // TODO: handle more frames

        if (result == 0) {
            check(av_buffersrc_add_frame(source_context, frame.get()));
            check(av_buffersink_get_frame(sink_context, frame.get()));
        }
    } while(result == AVERROR(EAGAIN) || result == AVERROR_EOF);

    check(result);

    std::cout << "Hello World!" << std::endl;
    return 0;
}
