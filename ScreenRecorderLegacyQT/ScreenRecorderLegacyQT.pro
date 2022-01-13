TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        GetAudioDevices.cpp \
        MemoryCheckLinux.cpp \
        ScreenRecorder.cpp \
        main.cpp

HEADERS += \
    GetAudioDevices.h \
    MemoryCheckLinux.h \
    ScreenRecorder.h

win32:LIBS += -lpthread -lole32 -loleaut32

win32: LIBS += -L$$PWD/lib/ -lavcodec -lavcodec  -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale

unix:LIBS += -lavformat -lavcodec -lavutil -lavdevice -lm -lswscale -lX11 -lpthread -lswresample -lasound

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include
