#include <QEvent>

#include "frameevent.h"

const QEvent::Type FrameEvent::FrameEventType = (QEvent::Type)QEvent::registerEventType();

FrameEvent::FrameEvent(Buffer *b)
    : QEvent(FrameEventType)
    , buffer(b)
{
    IN;
}
