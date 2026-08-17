/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Standalone Phase A test harness for the generic DXBC patcher toolkit.
 *
 * Usage:
 *   dxbc_test.exe <path-to.cso> [more.cso ...] [--roundtrip]
 *
 * For each .cso it:
 *   1. Parses the DXBC header + chunk table.
 *   2. Prints ISGN/OSGN signature entries (name, index, sysval, comptype,
 *      register, mask, rw) and RDEF resource bindings.
 *   3. Iterates the SHEX token stream (instruction count + opcode histogram).
 *   4. Recomputes the DXBC MD5 checksum and compares it to the stored one.
 *   5. With --roundtrip: appends a synthetic TEXCOORD signature entry to a
 *      copy of the blob, re-parses, and verifies chunk offsets, the new
 *      entry, and that the recomputed hash changed (meaning it is correct to
 *      rewrite the stored checksum).
 *
 * Self-contained: only depends on dxbc_patch.hpp + the C++ standard library.
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "dxbc_patch.hpp"

namespace dxbc = senkiseki3::dxbc;

namespace {

std::vector<std::byte> ReadBinaryFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return {};
  const auto size = static_cast<size_t>(f.tellg());
  std::vector<std::byte> data(size);
  f.seekg(0);
  f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
  return data;
}

bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& data) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  return !!f;
}

std::string Hex(const uint8_t* bytes, size_t n) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string s;
  s.reserve(n * 2);
  for (size_t i = 0; i < n; ++i) {
    s += kHex[bytes[i] >> 4];
    s += kHex[bytes[i] & 0xF];
  }
  return s;
}

const char* NameOfSystemValue(uint32_t v) {
  switch (v) {
    case 0: return "UNDEFINED";
    case 1: return "POSITION";
    case 2: return "CLIP_DISTANCE";
    case 3: return "CULL_DISTANCE";
    case 4: return "RENDER_TARGET_ARRAY_INDEX";
    case 5: return "VIEWPORT_ARRAY_INDEX";
    case 6: return "VERTEX_ID";
    case 7: return "PRIMITIVE_ID";
    case 8: return "INSTANCE_ID";
    case 9: return "IS_FRONT_FACE";
    case 10: return "SAMPLE_INDEX";
    case 11: return "FINAL_QUAD_EDGE_TESSFACTOR";
    case 12: return "FINAL_QUAD_INSIDE_TESSFACTOR";
    case 13: return "FINAL_TRI_EDGE_TESSFACTOR";
    case 14: return "FINAL_TRI_INSIDE_TESSFACTOR";
    case 15: return "FINAL_LINE_DETAIL_TESSFACTOR";
    case 16: return "FINAL_LINE_DENSITY_TESSFACTOR";
    case 17: return "BARYCENTRICS";
    case 18: return "SHADING_RATE";
    case 19: return "CULL_PRIMITIVE";
    case 20: return "DEPTH";
    case 21: return "COVERAGE";
    case 22: return "DEPTH_GREATER_EQUAL";
    case 23: return "DEPTH_LESS_EQUAL";
    case 24: return "STENCIL_REF";
    case 25: return "INNER_COVERAGE";
    default: return "?";
  }
}

const char* NameOfComponentType(uint32_t v) {
  switch (v) {
    case 0: return "UNKNOWN";
    case 1: return "UINT32";
    case 2: return "SINT32";
    case 3: return "FLOAT32";
    default: return "?";
  }
}

const char* NameOfInputType(uint32_t v) {
  switch (v) {
    case 0: return "CBUFFER";
    case 1: return "TBUFFER";
    case 2: return "TEXTURE";
    case 3: return "SAMPLER";
    case 4: return "UAV_RWTYPED";
    case 5: return "STRUCTURED";
    case 6: return "UAV_RWSTRUCTURED";
    case 7: return "BYTEADDRESS";
    case 8: return "UAV_RWBYTEADDRESS";
    case 9: return "UAV_APPEND_STRUCTURED";
    case 10: return "UAV_CONSUME_STRUCTURED";
    case 11: return "UAV_RWSTRUCTURED_WITH_COUNTER";
    default: return "?";
  }
}

const char* NameOfDimension(uint32_t v) {
  switch (v) {
    case 0: return "UNKNOWN";
    case 1: return "BUFFER";
    case 2: return "TEX1D";
    case 3: return "TEX1D_ARRAY";
    case 4: return "TEX2D";
    case 5: return "TEX2D_ARRAY";
    case 6: return "TEX2DMS";
    case 7: return "TEX2DMS_ARRAY";
    case 8: return "TEX3D";
    case 9: return "TEXCUBE";
    case 10: return "TEXCUBE_ARRAY";
    case 11: return "TEX2D_SHADOW";
    case 12: return "TEX2D_ARRAY_SHADOW";
    case 13: return "TEXCUBE_SHADOW";
    case 14: return "TEXCUBE_ARRAY_SHADOW";
    default: return "?";
  }
}

