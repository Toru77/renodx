#ifndef SRC_BEYONDTHEFALCOM_SHARED_H_
#define SRC_BEYONDTHEFALCOM_SHARED_H_

// Beyond The Falcom Engine: standalone DX11 scene-extraction addon.
// No tonemapping / swapchain upgrade state by design. This header only
// carries the small UI-bound capture settings shared with addon.cpp.

struct BeyondFalcomSettings {
  float capture_draw_index = 0.f;
};

#endif  // SRC_BEYONDTHEFALCOM_SHARED_H_
