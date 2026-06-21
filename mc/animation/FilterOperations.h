// mc::FilterOperations — alias to the compositor's FilterOperationsWrap.
//
// Several mc/ headers (ElementAnimations.h, MutatorHostClient.h, ...) include
// "mc/animation/FilterOperations.h" and refer to a bare `FilterOperations` type
// inside namespace mc. The concrete type the compositor's animation observers
// pass around is mc::FilterOperationsWrap (the GC wrapper that owns a
// blink::FilterOperations), e.g. LayerAnimationValueObserver::onFilterAnimated
// takes `const FilterOperationsWrap&`. This forwarding header provides the alias
// so those sources resolve the type and the observer overrides match the base.
#ifndef mc_animation_FilterOperations_h
#define mc_animation_FilterOperations_h

#include "mc/animation/FilterOperationsWrap.h"

namespace mc {
using FilterOperations = FilterOperationsWrap;
}

#endif // mc_animation_FilterOperations_h
