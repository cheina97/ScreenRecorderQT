cmake_minimum_required(VERSION 3.0.0)
project(ScreenRecorderQT VERSION 0.1.0)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE) 

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Multimedia CONFIG REQUIRED)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to create code from Qt resource files (*.qrc)
set(CMAKE_AUTORCC ON)
# UI ==> H generator from QT   # Instruct CMake to create code from Qt designer ui files
set(CMAKE_AUTOUIC ON)


add_executable(ScreenRecorderQT 
    main.cpp
    AreaSelector.cpp             AreaSelector.h 
    AreaSelectorButtons.cpp      AreaSelectorButtons.h 
    GetAudioDevices.cpp          GetAudioDevices.h   
    MemoryCheckLinux.cpp         MemoryCheckLinux.h 
    ScreenRecorder.cpp           ScreenRecorder.h 
    mainwindow.cpp               mainwindow.h               mainwindow.ui
    resource.qrc )

target_link_libraries(ScreenRecorderQT  
    Qt5::Widgets  Qt5::Multimedia 
    avcodec avcodec  avdevice avfilter avformat avutil postproc swresample swscale )


add_subdirectory("example")