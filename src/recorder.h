
#ifndef LIPSTICKRECORDER_RECORDER_H
#define LIPSTICKRECORDER_RECORDER_H

#include <QObject>
#include <QMutex>
#include <wayland-client.h>

class QScreen;

struct wl_display;
struct wl_registry;
struct lipstick_recorder_manager;
struct lipstick_recorder;

class Buffer;
class BuffersHandler;

class Recorder : public QObject
{
    Q_OBJECT
public:
    Recorder();
    ~Recorder();

private slots:
    void start();
    void recordFrame();

private:
    static void global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void globalRemove(void *data, wl_registry *registry, uint32_t id);
    static void frame(void *data, lipstick_recorder *recorder, int result, wl_buffer *buffer, uint32_t time);
    static void cancel(void *data, lipstick_recorder *recorder, wl_buffer *buffer);

    wl_display *m_display;
    wl_registry *m_registry;
    wl_shm *m_shm;
    lipstick_recorder_manager *m_manager;
    lipstick_recorder *m_recorder;
    QScreen *m_screen;
    QList<Buffer *> m_buffers;
    bool m_starving;
    QThread *m_buffersThread;
    BuffersHandler *m_buffersHandler;
    QMutex m_mutex;

    friend class BuffersHandler;
};

#endif
