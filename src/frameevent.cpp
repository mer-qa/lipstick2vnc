#include <QEvent>

#include "frameevent.h"

const QEvent::Type FrameEvent::FrameEventType = (QEvent::Type)QEvent::registerEventType();

FrameEvent::FrameEvent(Buffer *b, uint32_t time, int tr)
    : QEvent(FrameEventType)
    , buffer(b)
    , timestamp(time)
    , transform(tr)
{
    IN;
}
