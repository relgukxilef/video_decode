#include <iostream>
#include <stdexcept>
#include <memory>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

using namespace std;

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
    } while(result == AVERROR(EAGAIN) || result == AVERROR_EOF);

    check(result);

    cout << "Hello World!" << endl;
    return 0;
}
