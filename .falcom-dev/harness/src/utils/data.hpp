#pragma once
// Harness stub of src/utils/data.hpp: per-object private data kept in a
// locked map keyed by (object, type) instead of the API's private data.
#include <cstdint>
#include <map>
#include <mutex>
#include <utility>

#include <include/reshade.hpp>

namespace renodx::utils::data {

namespace detail {
inline std::mutex& Mutex() {
  static std::mutex mutex;
  return mutex;
}
// Never destroyed, so objects a test does not delete stay reachable (not
// reported as leaks at exit).
inline std::map<std::pair<const void*, const void*>, void*>& Store() {
  static auto* store = new std::map<std::pair<const void*, const void*>, void*>();
  return *store;
}
template <typename T>
inline const void* Tag() {
  static const char tag = 0;
  return &tag;
}
}  // namespace detail

template <typename T>
inline T* Get(const reshade::api::api_object* api_object) {
  std::lock_guard<std::mutex> lock(detail::Mutex());
  auto it = detail::Store().find({api_object, detail::Tag<T>()});
  return it == detail::Store().end() ? nullptr : static_cast<T*>(it->second);
}

template <typename T, typename... Args>
inline bool CreateOrGet(reshade::api::api_object* api_object, T*& private_data, Args&&... args) {
  std::lock_guard<std::mutex> lock(detail::Mutex());
  auto& slot = detail::Store()[{api_object, detail::Tag<T>()}];
  if (slot == nullptr) {
    slot = new T(std::forward<Args>(args)...);
    private_data = static_cast<T*>(slot);
    return true;
  }
  private_data = static_cast<T*>(slot);
  return false;
}

template <typename T>
inline void Delete(reshade::api::api_object* api_object, T* const private_data) {
  {
    std::lock_guard<std::mutex> lock(detail::Mutex());
    detail::Store().erase({api_object, detail::Tag<T>()});
  }
  delete private_data;
}

template <typename T>
inline void Delete(reshade::api::api_object* api_object) {
  T* private_data = Get<T>(api_object);
  if (private_data == nullptr) return;
  Delete(api_object, private_data);
}

}  // namespace renodx::utils::data
