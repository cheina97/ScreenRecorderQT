QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AreaSelector.cpp \
    AreaSelectorButtons.cpp \
    GetAudioDevices.cpp \
    MemoryCheckLinux.cpp \
    ScreenRecorder.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    AreaSelector.h \
    AreaSelectorButtons.h \
    GetAudioDevices.h \
    MemoryCheckLinux.h \
    ScreenRecorder.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

LIBS += -lpthread

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lavcodec
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lavcodecd
else:unix: LIBS += -L$$PWD/../lib/ -lavcodec

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lavdevice
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lavdeviced
else:unix: LIBS += -L$$PWD/../lib/ -lavdevice

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lavfilter
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lavfilterd
else:unix: LIBS += -L$$PWD/../lib/ -lavfilter

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lavformat
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lavformatd
else:unix: LIBS += -L$$PWD/../lib/ -lavformat

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lavutil
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lavutild
else:unix: LIBS += -L$$PWD/../lib/ -lavutil

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lpostproc
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lpostprocd
else:unix: LIBS += -L$$PWD/../lib/ -lpostproc

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lswresample
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lswresampled
else:unix: LIBS += -L$$PWD/../lib/ -lswresample

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lswscale
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lswscaled
else:unix: LIBS += -L$$PWD/../lib/ -lswscale

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include
