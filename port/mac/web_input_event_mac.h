// macOS backend (part 3): translate Cocoa NSEvents to blink WebInputEvents.
//
// The content view receives NSEvents (mouse/key); blink consumes WebInputEvents.
// This maps between them. The pure mapping helpers (modifiers, button) are
// testable without Cocoa; BuildWebMouseEvent takes a real NSEvent.
#ifndef MINIBLINK_PORT_MAC_WEB_INPUT_EVENT_MAC_H_
#define MINIBLINK_PORT_MAC_WEB_INPUT_EVENT_MAC_H_

namespace blink {
class WebMouseEvent;
}

namespace miniblink {
namespace mac {

// Map Cocoa NSEventModifierFlags to blink WebInputEvent::Modifiers bits.
// Pure (no Cocoa types) so it is unit-testable headlessly.
int WebModifiersFromCocoaFlags(unsigned long long cocoaModifierFlags);

} // namespace mac
} // namespace miniblink

#ifdef __OBJC__
@class NSEvent;
namespace miniblink {
namespace mac {

// Build a blink WebMouseEvent from a Cocoa mouse NSEvent. viewHeight (points) is
// used to flip Y from Cocoa's bottom-left window space to blink's top-left space.
// Returns false if the event is not a mouse event.
bool BuildWebMouseEvent(NSEvent* event, double viewHeight, blink::WebMouseEvent* out);

} // namespace mac
} // namespace miniblink
#endif // __OBJC__

#endif // MINIBLINK_PORT_MAC_WEB_INPUT_EVENT_MAC_H_