void PrintSignature(const char* label, const std::vector<dxbc::SignatureEntry>& sig) {
  std::cout << "  " << label << " (" << sig.size() << " entries):\n";
  for (const auto& e : sig) {
    std::cout << "    [" << e.register_index << "] \"" << e.name << "\""
              << " idx=" << e.semantic_index
              << " sysval=" << e.system_value_type << "(" << NameOfSystemValue(e.system_value_type) << ")"
              << " comptype=" << e.component_type << "(" << NameOfComponentType(e.component_type) << ")"
              << " mask=0x" << std::hex << e.mask << " rw=0x" << e.read_write_mask << std::dec
              << " name_off=" << e.name_offset << "\n";
  }
}

void PrintOpcodes(const std::vector<dxbc::Instruction>& insns) {
  std::cout << "  SHEX instructions: " << insns.size() << "\n";
  // Opcode histogram (only the interesting ones). Values VERIFIED vs fxc 10.1.
  static const std::vector<std::pair<uint32_t, const char*>> kInteresting = {
      {0x58, "dcl_resource"}, {0x59, "dcl_constant_buffer"}, {0x5A, "dcl_sampler"},
      {0x5F, "dcl_input"}, {0x60, "dcl_input_sgv"}, {0x61, "dcl_input_siv"},
      {0x65, "dcl_output"}, {0x66, "dcl_output_sgv"}, {0x67, "dcl_output_siv"},
      {0x68, "dcl_temps"}, {0x6A, "dcl_globalFlags"}, {0xA2, "dcl_resource_structured"},
      {0x2D, "ld"}, {0x2E, "ld_ms"}, {0xA7, "ld_structured"},
      {0x36, "mov"}, {0x32, "mad"}, {0x38, "mul"}, {0x34, "max"}, {0x11, "dp4"},
      {0x45, "sample"}, {0x48, "sample_l"}, {0x0E, "div"}, {0x3E, "ret"},
  };
  std::cout << "  opcode histogram (selected):\n";
  for (const auto& [op, name] : kInteresting) {
    uint32_t count = 0;
    for (const auto& i : insns)
      if (i.opcode == op) ++count;
    if (count > 0) std::cout << "    " << name << " (" << op << "): " << count << "\n";
  }
}

const char* NameOfOpcode(uint32_t op) {
  using namespace senkiseki3::dxbc;
  switch (op) {
    case OP_ADD: return "add"; case OP_AND: return "and"; case OP_BREAK: return "break";
    case OP_BREAKC: return "breakc"; case OP_CALL: return "call"; case OP_CALLC: return "callc";
    case OP_CASE: return "case"; case OP_CONTINUE: return "continue"; case OP_CONTINUEC: return "continuec";
    case OP_CUT: return "cut"; case OP_DEFAULT: return "default"; case OP_DERIV_RTX: return "deriv_rtx";
    case OP_DERIV_RTY: return "deriv_rty"; case OP_DISCARD: return "discard"; case OP_DIV: return "div";
    case OP_DP2: return "dp2"; case OP_DP3: return "dp3"; case OP_DP4: return "dp4";
    case OP_ELSE: return "else"; case OP_EMIT: return "emit"; case OP_EMITTHENCUT: return "emitthencut";
    case OP_ENDIF: return "endif"; case OP_ENDLOOP: return "endloop"; case OP_ENDSWITCH: return "endswitch";
    case OP_EQ: return "eq"; case OP_EXP: return "exp"; case OP_FRC: return "frc";
    case OP_GE: return "ge"; case OP_IADD: return "iadd"; case OP_IF: return "if";
    case OP_IEQ: return "ieq"; case OP_IGE: return "ige"; case OP_ILT: return "ilt";
    case OP_IMAD: return "imad"; case OP_IMAX: return "imax"; case OP_IMIN: return "imin";
    case OP_IMUL: return "imul"; case OP_INE: return "ine"; case OP_INEG: return "ineg";
    case OP_ISHL: return "ishl"; case OP_ISHR: return "ishr"; case OP_ITOF: return "itof";
    case OP_LT: return "lt"; case OP_MAD: return "mad"; case OP_MAX: return "max";
    case OP_MIN: return "min"; case OP_MOV: return "mov"; case OP_MOVC: return "movc";
    case OP_MUL: return "mul"; case OP_NE: return "ne"; case OP_NOP: return "nop";
    case OP_NOT: return "not"; case OP_OR: return "or"; case OP_RESINFO: return "resinfo";
    case OP_RET: return "ret"; case OP_RETC: return "retc"; case OP_ROUND_NE: return "round_ne";
    case OP_ROUND_NI: return "round_ni"; case OP_ROUND_PI: return "round_pi"; case OP_ROUND_Z: return "round_z";
    case OP_RSQ: return "rsq"; case OP_SAMPLE: return "sample"; case OP_SAMPLE_C: return "sample_c";
    case OP_SAMPLE_C_LZ: return "sample_c_lz"; case OP_SAMPLE_L: return "sample_l"; case OP_SAMPLE_D: return "sample_d";
    case OP_SAMPLE_B: return "sample_b"; case OP_SQRT: return "sqrt"; case OP_SWITCH: return "switch";
    case OP_SINCOS: return "sincos"; case OP_UDIV: return "udiv"; case OP_ULT: return "ult";
    case OP_UGE: return "uge"; case OP_UMUL: return "umul"; case OP_UMAD: return "umad";
    case OP_UMAX: return "umax"; case OP_UMIN: return "umin"; case OP_USHR: return "ushr";
    case OP_UTOF: return "utof"; case OP_XOR: return "xor";
    case OP_DCL_RESOURCE: return "dcl_resource"; case OP_DCL_CONSTANT_BUFFER: return "dcl_constant_buffer";
    case OP_DCL_SAMPLER: return "dcl_sampler"; case OP_DCL_INDEX_RANGE: return "dcl_index_range";
    case OP_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY: return "dcl_gs_output_primitive_topology";
    case OP_DCL_GS_INPUT_PRIMITIVE: return "dcl_gs_input_primitive";
    case OP_DCL_MAX_OUTPUT_VERTEX_COUNT: return "dcl_max_output_vertex_count";
    case OP_DCL_INPUT: return "dcl_input"; case OP_DCL_INPUT_SGV: return "dcl_input_sgv";
    case OP_DCL_INPUT_SIV: return "dcl_input_siv"; case OP_DCL_INPUT_PS: return "dcl_input_ps";
    case OP_DCL_INPUT_PS_SGV: return "dcl_input_ps_sgv"; case OP_DCL_INPUT_PS_SIV: return "dcl_input_ps_siv";
    case OP_DCL_OUTPUT: return "dcl_output"; case OP_DCL_OUTPUT_SGV: return "dcl_output_sgv";
    case OP_DCL_OUTPUT_SIV: return "dcl_output_siv"; case OP_DCL_TEMPS: return "dcl_temps";
    case OP_DCL_INDEXABLE_TEMP: return "dcl_indexableTemp"; case OP_DCL_GLOBAL_FLAGS: return "dcl_globalFlags";
    case OP_DCL_RESOURCE_STRUCTURED: return "dcl_resource_structured";
    case OP_LD: return "ld"; case OP_LD_MS: return "ld_ms";
    case OP_LD_STRUCTURED: return "ld_structured";
    default: return "?";
  }
}

