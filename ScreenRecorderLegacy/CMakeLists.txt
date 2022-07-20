cmake_minimum_required(VERSION 3.0.0)
project(ScreenRecorderLegacy VERSION 0.1.0)


add_executable(ScreenRecorderLegacy 
    main.cpp
    GetAudioDevices.cpp     GetAudioDevices.h
    MemoryCheckLinux.cpp    MemoryCheckLinux.h 
    ScreenRecorder.cpp      ScreenRecorder.h )

target_link_libraries(ScreenRecorderLegacy 
    avcodec avcodec  avdevice avfilter avformat avutil postproc swresample swscale )    

