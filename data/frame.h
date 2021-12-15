#pragma once

#include <memory>

struct frame {
    struct {
        std::unique_ptr<uint8_t[]> y, cb, cr;
    } pixels;
    uint64_t time;
    uint16_t width, height;
};

