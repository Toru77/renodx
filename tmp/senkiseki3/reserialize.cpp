// reserialize.cpp — re-serialize a DXBC blob through the SAME rebuild path
// as PatchSkinnedVertexShader but with NO changes (no injected body, no
// dcl_output, no OSGN append). Isolates the whole-blob rebuild itself.
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include "e:\RenoDX\renodx\src\games\senkiseki-dlaa\dxbc_patch.hpp"

int main(int argc, char** argv) {
  if (argc < 3) { printf("usage: reserialize in.cso out.cso\n"); return 2; }
  FILE* f = nullptr;
  fopen_s(&f, argv[1], "rb");
  if (!f) { printf("open fail %s\n", argv[1]); return 2; }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<std::byte> data((size_t)len);
  if (fread(data.data(), 1, (size_t)len, f) != (size_t)len) return 2;
  fclose(f);

  using namespace senkiseki3::dxbc;
  DXBCHeader header;
  std::vector<ChunkInfo> chunks;
  if (!ParseDXBC(data, header, chunks)) { printf("parse fail\n"); return 2; }

  // Rebuild the blob exactly like PatchSkinnedVertexShader: header + offset
  // table + contiguous chunk bodies, but copy every chunk verbatim.
  std::vector<std::byte> out;
  out.reserve(data.size());
  out.insert(out.end(), data.begin(), data.begin() + 32);
  for (const auto& c : chunks) {
    uint32_t off = c.offset;
    std::byte* p = reinterpret_cast<std::byte*>(&off);
    out.insert(out.end(), p, p + 4);
  }
  for (const auto& c : chunks) {
    const uint32_t clen = 8u + c.size;
    out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + clen);
  }
  uint32_t fsz = (uint32_t)out.size();
  std::memcpy(out.data() + 24u, &fsz, 4);  // file size
  WriteDXBCHash(out);

  f = nullptr;
  fopen_s(&f, argv[2], "wb");
  if (!f) return 2;
  fwrite(out.data(), 1, out.size(), f);
  fclose(f);
  printf("re-serialized %s -> %s (%zu bytes, orig %zu)\n", argv[1], argv[2], out.size(), data.size());
  return 0;
}
