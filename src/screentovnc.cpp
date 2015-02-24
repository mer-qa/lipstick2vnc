/****************************************************************************
 *
 *  screentovnc.cpp - Lipstick2vnc, a VNC server for Mer devices
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
 *
 *  This file incorporates work covered by the following copyright and
 *  permission notice:
 *
 *****************************************************************************
 *
 *  The following functions:
 *  - "makeRichCursor(rfbScreenInfoPtr rfbScreen)" from
 *    "static void MakeRichCursor(rfbScreenInfoPtr rfbScreen)"
 *
 *  - "makeRichCursorTouch(rfbScreenInfoPtr rfbScreen)" from
 *    "static void MakeRichCursor(rfbScreenInfoPtr rfbScreen)"
 *
 *  - "void mouseHandler(int buttonMask, int x, int y, rfbClientPtr cl)"
 *    from "static void doptr(int buttonMask,int x,int y,rfbClientPtr cl)"
 *
 *  - "void clientgone(rfbClientPtr cl)" from
 *    "static void clientgone(rfbClientPtr cl)"
 *
 *  - "rfbNewClientAction newclient(rfbClientPtr cl)" from
 *    "static enum rfbNewClientAction newclient(rfbClientPtr cl)"
 *
 *  are based on the LibVNCServer example.c code, copyright header:
 *
 *    @example example.c
 *    This is an example of how to use libvncserver.
 *
 *    libvncserver example
 *    Copyright (C) 2001 Johannes E. Schindelin <Johannes.Schindelin@gmx.de>
 *
 *    This is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This software is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this software; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *    USA.
 *
 ****************************************************************************/

#include "frameevent.h"
#include "screentovnc.h"

// TODO: make that configurable
#define POINTER_DELAY 10

#define test_bit(a, b) (a[b/8] & (1<<(b%8)))

Recorder *ScreenToVnc::m_recorder;

