QT += core dbus network
# needed for qpa/qplatformnativeinterface.h
QT += platformsupport-private

TEMPLATE = app
TARGET = mervncserver
target.path += /usr/bin
INSTALLS = target

CONFIG   += link_pkgconfig wayland-scanner
PKGCONFIG += libvncserver libsystemd-daemon wayland-client
WAYLANDCLIENTSOURCES += protocol/lipstick-recorder.xml


SOURCES += \
    screentovnc.cpp \
    main.cpp \
    recorder.cpp \
    frameevent.cpp \
    buffer.cpp


HEADERS += \
    screentovnc.h \
    logging.h \
    pointer_finger.h \
    pointer_finger_touch.h \
    empty_mouse.h \
    recorder.h \
    frameevent.h \
    buffer.h


CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}
