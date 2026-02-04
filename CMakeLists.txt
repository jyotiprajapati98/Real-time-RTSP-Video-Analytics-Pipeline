cmake_minimum_required(VERSION 3.14)
project(RTSP_Analytics_Pipeline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED COMPONENTS core dnn videoio imgproc imgcodecs highgui)

# Find FFmpeg
# Note: Find modules for FFmpeg are often custom. 
# We'll use pkg-config or standard paths if available, or a simple find_library approach.
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

# Find PostgreSQL (libpq)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED libpq)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWSCALE_INCLUDE_DIRS}
    ${LIBPQ_INCLUDE_DIRS}
    src
)

link_directories(
    ${AVCODEC_LIBRARY_DIRS}
    ${AVFORMAT_LIBRARY_DIRS}
    ${AVUTIL_LIBRARY_DIRS}
    ${LIBPQ_LIBRARY_DIRS}
)

add_executable(rtsp_pipeline
    src/main.cpp
    src/RTSPStreamer.cpp
    src/YoloDetector.cpp
    src/HLSRecorder.cpp
    src/DatabaseHandler.cpp
)

target_link_libraries(rtsp_pipeline
    ${OpenCV_LIBS}
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWSCALE_LIBRARIES}
    ${LIBPQ_LIBRARIES}
    pthread
)