ScreenToVnc::ScreenToVnc(QObject *parent) :
    QObject(parent)
{
    IN;
    m_allFine = true;
    // TODO: make that configurable?
    exitWhenLastClientGone = false;
    isEmptyMouse = false;
    lastPointerEvent = QDateTime::currentMSecsSinceEpoch();
    lastPointerMove = lastPointerEvent;
    m_repaintTimer = new QTimer(this);
    m_repaintTimer->setSingleShot(true);
    m_repaintTimer->setInterval(300);
    connect(m_repaintTimer,
            SIGNAL(timeout()),
            this,
            SLOT(repaintTimeOut()));

    m_wasRepaintTimeOut = false;

    // Unix Signal Handling set up
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, unixHupSignalFd))
        qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, unixTermSignalFd))
        qFatal("Couldn't create TERM socketpair");

    hupSignalNotifier = new QSocketNotifier(unixHupSignalFd[1], QSocketNotifier::Read, this);
    connect(hupSignalNotifier,
            SIGNAL(activated(int)),
            this,
            SLOT(qtHubSignalHandler()));

    termSignalNotifier = new QSocketNotifier(unixTermSignalFd[1], QSocketNotifier::Read, this);
    connect(termSignalNotifier,
            SIGNAL(activated(int)),
            this,
            SLOT(qtTermSignalHandler()));

    ScreenToVnc::m_recorder = new Recorder(this);

    connect(m_recorder,
            SIGNAL(ready()),
            this,
            SLOT(recorderReady()));

    m_screen = QGuiApplication::screens().first();

    // setup vnc server
    char *argv[0];
    m_server = rfbGetScreen(0,argv, m_screen->size().width(), m_screen->size().height(), 8, 3, m_screen->depth() / 8);

    if(!m_server){
        LOG() << "failed to create VNC server";
        m_allFine = false;
    }

    m_server->desktopName = "Mer VNC";
    m_server->frameBuffer=(char*)malloc(m_screen->size().width() * m_screen->size().height() * (m_screen->depth() / 8));

    m_server->alwaysShared=(1==1);

    m_server->newClientHook = newclient;
    m_server->ptrAddEvent = mouseHandler;

    // check if launched by systemd with a ready socket (LISTEN_FDS env var)
    int sd_fds = sd_listen_fds(1);
    if (sd_fds){
        for (int i = SD_LISTEN_FDS_START; i <= (SD_LISTEN_FDS_START + sd_fds - 1); i++){
            if (sd_is_socket(i, AF_INET6, SOCK_STREAM, 1)
                || sd_is_socket(i, AF_INET, SOCK_STREAM, 1)){
                LOG() << "using given socket at FD:" << i;
                m_server->autoPort = false;
                m_server->port = 0;
                m_server->ipv6port = 0;
                m_server->udpPort = 0;
                m_server->listenSock = i;
                FD_SET(m_server->listenSock, &(m_server->allFds));
                m_server->maxFd = m_server->listenSock;
                exitWhenLastClientGone = true;
            }
        }
    }

    // init the cursors
    init_fingerPointers();
    makeRichCursor(m_server);

    // Initialize the VNC server
    rfbInitServer(m_server);
    if (m_server->listenSock < 0){
        LOG() << "Server is not listening on any sockets! Quit";
        m_allFine = false;
    }

    m_processTimer = new QTimer(this);
    connect(m_processTimer,
            SIGNAL(timeout()),
            this,
            SLOT(rfbProcessTrigger()));

    // open the event device
    isTypeA = true;
    eventDev = -1;

    QDir dir;
    QStringList filters;
    filters << "event*" ;
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);
    dir.setFilter(QDir::System);
    dir.setPath("/dev/input/");

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        LOG() << "probe:" << fileInfo.absoluteFilePath();

        int fd = open(fileInfo.absoluteFilePath().toLocal8Bit().data(), O_RDWR);
        if(fd < 0) {
            LOG() << "can't open:" << fileInfo.absoluteFilePath();
        } else {
            char name[64];
            if (ioctl(fd, EVIOCGNAME(sizeof (name)), name) != -1){
                LOG() << "Device name:" << name;
            }
            input_id device_id;
            if (ioctl(fd, EVIOCGID, &device_id) != -1){
                LOG() << "Vendor:" << device_id.vendor;
                LOG() << "Product:" << device_id.product;
                LOG() << "Version:" << device_id.version;
            }
            unsigned char abscaps[(ABS_MAX / 8) + 1];
            memset(abscaps, '\0', sizeof (abscaps));

            if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof (abscaps)), abscaps) != -1){
                if (test_bit(abscaps, ABS_MT_POSITION_X)
                        && test_bit(abscaps, ABS_MT_POSITION_Y)
                        && test_bit(abscaps, ABS_MT_PRESSURE)){
                    LOG() << fileInfo.absoluteFilePath() << "looks right for a touchscreen type A";
                    isTypeA = true;
                    eventDev = fd;
                    if (test_bit(abscaps, ABS_MT_SLOT)
                            && test_bit(abscaps, ABS_MT_TRACKING_ID)){
                        LOG() << "OK seems to be touchscreen type B";
                        isTypeA = false;
                    }
                    break;
                }
            }
        }
        close(fd);
    }
    if (eventDev < 0){
        m_allFine = false;
    }


    QDBusInterface mceInterface("com.nokia.mce",
                                "/com/nokia/mce/signal",
                                "com.nokia.mce.signal",
                                QDBusConnection::systemBus(),
                                this);

    mceInterface.connection().connect("com.nokia.mce",
                                      "/com/nokia/mce/signal",
                                      "com.nokia.mce.signal",
                                      "display_status_ind",
                                      this,
                                      SLOT(mceBlankHandler(QString)));

    if(getDisplayStatus() == displayOff){
        mceBlankHandler("off");
    }

    if (m_allFine){
        // inform systemd that we started up
        sd_notifyf(0,
                   "READY=1\n"
                   "STATUS=Processing requests...\n"
                   "MAINPID=%lu",
                   (unsigned long) getpid());
    }

    OUT;
}

ScreenToVnc::~ScreenToVnc()
{
    IN;
    close(eventDev);
    free(m_server->frameBuffer);
    rfbScreenCleanup(m_server);
    OUT;
}

