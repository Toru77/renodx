#pragma once
// Harness stand-in for <include/reshade.hpp>: the real event/API declarations
// (from <repo>/external/reshade/include, on the include path), with event
// registration as no-ops.
#include <reshade_events.hpp>
#include <cstddef>
namespace reshade {
// Stub: no config is read, the caller keeps its default.
inline bool get_config_value(std::nullptr_t, const char*, const char*, int&) { return false; }
template <addon_event ev> inline void register_event(typename addon_event_traits<ev>::decl) {}
template <addon_event ev> inline void unregister_event(typename addon_event_traits<ev>::decl) {}
}
