// Out-of-line impls for the protocol-traits helpers declared in scoped_nsobject.h
// (Chromium ships these in this .mm; it was missing from the in-tree subset).
// Non-ARC manual retain/release (the engine builds without ARC).
#import <Foundation/Foundation.h>
#include "base/mac/scoped_nsobject.h"

namespace base {
namespace internal {

id ScopedNSProtocolTraitsRetain(__unsafe_unretained id obj) {
  return [obj retain];
}

id ScopedNSProtocolTraitsAutoRelease(__unsafe_unretained id obj) {
  return [obj autorelease];
}

void ScopedNSProtocolTraitsRelease(__unsafe_unretained id obj) {
  [obj release];
}

}  // namespace internal
}  // namespace base
