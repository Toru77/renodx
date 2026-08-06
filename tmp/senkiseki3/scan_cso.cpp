// Scan dumped original VS blobs: report b0 cbuffer size, cb/resource slots,
// and whether GameEdgeParameters RDEF bind point == 0 (i.e. b0 is the big
// _Globals cbuffer). Flags any shader where the minimal/no-bind cb0[10..13]
// read would be OUT OF BOUNDS (b0 declared < 14 registers).
#include <cstdio>
#include <cstdint>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>

#include "../src/games/senkiseki-dlaa/dxbc_patch.hpp"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: scan_cso <dir|file>\n"); return 2; }
  std::vector<fs::path> files;
  if (fs::is_directory(argv[1])) {
    for (auto& e : fs::directory_iterator(argv[1]))
      if (e.path().extension() == ".cso" && e.path().string().find(".patched") == std::string::npos)
        files.push_back(e.path());
  } else {
    files.push_back(argv[1]);
  }
  int bad = 0;
  for (auto& f : files) {
    std::ifstream in(f, std::ios::binary);
    if (!in) continue;
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    in.close();
    std::vector<std::byte> blob(raw.size());
    std::memcpy(blob.data(), raw.data(), raw.size());

    senkiseki3::dxbc::DXBCHeader header;
    std::vector<senkiseki3::dxbc::ChunkInfo> chunks;
    if (!senkiseki3::dxbc::ParseDXBC(blob, header, chunks)) { std::printf("%s: NOT DXBC\n", f.filename().string().c_str()); continue; }
    const senkiseki3::dxbc::ChunkInfo* shex = senkiseki3::dxbc::FindChunk(chunks, "SHEX");
    if (!shex) shex = senkiseki3::dxbc::FindChunk(chunks, "SHDR");
    if (!shex) { std::printf("%s: no SHEX\n", f.filename().string().c_str()); continue; }
    const uint8_t* shex_data = reinterpret_cast<const uint8_t*>(blob.data()) + shex->offset + senkiseki3::dxbc::kChunkHeaderSize;
    senkiseki3::dxbc::ShexScan scan;
    if (!senkiseki3::dxbc::ScanShex(shex_data, shex->size, scan)) { std::printf("%s: scan fail\n", f.filename().string().c_str()); continue; }

    // Re-walk to find WHICH instruction reports a temp >= 10 (phantom source).
    std::vector<senkiseki3::dxbc::Instruction> insns;
    if (senkiseki3::dxbc::IterateInstructions(shex_data, shex->size, insns)) {
      const uint32_t* dw = reinterpret_cast<const uint32_t*>(shex_data);
      for (const auto& ins : insns) {
        const uint32_t start = ins.offset / 4u;
        uint32_t idx = start + 1u;
        if ((dw[start] & 0xFFu) == 0xFFu) ++idx;
        while (idx < start + ins.length) {
          uint32_t first = 0u;
          const uint32_t otype = senkiseki3::dxbc::WalkOperand(dw, idx, &first);
          if (otype == senkiseki3::dxbc::OPERAND_TEMP && first >= 10u) {
            std::printf("  TEMP r%u at instr byte 0x%04X opcode=0x%02X len=%u raw=[", first,
                        ins.offset, ins.opcode, ins.length);
            for (uint32_t k = start; k < start + ins.length && k < dw[1]; ++k)
              std::printf("%08X%s", dw[k], k + 1 < start + ins.length ? " " : "");
            std::printf("]\n");
          }
        }
      }
    }

    // b0 declared size (globals_slot defaults to 0)
    uint32_t b0 = 0;
    for (auto& [slot, count] : scan.cb_slot_sizes) if (slot == 0) b0 = count;
    // RDEF: does GameEdgeParameters live in a cbuffer bound at 0?
    int gp_bind = -1;
    if (const senkiseki3::dxbc::ChunkInfo* rdef = senkiseki3::dxbc::FindChunk(chunks, "RDEF")) {
      const uint8_t* rd = reinterpret_cast<const uint8_t*>(blob.data()) + rdef->offset + senkiseki3::dxbc::kChunkHeaderSize;
      std::vector<senkiseki3::dxbc::RdefCbuffer> cbs;
      if (senkiseki3::dxbc::ParseRDEFCbuffers(rd, rdef->size, cbs)) {
        for (auto& cb : cbs)
          for (auto& v : cb.variables)
            if (v.name == "GameEdgeParameters") {
              std::vector<senkiseki3::dxbc::ResourceBinding> bindings;
              if (senkiseki3::dxbc::ParseRDEFBindings(rd, rdef->size, bindings))
                for (auto& b : bindings)
                  if (b.name == cb.name && b.input_type == 0u) { gp_bind = (int)b.bind_point; break; }
              break;
            }
      }
    }
    bool oob = (b0 != 0u && b0 < 14u);
    std::printf("%s: b0=%u dcl_temps=%u max_temp=%u cbSlots=[", f.filename().string().c_str(), b0,
                scan.has_temps ? scan.dcl_temps : 0xFFFFFFFFu, scan.max_temp);
    for (size_t i = 0; i < scan.cb_slots.size(); ++i) std::printf("%u%s", scan.cb_slots[i], i + 1 < scan.cb_slots.size() ? "," : "");
    std::printf("] res=[");
    for (size_t i = 0; i < scan.resource_slots.size(); ++i) std::printf("%u%s", scan.resource_slots[i], i + 1 < scan.resource_slots.size() ? "," : "");
    std::printf("] str=[");
    for (size_t i = 0; i < scan.structured_slots.size(); ++i) std::printf("%u%s", scan.structured_slots[i], i + 1 < scan.structured_slots.size() ? "," : "");
    std::printf("] GameEdge@b%d %s\n", gp_bind, oob ? "<-- OOB cb0[10..13]!!" : "");
    if (oob) ++bad;
  }
  std::printf("\n%d shader(s) with b0 < 14 (cb0[10..13] would be OOB)\n", bad);
  return 0;
}
