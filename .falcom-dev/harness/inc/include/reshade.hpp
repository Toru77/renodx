#pragma once
// Harness stand-in for <include/reshade.hpp>: the real event/API declarations
// (from <repo>/external/reshade/include, on the include path), with event
// registration as no-ops.
#include <reshade_events.hpp>
namespace reshade {
template <addon_event ev> inline void register_event(typename addon_event_traits<ev>::decl) {}
template <addon_event ev> inline void unregister_event(typename addon_event_traits<ev>::decl) {}
}
