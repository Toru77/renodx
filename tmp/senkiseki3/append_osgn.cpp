// append_osgn.cpp — append ONLY a TEXCOORD5/o7 OSGN entry to a blob (no
// injected instructions, no dcl_output). Isolates whether the OSGN append
// alone makes the driver fault at draw time.
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include "e:\RenoDX\renodx\src\games\senkiseki-dlaa\dxbc_patch.hpp"

int main(int argc, char** argv) {
  if (argc < 3) { printf("usage: append_osgn in.cso out.cso\n"); return 2; }
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
  const ChunkInfo* osgn = FindChunk(chunks, "OSGN");
  if (!osgn) { printf("no OSGN\n"); return 2; }

  // Append TEXCOORD5 at o7, mask 0xF, rw 0 (same as the working fresh-HLSL).
  if (!AppendSignatureEntry(data, *osgn, "TEXCOORD", 5u, 7u, 0xFu, 0x0u, 3u)) {
    printf("AppendSignatureEntry fail\n");
    return 2;
  }
  WriteDXBCHash(data);

  f = nullptr;
  fopen_s(&f, argv[2], "wb");
  if (!f) return 2;
  fwrite(data.data(), 1, data.size(), f);
  fclose(f);
  printf("appended OSGN TEXCOORD5/o7 -> %s (%zu bytes)\n", argv[2], data.size());
  return 0;
}
