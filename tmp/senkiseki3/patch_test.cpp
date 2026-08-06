// Offline validation harness for the Phase B generic patcher.
// Loads a real game VS blob, patches it (configurable options), writes the
// result to a .cso so `fxc /dumpbin` can validate the bytecode offline.
//
// Usage:
//   patch_test.exe <in.cso> <out.cso> [nobind] [outline] [minimal] [nooutput] [constant]
#include <cstdio>
#include <cstdint>
#include <vector>
#include <fstream>
#include <iterator>

#include "../src/games/senkiseki-dlaa/dxbc_patch.hpp"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: patch_test <in.cso> <out.cso> [nobind] [outline] [minimal] [nooutput] [constant]\n");
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) { std::fprintf(stderr, "cannot open input %s\n", argv[1]); return 1; }
  std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
  in.close();
  std::vector<std::byte> blob(raw.size());
  std::memcpy(blob.data(), raw.data(), raw.size());
  std::fprintf(stderr, "input %s: %zu bytes\n", argv[1], blob.size());

  senkiseki3::dxbc::PatchOptions options;
  options.enable_outline = (argc < 5) || (std::atoi(argv[4]) != 0);
  options.use_game_resources_only = (argc >= 4) && (std::atoi(argv[3]) != 0);
  options.minimal_patch = (argc >= 6) && (std::atoi(argv[5]) != 0);
  options.test_no_output = (argc >= 7) && (std::atoi(argv[6]) != 0);
  options.test_constant_output = (argc >= 8) && (std::atoi(argv[7]) != 0);

  senkiseki3::dxbc::PatchInfo info;
  uint32_t new_hash = 0u;
  if (!senkiseki3::dxbc::PatchSkinnedVertexShader(blob, &new_hash, &info, options)) {
    std::fprintf(stderr, "PATCH FAILED (not skinned / no free slot / no geometry output)\n");
    return 1;
  }
  std::fprintf(stderr,
               "patched: new_hash=0x%08X cb%u t%u TEXCOORD%u o%u outline=%d noBind=%d\n",
               new_hash, info.prev_vp_cb_slot, info.prev_bone_t_slot,
               info.texcoord_index, info.output_reg, (int)info.outline_applied,
               (int)info.needs_no_binding);
  std::ofstream out(argv[2], std::ios::binary);
  out.write(reinterpret_cast<const char*>(blob.data()), blob.size());
  out.close();
  std::fprintf(stderr, "wrote %s: %zu bytes\n", argv[2], blob.size());
  return 0;
}
