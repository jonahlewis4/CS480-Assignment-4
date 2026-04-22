#include "GlUtils.h"
#include <glad/glad.h>
#include <cstdio>

namespace glutils {

void checkGlError(const char* where) {
#ifndef NDEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::fprintf(stderr, "OpenGL error at %s: 0x%x\n", where, (unsigned)err);
    }
#else
    (void)where;
#endif
}

} // namespace glutils
