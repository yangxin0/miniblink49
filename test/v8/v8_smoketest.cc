// v8_smoketest.cc — minimal proof that the pinned V8 8.7 monolith links and
// executes JavaScript under the cross-platform CMake build. Exit code 0 = pass.
#include "v8.h"
#include "libplatform/libplatform.h"
#include <cstdio>
#include <cstring>
#include <memory>

int main(int argc, char* argv[]) {
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  v8::Isolate::CreateParams params;
  params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(params);

  int rc = 1;
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, "1 + 2 * 3").ToLocalChecked();
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
    v8::String::Utf8Value utf8(isolate, result);
    if (*utf8 && std::strcmp(*utf8, "7") == 0) {
      std::printf("v8 smoketest OK: 1 + 2 * 3 = %s\n", *utf8);
      rc = 0;
    } else {
      std::printf("v8 smoketest FAILED: got '%s', expected '7'\n",
                  *utf8 ? *utf8 : "(null)");
    }
  }

  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete params.array_buffer_allocator;
  return rc;
}
