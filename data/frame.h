#pragma once

#include <memory>

struct frame {
    struct {
        std::unique_ptr<uint8_t[]> y, cb, cr;
    } pixels;
    unsigned width, height;
};

