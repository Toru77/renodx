// Print the DXBC chunk table (offset, size, fourcc) for one or more .cso files,
// so we can compare the structure of original vs in-place-patched vs fresh-HLSL
// blobs. Usage: chunks.exe <file.cso> [more.cso ...]
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <iterator>

#include "../src/games/senkiseki-dlaa/dxbc_patch.hpp"

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: chunks <file.cso> [...]\n"); return 2; }
  for (int a = 1; a < argc; ++a) {
    std::ifstream in(argv[a], std::ios::binary);
    if (!in) { std::printf("%s: cannot open\n", argv[a]); continue; }
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    in.close();
    std::vector<std::byte> blob(raw.size());
    std::memcpy(blob.data(), raw.data(), raw.size());
    senkiseki3::dxbc::DXBCHeader header;
    std::vector<senkiseki3::dxbc::ChunkInfo> chunks;
    if (!senkiseki3::dxbc::ParseDXBC(blob, header, chunks)) {
      std::printf("%s: NOT DXBC\n", argv[a]); continue;
    }
    std::printf("=== %s: %zu bytes ===\n", argv[a], blob.size());
    for (auto& c : chunks) {
      std::printf("  %s off=0x%08X size=%u\n", c.name.c_str(), c.offset, c.size);
    }
  }
  return 0;
}