bool ScreenToVnc::event(QEvent *e)
{
    IN;

    if (e->type() == FrameEvent::FrameEventType) {
        LOG() << "push frame to vnc buffer";
        FrameEvent *fe = static_cast<FrameEvent *>(e);
        Buffer *buf = fe->buffer;
        QImage img = fe->transform == LIPSTICK_RECORDER_TRANSFORM_Y_INVERTED ? buf->image.mirrored(false, true) : buf->image;
        buf->busy = false;

        if (m_recorder->m_starving)
            m_recorder->recordFrame();

        memcpy(m_server->frameBuffer, img.bits(), img.width() * img.height() * img.depth() / 8);
        rfbMarkRectAsModified(m_server, 0, 0, img.width(), img.height());

        if (m_wasRepaintTimeOut){
            m_wasRepaintTimeOut = false;
        } else {
            m_repaintTimer->start();
        }

        return true;
    }
    return QObject::event(e);
}

void ScreenToVnc::recorderReady()
{
    IN;
    // start the process trigger timers
    m_processTimer->start();
}

void ScreenToVnc::repaintTimeOut()
{
    IN;
    m_wasRepaintTimeOut = true;
    m_recorder->repaint();
}

void ScreenToVnc::rfbProcessTrigger()
{
//    long usec;
//    usec = m_server->deferUpdateTime*1000;

    // TODO: make the 500ms configurable?!
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!isEmptyMouse && now - lastPointerMove > 500) {
        makeEmptyMouse(m_server);
    }

    rfbProcessEvents(m_server, 5000);
}

/****************************************************************************
 * Mouse Pointer Functions
 ****************************************************************************/

void ScreenToVnc::init_fingerPointers()
{
    pointerFingerPtr = rfbMakeXCursor(pointer_finger.width,
                                      pointer_finger.height,
                                      pointer_finger.bitmask,
                                      pointer_finger.bitmask);

    pointerFingerTouchPtr = rfbMakeXCursor(pointer_finger_touch.width,
                                           pointer_finger_touch.height,
                                           pointer_finger_touch.bitmask,
                                           pointer_finger_touch.bitmask);

    emptyMousePtr = rfbMakeXCursor(empty_mouse.width,
                                   empty_mouse.height,
                                   empty_mouse.bitmask,
                                   empty_mouse.bitmask);
}

void ScreenToVnc::mceUnblank()
{
    IN;
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface dbus_iface("com.nokia.mce",
                              "/com/nokia/mce/request",
                              "com.nokia.mce.request",
                              bus);

    dbus_iface.call("req_display_state_on");
}

void ScreenToVnc::mceBlankHandler(QString state)
{
    IN;
    LOG() << "state:" << state;

    if (state == "off"){
        LOG();

        for (int j=m_screen->size().height()-1;j>=0;j--){

            for(int i=m_screen->size().width()-1;i>=0;i--)
                for(int k=2;k>=0;k--)
                    m_server->frameBuffer[(j*m_screen->size().width()+i)*4+k]=0;

            for(int i=m_screen->size().width()*4;i<m_screen->size().width()*4;i++)
                m_server->frameBuffer[j*m_screen->size().width()*4+i]=0;
        }
        rfbMarkRectAsModified(m_server,0,0,m_screen->size().width() ,m_screen->size().height());
    }
}

enum displayState ScreenToVnc::getDisplayStatus()
{
    IN;

    QDBusMessage requestMsg;
    QDBusMessage replyMsg;
    QString status;

    requestMsg = QDBusMessage::createMethodCall("com.nokia.mce",
                                                "/com/nokia/mce/request",
                                                "com.nokia.mce.request",
                                                "get_display_status");

    replyMsg = QDBusConnection::systemBus().call(requestMsg, QDBus::Block, 3000);

    if (replyMsg.type() == QDBusMessage::ErrorMessage) {
        LOG() << "Failed to obtain display status:" << replyMsg.errorMessage();
    }
    else {
        status = replyMsg.arguments()[0].toString();

        if(status == "on")
            return displayOn;
        else
            return displayOff;
    }

    return displayOff;
}

