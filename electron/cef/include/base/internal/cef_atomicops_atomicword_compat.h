// Copyright (c) 2011 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license.
//
// Standard Chromium/CEF AtomicWord compatibility header (mac/bsd only).
// Restored from upstream; absent in-tree because miniblink only built on Windows.
//
// This header exists to add AtomicWord overloads of the Atomic* ops *only when*
// AtomicWord is a distinct type from the platform's native Atomic type. On this
// build's target (64-bit, where AtomicWord and Atomic64 are the same type) the
// Atomic64 overloads already cover AtomicWord, so adding more would be a
// redefinition — hence this header is intentionally empty here.

#ifndef CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_
#define CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_

// (No additional declarations needed: AtomicWord == Atomic64 on this target.)

#endif  // CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_