// Decode an operand token's raw fields (bit layout VERIFIED vs fxc 10.1).
std::string DecodeOperand(const uint32_t* dwords, uint32_t& idx) {
  const uint32_t t = dwords[idx];
  const uint32_t num_components = t & 0x3u;
  const uint32_t sel_mode = (t >> 2u) & 0x3u;
  const uint32_t sel = (t >> 4u) & 0xFFu;
  const uint32_t type = (t >> 12u) & 0xFFu;
  const uint32_t index_dim = (t >> 20u) & 0x3u;
  char buf[160];
  std::snprintf(buf, sizeof(buf), "operand{t=0x%08X nc=%u selmode=%u sel=0x%X type=%u dim=%u", t, num_components,
                sel_mode, sel, type, index_dim);
  std::string s = buf;
  // Index representations: 3 bits per dimension starting at bit 22.
  for (uint32_t d = 0; d < index_dim; ++d) {
    const uint32_t rep = (t >> (22u + 3u * d)) & 0x7u;
    std::snprintf(buf, sizeof(buf), " idx%d(rep=%u", d, rep);
    s += buf;
    if (rep == 0u) {  // immediate32
      ++idx;
      std::snprintf(buf, sizeof(buf), "=%u", dwords[idx]);
      s += buf;
    } else if (rep == 2u) {  // relative
      ++idx;
      std::snprintf(buf, sizeof(buf), "=r%d", dwords[idx]);
      s += buf;
    }
    s += ")";
  }
  s += "}";
  ++idx;
  return s;
}

void DumpShex(const uint8_t* chunk_data, uint32_t chunk_size) {
  std::vector<dxbc::Instruction> insns;
  if (!dxbc::IterateInstructions(chunk_data, chunk_size, insns)) {
    std::cout << "  ERROR: failed to iterate SHEX\n";
    return;
  }
  const uint32_t* dwords = reinterpret_cast<const uint32_t*>(chunk_data);
  std::cout << "  SHEX dump (" << insns.size() << " instructions):\n";
  for (const auto& ins : insns) {
    const uint32_t start = ins.offset / 4u;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "  [%04u] len=%u op=0x%X(%s)", start, ins.length, ins.opcode,
                  NameOfOpcode(ins.opcode));
    std::cout << buf;
    // Raw dwords.
    std::cout << " |";
    for (uint32_t i = 0; i < ins.length; ++i) {
      std::snprintf(buf, sizeof(buf), " %08X", dwords[start + i]);
      std::cout << buf;
    }
    std::cout << "\n";
    // Best-effort operand decode (skip opcode token).
    uint32_t idx = start + 1u;
    for (uint32_t i = 1; i < ins.length; ++i) {
      std::cout << "      " << DecodeOperand(dwords, idx) << "\n";
    }
  }
}

