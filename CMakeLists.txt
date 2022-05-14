cmake_minimum_required(VERSION 3.0.0)
project(ScreenRecorderQT)

# one cmake to rule them all

add_subdirectory("ScreenRecorderLegacy")

if(APPLE)
    add_subdirectory("ScreenRecorderLegacyMacOS")
endif()

add_subdirectory("ScreenRecorderQT/ScreenRecorderQT")
