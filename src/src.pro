QT += core
QT -= gui

TEMPLATE = app
TARGET = mervncserver
target.path += /usr/bin
INSTALLS = target

CONFIG   += link_pkgconfig
PKGCONFIG += libvncserver libsystemd-daemon

SOURCES += \
    screentovnc.cpp \
    main.cpp

HEADERS += \
    screentovnc.h \
    logging.h \
    pointer_finger.h \
    pointer_finger_touch.h

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

DEFINES += SD_DAEMON_DISABLE_MQ