// Build the core injected block (decls + prev-bone re-skin + prevVP) with the
// emitters and print the dwords, so we can diff against fxc ground truth.
void EmitSelfTest() {
  using namespace dxbc;
  std::cout << "  --- emitter self-test (core prev-bone block) ---\n";
  std::vector<uint32_t> out;

  // Decls.
  EmitDclConstantBuffer(out, 1u, 4u);          // cb1[4] = prevVP
  EmitDclResourceStructured(out, 1u, 64u);     // t1, 64 = prev bones
  EmitDclOutput(out, 9u);                       // o9 = prevClip
  EmitDclTemps(out, 12u);                       // bump to 12

  // rP = float4(v0,1)   (v0 = POSITION input reg 0)
  EmitMovInput(out, 0u, 0x7u, 0u, 0x7u);        // mov r0.xyz, v0.xyz
  EmitMovImm1(out, 0u, 0x8u, 0x3F800000u);      // mov r0.w, 1.0

  // 4-bone re-skin with t1 (blend reg 4, weights reg 5, order y,x,z,w).
  // Temps: r0=rP, r1=col0, r2=col1, r3=col2, r4=acc, r5=boneResult.
  // Bone 0 (index .y = comp1, weight .y = comp1, byte offsets 0/16/32):
  EmitLdStructured3(out, 1u, 4u, 1u, 0u, 1u, kSwizzleXYZW);   // r1.xyz = t1[v4.y] row0
  EmitLdStructured3(out, 2u, 4u, 1u, 16u, 1u, kSwizzleXYZW);  // r2.xyz = row1
  EmitLdStructured3(out, 3u, 4u, 1u, 32u, 1u, kSwizzleXYZW);  // r3.xyz = row2
  EmitDp4(out, 5u, 0x1u, 1u, kSwizzleXYZW, 0u, kSwizzleXYZW);  // r5.x = dot(r1, r0)
  EmitDp4(out, 5u, 0x2u, 2u, kSwizzleXYZW, 0u, kSwizzleXYZW);  // r5.y = dot(r2, r0)
  EmitDp4(out, 5u, 0x4u, 3u, kSwizzleXYZW, 0u, kSwizzleXYZW);  // r5.z = dot(r3, r0)
  EmitMadInput(out, 4u, 0x7u, 5u, kSwizzleYYYY, 5u, kSwizzleXYZW, 4u, kSwizzleXYZW);  // r4 += v5.y * r5

  // prevClip = prevVP (cb1) * r4
  EmitDp4Cb(out, 1u, 0x1u, 1u, 0u, 4u, kSwizzleXYZW);   // r1.x = dot(cb1[0], r4)
  EmitDp4Cb(out, 1u, 0x2u, 1u, 1u, 4u, kSwizzleXYZW);   // r1.y = dot(cb1[1], r4)
  EmitDp4Cb(out, 1u, 0x4u, 1u, 2u, 4u, kSwizzleXYZW);   // r1.z = dot(cb1[2], r4)
  EmitDp4Cb(out, 1u, 0x8u, 1u, 3u, 4u, kSwizzleXYZW);   // r1.w = dot(cb1[3], r4)
  EmitMovOutput(out, 9u, 1u, kSwizzleXYZW);              // o9 = r1
  EmitRet(out);                                          // ret

  // Print.
  std::cout << "  emitted " << out.size() << " dwords:\n";
  for (size_t i = 0; i < out.size(); i += 8) {
    std::cout << "    ";
    for (size_t j = i; j < i + 8 && j < out.size(); ++j) std::printf("%08X ", out[j]);
    std::cout << "\n";
  }
}

