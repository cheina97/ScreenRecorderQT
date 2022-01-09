cmake_minimum_required(VERSION 3.0.0)
project("ScreenRecorder" VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_VERBOSE_MAKEFILE OFF) # VERBOSE BUILD
add_compile_options(--std=c++20)

add_executable(main main.cpp GetAudioDevices.cpp MemoryCheckLinux.cpp ScreenRecorder.cpp) 

################### COMMON ###################
include(CPack)

# Threads lib
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
##############################################

SET(CMAKE_CXX_FLAGS_DEBUG "-0 -g")
MARK_AS_ADVANCED(CMAKE_CXX_FLAGS_DEBUG)

############### START APPLE ##################
IF(APPLE)
find_package(PkgConfig REQUIRED)
include_directories(BEFORE
        usr/local/opt
        /usr/local/include/
        #/usr/local/include/libavcodec
        #/usr/local/include/libavdevice
        #/usr/local/include/libavfilter
        #/usr/local/include/libavformat
        #/usr/local/include/libavresample
        #/usr/local/include/libavutil
        /usr/local/lib
        /usr/X11R6/include
        /usr/local/Cellar/libav/12.3_8/include
        /usr/local/Cellar/libav/12.3_8/lib
        /usr/X11R6/lib
        /opt/X11/include)
target_link_libraries(main PRIVATE -I/usr/local/lib -L/usr/X11R6/lib -I/usr/X11R6/include -lX11 -I/opt/X11/include)
target_link_libraries(main PRIVATE Threads::Threads PkgConfig::LIBAV)#PkgConfig::LIBAV
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
)
ENDIF(APPLE)
##############################################

#libav
#find_library(AV_LIBRARY avdevice avformat avcodec swscale avutil)
#target_link_libraries( main PRIVATE ${AV_LIBRARY})



