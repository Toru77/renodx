// patch_dt.cpp — patch dcl_temps in a DXBC blob, recompute hash via the real
// dxbc-patch header functions, write output. Usage: patch_dt in.cso out.cso NEW_TEMPS
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include "e:\RenoDX\renodx\src\games\senkiseki-dlaa\dxbc_patch.hpp"

int main(int argc, char** argv) {
  if (argc < 4) { printf("usage: patch_dt in.cso out.cso NEW_TEMPS\n"); return 2; }
  FILE* f = nullptr;
  fopen_s(&f, argv[1], "rb");
  if (!f) { printf("open fail %s\n", argv[1]); return 2; }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<std::byte> data((size_t)len);
  if (fread(data.data(), 1, (size_t)len, f) != (size_t)len) return 2;
  fclose(f);
  uint32_t new_temps = (uint32_t)atoi(argv[3]);

  using namespace senkiseki3::dxbc;
  DXBCHeader header;
  std::vector<ChunkInfo> chunks;
  if (!ParseDXBC(data, header, chunks)) { printf("parse fail\n"); return 2; }
  const ChunkInfo* shex = FindChunk(chunks, "SHEX");
  if (!shex) shex = FindChunk(chunks, "SHDR");
  if (!shex) { printf("no SHEX\n"); return 2; }

  const uint8_t* d = reinterpret_cast<const uint8_t*>(data.data()) + shex->offset + kChunkHeaderSize;
  uint32_t pos = 8;
  bool done = false;
  while (pos + 4 <= shex->size) {
    uint32_t token;
    std::memcpy(&token, d + pos, 4);
    uint32_t op = token & 0x7FFu;
    uint32_t ln = token >> 24u;
    if (ln == 0) break;
    if (op == OP_DCL_TEMPS && ln >= 2) {
      std::memcpy(reinterpret_cast<uint8_t*>(data.data()) + shex->offset + kChunkHeaderSize + pos + 4,
                  &new_temps, 4);
      printf("patched dcl_temps -> %u\n", new_temps);
      done = true;
      break;
    }
    pos += ln * 4;
  }
  if (!done) { printf("no dcl_temps found\n"); return 2; }

  // Also patch STAT temp count (field 1) if present.
  if (const ChunkInfo* stat = FindChunk(chunks, "STAT")) {
    if (stat->size >= 8u) {
      std::memcpy(reinterpret_cast<uint8_t*>(data.data()) + stat->offset + kChunkHeaderSize + 4,
                  &new_temps, 4);
      printf("patched STAT temp -> %u\n", new_temps);
    }
  }

  WriteDXBCHash(data);
  f = nullptr;
  fopen_s(&f, argv[2], "wb");
  if (!f) return 2;
  fwrite(data.data(), 1, data.size(), f);
  fclose(f);
  printf("wrote %s (%zu bytes)\n", argv[2], data.size());
  return 0;
}
