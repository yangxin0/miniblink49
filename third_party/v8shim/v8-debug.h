#ifndef MINIBLINK_V8_DEBUG_SHIM_H_
#define MINIBLINK_V8_DEBUG_SHIM_H_

// Compatibility shim: V8 8.7 removed <v8-debug.h> and the legacy v8::Debug API
// (debugging moved to v8-inspector / the v8::debug namespace). blink, written
// against V8 ~7.5, still includes this header. We provide the subset blink's
// non-inspector code needs so the bulk of core/bindings compiles against V8 8.7.
//
// The richer v8::Debug API actually *called* by core/inspector (DebugEvent enum,
// SetDebugEventListener, Call, ...) is brought up with the inspector subsystem;
// note the DebugEvent enumerator "Exception" would collide with v8.h's
// v8::Exception class, so the enum is deliberately left out here.

#include <v8.h>

namespace v8 {

class Debug {
public:
    // Carries the state passed to a legacy debug-event listener. The accessors
    // are stubbed; only the type completeness is needed for core to compile, and
    // the inspector (which reads these) supplies real wiring when brought up.
    class EventDetails {
    public:
        int GetEvent() const { return 0; }
        Local<Object> GetExecutionState() const { return Local<Object>(); }
        Local<Object> GetEventData() const { return Local<Object>(); }
        Local<Context> GetEventContext() const { return Local<Context>(); }
        Local<Value> GetCallbackData() const { return Local<Value>(); }
    };

    typedef void (*EventCallback)(const EventDetails&);

    // No separate "debug context" in 8.7; an empty handle keeps the common
    // "context != GetDebugContext(isolate)" check correct.
    static Local<Context> GetDebugContext(Isolate*) { return Local<Context>(); }

    // V8 8.7 removed v8::Debug::GetInternalProperties (it moved to the internal
    // debug/inspector interface). The inspector's InjectedScriptHost reads these to
    // surface object internals in devtools; return empty until the inspector debug
    // bridge is brought up (devtools simply shows no extra internal properties).
    static MaybeLocal<Array> GetInternalProperties(Isolate*, Local<Value>) { return MaybeLocal<Array>(); }
};

} // namespace v8

#endif // MINIBLINK_V8_DEBUG_SHIM_H_
