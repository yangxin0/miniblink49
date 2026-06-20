// Smoke test for the macOS backend's NSEvent -> blink WebInputEvent translation.
// Uses synthetic NSEvents (no window server) + the pure modifier mapping.
#include "port/mac/web_input_event_mac.h"
#include "public/web/WebInputEvent.h"
#import <Cocoa/Cocoa.h>
#include <cstdio>

using miniblink::mac::WebModifiersFromCocoaFlags;
using miniblink::mac::BuildWebMouseEvent;

int main() {
    bool ok = true;

    // 1. Pure modifier mapping.
    int m = WebModifiersFromCocoaFlags(NSEventModifierFlagShift | NSEventModifierFlagCommand);
    const bool modsOk = (m & blink::WebInputEvent::ShiftKey) &&
                        (m & blink::WebInputEvent::MetaKey) &&
                        !(m & blink::WebInputEvent::ControlKey);
    ok = ok && modsOk;

    // 2. Synthetic left-mouse-down at window (10, 80), view height 100.
    //    blink y = 100 - 80 = 20.
    NSEvent* down = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                       location:NSMakePoint(10, 80)
                                  modifierFlags:NSEventModifierFlagShift
                                      timestamp:1.5
                                   windowNumber:0
                                        context:nil
                                    eventNumber:0
                                     clickCount:1
                                       pressure:1.0];
    blink::WebMouseEvent ev;
    const bool built = BuildWebMouseEvent(down, /*viewHeight=*/100.0, &ev);
    const bool fieldsOk = built &&
        ev.type == blink::WebInputEvent::MouseDown &&
        ev.button == blink::WebMouseEvent::ButtonLeft &&
        ev.x == 10 && ev.y == 20 &&
        ev.clickCount == 1 &&
        (ev.modifiers & blink::WebInputEvent::ShiftKey);
    ok = ok && fieldsOk;

    // 3. Non-mouse event is rejected.
    const bool rejectsNonMouse = !BuildWebMouseEvent(nil, 100.0, &ev);
    ok = ok && rejectsNonMouse;

    printf("mods=%s mouse=%s(type=%d btn=%d x=%d y=%d clicks=%d) reject=%s\n",
           modsOk ? "ok" : "BAD", fieldsOk ? "ok" : "BAD",
           ev.type, ev.button, ev.x, ev.y, ev.clickCount,
           rejectsNonMouse ? "ok" : "BAD");
    printf("macOS input-event translation smoke: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
