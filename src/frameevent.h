#ifndef FRAMEEVENT_H
#define FRAMEEVENT_H

#include <QEvent>

#include "buffer.h"


class FrameEvent : public QEvent
{
public:
    FrameEvent(Buffer *b);
    Buffer *buffer;
    static const QEvent::Type FrameEventType;
};

#endif // FRAMEEVENT_H
