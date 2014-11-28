/****************************************************************************
 *
 *  recorder.cpp - Mervncserver, a VNC server for Mer devices
 *  Copyright (C) 2014 Jolla Ltd.
 *  Contact: Reto Zingg <reto.zingg@jolla.com>
 *
 *  This file is part of Mervncserver.
 *
 *  Mervncserver is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 ****************************************************************************/

#include <QGuiApplication>
#include <QScreen>
#include <QImage>
#include <qpa/qplatformnativeinterface.h>
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>

#include "wayland-lipstick-recorder-client-protocol.h"
#include "recorder.h"
#include "logging.h"


//class BuffersHandler : public QObject
//{
//public:
//    bool event(QEvent *e) Q_DECL_OVERRIDE
//    {
//        IN << e->type() << FrameEvent::FrameEventType;
//        if (e->type() == FrameEvent::FrameEventType) {
//            LOG() << "save frame";
//            FrameEvent *fe = static_cast<FrameEvent *>(e);
//            Buffer *buf = fe->buffer;
//            static int id = 0;
//            QElapsedTimer timer;
//            timer.start();
//            QImage img = fe->transform == LIPSTICK_RECORDER_TRANSFORM_Y_INVERTED ? buf->image.mirrored(false, true) : buf->image;
//            buf->busy = false;
//            if (rec->m_starving)
//                rec->recordFrame();
//            int t1 = timer.restart();
//            img.save(QString("frame%1.bmp").arg(id++, 3, 10, QChar('0')));
//            qDebug()<<t1<<timer.elapsed();
//            QMutexLocker lock(&rec->m_mutex);
//                        qDebug()<<"fe"<<buf<<rec->m_starving<<id;
//            return true;
//        }
//        return QObject::event(e);
//    }

//    Recorder *rec;
//};

static void callback(void *data, wl_callback *cb, uint32_t time)
{
    IN;
    Q_UNUSED(time)
    wl_callback_destroy(cb);
    QMetaObject::invokeMethod(static_cast<Recorder *>(data), "start");
}

Recorder::Recorder(ScreenToVnc *screenToVnc)
        : QObject()
        , m_manager(Q_NULLPTR)
        , m_starving(false)
{
    IN;
    m_screenToVnc = screenToVnc;
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
    m_registry = wl_display_get_registry(m_display);

    static const wl_registry_listener registryListener = {
        global,
        globalRemove
    };
    wl_registry_add_listener(m_registry, &registryListener, this);

    wl_callback *cb = wl_display_sync(m_display);
    static const wl_callback_listener callbackListener = {
        callback
    };
    wl_callback_add_listener(cb, &callbackListener, this);

//    m_buffersThread = new QThread;
//    m_buffersHandler = new BuffersHandler;
//    m_buffersHandler->rec = this;
//    m_buffersHandler->moveToThread(m_buffersThread);
//    m_buffersThread->start();
    OUT;
}

Recorder::~Recorder()
{
    IN;
//    m_buffersThread->quit();
//    m_buffersThread->wait();
    lipstick_recorder_destroy(m_lipstickRecorder);
//    delete m_buffersHandler;
//    delete m_buffersThread;
}

void Recorder::start()
{
    IN;
    if (!m_manager)
        qFatal("The lipstick_recorder_manager global is not available.");

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_screen = QGuiApplication::screens().first();
    wl_output *output = static_cast<wl_output *>(native->nativeResourceForScreen("output", m_screen));

    m_lipstickRecorder = lipstick_recorder_manager_create_recorder(m_manager, output);
    static const lipstick_recorder_listener recorderListener = {
        frame,
        failed,
        cancel
    };
    lipstick_recorder_add_listener(m_lipstickRecorder, &recorderListener, this);

    for (int i = 0; i < 6; ++i) {
        Buffer *buffer = Buffer::create(m_shm, m_screen);
        if (!buffer)
            qFatal("Failed to create a buffer.");
        m_buffers << buffer;
    }
    recordFrame();
    emit ready();
}

void Recorder::recordFrame()
{
    IN;
    Buffer *buf = Q_NULLPTR;
    foreach (Buffer *b, m_buffers) {
        if (!b->busy) {
            buf = b;
            break;
        }
    }
    if (buf) {
        lipstick_recorder_record_frame(m_lipstickRecorder, buf->buffer);
        wl_display_flush(m_display);
        buf->busy = true;
        m_starving = false;
        qDebug()<<"request"<<buf;
    } else {
        qWarning("No free buffers.");
        m_starving = true;
    }
}

void Recorder::repaint()
{
    IN;
    lipstick_recorder_repaint(m_lipstickRecorder);
}

void Recorder::frame(void *data, lipstick_recorder *recorder, wl_buffer *buffer, uint32_t timestamp, int transform)
{
    IN;
    Q_UNUSED(recorder)

    Recorder *rec = static_cast<Recorder *>(data);
    static uint32_t time = 0;

    QMutexLocker lock(&rec->m_mutex);
    rec->recordFrame();
    Buffer *buf = static_cast<Buffer *>(wl_buffer_get_user_data(buffer));

    qDebug()<<"frame"<<timestamp - time<<buf;
    time = timestamp;

//    qApp->postEvent(rec->m_buffersHandler, new FrameEvent(buf, timestamp, transform));
    qApp->postEvent(rec->m_screenToVnc, new FrameEvent(buf, timestamp, transform));
}

void Recorder::failed(void *data, lipstick_recorder *recorder, int result, wl_buffer *buffer)
{
    IN;
    Q_UNUSED(data)
    Q_UNUSED(recorder)
    Q_UNUSED(buffer)

    qFatal("Failed to record a frame, result %d.", result);
}

void Recorder::cancel(void *data, lipstick_recorder *recorder, wl_buffer *buffer)
{
    IN;
    Q_UNUSED(recorder)

    Recorder *rec = static_cast<Recorder *>(data);

    QMutexLocker lock(&rec->m_mutex);
    Buffer *buf = static_cast<Buffer *>(wl_buffer_get_user_data(buffer));
    buf->busy = false;
}

void Recorder::global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    IN;
    Q_UNUSED(registry)

    Recorder *rec = static_cast<Recorder *>(data);
    if (strcmp(interface, "lipstick_recorder_manager") == 0) {
        rec->m_manager = static_cast<lipstick_recorder_manager *>(wl_registry_bind(registry, id, &lipstick_recorder_manager_interface, qMin(version, 1u)));
    } else if (strcmp(interface, "wl_shm") == 0) {
        rec->m_shm = static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, qMin(version, 1u)));
    }
}

void Recorder::globalRemove(void *data, wl_registry *registry, uint32_t id)
{
    IN;
    Q_UNUSED(data)
    Q_UNUSED(registry)
    Q_UNUSED(id)
}
