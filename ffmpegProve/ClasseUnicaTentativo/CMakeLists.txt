cmake_minimum_required(VERSION 3.0.0)
project(ClasseUnica VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
add_compile_options(--std=c++20)

add_executable(main main.cpp GetAudioDevices.cpp MemoryCheckLinux.cpp ScreenRecorder.cpp) 

#threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(main PRIVATE Threads::Threads PkgConfig::LIBAV)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
)
#libav
#find_library(AV_LIBRARY avdevice avformat avcodec swscale avutil)
#target_link_libraries( main PRIVATE ${AV_LIBRARY})

include_directories(${FFMPEG_DEV_ROOT}/include)
include_directories(/opt/homebrew/include)
link_directories(${FFMPEG_DEV_ROOT}/lib)
link_libraries(
    avcodec
    avformat
    avfilter
    avdevice
    swresample
    swscale
    avutil
)

include(CPack)


include_directories(
        BEFORE /usr/X11R6/include /usr/X11R6/lib /opt/X11/include /opt/homebrew/include
)

target_link_libraries(main PRIVATE
        -L/usr/X11R6/lib -I/usr/X11R6/include -lX11 -I/opt/X11/include -I/opt/homebrew/include
        )


