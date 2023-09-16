TEMPLATE = subdirs

src_fb.subdir = src
src_fb.target = sub-src


SUBDIRS = src_fb

OTHER_FILES += \
    rpm/lipstick2vnc.spec \
    data/vnc.socket \
    data/vnc.service \
    data/20-lipstick2vnc-configurator \
    data/cursor_pointer.png \
    data/cursor_pointer_touch.png \
    data/cursor_empty.png

systemd_vnc.files = \
    data/vnc.socket \
    data/vnc.service

systemd_vnc.path = /usr/lib/systemd/user/

oneshot.files = data/20-lipstick2vnc-configurator
oneshot.path  = /usr/lib/oneshot.d/

cursor_image.files = \
    data/cursor_pointer.png \
    data/cursor_pointer_touch.png \
    data/cursor_empty.png

cursor_image.path = /usr/share/lipstick2vnc/

INSTALLS = systemd_vnc oneshot cursor_image
