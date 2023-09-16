QT += core dbus network
# needed for qpa/qplatformnativeinterface.h
QT += gui-private

TEMPLATE = app
TARGET = lipstick2vnc
target.path += /usr/bin
INSTALLS = target

CONFIG   += link_pkgconfig wayland-scanner
PKGCONFIG += libvncserver libsystemd wayland-client
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
    recorder.h \
    frameevent.h \
    buffer.h


CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}
