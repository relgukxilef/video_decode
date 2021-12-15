#pragma once

#include <map>
#include <tuple>
#include <memory>
#include <queue>

#include "frame.h"
#include "../io/io.h"

struct frame_key {
    file* file;
    uint64_t time_stamp;
    uint32_t level; // 0 is full frame, 1 is half scale on both dimension

    bool operator<(frame_key o) const;
};

struct frame_cache {
    frame_cache() = default;

    /**
     * @brief get_frame looks up the largest cached version of a frame at least
     * as large as the given key.
     * @param key is the file, time and level to look up in the cache.
     * @return the cached frame or nullptr if the frame is not in the cache.
     */
    std::shared_ptr<frame> get_frame(frame_key key);

    /**
     * @brief put_frame inserts the given frame into the cache, potentially
     * downscaling or removing frames already in the cached to stay within the
     * memory limit.
     * @param key is the file and time of the frame.
     * @param frame is the frame to store.
     */
    void put_frame(frame_key key, frame&& frame);

    std::map<frame_key, std::shared_ptr<frame>> frames;
    std::priority_queue<std::pair<uint32_t, frame_key>> eviction_queue;

    // one 1080p 4:2:0 frame takes up ~3MB
    uint32_t memory_limit = 32*1024*1024;
    uint32_t memory_usage = 0;
};


inline bool frame_key::operator<(frame_key o) const {
    return
        std::tie(file, time_stamp, level) <
        std::tie(o.file, o.time_stamp, o.level);
}
