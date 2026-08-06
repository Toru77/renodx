// splice_inject.cpp — inject tokens into a DXBC SHEX via the SAME splice path
// as dxbc_patch.hpp, but with fine-grained control to bisect the TDR:
//   splice_inject in.cso out.cso [declout] [movs] [movout]
//   declout : add dcl_output o7 declaration (reg 7)
//   movs    : add the 4-mov constant block (mov r5.xyz,v0.xyz / mov r5.w,1 /
//             mov r7,(0,0,0,1))  [no output write]
//   movout  : add mov o7.xyzw, r7.xyzw at end of body (needs declout)
// Reuses the real Emit* token emitters + ScanShex + rebuild path.
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include "e:\RenoDX\renodx\src\games\senkiseki-dlaa\dxbc_patch.hpp"

using namespace senkiseki3::dxbc;

int main(int argc, char** argv) {
  if (argc < 4) { printf("usage: splice_inject in.cso out.cso [declout] [movs] [movout] [before_temps]\n"); return 2; }
  bool declout = false, movs = false, movout = false, before_temps = false;
  for (int i = 3; i < argc; ++i) {
    if (!strcmp(argv[i], "declout")) declout = true;
    if (!strcmp(argv[i], "movs")) movs = true;
    if (!strcmp(argv[i], "movout")) movout = true;
    if (!strcmp(argv[i], "before_temps")) before_temps = true;
  }
  FILE* f = nullptr;
  fopen_s(&f, argv[1], "rb");
  if (!f) { printf("open fail %s\n", argv[1]); return 2; }
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<std::byte> data((size_t)len);
  if (fread(data.data(), 1, (size_t)len, f) != (size_t)len) return 2;
  fclose(f);

  DXBCHeader header;
  std::vector<ChunkInfo> chunks;
  if (!ParseDXBC(data, header, chunks)) { printf("parse fail\n"); return 2; }
  ChunkInfo* shex = nullptr;
  for (auto& c : chunks) if (c.name == "SHEX") { shex = &c; break; }
  if (!shex) { printf("no SHEX\n"); return 2; }
  const uint8_t* shex_data = reinterpret_cast<const uint8_t*>(data.data()) + shex->offset + kChunkHeaderSize;

  ShexScan scan;
  if (!ScanShex(shex_data, shex->size, scan)) { printf("scan fail\n"); return 2; }

  const uint32_t* tokens = reinterpret_cast<const uint32_t*>(shex_data + 8u);
  const uint32_t old_token_dwords = (shex->size - 8u) / 4u;
  uint32_t first_non_decl_dw = scan.first_non_decl / 4u - 2u;
  uint32_t ret_dw = scan.ret_offset / 4u - 2u;

  std::vector<uint32_t> new_tokens(tokens, tokens + old_token_dwords);

  // Build decls to inject.
  std::vector<uint32_t> decls;
  if (declout) EmitDclOutput(decls, 7u);  // o7

  // Build body to inject.
  std::vector<uint32_t> body;
  if (movs) {
    EmitMovInput(body, 5u, 0x7u, 0u, 0x7u);          // mov r5.xyz, v0.xyz
    EmitMovImm1(body, 5u, 0x8u, 0x3F800000u);        // mov r5.w, 1.0
    EmitMovImm4(body, 7u, 0xFu, 0u, 0u, 0u, 0x3F800000u);  // mov r7,(0,0,0,1)
  }
  if (movout) EmitMovOutput(body, 7u, 7u, kSwizzleXYZW);  // mov o7.xyzw, r7.xyzw

  // Find dcl_temps position in the token stream if present.
  uint32_t dcl_temps_dw = first_non_decl_dw;
  if (before_temps && scan.has_temps) {
    for (uint32_t i = 0; i < first_non_decl_dw;) {
      const uint32_t op = new_tokens[i] & 0x7FFu;
      const uint32_t len = new_tokens[i] >> 24u;
      if (op == OP_DCL_TEMPS && len >= 2u) { dcl_temps_dw = i; break; }
      i += len;
    }
  }

  // Splice (identical to dxbc_patch.hpp).
  std::vector<uint32_t> assembled;
  assembled.reserve(new_tokens.size() + decls.size() + body.size());
  assembled.insert(assembled.end(), new_tokens.begin(), new_tokens.begin() + dcl_temps_dw);
  assembled.insert(assembled.end(), decls.begin(), decls.end());
  assembled.insert(assembled.end(), new_tokens.begin() + dcl_temps_dw, new_tokens.begin() + ret_dw);
  assembled.insert(assembled.end(), body.begin(), body.end());
  assembled.insert(assembled.end(), new_tokens.begin() + ret_dw, new_tokens.end());

  // Rebuild blob (same as PatchSkinnedVertexShader).
  const uint32_t new_shex_size = 8u + (uint32_t)assembled.size() * 4u;
  const uint32_t delta = new_shex_size - shex->size;
  const uint32_t new_file_size = (uint32_t)data.size() + delta;
  std::vector<std::byte> out;
  out.reserve(new_file_size);
  out.insert(out.end(), data.begin(), data.begin() + 32);
  for (const auto& c : chunks) {
    uint32_t off = c.offset;
    if (c.offset > shex->offset) off += delta;
    std::byte* p = reinterpret_cast<std::byte*>(&off);
    out.insert(out.end(), p, p + 4);
  }
  for (const auto& c : chunks) {
    if (c.offset == shex->offset) {
      out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + 4);  // name
      std::byte* szp = reinterpret_cast<std::byte*>(const_cast<uint32_t*>(&new_shex_size));
      out.insert(out.end(), szp, szp + 4);
      out.insert(out.end(), data.begin() + c.offset + 8, data.begin() + c.offset + 12);  // version/type
      uint32_t asz = (uint32_t)assembled.size() + 2u;  // length counts version+length+all tokens
      std::byte* ap = reinterpret_cast<std::byte*>(&asz);
      out.insert(out.end(), ap, ap + 4);
      const std::byte* tb = reinterpret_cast<const std::byte*>(assembled.data());
      out.insert(out.end(), tb, tb + assembled.size() * 4u);
    } else {
      const uint32_t clen = 8u + c.size;
      out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + clen);
    }
  }
  std::memcpy(out.data() + 24u, &new_file_size, 4);
  WriteDXBCHash(out);
  f = nullptr;
  fopen_s(&f, argv[2], "wb");
  if (!f) return 2;
  fwrite(out.data(), 1, out.size(), f);
  fclose(f);
  printf("wrote %s (%zu bytes)\n", argv[2], out.size());
  return 0;
}