// Patch a skinned VS in place, write the patched blob to `out_path`, and
// re-validate (parse + signatures + hash self-consistency). With `velocity`
// the RT2-channel per-object velocity encode is emitted too.
int PatchOne(const std::string& path, const std::string& out_path, bool carrier = false) {
  auto data = ReadBinaryFile(path);
  if (data.empty()) {
    std::cerr << "  ERROR: cannot read file\n";
    return 1;
  }
  uint32_t new_hash = 0;
  dxbc::PatchInfo info;
  dxbc::PatchOptions options;
  options.emit_velocity_carrier = carrier;
  const bool patched = dxbc::PatchSkinnedVertexShader(data, &new_hash, &info, options);
  if (!patched) {
    std::cout << "  NOT PATCHED (not a skinned VS or parse failed)\n";
    // Re-parse to confirm it's a valid DXBC anyway.
    dxbc::DXBCHeader h;
    std::vector<dxbc::ChunkInfo> chunks;
    if (dxbc::ParseDXBC(data, h, chunks)) {
      uint8_t d[16];
      dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), d);
      std::cout << "  (valid DXBC, hash " << (std::memcmp(d, h.checksum, 16) == 0 ? "MATCH" : "MISMATCH") << ")\n";
    }
    return 0;  // not an error for non-skinned files
  }

  if (!WriteBinaryFile(out_path, data)) {
    std::cerr << "  ERROR: cannot write " << out_path << "\n";
    return 1;
  }
  std::cout << "  PATCHED: new CRC32=0x" << std::hex << new_hash << std::dec
            << " size=" << data.size() << " -> " << out_path << "\n";
  std::cout << "  velocity=" << (info.velocity_emitted ? "EMITTED" : "no")
            << " tex10=o" << info.tex10_out_reg
            << " outline=" << (info.outline_applied ? "yes" : "no")
            << " prevVP=b" << info.prev_vp_cb_slot << " prevBone=t" << info.prev_bone_t_slot
            << " gameBone=t" << info.bone_game_slot << "\n";

  // Re-validate the patched blob.
  dxbc::DXBCHeader h;
  std::vector<dxbc::ChunkInfo> chunks;
  if (!dxbc::ParseDXBC(data, h, chunks)) {
    std::cout << "  ERROR: patched blob fails ParseDXBC\n";
    return 1;
  }
  uint8_t d[16];
  dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), d);
  const bool hash_ok = std::memcmp(d, h.checksum, 16) == 0;
  std::cout << "  reparse: OK hash=" << (hash_ok ? "SELF-CONSISTENT" : "MISMATCH (BUG)") << "\n";

  // Show the OSGN entries (velocity mode appends nothing; full mode appends prevClip).
  if (const dxbc::ChunkInfo* osgn = dxbc::FindChunk(chunks, "OSGN")) {
    std::vector<dxbc::SignatureEntry> sig;
    if (dxbc::ParseSignature(
            reinterpret_cast<const uint8_t*>(data.data()) + osgn->offset + dxbc::kChunkHeaderSize,
            osgn->size, sig)) {
      std::cout << "  OSGN now " << sig.size() << " entries:";
      for (const auto& e : sig) {
        std::cout << " \"" << e.name << (e.semantic_index ? std::to_string(e.semantic_index) : "")
                  << "\"@o" << e.register_index;
      }
      std::cout << "\n";
    }
  }
  return 0;
}

// Patch a G-buffer pixel shader in place (Phase E), write the patched blob to
// `out_path`, and re-validate (parse + signatures + hash self-consistency).
// `texidx` (0 = auto) is the paired patched VS's prevClip TEXCOORD index that
// the PS input MUST reuse so the VS->PS semantic linkage matches.
int PatchOnePs(const std::string& path, const std::string& out_path, uint32_t texidx = 0u,
               bool existing_carrier = false) {
  auto data = ReadBinaryFile(path);
  if (data.empty()) {
    std::cerr << "  ERROR: cannot read file\n";
    return 1;
  }
  uint32_t new_hash = 0;
  dxbc::PixelShaderPatchOptions options;
  options.existing_velocity_carrier = existing_carrier;
  const bool patched = dxbc::PatchPerObjectPixelShader(data, &new_hash, texidx, options);
  if (!patched) {
    std::cout << "  NOT PATCHED (not a G-buffer PS or gate rejected)\n";
    dxbc::DXBCHeader h;
    std::vector<dxbc::ChunkInfo> chunks;
    if (dxbc::ParseDXBC(data, h, chunks)) {
      uint8_t d[16];
      dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), d);
      std::cout << "  (valid DXBC, hash " << (std::memcmp(d, h.checksum, 16) == 0 ? "MATCH" : "MISMATCH") << ")\n";
    }
    return 0;  // not an error for non-G-buffer files
  }

  if (!WriteBinaryFile(out_path, data)) {
    std::cerr << "  ERROR: cannot write " << out_path << "\n";
    return 1;
  }
  std::cout << "  PATCHED: new CRC32=0x" << std::hex << new_hash << std::dec
            << " size=" << data.size() << " -> " << out_path << "\n";

  // Re-validate the patched blob.
  dxbc::DXBCHeader h;
  std::vector<dxbc::ChunkInfo> chunks;
  if (!dxbc::ParseDXBC(data, h, chunks)) {
    std::cout << "  ERROR: patched blob fails ParseDXBC\n";
    return 1;
  }
  uint8_t d[16];
  dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), d);
  const bool hash_ok = std::memcmp(d, h.checksum, 16) == 0;
  std::cout << "  reparse: OK hash=" << (hash_ok ? "SELF-CONSISTENT" : "MISMATCH (BUG)") << "\n";

  // Show the ISGN TEXCOORD5 + OSGN SV_TARGET3 entries.
  for (const char* name : {"ISGN", "OSGN"}) {
    if (const dxbc::ChunkInfo* sigc = dxbc::FindChunk(chunks, name)) {
      std::vector<dxbc::SignatureEntry> sig;
      if (dxbc::ParseSignature(
              reinterpret_cast<const uint8_t*>(data.data()) + sigc->offset + dxbc::kChunkHeaderSize,
              sigc->size, sig)) {
        std::cout << "  " << name << " now " << sig.size() << " entries:";
        for (const auto& e : sig) {
          std::cout << " \"" << e.name << (e.semantic_index ? std::to_string(e.semantic_index) : "")
                    << "\"@v" << e.register_index;
        }
        std::cout << "\n";
      }
    }
  }

  // Show the injected block (last instructions before ret).
  const dxbc::ChunkInfo* shex = dxbc::FindChunk(chunks, "SHEX");
  if (!shex) shex = dxbc::FindChunk(chunks, "SHDR");
  if (shex) {
    std::vector<dxbc::Instruction> insns;
    if (dxbc::IterateInstructions(
            reinterpret_cast<const uint8_t*>(data.data()) + shex->offset + dxbc::kChunkHeaderSize,
            shex->size, insns)) {
      std::cout << "  SHEX instr count=" << insns.size() << " (last 10):\n";
      size_t start = insns.size() > 10u ? insns.size() - 10u : 0u;
      for (size_t i = start; i < insns.size(); ++i) {
        std::cout << "    " << NameOfOpcode(insns[i].opcode) << "\n";
      }
    }
  }
  return 0;
}

