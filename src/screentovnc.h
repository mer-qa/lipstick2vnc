/****************************************************************************
 *
 *  screentovnc.h - Lipstick2vnc, a VNC server for Mer devices
 *  Copyright (C) 2014 Jolla Ltd.
 *  Contact: Reto Zingg <reto.zingg@jolla.com>
 *
 *  This file is part of Lipstick2vnc.
 *
 *  Lipstick2vnc is free software; you can redistribute it and/or modify
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

#ifndef SCREENTOVNC_H
#define SCREENTOVNC_H

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/input.h>

#include <systemd/sd-daemon.h>

#include <QGuiApplication>
#include <QDateTime>
#include <QImage>
#include <QNetworkInterface>
#include <QObject>
#include <QScreen>
#include <QSocketNotifier>
#include <QtDBus/QtDBus>
#include <QTimer>

#include "recorder.h"
#include "logging.h"
#include "empty_mouse.h"
#include "pointer_finger.h"
#include "pointer_finger_touch.h"
#include "buffer.h"
#include "frameevent.h"

#include "wayland-lipstick-recorder-client-protocol.h"

class Recorder;

extern "C"{
#include <rfb/rfb.h>

typedef struct ClientData {
    rfbBool oldButton;
    int oldx,oldy;
    bool dragMode;
    int eventId;
} ClientData;
}

enum Orientation {
  Portrait,
  Landscape,
  PortraitInverted,
  LandscapeInverted
};

// static vars
static rfbCursor *emptyMousePtr;
static rfbCursor *pointerFingerPtr;
static rfbCursor *pointerFingerTouchPtr;
static qint64 lastPointerEvent;
static qint64 lastPointerMove;
static int eventDev;
static bool exitWhenLastClientGone;
static bool isEmptyMouse;
static bool isTypeA;
static bool hasAbsMtPressure;
static int mtPressureValue;
static bool hasAbsMtTrackingId;
static bool hasAbsMtTouchMajor;
static bool hasAbsMtWidthMajor;
static bool hasBntTouch;
static float mtAbsCorrecturX;
static float mtAbsCorrecturY;
static int uinputKeyboardDeviceFD;

enum displayState{
    displayOn,
    displayOff
};

class ScreenToVnc : public QObject
{
    Q_OBJECT
public:
    explicit ScreenToVnc(QObject *parent = 0, bool smoothScaling = false, float scalingFactor = 1, Orientation orientation = Portrait, int usec = 5000, int buffers = 2, int processTimerInterval = 0, bool doMouseHandling = true);
    ~ScreenToVnc();

    bool event(QEvent *e) Q_DECL_OVERRIDE;

    // Unix signal handlers.
    static void unixHupSignalHandler(int unused);
    static void unixTermSignalHandler(int unused);

    static Recorder *m_recorder;
    static Orientation m_orientation;
    bool m_allFine;

signals:

public slots:
    // QTimer triggered processing
    void rfbProcessTrigger();

    // Qt unix signal handlers.
    void qtHubSignalHandler();
    void qtTermSignalHandler();

    void mceBlankHandler(QString state);

    void recorderReady();
    void repaintTimeOut();

private: 
    // Unix Signal Handler vars
    static int unixHupSignalFd[2];
    static int unixTermSignalFd[2];
    QSocketNotifier *hupSignalNotifier;
    QSocketNotifier *termSignalNotifier;

    rfbScreenInfoPtr m_server;
    QTimer *m_processTimer;

    QScreen *m_screen;

    QTimer *m_repaintTimer;

    // mouse handling
    static void init_fingerPointers();
    static void mceUnblank();
    static void makeEmptyMouse(rfbScreenInfoPtr rfbScreen);
    static void makeRichCursor(rfbScreenInfoPtr rfbScreen);
    static void makeRichCursorTouch(rfbScreenInfoPtr rfbScreen);
    static void updateClientCursors(rfbScreenInfoPtr rfbScreen, bool emptyMouse);
    static void keyboardHandler(rfbBool down, rfbKeySym k, rfbClientPtr cl);
    static void mouseHandler(int buttonMask,int x,int y,rfbClientPtr cl);
    static void uinputCreateKeyboardDevice();
    static void emitKeystroke(int type, int code, int val);

    // client handling
    static void clientgone(rfbClientPtr cl);
    static enum rfbNewClientAction newclient(rfbClientPtr cl);

    enum displayState getDisplayStatus();

    bool m_smoothScaling;
    float m_scalingFactor;
    int m_usec;
    int m_screen_height;
    int m_screen_width;
    bool m_doMouseHandling;
    bool m_isScreenBlank;
};

#endif // SCREENTOVNC_H
