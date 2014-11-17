
#ifndef LIPSTICKRECORDER_RECORDER_H
#define LIPSTICKRECORDER_RECORDER_H

#include <QObject>
#include <QMutex>
#include <wayland-client.h>

#include "screentovnc.h"
#include "buffer.h"
#include "frameevent.h"

class QScreen;

struct wl_display;
struct wl_registry;
struct lipstick_recorder_manager;
struct lipstick_recorder;

class Buffer;
class BuffersHandler;
class ScreenToVnc;

class Recorder : public QObject
{
    Q_OBJECT
public:
    Recorder(ScreenToVnc *screenToVnc);
    ~Recorder();
    void recordFrame();
    void repaint();

    bool m_starving;

private slots:
    void start();

private:
    static void global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void globalRemove(void *data, wl_registry *registry, uint32_t id);
    static void frame(void *data, lipstick_recorder *recorder, wl_buffer *buffer, uint32_t time, int transform);
    static void failed(void *data, lipstick_recorder *recorder, int result, wl_buffer *buffer);
    static void cancel(void *data, lipstick_recorder *recorder, wl_buffer *buffer);

    ScreenToVnc *m_screenToVnc;

    wl_display *m_display;
    wl_registry *m_registry;
    wl_shm *m_shm;
    lipstick_recorder_manager *m_manager;
    lipstick_recorder *m_lipstickRecorder;
    QScreen *m_screen;
    QList<Buffer *> m_buffers;
//    QThread *m_buffersThread;
//    BuffersHandler *m_buffersHandler;
    QMutex m_mutex;

    friend class BuffersHandler;
};

#endif