// Patch a rigid VS in place (Phase W discriminator test), write the patched blob
// to `out_path`, and re-validate. Reports the discriminator signals (LightDirForChar
// / RimLitColor / WorldViewProjection / World) so the book-vs-wall classification
// is visible without decompiling.
int PatchOneRigid(const std::string& path, const std::string& out_path) {
  auto data = ReadBinaryFile(path);
  if (data.empty()) {
    std::cerr << "  ERROR: cannot read file\n";
    return 1;
  }
  uint32_t new_hash = 0;
  dxbc::PatchInfo info;
  dxbc::PatchOptions options;
  options.emit_velocity_carrier = true;  // the rigid path only runs when the carrier is on
  const bool patched = dxbc::PatchRigidVertexShader(data, &new_hash, &info, options);
  if (!patched) {
    std::cout << "  NOT PATCHED (not a rigid character-attached VS, or parse failed)\n";
    return 0;  // not an error: wall-style shaders are correctly rejected
  }

  if (!WriteBinaryFile(out_path, data)) {
    std::cerr << "  ERROR: cannot write " << out_path << "\n";
    return 1;
  }
  std::cout << "  PATCHED: new CRC32=0x" << std::hex << new_hash << std::dec
            << " size=" << data.size() << " -> " << out_path << "\n";
  std::cout << "  cbWorld=" << info.prev_world_cb_slot
            << " cbVP=" << info.prev_vp_cb_slot
            << " world@c" << info.world_cb_element
            << " tex10=o" << info.tex10_out_reg
            << " rigid=" << (info.rigid_patch ? "yes" : "no") << "\n";

  // Re-validate the patched blob.
  dxbc::DXBCHeader h;
  std::vector<dxbc::ChunkInfo> chunks;
  if (!dxbc::ParseDXBC(data, h, chunks)) {
    std::cout << "  ERROR: patched blob fails ParseDXBC\n";
    return 1;
  }
  uint8_t d[16];
  dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), d);
  const bool hash_ok = std::memcmp(d, h.checksum, 16) == 0;
  std::cout << "  reparse: OK hash=" << (hash_ok ? "SELF-CONSISTENT" : "MISMATCH (BUG)") << "\n";
  return 0;
}