/******************************************************************
 * The functions:
 * - makeEmptyMouse(rfbScreenInfoPtr rfbScreen)
 * - makeRichCursor(rfbScreenInfoPtr rfbScreen)
 * - makeRichCursorTouch(rfbScreenInfoPtr rfbScreen)
 *
 * are based on the example.c example from (GPLv2+)
 * LibVNCServer: http://libvncserver.sourceforge.net/
 ******************************************************************/
void ScreenToVnc::makeEmptyMouse(rfbScreenInfoPtr rfbScreen)
{
    rfbScreen->cursor = emptyMousePtr;
    rfbScreen->cursor->richSource = empty_mouse.pixel_data;

    rfbScreen->cursor->xhot = 1;
    rfbScreen->cursor->yhot = 1;

    updateClientCursors(rfbScreen, true);
}

void ScreenToVnc::makeRichCursor(rfbScreenInfoPtr rfbScreen)
{
    rfbScreen->cursor = pointerFingerPtr;
    rfbScreen->cursor->richSource = pointer_finger.pixel_data;

    rfbScreen->cursor->xhot = 32;
    rfbScreen->cursor->yhot = 32;

    updateClientCursors(rfbScreen, false);
}

void ScreenToVnc::makeRichCursorTouch(rfbScreenInfoPtr rfbScreen)
{
    rfbScreen->cursor = pointerFingerTouchPtr;
    rfbScreen->cursor->richSource = pointer_finger_touch.pixel_data;

    rfbScreen->cursor->xhot = 32;
    rfbScreen->cursor->yhot = 32;

    updateClientCursors(rfbScreen, false);
}

void ScreenToVnc::updateClientCursors(rfbScreenInfoPtr rfbScreen, bool emptyMouse)
{
    rfbClientIteratorPtr iter;
    rfbClientPtr cl;

    iter = rfbGetClientIterator(rfbScreen);
    while( (cl = rfbClientIteratorNext(iter)) ) {
        cl->cursorWasChanged = true;
    }
    rfbReleaseClientIterator(iter);

    isEmptyMouse = emptyMouse;
}

/****************************************************************************
 *
 * buttonMask: bits 0-7 are buttons 1-8, 0=up, 1=down
 * button 1: left    : bit 1 -> 1
 * button 2: middle  : bit 2 -> 2
 * button 3: right   : bit 3 -> 4
 *
 * inspired by:
 * doptr(int buttonMask,int x,int y,rfbClientPtr cl)
 * from example.c example from (GPLv2+)
 * LibVNCServer: http://libvncserver.sourceforge.net/
 *
 ****************************************************************************/

