#ifndef FRAMEEVENT_H
#define FRAMEEVENT_H

#include <QEvent>

#include "buffer.h"


class FrameEvent : public QEvent
{
public:
    FrameEvent(Buffer *b, uint32_t time, int tr);
    Buffer *buffer;
    uint32_t timestamp;
    int transform;
    static const QEvent::Type FrameEventType;
};

#endif // FRAMEEVENT_H
