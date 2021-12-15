#include "frame_cache.h"

void scale_down(
    uint8_t* source, uint8_t* destination, uint16_t width, uint16_t height
) {
    for (uint16_t y = 0; y < height / 2; y++) {
        uint8_t* source_pixel = source;
        for (uint16_t x = 0; x < width / 2; x++) {
            uint16_t sum = 0;
            sum += *source_pixel;
            sum += *(source_pixel + width);
            source_pixel++;
            sum += *source_pixel;
            sum += *(source_pixel + width);
            source_pixel++;

            *destination = sum / 4;
            destination++;
        }
        source += width * 2;
    }
}

std::shared_ptr<frame> frame_cache::get_frame(frame_key key) {
    auto i = frames.upper_bound(key);
    if (
        i != frames.end() &&
        i->first.file == key.file // TODO: check time-stamp
    ) {
        return i->second;
    }
    return nullptr;
}

void frame_cache::put_frame(frame_key key, frame&& frame) {
    auto cost = frame.width * frame.height * 3 / 2;
    if (
        frames.emplace(key, std::make_shared<::frame>(std::move(frame))).second
    ) {
        eviction_queue.emplace(cost, key);
        memory_usage += cost;

        while (memory_usage > memory_limit) {
            auto evicted = frames.find(eviction_queue.top().second);
            auto evicted_frame = std::move(evicted->second);

            key = {
                evicted->first.file, evicted->first.time_stamp,
                evicted->first.level + 1
            };

            memory_usage -= eviction_queue.top().first;
            eviction_queue.pop();
            frames.erase(evicted);

            // TODO: what to do with 1x1 frames?
            frame = {
                .pixels = {
                    .y = std::make_unique<uint8_t[]>(
                        evicted_frame->height * evicted_frame->width / 4
                    ),
                    .cb = std::make_unique<uint8_t[]>(
                        evicted_frame->height * evicted_frame->width / 4 / 4
                    ),
                    .cr = std::make_unique<uint8_t[]>(
                        evicted_frame->height * evicted_frame->width / 4 / 4
                    ),
                },
                .width = static_cast<uint16_t>(evicted_frame->width / 2),
                .height = static_cast<uint16_t>(evicted_frame->height / 2),
            };

            scale_down(
                evicted_frame->pixels.y.get(), frame.pixels.y.get(),
                evicted_frame->width, evicted_frame->height
            );
            scale_down(
                evicted_frame->pixels.cb.get(), frame.pixels.cb.get(),
                evicted_frame->width / 2, evicted_frame->height / 2
            );
            scale_down(
                evicted_frame->pixels.cr.get(), frame.pixels.cr.get(),
                evicted_frame->width / 2, evicted_frame->height / 2
            );

            auto cost = frame.width * frame.height * 3 / 2;
            if (
                frames.emplace(
                    key, std::make_shared<::frame>(std::move(frame))
                ).second
            ) {
                eviction_queue.emplace(cost, key);
                memory_usage += cost;
            }
        }
    }
}
