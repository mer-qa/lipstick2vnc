#ifndef BUFFER_H
#define BUFFER_H

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <QImage>
#include <QScreen>
#include <wayland-client.h>

#include "logging.h"

class Buffer
{
public:
    static Buffer *create(wl_shm *shm, QScreen *screen);

    wl_buffer *buffer;
    uchar *data;
    QImage image;
    bool busy;
};

#endif // BUFFER_H
