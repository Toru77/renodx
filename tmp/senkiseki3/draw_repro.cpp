// draw_repro.cpp — full repro: create patched VS + hand-patched PS, bind a
// 3-RT G-buffer + appended motion RTV (like the addon), and draw a triangle.
// Tests the VS<->PS linkage and OM state on the real driver.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <vector>

static bool LoadFile(const char* path, std::vector<BYTE>& out) {
  FILE* f = nullptr;
  fopen_s(&f, path, "rb");
  if (!f) return false;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  out.resize(len);
  size_t rd = fread(out.data(), 1, (size_t)len, f);
  fclose(f);
  return rd == (size_t)len;
}

int main(int argc, char** argv) {
  const char* vs_path = (argc > 1) ? argv[1] : "0x0D5DABC6.minimal.cso";
  const char* ps_path = (argc > 2) ? argv[2] : "fresh_ps.cso";

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
  if (FAILED(hr) || !dev) { printf("device fail 0x%08X\n", (unsigned)hr); return 2; }
  printf("device: %s\n", drv);

  std::vector<BYTE> vs_code, ps_code;
  if (!LoadFile(vs_path, vs_code) || !LoadFile(ps_path, ps_code)) {
    printf("load fail: %s / %s\n", vs_path, ps_path);
    return 2;
  }

  ID3D11VertexShader* vs = nullptr;
  hr = dev->CreateVertexShader(vs_code.data(), vs_code.size(), nullptr, &vs);
  printf("CreateVertexShader %s: 0x%08X\n", vs_path, (unsigned)hr);
  if (FAILED(hr)) return 1;

  ID3D11PixelShader* ps = nullptr;
  hr = dev->CreatePixelShader(ps_code.data(), ps_code.size(), nullptr, &ps);
  printf("CreatePixelShader %s: 0x%08X\n", ps_path, (unsigned)hr);
  if (FAILED(hr)) return 1;

  // Input layout: all 6 inputs the face VS declares (from its ISGN).
  D3D11_INPUT_ELEMENT_DESC elems[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  ID3D11InputLayout* layout = nullptr;
  hr = dev->CreateInputLayout(elems, 6, vs_code.data(), vs_code.size(), &layout);
  printf("CreateInputLayout: 0x%08X\n", (unsigned)hr);
  if (FAILED(hr)) return 1;

  // Vertex buffer: 3 vertices, 72 bytes stride.
  struct V { float x,y,z; float nx,ny,nz; float u,v; unsigned bi[4]; float bw[4]; float t3u,t3v; };
  V verts[3] = {};
  for (int i = 0; i < 3; ++i) {
    verts[i].x = (i==0)?-0.5f:(i==1)?0.f:0.5f;
    verts[i].y = (i==1)?0.5f:-0.5f;
    verts[i].z = 0.f;
    verts[i].u = 0.f; verts[i].v = 0.f;
    verts[i].t3u = 0.f; verts[i].t3v = 0.f;
  }
  D3D11_BUFFER_DESC vb_desc = {};
  vb_desc.ByteWidth = sizeof(verts);
  vb_desc.Usage = D3D11_USAGE_DEFAULT;
  vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  D3D11_SUBRESOURCE_DATA vb_init = {verts, 0, 0};
  ID3D11Buffer* vb = nullptr;
  dev->CreateBuffer(&vb_desc, &vb_init, &vb);
  UINT stride = sizeof(V), offset = 0;

  // 4 render targets: 3 game-like (R8G8B8A8) + 1 motion (R16G16B16A16_FLOAT).
  ID3D11Texture2D* rts_tex[4] = {};
  ID3D11RenderTargetView* rts_rtv[4] = {};
  for (int i = 0; i < 4; ++i) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 256; td.Height = 256; td.MipLevels = 1; td.ArraySize = 1;
    td.SampleDesc.Count = 1;
    td.Format = (i == 3) ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    dev->CreateTexture2D(&td, nullptr, &rts_tex[i]);
    dev->CreateRenderTargetView(rts_tex[i], nullptr, &rts_rtv[i]);
  }
  // Depth stencil.
  D3D11_TEXTURE2D_DESC dd = {};
  dd.Width = 256; dd.Height = 256; dd.MipLevels = 1; dd.ArraySize = 1;
  dd.SampleDesc.Count = 1; dd.Format = DXGI_FORMAT_D32_FLOAT;
  dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  ID3D11Texture2D* depth_tex = nullptr;
  ID3D11DepthStencilView* dsv = nullptr;
  dev->CreateTexture2D(&dd, nullptr, &depth_tex);
  dev->CreateDepthStencilView(depth_tex, nullptr, &dsv);

  // Bind 4 RTs (3 game + motion at slot 3, like the addon's append).
  ID3D11RenderTargetView* rtv_array[4] = {rts_rtv[0], rts_rtv[1], rts_rtv[2], rts_rtv[3]};
  ctx->OMSetRenderTargets(4, rtv_array, dsv);

  // Viewport.
  D3D11_VIEWPORT vp = {0,0,256,256,0,1};
  ctx->RSSetViewports(1, &vp);

  // Bind VS/PS/IA.
  ctx->VSSetShader(vs, nullptr, 0);
  ctx->PSSetShader(ps, nullptr, 0);
  ctx->IASetInputLayout(layout);
  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

  // ── Bind a cb0 like the game: 66 registers of _Globals (ViewProjection at
  // c10..c13). The game always binds this; without it the VS reads NULL cb0,
  // which is NOT how the game runs. Also bind t0 as a stride-64 structured
  // bone buffer like the game (bone index 0 = identity). ──
  {
    std::vector<float> globals(66 * 4, 0.f);
    // ViewProjection at c10..c13 = identity-ish (clip: x,y,z,w = x,y,z,1)
    globals[10*4+0]=1.f; globals[10*4+1]=0.f; globals[10*4+2]=0.f; globals[10*4+3]=0.f;
    globals[11*4+0]=0.f; globals[11*4+1]=1.f; globals[11*4+2]=0.f; globals[11*4+3]=0.f;
    globals[12*4+0]=0.f; globals[12*4+1]=0.f; globals[12*4+2]=1.f; globals[12*4+3]=0.f;
    globals[13*4+0]=0.f; globals[13*4+1]=0.f; globals[13*4+2]=0.f; globals[13*4+3]=1.f;
    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = (UINT)(globals.size() * sizeof(float));
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbinit = {globals.data(), 0, 0};
    ID3D11Buffer* cb0 = nullptr;
    dev->CreateBuffer(&cbd, &cbinit, &cb0);
    ctx->VSSetConstantBuffers(0, 1, &cb0);
    // t0: stride-64 structured buffer, 8 bones of identity.
    std::vector<float> bones(8 * 16, 0.f);
    for (int b = 0; b < 8; ++b) { bones[b*16+0]=1.f; bones[b*16+5]=1.f; bones[b*16+10]=1.f; bones[b*16+15]=1.f; }
    D3D11_BUFFER_DESC sbd = {};
    sbd.ByteWidth = (UINT)(bones.size() * sizeof(float));
    sbd.Usage = D3D11_USAGE_DEFAULT;
    sbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    sbd.StructureByteStride = 64;
    D3D11_SUBRESOURCE_DATA sbinit = {bones.data(), 0, 0};
    ID3D11Buffer* sb = nullptr;
    dev->CreateBuffer(&sbd, &sbinit, &sb);
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = DXGI_FORMAT_UNKNOWN;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srvd.BufferEx.FirstElement = 0;
    srvd.BufferEx.NumElements = 8;
    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(sb, &srvd, &srv);
    ctx->VSSetShaderResources(0, 1, &srv);
    if (cb0) cb0->Release();
    if (srv) srv->Release();
    if (sb) sb->Release();
  }

  // Draw many times to stress (like the hundreds of face draws in-game).
  printf("drawing...\n");
  for (int i = 0; i < 1000; ++i) {
    ctx->Draw(3, 0);
  }
  ctx->Flush();
  hr = dev->GetDeviceRemovedReason();
  printf("after 1000 draws: GetDeviceRemovedReason = 0x%08X %s\n",
         (unsigned)hr, hr == S_OK ? "OK (no TDR)" : "DEVICE REMOVED/TDR!");

  // Cleanup.
  for (int i = 0; i < 4; ++i) { if (rts_rtv[i]) rts_rtv[i]->Release(); if (rts_tex[i]) rts_tex[i]->Release(); }
  if (dsv) dsv->Release(); if (depth_tex) depth_tex->Release();
  if (vb) vb->Release(); if (layout) layout->Release();
  if (ps) ps->Release(); if (vs) vs->Release();
  if (ctx) ctx->Release(); if (dev) dev->Release();
  return hr == S_OK ? 0 : 1;
}
