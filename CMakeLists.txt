# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in this exact order for cmake
# to work correctly
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "2.1.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiaozhi)

# Root-cause fix for esp_video vs lwIP: copy guarded linux/ioctl.h after deps resolve.
# (JijiFace / display code did not cause this — it's V4L2 headers vs lwip/sockets.h.)
if(DEFINED ENV{PYTHON})
  set(_JIJI_PYTHON "$ENV{PYTHON}")
else()
  find_program(_JIJI_PYTHON python3 REQUIRED)
endif()
execute_process(
  COMMAND ${_JIJI_PYTHON} "${CMAKE_SOURCE_DIR}/scripts/patch_esp_video_ioctl.py"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE _jiji_ioctl_patch_rc
)

# Struct init style in third-party / generated code only
add_compile_options(-Wno-missing-field-initializers)
