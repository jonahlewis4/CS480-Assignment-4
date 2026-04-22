# CMakeLists.txt
# Role: Build configuration for the ImageProcessing app.
# What it does:
#   - Finds vcpkg dependencies (glfw3, glad, stb).
#   - Builds ImGui from the bundled repo and tinyfiledialogs from local source.
#   - Builds the ImageProcessing executable and links all required libraries.
#   - Copies the assets/ folder next to the executable after build.
#   - Adds Assignment 2, 3, and 4 source files.

cmake_minimum_required(VERSION 3.20)
project(ImageProcessing LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------
# vcpkg deps
# -----------------------
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# -----------------------
# ImGui (build from cloned repo)
# -----------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/third_party/imgui)

add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${IMGUI_DIR}/misc/cpp
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_link_libraries(imgui PUBLIC
  glfw
  glad::glad
  glm::glm
)

# -----------------------
# tinyfiledialogs (Open/Save dialogs)
# -----------------------
add_library(tinyfiledialogs STATIC
  src/third_party/tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

# -----------------------
# Main executable
# -----------------------
add_executable(ImageProcessing
  src/main.cpp
  src/app/App.cpp
  src/app/App.h

  src/core/Image.cpp
  src/core/Color.cpp
  src/core/Srgb.cpp

  src/ops/Brightness.cpp
  src/ops/Composite.cpp
  src/ops/BlendModes.cpp

  src/gl/GlUtils.cpp
  src/gl/Shader.cpp
  src/gl/Texture2D.cpp
  src/gl/Quad.cpp

  src/math/Transform2D.h
  src/math/Transform2D.cpp
  src/math/Transform3D.h
  src/math/Transform3D.cpp

  src/pipeline/Assignment4Renderer.h
  src/pipeline/Assignment4Renderer.cpp

  
  
  
  
  
  

  # STB implementations live here (ONE TU)
  src/third_party/stb_impl.cpp
)

target_include_directories(ImageProcessing PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ImageProcessing PRIVATE
  imgui
  tinyfiledialogs
  glfw
  glad::glad
  glm::glm
)

# OpenGL on Windows
if (WIN32)
  target_link_libraries(ImageProcessing PRIVATE opengl32)
endif()

# Warnings
if (MSVC)
  target_compile_options(ImageProcessing PRIVATE /W4)
else()
  target_compile_options(ImageProcessing PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy assets folder next to exe
add_custom_command(TARGET ImageProcessing POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:ImageProcessing>/assets
)