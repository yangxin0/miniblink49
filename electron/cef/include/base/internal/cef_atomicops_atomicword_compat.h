// Copyright (c) 2011 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license.
//
// Standard Chromium/CEF AtomicWord compatibility header (mac/bsd only).
// Restored from upstream; it was absent in-tree because miniblink only built on
// Windows, where this include is not used. On 64-bit macOS, AtomicWord
// (intptr_t / long) is a distinct type from Atomic64 (int64_t / long long), so
// these overloads forward the AtomicWord ops to the Atomic64 implementations.

#ifndef CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_
#define CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_

namespace base {
namespace subtle {

inline AtomicWord NoBarrier_CompareAndSwap(volatile AtomicWord* ptr,
                                           AtomicWord old_value,
                                           AtomicWord new_value) {
  return NoBarrier_CompareAndSwap(
      reinterpret_cast<volatile Atomic64*>(ptr), old_value, new_value);
}

inline AtomicWord NoBarrier_AtomicExchange(volatile AtomicWord* ptr,
                                           AtomicWord new_value) {
  return NoBarrier_AtomicExchange(
      reinterpret_cast<volatile Atomic64*>(ptr), new_value);
}

inline AtomicWord NoBarrier_AtomicIncrement(volatile AtomicWord* ptr,
                                            AtomicWord increment) {
  return NoBarrier_AtomicIncrement(
      reinterpret_cast<volatile Atomic64*>(ptr), increment);
}

inline AtomicWord Barrier_AtomicIncrement(volatile AtomicWord* ptr,
                                          AtomicWord increment) {
  return Barrier_AtomicIncrement(
      reinterpret_cast<volatile Atomic64*>(ptr), increment);
}

inline AtomicWord Acquire_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  return base::subtle::Acquire_CompareAndSwap(
      reinterpret_cast<volatile Atomic64*>(ptr), old_value, new_value);
}

inline AtomicWord Release_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  return base::subtle::Release_CompareAndSwap(
      reinterpret_cast<volatile Atomic64*>(ptr), old_value, new_value);
}

inline void NoBarrier_Store(volatile AtomicWord* ptr, AtomicWord value) {
  NoBarrier_Store(reinterpret_cast<volatile Atomic64*>(ptr), value);
}

inline void Acquire_Store(volatile AtomicWord* ptr, AtomicWord value) {
  return base::subtle::Acquire_Store(
      reinterpret_cast<volatile Atomic64*>(ptr), value);
}

inline void Release_Store(volatile AtomicWord* ptr, AtomicWord value) {
  return base::subtle::Release_Store(
      reinterpret_cast<volatile Atomic64*>(ptr), value);
}

inline AtomicWord NoBarrier_Load(volatile const AtomicWord* ptr) {
  return NoBarrier_Load(reinterpret_cast<volatile const Atomic64*>(ptr));
}

inline AtomicWord Acquire_Load(volatile const AtomicWord* ptr) {
  return base::subtle::Acquire_Load(
      reinterpret_cast<volatile const Atomic64*>(ptr));
}

inline AtomicWord Release_Load(volatile const AtomicWord* ptr) {
  return base::subtle::Release_Load(
      reinterpret_cast<volatile const Atomic64*>(ptr));
}

}  // namespace subtle
}  // namespace base

#endif  // CEF_INCLUDE_BASE_INTERNAL_CEF_ATOMICOPS_ATOMICWORD_COMPAT_H_
