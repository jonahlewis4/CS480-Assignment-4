// src/third_party/stb_impl.cpp
// Role: Single translation unit that defines stb_image / stb_image_write implementations.
// Why: stb requires its implementation macros to appear in exactly one .cpp file.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
