// shader_accept.cpp — ask D3D11 (hardware, like the game) whether it accepts a
// given .cso blob via CreateVertexShader / CreatePixelShader. fxc /dumpbin is
// NOT the runtime; this is the definitive driver-acceptance test.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("usage: shader_accept file.cso [VS|PS]\n");
    return 2;
  }
  FILE* f = nullptr;
  fopen_s(&f, argv[1], "rb");
  if (!f) { printf("cannot open %s\n", argv[1]); return 2; }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<BYTE> data(len);
  if (fread(data.data(), 1, (size_t)len, f) != (size_t)len) { printf("read fail\n"); return 2; }
  fclose(f);

  bool is_ps = argc > 2 && _stricmp(argv[2], "PS") == 0;

  // Try hardware first, then WARP as a fallback so we always get a device.
  ID3D11Device* dev = nullptr;
  ID3D11DeviceContext* ctx = nullptr;
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                 nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
  const char* drv = "HARDWARE";
  if (FAILED(hr)) {
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                           nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
    drv = "WARP";
  }
  if (FAILED(hr) || !dev) {
    printf("D3D11CreateDevice failed: 0x%08X\n", (unsigned)hr);
    return 2;
  }

  HRESULT create_hr;
  if (is_ps) {
    ID3D11PixelShader* ps = nullptr;
    create_hr = dev->CreatePixelShader(data.data(), (SIZE_T)data.size(), nullptr, &ps);
    if (SUCCEEDED(create_hr) && ps) ps->Release();
  } else {
    ID3D11VertexShader* vs = nullptr;
    create_hr = dev->CreateVertexShader(data.data(), (SIZE_T)data.size(), nullptr, &vs);
    if (SUCCEEDED(create_hr) && vs) vs->Release();
  }

  printf("%s %s (%s, %ld bytes): CreateV%sShader = 0x%08X %s\n",
         argv[1], is_ps ? "PS" : "VS", drv, len, is_ps ? "P" : "",
         (unsigned)create_hr, SUCCEEDED(create_hr) ? "ACCEPTED" : "REJECTED");

  ctx->Release();
  dev->Release();
  return SUCCEEDED(create_hr) ? 0 : 1;
}