void ScreenToVnc::mouseHandler(int buttonMask, int x, int y, rfbClientPtr cl)
{
    ClientData* cd=(ClientData*)cl->clientData;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    lastPointerMove = now;
    int nextClientId = 0;

    // TODO: smarter way to dedect if in dragMode or not
    switch (buttonMask){
    case 0: /*all buttons up */
        if (cd->dragMode){
            struct input_event event_mt_report, event_end, event_mt_tracking_id;
            memset(&event_mt_report, 0, sizeof(event_mt_report));
            memset(&event_end, 0, sizeof(event_end));
            memset(&event_mt_tracking_id, 0, sizeof(event_mt_tracking_id));

            event_mt_report.type = EV_SYN;
            event_mt_report.code = SYN_MT_REPORT;
            event_mt_report.value = 0;

            event_end.type = EV_SYN;
            event_end.code = SYN_REPORT;
            event_end.value = 0;

            event_mt_tracking_id.type = EV_ABS;
            event_mt_tracking_id.code = ABS_MT_TRACKING_ID;
            event_mt_tracking_id.value = 0xffffffff;

            if (isTypeA) {
                if(write(eventDev, &event_mt_report, sizeof(event_mt_report)) < sizeof(event_mt_report)) {
                    LOG() << "write event_mt_report failed: " << strerror(errno);
                    return;
                }
            } else {
                if(write(eventDev, &event_mt_tracking_id, sizeof(event_mt_tracking_id)) < sizeof(event_mt_tracking_id)) {
                    LOG() << "write event_mt_tracking_id failed: " << strerror(errno);
                    return;
                }
            }

            if(write(eventDev, &event_end, sizeof(event_end)) < sizeof(event_end)) {
                LOG() << "write event_end failed: " << strerror(errno);
                return;
            }
            rfbDefaultPtrAddEvent(buttonMask,x,y,cl);
            cd->dragMode = false;
        }
        makeRichCursor(cl->screen);
        break;
    case 1: /* left button down */
        if(x>=0 && y>=0 && x< cl->screen->width && y< cl->screen->height && now - lastPointerEvent > POINTER_DELAY) {
            struct input_event event_x, event_y, event_pressure, event_mt_report, event_end, event_mt_tracking_id, event_mt_touch_major;
            memset(&event_x, 0, sizeof(event_x));
            memset(&event_y, 0, sizeof(event_y));
            memset(&event_pressure, 0, sizeof(event_pressure));
            memset(&event_mt_report, 0, sizeof(event_mt_report));
            memset(&event_end, 0, sizeof(event_end));
            memset(&event_mt_tracking_id, 0, sizeof(event_mt_tracking_id));
            memset(&event_mt_touch_major, 0, sizeof(event_mt_touch_major));

            nextClientId = cd->eventId + 1;

            event_x.type = EV_ABS;
            event_x.code = ABS_MT_POSITION_X;
            if (isTypeA)
                event_x.value = x*2; // TODO: where is the 2 comming from?
            else
                event_x.value = x;

            event_y.type = EV_ABS;
            event_y.code = ABS_MT_POSITION_Y;
            if (isTypeA)
                event_y.value = y*2; // TODO: where is the 2 comming from?
            else
                event_y.value = y;

            event_pressure.type = EV_ABS;
            event_pressure.code = ABS_MT_PRESSURE;
            event_pressure.value = 68;

            event_mt_report.type = EV_SYN;
            event_mt_report.code = SYN_MT_REPORT;
            event_mt_report.value = 0;

            event_end.type = EV_SYN;
            event_end.code = SYN_REPORT;
            event_end.value = 0;

            event_mt_tracking_id.type = EV_ABS;
            event_mt_tracking_id.code = ABS_MT_TRACKING_ID;
            event_mt_tracking_id.value = nextClientId;

            event_mt_touch_major.type = EV_ABS;
            event_mt_touch_major.code = ABS_MT_TOUCH_MAJOR;
            event_mt_touch_major.value = 0x6;

            if(!isTypeA && !cd->dragMode){
                if(write(eventDev, &event_mt_tracking_id, sizeof(event_mt_tracking_id)) < sizeof(event_mt_tracking_id)) {
                    LOG() << "write event_mt_tracking_id failed: " << strerror(errno);
                    return;
                }
            }

            if(write(eventDev, &event_x, sizeof(event_x)) < sizeof(event_x)) {
                LOG() << "write event_x failed: " << strerror(errno);
                return;
            }

            if(write(eventDev, &event_y, sizeof(event_y)) < sizeof(event_y)) {
                LOG() << "write event_y failed: " << strerror(errno);
                return;
            }

            if(write(eventDev, &event_pressure, sizeof(event_pressure)) < sizeof(event_pressure)) {
                LOG() << "write event_pressure failed: " << strerror(errno);
                return;
            }

            if(isTypeA) {
                if(write(eventDev, &event_mt_report, sizeof(event_mt_report)) < sizeof(event_mt_report)) {
                    LOG() << "write event_mt_report failed: " << strerror(errno);
                    return;
                }
            } else {
                if(write(eventDev, &event_mt_touch_major, sizeof(event_mt_touch_major)) < sizeof(event_mt_touch_major)) {
                    LOG() << "write event_mt_touch_major failed: " << strerror(errno);
                    return;
                }
            }

            if(write(eventDev, &event_end, sizeof(event_end)) < sizeof(event_end)) {
                LOG() << "write event_end failed: " << strerror(errno);
                return;
            }

            makeRichCursorTouch(cl->screen);
            rfbDefaultPtrAddEvent(buttonMask,x,y,cl);
            cd->dragMode = true;
            lastPointerEvent = QDateTime::currentMSecsSinceEpoch();
            cd->eventId = nextClientId;
        }
        break;
    case 4: /* right button down */
        if(x>=0 && y>=0 && x< cl->screen->width && y< cl->screen->height && now - lastPointerEvent > POINTER_DELAY) {
            mceUnblank();
        }
        break;
    default:
        makeRichCursor(cl->screen);
        break;
    }

    cd->oldx=x;
    cd->oldy=y;
    cd->oldButton=buttonMask;
}

