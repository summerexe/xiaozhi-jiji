# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in this exact order for cmake
# to work correctly
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "2.1.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiaozhi)

# Struct init style in third-party / generated code only
add_compile_options(-Wno-missing-field-initializers)
