#include <iostream>
#include <stdexcept>
#include <memory>

#include "io/io.h"

int main()
{
    // demuxer
    const char* filename = "file:test.mkv";

    file video(filename);

    frame f = video.get_frame();

    std::cout << "Hello World!" << std::endl;
    return 0;
}
