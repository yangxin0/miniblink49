// macOS backend (part 3): NSEvent -> blink WebInputEvent. See header.
#include "port/mac/web_input_event_mac.h"

#include "public/web/WebInputEvent.h"
#import <Cocoa/Cocoa.h>

namespace miniblink {
namespace mac {

int WebModifiersFromCocoaFlags(unsigned long long flags) {
    int modifiers = 0;
    if (flags & NSEventModifierFlagShift)
        modifiers |= blink::WebInputEvent::ShiftKey;
    if (flags & NSEventModifierFlagControl)
        modifiers |= blink::WebInputEvent::ControlKey;
    if (flags & NSEventModifierFlagOption)
        modifiers |= blink::WebInputEvent::AltKey;
    if (flags & NSEventModifierFlagCommand)
        modifiers |= blink::WebInputEvent::MetaKey;
    if (flags & NSEventModifierFlagCapsLock)
        modifiers |= blink::WebInputEvent::CapsLockOn;
    return modifiers;
}

bool BuildWebMouseEvent(NSEvent* event, double viewHeight, blink::WebMouseEvent* out) {
    if (!event || !out)
        return false;

    blink::WebInputEvent::Type type;
    blink::WebMouseEvent::Button button = blink::WebMouseEvent::ButtonNone;
    switch ([event type]) {
    case NSEventTypeLeftMouseDown:  type = blink::WebInputEvent::MouseDown; button = blink::WebMouseEvent::ButtonLeft; break;
    case NSEventTypeLeftMouseUp:    type = blink::WebInputEvent::MouseUp;   button = blink::WebMouseEvent::ButtonLeft; break;
    case NSEventTypeRightMouseDown: type = blink::WebInputEvent::MouseDown; button = blink::WebMouseEvent::ButtonRight; break;
    case NSEventTypeRightMouseUp:   type = blink::WebInputEvent::MouseUp;   button = blink::WebMouseEvent::ButtonRight; break;
    case NSEventTypeOtherMouseDown: type = blink::WebInputEvent::MouseDown; button = blink::WebMouseEvent::ButtonMiddle; break;
    case NSEventTypeOtherMouseUp:   type = blink::WebInputEvent::MouseUp;   button = blink::WebMouseEvent::ButtonMiddle; break;
    case NSEventTypeMouseMoved:
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
        type = blink::WebInputEvent::MouseMove;
        if ([event type] == NSEventTypeLeftMouseDragged) button = blink::WebMouseEvent::ButtonLeft;
        else if ([event type] == NSEventTypeRightMouseDragged) button = blink::WebMouseEvent::ButtonRight;
        else if ([event type] == NSEventTypeOtherMouseDragged) button = blink::WebMouseEvent::ButtonMiddle;
        break;
    default:
        return false;
    }

    const NSPoint loc = [event locationInWindow];  // window space, bottom-left
    const int x = (int)loc.x;
    const int y = (int)(viewHeight - loc.y);       // -> top-left (blink) space

    out->type = type;
    out->button = button;
    out->modifiers = WebModifiersFromCocoaFlags((unsigned long long)[event modifierFlags]);
    out->timeStampSeconds = [event timestamp];
    out->x = x;
    out->y = y;
    out->windowX = x;
    out->windowY = y;
    out->globalX = x;  // screen mapping refined once a real window is wired
    out->globalY = y;
    out->clickCount = (type == blink::WebInputEvent::MouseDown ||
                       type == blink::WebInputEvent::MouseUp) ? (int)[event clickCount] : 0;
    return true;
}

} // namespace mac
} // namespace miniblink
