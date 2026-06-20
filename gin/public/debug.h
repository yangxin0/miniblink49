// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_DEBUG_H_
#define GIN_PUBLIC_DEBUG_H_

#include "build/build_config.h"
#include "gin/gin_export.h"
//#include "v8/include/v8.h"
#include "v8.h"

// V8 8.7 removed v8::FunctionEntryHook (the JIT entry-hook feature is gone).
// Provide a compatible typedef so gin's (now no-op) entry-hook API still
// compiles. See isolate_holder.cc, which no longer wires it up.
#if !defined(V8_FUNCTION_ENTRY_HOOK_COMPAT)
#define V8_FUNCTION_ENTRY_HOOK_COMPAT
#include <stdint.h>
namespace v8 {
typedef void (*FunctionEntryHook)(uintptr_t function,
                                  uintptr_t return_addr_location);
}  // namespace v8
#endif

namespace gin {

class GIN_EXPORT Debug {
 public:
  /* Installs a callback that is invoked on entry to every V8-generated
   * function.
   *
   * This only affects IsolateHolder instances created after
   * SetFunctionEntryHook was invoked.
   */
  static void SetFunctionEntryHook(v8::FunctionEntryHook entry_hook);

  /* Installs a callback that is invoked each time jit code is added, moved,
   * or removed.
   *
   * This only affects IsolateHolder instances created after
   * SetJitCodeEventHandler was invoked.
   */
  static void SetJitCodeEventHandler(v8::JitCodeEventHandler event_handler);

#if defined(OS_WIN)
  typedef void (__cdecl *CodeRangeCreatedCallback)(void*, size_t);

  /* Sets a callback that is invoked for every new code range being created.
   *
   * On Win64, exception handling in jitted code is broken due to the fact
   * that JS stack frames are not ABI compliant. It is possible to install
   * custom handlers for the code range which holds the jitted code to work
   * around this issue.
   *
   * https://code.google.com/p/v8/issues/detail?id=3598
   */
  static void SetCodeRangeCreatedCallback(CodeRangeCreatedCallback callback);

  typedef void (__cdecl *CodeRangeDeletedCallback)(void*);

  /* Sets a callback that is invoked for every previously registered code range
   * when it is deleted.
   */
  static void SetCodeRangeDeletedCallback(CodeRangeDeletedCallback callback);
#endif
};

}  // namespace gin

#endif  // GIN_PUBLIC_DEBUG_H_