int RunOne(const std::string& path, bool roundtrip, bool dump) {
  auto data = ReadBinaryFile(path);
  if (data.empty()) {
    std::cerr << "  ERROR: cannot read file\n";
    return 1;
  }

  dxbc::DXBCHeader header;
  std::vector<dxbc::ChunkInfo> chunks;
  if (!dxbc::ParseDXBC(data, header, chunks)) {
    std::cerr << "  ERROR: not a valid DXBC blob\n";
    return 1;
  }

  std::cout << "  DXBC header: version=" << header.version
            << " file_size=" << header.file_size
            << " chunk_count=" << header.chunk_count
            << " actual_size=" << data.size() << "\n";
  if (header.file_size != data.size()) {
    std::cout << "  WARNING: file_size field != actual size\n";
  }

  std::cout << "  chunks:\n";
  for (const auto& c : chunks) {
    std::cout << "    " << c.name << " off=0x" << std::hex << c.offset
              << " size=" << std::dec << c.size << "\n";
  }

  // Signatures.
  for (const auto& name : {"ISGN", "OSGN"}) {
    const dxbc::ChunkInfo* chunk = dxbc::FindChunk(chunks, name);
    if (!chunk) continue;
    std::vector<dxbc::SignatureEntry> sig;
    if (!dxbc::ParseSignature(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                              chunk->size, sig)) {
      std::cout << "  ERROR: failed to parse " << name << "\n";
      return 1;
    }
    PrintSignature(name, sig);
  }

  // RDEF bindings.
  if (const dxbc::ChunkInfo* chunk = dxbc::FindChunk(chunks, "RDEF")) {
    std::vector<dxbc::ResourceBinding> bindings;
    if (dxbc::ParseRDEFBindings(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                                chunk->size, bindings)) {
      std::cout << "  RDEF (" << bindings.size() << " bindings):\n";
      for (const auto& b : bindings) {
        std::cout << "    \"" << b.name << "\""
                  << " type=" << b.input_type << "(" << NameOfInputType(b.input_type) << ")"
                  << " bind=" << b.bind_point
                  << " count=" << b.bind_count
                  << " dim=" << b.dimension << "(" << NameOfDimension(b.dimension) << ")"
                  << " ret=" << b.return_type << "\n";
      }
    } else {
      std::cout << "  ERROR: failed to parse RDEF\n";
    }
  }

  // SHEX instructions.
  if (const dxbc::ChunkInfo* chunk = dxbc::FindChunk(chunks, "SHEX")) {
    std::vector<dxbc::Instruction> insns;
    if (dxbc::IterateInstructions(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                                  chunk->size, insns)) {
      PrintOpcodes(insns);
      if (dump) {
        DumpShex(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                 chunk->size);
      }
    } else {
      std::cout << "  ERROR: failed to iterate SHEX\n";
    }
  }

  // SHDR (same token layout as SHEX, different chunk name).
  if (const dxbc::ChunkInfo* chunk = dxbc::FindChunk(chunks, "SHDR")) {
    std::vector<dxbc::Instruction> insns;
    if (dxbc::IterateInstructions(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                                  chunk->size, insns)) {
      PrintOpcodes(insns);
      if (dump) {
        DumpShex(reinterpret_cast<const uint8_t*>(data.data()) + chunk->offset + dxbc::kChunkHeaderSize,
                 chunk->size);
      }
    } else {
      std::cout << "  ERROR: failed to iterate SHDR\n";
    }
  }

  // MD5 checksum verification.
  uint8_t digest[16];
  dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  const bool hash_match = std::memcmp(digest, header.checksum, 16) == 0;
  std::cout << "  stored checksum: " << Hex(header.checksum, 16) << "\n";
  std::cout << "  recomputed     : " << Hex(digest, 16) << "\n";
  std::cout << "  hash " << (hash_match ? "MATCH" : "MISMATCH") << "\n";

  // Round-trip: append a signature entry to a copy and verify.
  if (roundtrip) {
    const dxbc::ChunkInfo* osgn = dxbc::FindChunk(chunks, "OSGN");
    const dxbc::ChunkInfo* isgn = dxbc::FindChunk(chunks, "ISGN");
    const dxbc::ChunkInfo* sig = osgn ? osgn : isgn;
    if (!sig) {
      std::cout << "  roundtrip: skipped (no OSGN/ISGN chunk)\n";
      return hash_match ? 0 : 1;
    }

    auto copy = data;
    const bool ok = dxbc::AppendSignatureEntry(
        copy, *sig, "TEXCOORD", 99u, 15u, 0xFu, 0xFu,
        2u /* D3D10_SB_REGISTER_COMPONENT_FLOAT32 */);

    dxbc::DXBCHeader h2;
    std::vector<dxbc::ChunkInfo> chunks2;
    const bool reparsed = ok && dxbc::ParseDXBC(copy, h2, chunks2);

    std::cout << "  roundtrip: insert=" << (ok ? "OK" : "FAILED")
              << " reparse=" << (reparsed ? "OK" : "FAILED") << "\n";
    if (reparsed) {
      const dxbc::ChunkInfo* sig2 = dxbc::FindChunk(chunks2, sig->name.c_str());
      std::vector<dxbc::SignatureEntry> sig_entries;
      const bool parsed = dxbc::ParseSignature(
          reinterpret_cast<const uint8_t*>(copy.data()) + sig2->offset + dxbc::kChunkHeaderSize,
          sig2->size, sig_entries);
      std::cout << "  roundtrip: sig parse=" << (parsed ? "OK" : "FAILED")
                << " entries=" << sig_entries.size() << "\n";
      if (parsed && !sig_entries.empty()) {
        const auto& last = sig_entries.back();
        std::cout << "  roundtrip: new entry = \"" << last.name << "\" idx=" << last.semantic_index
                  << " reg=" << last.register_index << " mask=0x" << std::hex
                  << last.mask << " rw=0x" << last.read_write_mask << std::dec << "\n";
        const bool entry_ok = (last.name == "TEXCOORD" && last.semantic_index == 99u &&
                               last.register_index == 15u && last.mask == 0xFu &&
                               last.read_write_mask == 0xFu);
        std::cout << "  roundtrip: new entry " << (entry_ok ? "VERIFIED" : "WRONG") << "\n";

        // Recompute hash on the patched blob (must differ from stored).
        uint8_t digest2[16];
        dxbc::ComputeDXBCHash(reinterpret_cast<const uint8_t*>(copy.data()), copy.size(), digest2);
        const bool hash_changed = std::memcmp(digest2, header.checksum, 16) != 0;
        std::cout << "  roundtrip: hash changed=" << (hash_changed ? "YES (correct - must rewrite)" : "NO (BUG)") << "\n";
      }
    }
  }

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  bool roundtrip = false;
  bool dump = false;
  bool emittest = false;
  bool patch = false;
  bool patchcarrier = false;
  bool patchrigid = false;
  bool patchps = false;
  bool patchpscarrier = false;
  uint32_t texidx = 0u;
  std::string patch_out;
  std::vector<std::string> paths;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--roundtrip") {
      roundtrip = true;
    } else if (arg == "--dump") {
      dump = true;
    } else if (arg == "--emittest") {
      emittest = true;
    } else if (arg == "--patch") {
      patch = true;
    } else if (arg == "--patchcarrier") {
      patchcarrier = true;
    } else if (arg == "--patchrigid") {
      patchrigid = true;
    } else if (arg == "--patchps") {
      patchps = true;
    } else if (arg == "--patchpscarrier") {
      patchpscarrier = true;
    } else if (arg == "--texidx") {
      if (i + 1 < argc) texidx = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 0));
    } else if (arg == "--out") {
      if (i + 1 < argc) patch_out = argv[++i];
    } else {
      paths.push_back(arg);
    }
  }

  if (paths.empty() && !emittest) {
    std::cerr << "USAGE: dxbc_test.exe <path-to.cso> [more.cso ...] [--roundtrip] [--dump]\n";
    std::cerr << "       dxbc_test.exe --patch [<out.cso>] <in.cso>\n";
    std::cerr << "       dxbc_test.exe --patchcarrier [<out.cso>] <in.cso>\n";
    std::cerr << "       dxbc_test.exe --patchrigid [<out.cso>] <in.cso>\n";
    std::cerr << "       dxbc_test.exe --patchps [<out.cso>] <in.cso>\n";
    std::cerr << "       dxbc_test.exe --patchpscarrier [<out.cso>] <in.cso>\n";
    std::cerr << "  Parses DXBC blobs, prints signatures/RDEF/SHEX, verifies the MD5\n";
    std::cerr << "  checksum, and (with --roundtrip) tests signature insertion.\n";
    std::cerr << "  --dump prints every SHEX instruction's raw dwords + decode.\n";
    std::cerr << "  --emittest prints the emitter-built injected block (no files needed).\n";
    std::cerr << "  --patch patches a skinned VS (generic Phase B) and writes out.cso.\n";
    std::cerr << "  --patchps [--texidx <n>] patches a G-buffer PS (Phase E) and writes out.cso.\n";
    std::cerr << "  --patchpscarrier patches a PS using the existing TEXCOORD10.xy carrier.\n";
    std::cerr << "    --texidx overrides the prevClip TEXCOORD index with the paired VS's (0=auto).\n";
    return 1;
  }

  if (emittest) {
    std::cout << "=== emitter self-test ===\n";
    EmitSelfTest();
    std::cout << "\n";
    return 0;
  }

  if ((patch || patchcarrier) && !paths.empty()) {
    std::string in = paths[0];
    std::string out = patch_out.empty() ? (in + ".patched.cso") : patch_out;
    std::cout << "=== " << in << " ===\n";
    int rc = PatchOne(in, out, patchcarrier);
    std::cout << "\n";
    return rc;
  }

  if (patchrigid && !paths.empty()) {
    std::string in = paths[0];
    std::string out = patch_out.empty() ? (in + ".patched.cso") : patch_out;
    std::cout << "=== " << in << " ===\n";
    int rc = PatchOneRigid(in, out);
    std::cout << "\n";
    return rc;
  }

  if ((patchps || patchpscarrier) && !paths.empty()) {
    std::string in = paths[0];
    std::string out = patch_out.empty() ? (in + ".patched.cso") : patch_out;
    std::cout << "=== " << in << " ===\n";
    int rc = PatchOnePs(in, out, texidx, patchpscarrier);
    std::cout << "\n";
    return rc;
  }

  int failures = 0;
  for (const auto& path : paths) {
    std::cout << "=== " << path << " ===\n";
    failures += RunOne(path, roundtrip, dump);
    std::cout << "\n";
  }
  std::cout << (failures == 0 ? "ALL FILES OK\n" : "FAILURES: some files errored\n");
  return failures == 0 ? 0 : 1;
}
