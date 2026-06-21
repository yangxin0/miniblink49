// V8 compatibility shims for the macOS port (V8 8.7 monolith, unpatched).
//
// miniblink's bundled V8 carried a custom patch exposing v8::g_patchForCreateDataProperty,
// a global the generated bindings toggle (gen/blink/bindings/.../V8File.cpp etc.) under
// `#if V8_MINOR_VERSION == 7`. V8 8.7 has minor version 7 too, so that branch is live,
// but the stock 8.7 monolith never defined the global. Define it here (the stock V8 8.7
// CreateDataProperty path does not read it, so this is an inert compatibility global).
namespace v8 {
bool g_patchForCreateDataProperty = false;
}
