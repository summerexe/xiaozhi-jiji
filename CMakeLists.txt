# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "2.1.0")

# Suppress known third-party / IDF noise:
# - missing-field-initializers: struct init style in generated code
# - macro-redefined: espressif/esp_video linux/ioctl.h vs lwIP sockets.h (_IO/_IOR/_IOW)
add_compile_options(-Wno-missing-field-initializers -Wno-macro-redefined)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiaozhi)