/****************************************************************************
 * Client Handling Functions
 ****************************************************************************/
/****************************************************************************
 * The functions:
 * - clientgone(rfbClientPtr cl)
 * - newclient(rfbClientPtr cl)
 *
 * are based on the example.c example from
 * LibVNCServer: http://libvncserver.sourceforge.net/
 ****************************************************************************/
void ScreenToVnc::clientgone(rfbClientPtr cl)
{
    IN;

    rfbScreenInfoPtr screen = cl->screen;
    free(cl->clientData);

    if (screen->clientHead == NULL && exitWhenLastClientGone){
        QCoreApplication::exit(0);
    }

}

rfbNewClientAction ScreenToVnc::newclient(rfbClientPtr cl)
{
    IN;

    bool allowConnection = false;

    // TODO: make that configurable, usb device interface is not always rndis0!
    QNetworkInterface usbIf = QNetworkInterface::interfaceFromName("rndis0");

    QHostAddress remoteAddr = QHostAddress(QString::fromLatin1(cl->host));

    if (remoteAddr.protocol() == QAbstractSocket::IPv6Protocol
        && remoteAddr.toString().startsWith("::ffff:")){
        // this is an IPv4-mapped IPv6 address
        // see: http://www.tcpipguide.com/free/t_IPv6IPv4AddressEmbedding-2.htm
        QString remoteAddrIPv4 = remoteAddr.toString().remove("::ffff:");
        LOG() << "remoteAddrIPv4" << remoteAddrIPv4;
        remoteAddr = QHostAddress(remoteAddrIPv4);
    }

    foreach (QNetworkAddressEntry entry, usbIf.addressEntries()){
        if (remoteAddr.protocol() == entry.ip().protocol()
            && remoteAddr.isInSubnet(entry.ip(), entry.prefixLength())){
            allowConnection = true;
        }
    }

    if (allowConnection){
        cl->clientData = (void*)calloc(sizeof(ClientData),1);
        ClientData* cd=(ClientData*)cl->clientData;
        cd->dragMode = false;
        cd->eventId = 0;
        cl->clientGoneHook = clientgone;
        m_recorder->repaint();
        return RFB_CLIENT_ACCEPT;
    } else {
        LOG() << "RFB_CLIENT_REFUSE";
        cl->clientGoneHook = clientgone;
        return RFB_CLIENT_REFUSE;
    }
}

/****************************************************************************
 * Unix Signal Handler Functions
 ****************************************************************************/
int ScreenToVnc::unixHupSignalFd[2];
int ScreenToVnc::unixTermSignalFd[2];

void ScreenToVnc::unixHupSignalHandler(int)
{
    IN;
    LOG() << "HUP Signal received";
    char a = '1';
    ::write(unixHupSignalFd[0], &a, sizeof(a));
}

void ScreenToVnc::unixTermSignalHandler(int)
{
    IN;
    LOG() << "TERM Signal received";
    char a = '2';
    ::write(unixTermSignalFd[0], &a, sizeof(a));
}

void ScreenToVnc::qtTermSignalHandler()
{
    IN;
    termSignalNotifier->setEnabled(false);
    char tmp;
    ::read(unixTermSignalFd[1], &tmp, sizeof(tmp));

    LOG() << "TERM Signal received, about to exit...";
    m_processTimer->stop();

    termSignalNotifier->setEnabled(true);

    QCoreApplication::exit(0);
}

void ScreenToVnc::qtHubSignalHandler()
{
    IN;
    hupSignalNotifier->setEnabled(false);
    char tmp;
    ::read(unixHupSignalFd[1], &tmp, sizeof(tmp));

    LOG() << "HUP Signal received, currently do nothing...";

    hupSignalNotifier->setEnabled(true);
}
