#include "buffer.h"

Buffer *Buffer::create(wl_shm *shm, QScreen *screen)
{
    IN;
    int width = screen->size().width();
    int height = screen->size().height();
    int stride = width * 4;
    int size = stride * height;

    char filename[] = "/tmp/lipstick-recorder-shm-XXXXXX";
    int fd = mkstemp(filename);
    if (fd < 0) {
        qWarning("creating a buffer file for %d B failed: %m\n", size);
        return Q_NULLPTR;
    }
    int flags = fcntl(fd, F_GETFD);
    if (flags != -1)
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

    if (ftruncate(fd, size) < 0) {
        qWarning("ftruncate failed: %s", strerror(errno));
        close(fd);
        return Q_NULLPTR;
    }

    uchar *data = (uchar *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unlink(filename);
    if (data == (uchar *)MAP_FAILED) {
        qWarning("mmap failed: %m\n");
        close(fd);
        return Q_NULLPTR;
    }

    Buffer *buf = new Buffer;

    wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    buf->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_buffer_set_user_data(buf->buffer, buf);
    wl_shm_pool_destroy(pool);
    buf->data = data;
    buf->image = QImage(data, width, height, stride, QImage::Format_RGBA8888);
    buf->busy = false;
    close(fd);
    return buf;
}
