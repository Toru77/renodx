/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * DXBC bytecode parsing + token-emit toolkit for the Senkiseki3 DLAA addon.
 *
 * Phase A: parse/validate DXBC, enumerate chunks, read ISGN/OSGN signatures,
 * read RDEF resource bindings, iterate SHEX instructions, and recompute the
 * DXBC MD5 hash (dxbc-spirv algorithm). Also provides the D3D10_SB token
 * encoders needed by Phase B/C (skinned-VS / G-buffer-PS injection) and a
 * signature-entry insertion helper that fixes up chunk offsets so a patched
 * blob round-trips.
 *
 * Ported/adapted from the Luma Framework (MIT) and "dxbc-spirv" by Philip
 * Rebohle (MIT). Self-contained: no ReShade/d3dcompiler headers required.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace senkiseki3::dxbc {

// ── DXBC file header (matches dxbc-spirv / D3D layout) ──
struct DXBCHeader {
  char magic[4];        // "DXBC"
  uint8_t checksum[16]; // MD5 over everything from `version` onward
  uint32_t version;     // always 1
  uint32_t file_size;
  uint32_t chunk_count;
  // uint32_t chunk_offsets[chunk_count];
};

static constexpr uint32_t kChunkHeaderSize = 8u;  // name[4] + size[4]

// Chunk header: char name[4]; uint32_t size (size of data AFTER this header).
struct ChunkInfo {
  std::string name;
  uint32_t offset = 0u;  // absolute offset of the chunk header in the blob
  uint32_t size = 0u;    // chunk data size (excludes the 8-byte chunk header)
};

// Validate magic + bounds. Fills chunk list on success.
inline bool ParseDXBC(const uint8_t* data, size_t size, DXBCHeader& header,
                      std::vector<ChunkInfo>& chunks) {
  if (size < sizeof(DXBCHeader)) return false;
  if (std::memcmp(data, "DXBC", 4) != 0) return false;
  std::memcpy(&header, data, sizeof(DXBCHeader));
  if (header.version != 1u) return false;
  if (header.file_size != size) return false;
  if (header.chunk_count > 64u) return false;  // sanity guard
  chunks.clear();
  chunks.reserve(header.chunk_count);
  const uint32_t* offsets = reinterpret_cast<const uint32_t*>(data + sizeof(DXBCHeader));
  for (uint32_t i = 0; i < header.chunk_count; ++i) {
    const uint32_t off = offsets[i];
    if (off + kChunkHeaderSize > size) return false;
    ChunkInfo ci;
    ci.name.assign(reinterpret_cast<const char*>(data + off), 4);
    std::memcpy(&ci.size, data + off + 4, 4);
    ci.offset = off;
    if (off + kChunkHeaderSize + ci.size > size) return false;
    chunks.push_back(std::move(ci));
  }
  return true;
}

inline bool ParseDXBC(const std::vector<std::byte>& data, DXBCHeader& header,
                      std::vector<ChunkInfo>& chunks) {
  return ParseDXBC(reinterpret_cast<const uint8_t*>(data.data()), data.size(), header, chunks);
}

inline const ChunkInfo* FindChunk(const std::vector<ChunkInfo>& chunks, std::string_view name) {
  for (const auto& c : chunks)
    if (c.name == name) return &c;
  return nullptr;
}

// ── Signature entries (ISGN / OSGN / PC2S etc.) ──
// On-disk entry format (24 bytes, verified against real .cso dumps):
//   +0  uint32 name_offset       (relative to the chunk DATA start, after the 8-byte chunk header)
//   +4  uint32 semantic_index
//   +8  uint32 system_value_type (D3D10_SB_NAME_*)
//   +12 uint32 component_type    (D3D10_SB_REGISTER_COMPONENT_TYPE_*)
//   +16 uint32 register_index    (vN)
//   +20 uint8  component_mask
//   +21 uint8  read_write_mask
//   +22..23 padding
// The chunk itself is: uint32 count; uint32 unknown(=8); entry[count]; strings.
static constexpr uint32_t kSignatureEntrySize = 24u;
struct SignatureEntry {
  std::string name;            // semantic name ("SV_POSITION", "TEXCOORD", ...)
  uint32_t name_offset = 0;    // raw on-disk offset (relative to chunk data start)
  uint32_t semantic_index = 0;
  uint32_t system_value_type = 0;  // D3D10_SB_NAME_*
  uint32_t component_type = 0;     // D3D10_SB_REGISTER_COMPONENT_TYPE_*
  uint32_t register_index = 0;     // vN
  uint32_t mask = 0;               // component mask
  uint32_t read_write_mask = 0;    // 0 = read-only (OSGN), 0xF = RW (ISGN)
  uint32_t stream = 0;             // unused (D3D11 only)
};

// Parse one signature chunk (ISGN/OSGN/OSG5/PC2S...). `chunk_data` must point
// at the chunk DATA (after the 8-byte chunk header); `chunk_size` is the data
// size. Layout (verified on-disk):
//   uint32 count; uint32 unknown(=8); entry[count] (24 bytes each); strings.
inline bool ParseSignature(const uint8_t* chunk_data, uint32_t chunk_size,
                           std::vector<SignatureEntry>& out) {
  constexpr uint32_t kEntriesOffset = 8u;  // skip count + unknown
  if (chunk_size < kEntriesOffset) return false;
  const uint32_t count = *reinterpret_cast<const uint32_t*>(chunk_data);
  if (kEntriesOffset + count * kSignatureEntrySize > chunk_size) return false;
  out.clear();
  out.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t* e = chunk_data + kEntriesOffset + i * kSignatureEntrySize;
    SignatureEntry se;
    std::memcpy(&se.name_offset, e + 0, 4);
    std::memcpy(&se.semantic_index, e + 4, 4);
    std::memcpy(&se.system_value_type, e + 8, 4);
    std::memcpy(&se.component_type, e + 12, 4);
    std::memcpy(&se.register_index, e + 16, 4);
    se.mask = e[20];
    se.read_write_mask = e[21];
    if (se.name_offset >= chunk_size) return false;
    se.name = reinterpret_cast<const char*>(chunk_data + se.name_offset);
    out.push_back(std::move(se));
  }
  return true;
}

inline bool ParseSignature(const std::vector<std::byte>& data, const ChunkInfo& chunk,
                           std::vector<SignatureEntry>& out) {
  if (chunk.offset + kChunkHeaderSize + chunk.size > data.size()) return false;
  return ParseSignature(reinterpret_cast<const uint8_t*>(data.data()) + chunk.offset + kChunkHeaderSize,
                        chunk.size, out);
}

// Find the largest TEXCOORDn register index currently used in a signature.
inline int MaxTexcoordIndex(const std::vector<SignatureEntry>& sig) {
  int max_idx = -1;
  for (const auto& e : sig)
    if (e.name == "TEXCOORD" && (int)e.semantic_index > max_idx) max_idx = (int)e.semantic_index;
  return max_idx;
}

// Find the first free TEXCOORDn register (used by Phase B to add prevClip).
inline int NextFreeTexcoord(const std::vector<SignatureEntry>& sig) {
  return MaxTexcoordIndex(sig) + 1;
}

// ── RDEF resource bindings ──
struct ResourceBinding {
  std::string name;
  uint32_t input_type = 0;       // D3D_SIT_* (0=cbuffer, 6=texture, 7=SRV/structured)
  uint32_t bind_point = 0;       // register (t0, b0, ...)
  uint32_t bind_count = 1;
  uint32_t return_type = 0;
  uint32_t dimension = 0;
};

inline bool ParseRDEFBindings(const uint8_t* chunk_data, uint32_t chunk_size,
                              std::vector<ResourceBinding>& out) {
  // RDEF layout: header dwords then offset tables (constants/refl type).
  if (chunk_size < 40u) return false;
  const uint32_t* h = reinterpret_cast<const uint32_t*>(chunk_data);
  // h[0] = cbuffer count, h[1] = cbuffer offset
  // h[2] = resource binding count, h[3] = resource binding offset
  const uint32_t binding_count = h[2];
  const uint32_t binding_offset = h[3];
  constexpr uint32_t kBindingSize = 32u;
  if (binding_offset + binding_count * kBindingSize > chunk_size) return false;
  out.clear();
  out.reserve(binding_count);
  for (uint32_t i = 0; i < binding_count; ++i) {
    const uint8_t* b = chunk_data + binding_offset + i * kBindingSize;
    uint32_t name_offset, input_type, return_type, dimension, sample_count;
    uint32_t bind_point, bind_count, flags;
    std::memcpy(&name_offset, b + 0, 4);
    std::memcpy(&input_type, b + 4, 4);
    std::memcpy(&return_type, b + 8, 4);
    std::memcpy(&dimension, b + 12, 4);
    std::memcpy(&sample_count, b + 16, 4);
    std::memcpy(&bind_point, b + 20, 4);
    std::memcpy(&bind_count, b + 24, 4);
    std::memcpy(&flags, b + 28, 4);
    (void)sample_count;
    (void)flags;
    ResourceBinding rb;
    rb.name = (name_offset < chunk_size) ? reinterpret_cast<const char*>(chunk_data + name_offset) : "";
    rb.input_type = input_type;
    rb.return_type = return_type;
    rb.dimension = dimension;
    rb.bind_point = bind_point;
    rb.bind_count = bind_count;
    out.push_back(std::move(rb));
  }
  return true;
}

// ── SHEX instruction iteration ──
// SHEX header: 8-byte chunk header (already stripped by caller), then
//   uint32 version_and_type; uint32 dword_count; then token stream.
struct ShexHeader {
  uint32_t version_type = 0;
  uint32_t dword_count = 0;
};

inline bool ParseShexHeader(const uint8_t* chunk_data, uint32_t chunk_size, ShexHeader& out) {
  if (chunk_size < 8u) return false;
  std::memcpy(&out.version_type, chunk_data, 4);
  std::memcpy(&out.dword_count, chunk_data + 4, 4);
  return true;
}

// One tokenized instruction's position/length in the SHEX stream.
struct Instruction {
  uint32_t offset = 0u;  // byte offset of the opcode token (relative to chunk data)
  uint32_t length = 0u;  // total length in dwords
  uint32_t opcode = 0u;  // D3D10_SB_OPCODE_TYPE
};

inline bool IterateInstructions(const uint8_t* chunk_data, uint32_t chunk_size,
                                std::vector<Instruction>& out) {
  ShexHeader h;
  if (!ParseShexHeader(chunk_data, chunk_size, h)) return false;
  uint32_t pos = 8u;  // skip SHEX header (version/type + dword count)
  const uint32_t end = chunk_size;
  out.clear();
  while (pos + 4u <= end) {
    uint32_t token;
    std::memcpy(&token, chunk_data + pos, 4);
    const uint32_t opcode = token & 0x7FFu;
    const uint32_t len = token >> 24u;
    if (len == 0u) break;  // malformed
    Instruction ins;
    ins.offset = pos;
    ins.length = len;
    ins.opcode = opcode;
    out.push_back(ins);
    pos += len * 4u;
  }
  return true;
}

// ── D3D10_SB opcode types ──
// Values VERIFIED against fxc 10.1 output (D3D12TokenizedProgramFormat.hpp):
//   ADD=0 DP4=17 MAD=50 MIN=51 MAX=52 MOV=54 MUL=56 RSQ=68 RET=62
//   DCL_CONSTANT_BUFFER=89 DCL_INPUT=95 DCL_OUTPUT=101 DCL_OUTPUT_SIV=103
//   DCL_TEMPS=104 DCL_GLOBAL_FLAGS=106 DCL_RESOURCE_STRUCTURED=162 LD_STRUCTURED=167
enum OpcodeType : uint32_t {
  OP_ADD = 0, OP_AND = 1, OP_BREAK = 2, OP_BREAKC = 3, OP_CALL = 4, OP_CALLC = 5,
  OP_CASE = 6, OP_CONTINUE = 7, OP_CONTINUEC = 8, OP_CUT = 9, OP_DEFAULT = 10,
  OP_DERIV_RTX = 11, OP_DERIV_RTY = 12, OP_DISCARD = 13, OP_DIV = 14, OP_DP2 = 15,
  OP_DP3 = 16, OP_DP4 = 17, OP_ELSE = 18, OP_EMIT = 19, OP_EMITTHENCUT = 20,
  OP_ENDIF = 21, OP_ENDLOOP = 22, OP_ENDSWITCH = 23, OP_EQ = 24, OP_EXP = 25,
  OP_FRC = 26, OP_FTOI = 27, OP_FTOU = 28, OP_GE = 29, OP_IADD = 30, OP_IF = 31,
  OP_IEQ = 32, OP_IGE = 33, OP_ILT = 34, OP_IMAD = 35, OP_IMAX = 36, OP_IMIN = 37,
  OP_IMUL = 38, OP_INE = 39, OP_INEG = 40, OP_ISHL = 41, OP_ISHR = 42, OP_ITOF = 43,
  OP_LABEL = 44, OP_LD = 45, OP_LD_MS = 46, OP_LOG = 47, OP_LOOP = 48, OP_LT = 49,
  OP_MAD = 50, OP_MIN = 51, OP_MAX = 52, OP_CUSTOMDATA = 53, OP_MOV = 54, OP_MOVC = 55,
  OP_MUL = 56, OP_NE = 57, OP_NOP = 58, OP_NOT = 59, OP_OR = 60, OP_RESINFO = 61,
  OP_RET = 62, OP_RETC = 63, OP_ROUND_NE = 64, OP_ROUND_NI = 65, OP_ROUND_PI = 66,
  OP_ROUND_Z = 67, OP_RSQ = 68, OP_SAMPLE = 69, OP_SAMPLE_C = 70, OP_SAMPLE_C_LZ = 71,
  OP_SAMPLE_L = 72, OP_SAMPLE_D = 73, OP_SAMPLE_B = 74, OP_SQRT = 75, OP_SWITCH = 76,
  OP_SINCOS = 77, OP_UDIV = 78, OP_ULT = 79, OP_UGE = 80, OP_UMUL = 81, OP_UMAD = 82,
  OP_UMAX = 83, OP_UMIN = 84, OP_USHR = 85, OP_UTOF = 86, OP_XOR = 87,
  OP_DCL_RESOURCE = 88, OP_DCL_CONSTANT_BUFFER = 89, OP_DCL_SAMPLER = 90,
  OP_DCL_INDEX_RANGE = 91, OP_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY = 92,
  OP_DCL_GS_INPUT_PRIMITIVE = 93, OP_DCL_MAX_OUTPUT_VERTEX_COUNT = 94,
  OP_DCL_INPUT = 95, OP_DCL_INPUT_SGV = 96, OP_DCL_INPUT_SIV = 97,
  OP_DCL_INPUT_PS = 98, OP_DCL_INPUT_PS_SGV = 99, OP_DCL_INPUT_PS_SIV = 100,
  OP_DCL_OUTPUT = 101, OP_DCL_OUTPUT_SGV = 102, OP_DCL_OUTPUT_SIV = 103,
  OP_DCL_TEMPS = 104, OP_DCL_INDEXABLE_TEMP = 105, OP_DCL_GLOBAL_FLAGS = 106,
  // D3D11 opcodes we need (verified):
  OP_DCL_RESOURCE_STRUCTURED = 162,  // verified vs fxc (0xA2)
  OP_LD_STRUCTURED = 167,            // verified vs fxc (0xA7)
};

// ── D3D10_SB token encoders (bit fields VERIFIED vs fxc 10.1) ──
// Opcode token: [10:00] opcode, [23:11] controls, [30:24] length, [31] extended.
constexpr uint32_t EncodeOpcode(uint32_t opcode) { return opcode & 0x7FFu; }
constexpr uint32_t EncodeInstructionLength(uint32_t len) { return (len & 0x7Fu) << 24u; }
constexpr uint32_t EncodeSaturate(uint32_t sat) { return (sat & 1u) << 20u; }
constexpr uint32_t EncodeOpcodeExtended(uint32_t ext) { return (ext & 1u) << 31u; }

// Operand token bit fields (from D3D12TokenizedProgramFormat.hpp):
//   [01:00] num_components  (0=0,1=1,2=4)
//   [03:02] selection mode  (0=mask, 1=swizzle, 2=select_1)
//   [07:04] mask | swizzle | select_1 component
//   [19:12] operand type
//   [21:20] index dimension (0D/1D/2D/3D)
//   [22+3*d : 24+3*d] index representation for dim d (IMMEDIATE32=0, IMMEDIATE64=1, RELATIVE=2)
// Index values (1 per dimension) follow the operand token as extra DWORDs.
constexpr uint32_t kOperandNumComponentsShift = 0u;
constexpr uint32_t kOperandSelectionModeShift = 2u;
constexpr uint32_t kOperandSelectionShift = 4u;
constexpr uint32_t kOperandTypeShift = 12u;
constexpr uint32_t kOperandIndexDimShift = 20u;
constexpr uint32_t kOperandIndexRepShift = 22u;  // + 3*dim

enum OperandType : uint32_t {
  OPERAND_TEMP = 0, OPERAND_INPUT = 1, OPERAND_OUTPUT = 2, OPERAND_INDEXABLE_TEMP = 3,
  OPERAND_IMMEDIATE32 = 4, OPERAND_IMMEDIATE64 = 5, OPERAND_SAMPLER = 6,
  OPERAND_RESOURCE = 7, OPERAND_CONSTANT_BUFFER = 8, OPERAND_IMMEDIATE_CONSTANT_BUFFER = 9,
  OPERAND_LABEL = 10, OPERAND_INPUT_PRIMITIVEID = 11, OPERAND_OUTPUT_DEPTH = 12,
  OPERAND_NULL = 13, OPERAND_RASTERIZER = 14, OPERAND_OUTPUT_COVERAGE_MASK = 15,
  OPERAND_STREAM = 16, OPERAND_FUNCTION_BODY = 17, OPERAND_FUNCTION_TABLE = 18,
  OPERAND_INTERFACE = 19, OPERAND_FUNCTION_INPUT = 20, OPERAND_FUNCTION_OUTPUT = 21,
  OPERAND_OUTPUT_CONTROL_POINT_ID = 22, OPERAND_INPUT_FORK_INSTANCE_ID = 23,
  OPERAND_INPUT_JOIN_INSTANCE_ID = 24, OPERAND_INPUT_CONTROL_POINT = 25,
  OPERAND_OUTPUT_CONTROL_POINT = 26, OPERAND_INPUT_PATCH_CONSTANT = 27,
  OPERAND_INPUT_DOMAIN_POINT = 28, OPERAND_THIS_POINTER = 29,
  OPERAND_UNORDERED_ACCESS_VIEW = 30, OPERAND_THREAD_GROUP_SHARED_MEMORY = 31,
  OPERAND_INPUT_THREAD_ID = 32, OPERAND_INPUT_THREAD_GROUP_ID = 33,
  OPERAND_INPUT_THREAD_ID_IN_GROUP = 34, OPERAND_INPUT_COVERAGE_MASK = 35,
  OPERAND_INPUT_THREAD_ID_IN_GROUP_FLATTENED = 36, OPERAND_INPUT_GS_INSTANCE_ID = 37,
  OPERAND_OUTPUT_DEPTH_GREATER_EQUAL = 38, OPERAND_OUTPUT_DEPTH_LESS_EQUAL = 39,
  OPERAND_CYCLE_COUNTER = 40, OPERAND_OUTPUT_STENCIL_REF = 41, OPERAND_INNER_COVERAGE = 42,
};

enum IndexRepresentation : uint32_t {
  INDEX_IMMEDIATE32 = 0, INDEX_IMMEDIATE64 = 1, INDEX_RELATIVE = 2,
  INDEX_IMMEDIATE32_PLUS_RELATIVE = 3, INDEX_IMMEDIATE64_PLUS_RELATIVE = 4,
};

// Number-of-components encoding: 0,1,4 -> 0,1,2
constexpr uint32_t NumComponentsOf(uint32_t n) {
  return (n == 1u) ? 1u : (n == 4u ? 2u : 0u);
}

// Build a single-dword operand token with the given index representations
// (each 3-bit). Index VALUES are appended separately via AppendOperandIndex.
inline uint32_t EncodeOperand(uint32_t num_components, uint32_t selection_mode, uint32_t selection,
                              uint32_t operand_type, uint32_t index_dim,
                              const uint32_t* index_reps) {
  uint32_t t = 0;
  t |= (num_components & 0x3u) << kOperandNumComponentsShift;
  t |= (selection_mode & 0x3u) << kOperandSelectionModeShift;
  t |= (selection & 0xFu) << kOperandSelectionShift;
  t |= (operand_type & 0xFFu) << kOperandTypeShift;
  t |= (index_dim & 0x3u) << kOperandIndexDimShift;
  for (uint32_t d = 0; d < index_dim; ++d)
    t |= (index_reps[d] & 0x7u) << (kOperandIndexRepShift + 3u * d);
  return t;
}

// Append the index value(s) for an operand (one DWORD per immediate32 index,
// or one operand-token+value pair per relative index).
inline void AppendOperandIndex(std::vector<uint32_t>& out, uint32_t rep, uint32_t value) {
  out.push_back(value);
}

// Build an instruction opcode token (non-extended).
inline uint32_t EncodeInstruction(uint32_t opcode, uint32_t length, uint32_t saturate = 0u) {
  return EncodeOpcode(opcode) | EncodeInstructionLength(length) | EncodeSaturate(saturate);
}

// ── Instruction emitters ──
// All byte patterns VERIFIED against fxc 10.1 output (probe_decls/probe_ops).
// Operand templates (1D, immediate32 index):
//   temp dst (mask):       0x00100000 | 2 | (mask<<4)            + [reg]
//   temp src (swizzle):    0x00100006 | (swizzle<<4)             + [reg]
//   temp select_1:         0x0010000A | (comp<<4)                + [reg]
//   input src (swizzle):   0x00101006 | (swizzle<<4)             + [reg]
//   input select_1:        0x0010100A | (comp<<4)                + [reg]
//   output dst (mask):     0x00102000 | 2 | (mask<<4)            + [reg]
//   resource src (swizzle):0x00107006 | (swizzle<<4)             + [reg]
//   resource dcl (none):   0x00107000                            + [reg]
//   cbuffer src .xyzw:     0x00208E46                            + [slot, element]
//   immediate 1 comp:      0x00004001                            + [value]
//   immediate 4 comp:      0x00004002                            + [4 values]
// Swizzle packing: [1:0]=dest.x, [3:2]=dest.y, [5:4]=dest.z, [7:6]=dest.w.
//   .xyzw=0xE4 .xxxx=0x00 .yyyy=0x55 .zzzz=0xAA .wwww=0xFF

constexpr uint32_t kSwizzleXYZW = 0xE4u;
constexpr uint32_t kSwizzleXXXX = 0x00u;
constexpr uint32_t kSwizzleYYYY = 0x55u;
constexpr uint32_t kSwizzleZZZZ = 0xAAu;
constexpr uint32_t kSwizzleWWWW = 0xFFu;

inline void EmitOpcode(std::vector<uint32_t>& out, uint32_t opcode, uint32_t length) {
  out.push_back(EncodeInstruction(opcode, length));
}

// temp dst operand (mask mode)
inline void EmitTempDst(std::vector<uint32_t>& out, uint32_t reg, uint32_t mask) {
  out.push_back(0x00100000u | 2u | (mask << 4u));
  out.push_back(reg);
}

// temp src operand (swizzle mode)
inline void EmitTempSrc(std::vector<uint32_t>& out, uint32_t reg, uint32_t swizzle) {
  out.push_back(0x00100006u | (swizzle << 4u));
  out.push_back(reg);
}

// temp src operand (select_1)
inline void EmitTempSel1(std::vector<uint32_t>& out, uint32_t reg, uint32_t comp) {
  out.push_back(0x0010000Au | (comp << 4u));
  out.push_back(reg);
}

// input src operand (swizzle mode)
inline void EmitInputSrc(std::vector<uint32_t>& out, uint32_t reg, uint32_t swizzle) {
  out.push_back(0x00101006u | (swizzle << 4u));
  out.push_back(reg);
}

// input src operand (select_1: x=0 y=1 z=2 w=3)
inline void EmitInputSel1(std::vector<uint32_t>& out, uint32_t reg, uint32_t comp) {
  out.push_back(0x0010100Au | (comp << 4u));
  out.push_back(reg);
}

// output dst operand (mask mode)
inline void EmitOutputDst(std::vector<uint32_t>& out, uint32_t reg, uint32_t mask) {
  out.push_back(0x00102000u | 2u | (mask << 4u));
  out.push_back(reg);
}

// resource src operand (swizzle mode)
inline void EmitResourceSrc(std::vector<uint32_t>& out, uint32_t reg, uint32_t swizzle) {
  out.push_back(0x00107006u | (swizzle << 4u));
  out.push_back(reg);
}

// cbuffer src operand (.xyzw read of cbN[element])
inline void EmitCbSrc(std::vector<uint32_t>& out, uint32_t slot, uint32_t element) {
  out.push_back(0x00208E46u);
  out.push_back(slot);
  out.push_back(element);
}

// immediate32 operand, 1 component
inline void EmitImm1(std::vector<uint32_t>& out, uint32_t bits) {
  out.push_back(0x00004001u);
  out.push_back(bits);
}

// immediate32 operand, 4 components (bits order x,y,z,w)
inline void EmitImm4(std::vector<uint32_t>& out, uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  out.push_back(0x00004002u);
  out.push_back(x);
  out.push_back(y);
  out.push_back(z);
  out.push_back(w);
}

// mov dst.xyzw, src  (opcode 54, len 5)
inline void EmitMov(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t src_reg, uint32_t src_swizzle) {
  EmitOpcode(out, OP_MOV, 5u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, src_reg, src_swizzle);
}

// mov dst.xyzw, src (1-comp select_1 source)
inline void EmitMovSel1(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t src_reg, uint32_t src_comp) {
  EmitOpcode(out, OP_MOV, 5u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSel1(out, src_reg, src_comp);
}

// mov dst.xyzw, imm (4-comp immediate)
inline void EmitMovImm4(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
  EmitOpcode(out, OP_MOV, 8u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitImm4(out, x, y, z, w);
}

// mov dst.xyz, vN.xyz  (input src, mask dst)
inline void EmitMovInput(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                         uint32_t input_reg, uint32_t input_mask) {
  EmitOpcode(out, OP_MOV, 5u);
  EmitTempDst(out, dst_reg, dst_mask);
  out.push_back(0x00101002u | (input_mask << 4u));  // input, mask mode
  out.push_back(input_reg);
}

// mov dst.w, imm (1-comp immediate)
inline void EmitMovImm1(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t bits) {
  EmitOpcode(out, OP_MOV, 5u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitImm1(out, bits);
}

// mul dst, src0, src1  (opcode 56, len 7)
inline void EmitMul(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_MUL, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// mul dst, src0.xyzw, imm (4-comp)
inline void EmitMulImm4(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t s0_reg, uint32_t s0_swizzle, uint32_t x, uint32_t y, uint32_t z,
                        uint32_t w) {
  EmitOpcode(out, OP_MUL, 10u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitImm4(out, x, y, z, w);
}

// mad dst, src0, src1, src2  (opcode 50, len 9)
inline void EmitMad(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle,
                    uint32_t s2_reg, uint32_t s2_swizzle) {
  EmitOpcode(out, OP_MAD, 9u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
  EmitTempSrc(out, s2_reg, s2_swizzle);
}

// mad dst, src0 (swizzle), imm (4-comp), src2 (swizzle)
inline void EmitMadImm4(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t s0_reg, uint32_t s0_swizzle, uint32_t x, uint32_t y, uint32_t z,
                        uint32_t w, uint32_t s2_reg, uint32_t s2_swizzle) {
  EmitOpcode(out, OP_MAD, 12u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitImm4(out, x, y, z, w);
  EmitTempSrc(out, s2_reg, s2_swizzle);
}

// mad dst, cbN[e].xyzw, src1, src2  (opcode 50, len 10)
inline void EmitMadCb(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                      uint32_t cb_slot, uint32_t cb_element, uint32_t s1_reg, uint32_t s1_swizzle,
                      uint32_t s2_reg, uint32_t s2_swizzle) {
  EmitOpcode(out, OP_MAD, 10u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitCbSrc(out, cb_slot, cb_element);
  EmitTempSrc(out, s1_reg, s1_swizzle);
  EmitTempSrc(out, s2_reg, s2_swizzle);
}

// mul dst, vN.swizzle (input src0), src1  (opcode 56, len 7)
inline void EmitMulInput(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                         uint32_t input_reg, uint32_t input_swizzle, uint32_t s1_reg,
                         uint32_t s1_swizzle) {
  EmitOpcode(out, OP_MUL, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitInputSrc(out, input_reg, input_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// mad dst, vN.swizzle (input src0), src1, src2  (opcode 50, len 9)
inline void EmitMadInput(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                         uint32_t input_reg, uint32_t input_swizzle, uint32_t s1_reg,
                         uint32_t s1_swizzle, uint32_t s2_reg, uint32_t s2_swizzle) {
  EmitOpcode(out, OP_MAD, 9u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitInputSrc(out, input_reg, input_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
  EmitTempSrc(out, s2_reg, s2_swizzle);
}

// dp3 dst, src0, src1  (opcode 16, len 7)
inline void EmitDp3(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_DP3, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// add dst, src0, src1 (opcode 0, len 7)
inline void EmitAdd(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_ADD, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// add dst, src0, cbN[e].xyzw  (opcode 0, len 8)
inline void EmitAddCb(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                      uint32_t s0_reg, uint32_t s0_swizzle, uint32_t cb_slot,
                      uint32_t cb_element) {
  EmitOpcode(out, OP_ADD, 8u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitCbSrc(out, cb_slot, cb_element);
}

// dp4 dst, src0, src1  (opcode 17, len 7)
inline void EmitDp4(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_DP4, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// dp4 dst.x, cbN[e].xyzw, src  (opcode 17, len 8)
inline void EmitDp4Cb(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                      uint32_t cb_slot, uint32_t cb_element, uint32_t s1_reg,
                      uint32_t s1_swizzle) {
  EmitOpcode(out, OP_DP4, 8u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitCbSrc(out, cb_slot, cb_element);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// max dst, src0, src1 (opcode 52, len 7)
inline void EmitMax(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_MAX, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// min dst, src0, src1 (opcode 51, len 7)
inline void EmitMin(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg, uint32_t s1_swizzle) {
  EmitOpcode(out, OP_MIN, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// max dst, src, l(imm) (opcode 52, len 7)
inline void EmitMaxImm1(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t s0_reg, uint32_t s0_swizzle, uint32_t imm_bits) {
  EmitOpcode(out, OP_MAX, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitImm1(out, imm_bits);
}

// div dst, src0, src1  (opcode 14, len 7)
inline void EmitDiv(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle, uint32_t s1_reg,
                    uint32_t s1_swizzle) {
  EmitOpcode(out, OP_DIV, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// min dst, src, l(imm) (opcode 51, len 7)
inline void EmitMinImm1(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t s0_reg, uint32_t s0_swizzle, uint32_t imm_bits) {
  EmitOpcode(out, OP_MIN, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitImm1(out, imm_bits);
}

// add dst, src, l(imm) (opcode 0, len 7)
inline void EmitAddImm1(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                        uint32_t s0_reg, uint32_t s0_swizzle, uint32_t imm_bits) {
  EmitOpcode(out, OP_ADD, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
  EmitImm1(out, imm_bits);
}

// rsq dst, src (opcode 68, len 5)
inline void EmitRsq(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                    uint32_t s0_reg, uint32_t s0_swizzle) {
  EmitOpcode(out, OP_RSQ, 5u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitTempSrc(out, s0_reg, s0_swizzle);
}

// ld_structured dst.xyz, vBlend.comp (select_1), l(byte_off), tPrev (opcode 167, len 9)
inline void EmitLdStructured(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                             uint32_t index_input_reg, uint32_t index_comp, uint32_t byte_off,
                             uint32_t resource_reg, uint32_t resource_swizzle) {
  EmitOpcode(out, OP_LD_STRUCTURED, 9u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitInputSel1(out, index_input_reg, index_comp);
  EmitImm1(out, byte_off);
  EmitResourceSrc(out, resource_reg, resource_swizzle);
}

// ld_structured dst.xyz, vBlend.comp, l(byte_off), tPrev  -- with .xyzx load (3 comps from 4)
inline void EmitLdStructured3(std::vector<uint32_t>& out, uint32_t dst_reg,
                              uint32_t index_input_reg, uint32_t index_comp, uint32_t byte_off,
                              uint32_t resource_reg, uint32_t resource_swizzle) {
  EmitLdStructured(out, dst_reg, 0x7u, index_input_reg, index_comp, byte_off, resource_reg,
                   resource_swizzle);
}

// mov dst.xyzw, src (output write)
inline void EmitMovOutput(std::vector<uint32_t>& out, uint32_t out_reg, uint32_t src_reg,
                          uint32_t src_swizzle) {
  EmitOpcode(out, OP_MOV, 5u);
  EmitOutputDst(out, out_reg, 0xFu);
  EmitTempSrc(out, src_reg, src_swizzle);
}

// mov dst.xyzw, cbN[e].xyzw  (opcode 54, len 6)
inline void EmitMovCb(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                      uint32_t cb_slot, uint32_t cb_element) {
  EmitOpcode(out, OP_MOV, 6u);
  EmitTempDst(out, dst_reg, dst_mask);
  EmitCbSrc(out, cb_slot, cb_element);
}

// dp3 dst, vN.xyz (input src0), src1  (opcode 16, len 7)
inline void EmitDp3Input(std::vector<uint32_t>& out, uint32_t dst_reg, uint32_t dst_mask,
                         uint32_t input_reg, uint32_t input_mask, uint32_t s1_reg,
                         uint32_t s1_swizzle) {
  EmitOpcode(out, OP_DP3, 7u);
  EmitTempDst(out, dst_reg, dst_mask);
  out.push_back(0x00101002u | (input_mask << 4u));  // input, mask mode
  out.push_back(input_reg);
  EmitTempSrc(out, s1_reg, s1_swizzle);
}

// ret (opcode 62, len 1)
inline void EmitRet(std::vector<uint32_t>& out) { EmitOpcode(out, OP_RET, 1u); }

// dcl_constantbuffer cbN[count]  (opcode 89, len 4; bit11=0 immediateIndexed)
inline void EmitDclConstantBuffer(std::vector<uint32_t>& out, uint32_t slot, uint32_t count) {
  out.push_back(EncodeOpcode(OP_DCL_CONSTANT_BUFFER) | EncodeInstructionLength(4u));
  out.push_back(0x00208E46u);
  out.push_back(slot);
  out.push_back(count);
}

// dcl_resource_structured tN, stride  (opcode 162, len 4)
inline void EmitDclResourceStructured(std::vector<uint32_t>& out, uint32_t slot,
                                      uint32_t stride) {
  out.push_back(EncodeOpcode(OP_DCL_RESOURCE_STRUCTURED) | EncodeInstructionLength(4u));
  out.push_back(0x00107000u);
  out.push_back(slot);
  out.push_back(stride);
}

// dcl_output oN.xyzw  (opcode 101, len 3)
inline void EmitDclOutput(std::vector<uint32_t>& out, uint32_t reg) {
  out.push_back(EncodeOpcode(OP_DCL_OUTPUT) | EncodeInstructionLength(3u));
  out.push_back(0x001020F2u);
  out.push_back(reg);
}

// dcl_input_ps vN.xyzw  (opcode 98, len 3; interpolation LINEAR = 2 in bits
// [13:11] — verified bytes 0x03001062 0x001010F2 <N> from a compiled ps_4_1)
inline void EmitDclInputPs(std::vector<uint32_t>& out, uint32_t reg) {
  out.push_back(EncodeOpcode(OP_DCL_INPUT_PS) | EncodeInstructionLength(3u) | (2u << 11u));
  out.push_back(0x001010F2u);
  out.push_back(reg);
}

// dcl_temps N  (opcode 104, len 2)
inline void EmitDclTemps(std::vector<uint32_t>& out, uint32_t count) {
  out.push_back(EncodeOpcode(OP_DCL_TEMPS) | EncodeInstructionLength(2u));
  out.push_back(count);
}

// ── Signature insertion (Phase B/C support) ──
// Append a new signature entry + its semantic name to a signature chunk in the
// blob, fixing up the chunk size, ALL existing entries' name offsets, the
// following chunk offsets, and the file size, so the patched blob round-trips.
inline bool AppendSignatureEntry(std::vector<std::byte>& data, const ChunkInfo& chunk,
                                 std::string_view semantic, uint32_t semantic_index,
                                 uint32_t reg, uint32_t mask, uint32_t rw_mask,
                                 uint32_t component_type /* D3D10_SB_REGISTER_COMPONENT_* */) {
  constexpr uint32_t kEntriesOffset = 8u;  // skip count + unknown
  // Locate chunk data start.
  const uint32_t data_off = chunk.offset + kChunkHeaderSize;
  if (data_off + chunk.size > data.size()) return false;
  if (chunk.size < kEntriesOffset) return false;
  uint8_t* chunk_data = reinterpret_cast<uint8_t*>(data.data()) + data_off;
  const uint32_t count = *reinterpret_cast<uint32_t*>(chunk_data);
  const uint32_t entries_end = kEntriesOffset + count * kSignatureEntrySize;  // rel. to data start
  if (entries_end > chunk.size) return false;

  // CRITICAL: DXBC chunks must start on a 4-byte boundary. The appended entry
  // (24 bytes) + semantic name string (e.g. "TEXCOORD\0" = 9 bytes) = 33 bytes
  // for this patch, which is NOT a multiple of 4 — growing the OSGN by that raw
  // amount pushes every following chunk (SHEX/STAT) onto an unaligned address.
  // The NVIDIA runtime compiler rejects the misaligned chunk -> TDR on every
  // patched VS regardless of content (fxc tolerates it; the driver doesn't).
  // Round the growth up to 4 and pad the chunk data so subsequent chunks stay
  // 4-byte aligned.
  const uint32_t raw_delta = kSignatureEntrySize + (uint32_t)semantic.size() + 1u;
  const uint32_t delta = (raw_delta + 3u) & ~3u;
  const uint32_t pad = delta - raw_delta;

  // Insert the new 24-byte entry right after the last existing entry. This
  // shifts the string table by kSignatureEntrySize, so existing entries' name
  // offsets must be bumped as well.
  data.insert(data.begin() + data_off + entries_end, kSignatureEntrySize, std::byte{0});
  uint8_t* dst = reinterpret_cast<uint8_t*>(data.data()) + data_off + entries_end;
  const uint32_t str_off = chunk.size + kSignatureEntrySize;  // new name offset (rel. to data start)
  std::memcpy(dst + 0, &str_off, 4);
  std::memcpy(dst + 4, &semantic_index, 4);
  const uint32_t sysval = 0u;  // D3D10_SB_NAME_UNDEFINED for TEXCOORD
  std::memcpy(dst + 8, &sysval, 4);
  std::memcpy(dst + 12, &component_type, 4);
  std::memcpy(dst + 16, &reg, 4);
  dst[20] = (uint8_t)mask;
  dst[21] = (uint8_t)rw_mask;
  dst[22] = 0;
  dst[23] = 0;

  // Fix existing entries' name offsets (string table moved by +kSignatureEntrySize).
  chunk_data = reinterpret_cast<uint8_t*>(data.data()) + data_off;
  for (uint32_t i = 0; i < count; ++i) {
    uint8_t* e = chunk_data + kEntriesOffset + i * kSignatureEntrySize;
    uint32_t off;
    std::memcpy(&off, e, 4);
    off += kSignatureEntrySize;
    std::memcpy(e, &off, 4);
  }

  // Bump the entry count.
  const uint32_t new_count = count + 1u;
  std::memcpy(chunk_data, &new_count, 4);

  // Append the semantic name string at the end of the chunk data.
  const std::string sem(semantic);
  const size_t str_at = (size_t)data_off + str_off;
  data.insert(data.begin() + str_at, sem.size() + 1u, std::byte{0});
  std::memcpy(data.data() + str_at, sem.data(), sem.size());

  // Pad the chunk data to a 4-byte multiple (keeps following chunks aligned).
  if (pad != 0u)
    data.insert(data.begin() + (size_t)data_off + chunk.size + raw_delta, pad, std::byte{0});

  // Update the chunk size (padded).
  const uint32_t new_chunk_size = chunk.size + delta;
  std::memcpy(data.data() + chunk.offset + 4, &new_chunk_size, 4);

  // Update the file size field (byte 24) so the blob still validates.
  const uint32_t new_size = (uint32_t)data.size();
  std::memcpy(data.data() + 24u, &new_size, 4);

  // Update all following chunk offsets. We must NOT re-parse the blob here:
  // the not-yet-fixed old offsets now point inside the grown chunk and would
  // read garbage sizes. Just read the header's chunk count and fix the array.
  uint32_t chunk_count = 0;
  std::memcpy(&chunk_count, data.data() + 28u, 4);
  uint32_t* offsets = reinterpret_cast<uint32_t*>(data.data() + sizeof(DXBCHeader));
  for (uint32_t i = 0; i < chunk_count; ++i)
    if (offsets[i] > chunk.offset) offsets[i] += delta;
  return true;
}

// ── DXBC MD5 hash (dxbc-spirv algorithm, from the Luma Framework, MIT) ──
// The DXBC checksum is the MD5 of the file starting at `version` (byte 20),
// with a NON-standard finalization that depends on the stream size.
inline void ComputeDXBCHash(const uint8_t* data, size_t size, uint8_t out_digest[16]);

namespace detail {

constexpr uint32_t kMD5BlockSize = 64u;

inline uint32_t MD5ReadDword(const unsigned char* src) {
  return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

inline void MD5ProcessBlock(const unsigned char* data, std::array<uint32_t, 4>& state) {
  static constexpr std::array<uint8_t, 64u> kShifts = {
      7u, 12u, 17u, 22u,  7u, 12u, 17u, 22u,  7u, 12u, 17u, 22u,  7u, 12u, 17u, 22u,
      5u,  9u, 14u, 20u,  5u,  9u, 14u, 20u,  5u,  9u, 14u, 20u,  5u,  9u, 14u, 20u,
      4u, 11u, 16u, 23u,  4u, 11u, 16u, 23u,  4u, 11u, 16u, 23u,  4u, 11u, 16u, 23u,
      6u, 10u, 15u, 21u,  6u, 10u, 15u, 21u,  6u, 10u, 15u, 21u,  6u, 10u, 15u, 21u,
  };
  static constexpr std::array<uint32_t, 64u> kConstants = {
      0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
      0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
      0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
      0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
      0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
      0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
      0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
      0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
      0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
      0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
      0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
      0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
      0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
      0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
      0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
      0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u,
  };
  std::array<uint32_t, 4u> s = state;
  auto iteration = [&](uint32_t i, uint32_t f, uint32_t g) {
    f = f + s[0u] + kConstants[i] + MD5ReadDword(&data[4u * g]);
    s[0u] = s[3u];
    s[3u] = s[2u];
    s[2u] = s[1u];
    s[1u] += (f << kShifts[i]) + (f >> (32u - kShifts[i]));
  };
  for (uint32_t i = 0u; i < 16u; i++) {
    const uint32_t f = (s[1u] & s[2u]) | (~s[1u] & s[3u]);
    iteration(i, f, i);
  }
  for (uint32_t i = 16u; i < 32u; i++) {
    const uint32_t f = (s[3u] & s[1u]) | (~s[3u] & s[2u]);
    iteration(i, f, (5u * i + 1u) % 16u);
  }
  for (uint32_t i = 32u; i < 48u; i++) {
    const uint32_t f = s[1u] ^ s[2u] ^ s[3u];
    iteration(i, f, (3u * i + 5u) % 16u);
  }
  for (uint32_t i = 48u; i < 64u; i++) {
    const uint32_t f = s[2u] ^ (s[1u] | ~s[3u]);
    iteration(i, f, (7u * i) % 16u);
  }
  for (uint32_t i = 0u; i < state.size(); i++)
    state[i] += s[i];
}

inline void MD5Update(std::array<uint8_t, kMD5BlockSize>& block, uint64_t& size,
                      std::array<uint32_t, 4>& state, const void* data, size_t n) {
  auto src = reinterpret_cast<const unsigned char*>(data);
  size_t block_offset = (size_t)(size % kMD5BlockSize);
  size += n;
  if (block_offset) {
    const size_t fill = n < (kMD5BlockSize - block_offset) ? n : (kMD5BlockSize - block_offset);
    std::memcpy(&block[block_offset], src, fill);
    n -= fill;
    src += fill;
    if (block_offset + fill >= kMD5BlockSize) MD5ProcessBlock(block.data(), state);
  }
  while (n >= kMD5BlockSize) {
    MD5ProcessBlock(src, state);
    src += kMD5BlockSize;
    n -= kMD5BlockSize;
  }
  if (n) std::memcpy(block.data(), src, n);
}

}  // namespace detail

inline void ComputeDXBCHash(const uint8_t* data, size_t size, uint8_t out_digest[16]) {
  // Skip the header up to `version` (byte 20), like dxbc-spirv.
  constexpr size_t kOffset = 20u;
  if (size < kOffset) {
    std::memset(out_digest, 0, 16);
    return;
  }
  const uint8_t* bytes = data + kOffset;
  size_t n = size - kOffset;

  const uint32_t aNum = (uint32_t)n * 8u;
  const uint32_t bNum = (aNum >> 2u) | 1u;
  std::array<uint8_t, 4u> a = {};
  std::array<uint8_t, 4u> b = {};
  for (uint32_t i = 0u; i < 4u; i++) {
    a[i] = (uint8_t)((aNum >> (8u * i)) & 0xFFu);
    b[i] = (uint8_t)((bNum >> (8u * i)) & 0xFFu);
  }

  const size_t remainder = n % detail::kMD5BlockSize;
  const size_t paddingSize = detail::kMD5BlockSize - remainder;

  std::array<uint8_t, detail::kMD5BlockSize> block = {};
  uint64_t stream_size = 0u;
  std::array<uint32_t, 4u> state = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};
  std::array<uint8_t, detail::kMD5BlockSize> s_padding = {};
  s_padding[0] = 0x80u;

  detail::MD5Update(block, stream_size, state, bytes, n - remainder);

  if (remainder >= 56u) {
    detail::MD5Update(block, stream_size, state, &bytes[n - remainder], remainder);
    detail::MD5Update(block, stream_size, state, s_padding.data(), paddingSize);
    detail::MD5Update(block, stream_size, state, a.data(), a.size());
    detail::MD5Update(block, stream_size, state, s_padding.data() + a.size(),
                      s_padding.size() - a.size() - b.size());
    detail::MD5Update(block, stream_size, state, b.data(), b.size());
  } else {
    detail::MD5Update(block, stream_size, state, a.data(), a.size());
    if (remainder) detail::MD5Update(block, stream_size, state, &bytes[n - remainder], remainder);
    detail::MD5Update(block, stream_size, state, s_padding.data(), paddingSize - a.size() - b.size());
    detail::MD5Update(block, stream_size, state, b.data(), b.size());
  }

  for (uint32_t i = 0u; i < state.size(); i++) {
    const uint32_t dw = state[i];
    for (uint32_t j = 0u; j < 4u; j++) out_digest[4u * i + j] = (uint8_t)((dw >> (8u * j)) & 0xFFu);
  }
}

// Write the recomputed hash into the header (bytes 4..20 of the blob).
inline void WriteDXBCHash(std::vector<std::byte>& data) {
  uint8_t digest[16];
  ComputeDXBCHash(reinterpret_cast<const uint8_t*>(data.data()), data.size(), digest);
  std::memcpy(data.data() + 4u, digest, 16);
}

// ── CRC32 (self-contained, matches the standard CRC-32 used for shader hashes) ──
inline uint32_t ComputeCRC32(const uint8_t* data, size_t size) {
  static uint32_t table[256];
  static bool init = false;
  if (!init) {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (uint32_t k = 0; k < 8; ++k) c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
      table[i] = c;
    }
    init = true;
  }
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < size; ++i) crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8u);
  return crc ^ 0xFFFFFFFFu;
}

// ── RDEF cbuffer / variable parsing (for outline GameEdgeParameters lookup) ──
// RDEF body layout (verified vs fxc 10.1):
//   +0 cbuffer_count, +4 cbuffer_offset, +8 resource_count, +12 resource_offset
// cbuffer entry (24 bytes): name_off, variable_count, variable_offset, size, flags, pad
// variable entry (24 bytes): name_off, start_offset, size, flags, default_off, pad
struct RdefCbuffer {
  std::string name;
  uint32_t variable_offset = 0;
  uint32_t variable_count = 0;
  uint32_t size = 0;
  struct Var {
    std::string name;
    uint32_t start_offset = 0;
    uint32_t size = 0;
  };
  std::vector<Var> variables;
};

// Parse the cbuffer table + variable table from an RDEF chunk (data start =
// after the 8-byte chunk header). Returns false if the layout is unrecognized.
inline bool ParseRDEFCbuffers(const uint8_t* chunk_data, uint32_t chunk_size,
                              std::vector<RdefCbuffer>& out) {
  if (chunk_size < 16u) return false;
  const uint32_t cb_count = *reinterpret_cast<const uint32_t*>(chunk_data);
  const uint32_t cb_off = *reinterpret_cast<const uint32_t*>(chunk_data + 4u);
  out.clear();
  constexpr uint32_t kCbEntry = 24u;
  constexpr uint32_t kVarEntry = 24u;
  if (cb_off + cb_count * kCbEntry > chunk_size) return false;
  for (uint32_t i = 0; i < cb_count; ++i) {
    const uint8_t* e = chunk_data + cb_off + i * kCbEntry;
    RdefCbuffer cb;
    uint32_t name_off, var_off;
    std::memcpy(&name_off, e + 0, 4);
    std::memcpy(&cb.variable_count, e + 4, 4);
    std::memcpy(&var_off, e + 8, 4);
    std::memcpy(&cb.size, e + 12, 4);
    if (name_off < chunk_size) cb.name = reinterpret_cast<const char*>(chunk_data + name_off);
    cb.variable_offset = var_off;
    if (var_off + cb.variable_count * kVarEntry <= chunk_size) {
      for (uint32_t v = 0; v < cb.variable_count; ++v) {
        const uint8_t* ve = chunk_data + var_off + v * kVarEntry;
        RdefCbuffer::Var var;
        uint32_t vno;
        std::memcpy(&vno, ve + 0, 4);
        std::memcpy(&var.start_offset, ve + 4, 4);
        std::memcpy(&var.size, ve + 8, 4);
        if (vno < chunk_size) var.name = reinterpret_cast<const char*>(chunk_data + vno);
        cb.variables.push_back(std::move(var));
      }
    }
    out.push_back(std::move(cb));
  }
  return true;
}

// Find the byte offset of a top-level cbuffer variable by name.
inline bool FindCbufferVariable(const std::vector<RdefCbuffer>& cbs, std::string_view cb_name,
                                std::string_view var_name, uint32_t& out_offset) {
  for (const auto& cb : cbs) {
    if (cb.name != cb_name) continue;
    for (const auto& v : cb.variables)
      if (v.name == var_name) {
        out_offset = v.start_offset;
        return true;
      }
  }
  return false;
}

// ── SHEX scanner ──
struct ShexScan {
  bool has_temps = false;
  uint32_t dcl_temps = 0;         // declared temp count (r0..dcl_temps-1)
  uint32_t max_temp = 0;          // highest temp register index actually used
  uint32_t first_non_decl = 0;    // byte offset (rel. to token stream start) of first non-dcl instr
  uint32_t ret_offset = 0;        // byte offset of the last ret instruction
  bool has_ret = false;
  std::vector<uint32_t> cb_slots;        // cbuffer slots from dcl_constantbuffer
  std::vector<std::pair<uint32_t, uint32_t>> cb_slot_sizes;  // (slot, register count) pairs
  std::vector<uint32_t> resource_slots;  // t-slots from ANY dcl_resource* decl (textures + structured + raw + typed)
  std::vector<uint32_t> structured_slots;  // t-slots declared with dcl_resource_structured (0xA2) only
};

inline bool IsDeclOpcode(uint32_t op) {
  // D3D10/10.1/11 declaration opcodes (vs_4_1 subset: DCL_RESOURCE..DCL_GLOBAL_FLAGS + DCL_RESOURCE_STRUCTURED).
  return (op >= 88u && op <= 106u) || op == 162u;
}

// Walk one operand: consume its token + index value dwords. Returns operand
// type; writes the first index value (when rep0 == immediate32) to out_first.
// The operand token is consumed FIRST; then, if bit 31 (extended) is set, the
// modifier dword (NEG/ABS/etc.) that follows the token is skipped BEFORE the
// index values — otherwise that modifier (e.g. 0x00000041 = NEG) is misread as
// an index value -> phantom temp 65 -> the injected block gets placed at
// r66-r77 and dcl_temps balloons to 78 on shaders that only use r0-r5.
inline uint32_t WalkOperand(const uint32_t* dwords, uint32_t& idx, uint32_t* out_first) {
  const uint32_t t = dwords[idx];
  ++idx;  // the operand token itself
  if ((t & 0x80000000u) != 0u) ++idx;  // extended operand modifier dword
  const uint32_t type = (t >> kOperandTypeShift) & 0xFFu;
  const uint32_t index_dim = (t >> kOperandIndexDimShift) & 0x3u;
  for (uint32_t d = 0; d < index_dim; ++d) {
    const uint32_t rep = (t >> (kOperandIndexRepShift + 3u * d)) & 0x7u;
    if (rep == 0u) {  // immediate32: the CURRENT dword is the value
      if (d == 0u && out_first != nullptr) *out_first = dwords[idx];
      ++idx;
    } else if (rep == 1u) {  // immediate64: 2 value dwords
      idx += 2u;
    } else if (rep == 2u) {  // relative: nested operand token + its index
      const uint32_t rt = dwords[idx];  // nested operand token
      ++idx;                            // consume it
      if ((rt & 0x80000000u) != 0u) ++idx;  // nested extended modifier (rare)
      const uint32_t rdim = (rt >> kOperandIndexDimShift) & 0x3u;
      const uint32_t rrep0 = (rt >> kOperandIndexRepShift) & 0x7u;
      if (rdim >= 1u && rrep0 == 0u) {
        ++idx;  // consume nested index value
      } else if (rdim >= 1u && rrep0 == 2u) {
        idx += 2u;  // double-relative (rare)
      }
    } else {  // rep 3/4: immediate value + relative operand token + its index
      ++idx;  // skip immediate value (now at relative operand token)
      if ((dwords[idx] & 0x80000000u) != 0u) ++idx;  // relative extended modifier
      ++idx;  // skip relative operand token (now at relative index value)
      ++idx;  // skip relative index value
    }
  }
  return type;
}

// Scan a SHEX/SHDR token stream (data starts at the version/type dword).
inline bool ScanShex(const uint8_t* chunk_data, uint32_t chunk_size, ShexScan& out) {
  std::vector<Instruction> insns;
  if (!IterateInstructions(chunk_data, chunk_size, insns)) return false;
  const uint32_t* dwords = reinterpret_cast<const uint32_t*>(chunk_data);
  const uint32_t dword_count = chunk_size / 4u;
  uint32_t pos = 0u;  // dword index of the current instruction
  out = ShexScan{};
  for (const auto& ins : insns) {
    const uint32_t start = ins.offset / 4u;
    const uint32_t op = ins.opcode;
    if (!IsDeclOpcode(op) && out.first_non_decl == 0u) out.first_non_decl = ins.offset;
    if (op == OP_DCL_TEMPS && !out.has_temps && ins.length >= 2u) {
      out.has_temps = true;
      out.dcl_temps = dwords[start + 1u];
    }
    if (op == OP_DCL_CONSTANT_BUFFER && ins.length >= 4u) {
      const uint32_t slot = dwords[start + 2u];  // [opcode][cb operand][slot][count]
      out.cb_slots.push_back(slot);
      out.cb_slot_sizes.emplace_back(slot, dwords[start + 3u]);
    }
    // Every dcl_resource* occupies a t-slot regardless of type. Missing a
    // texture decl (DCL_RESOURCE=88) and then choosing that slot for our
    // structured prev-bone buffer would DOUBLE-DECLARE the slot with a
    // conflicting type -> driver GPU fault (TDR). Layout for all of these is
    // [opcode][resource operand][slot][...] so the slot is dwords[start+2].
    if (op == OP_DCL_RESOURCE_STRUCTURED && ins.length >= 4u) {  // 0xA2
      out.resource_slots.push_back(dwords[start + 2u]);
      out.structured_slots.push_back(dwords[start + 2u]);
    } else if (op == OP_DCL_RESOURCE && ins.length >= 3u) {      // 0x58 texture
      out.resource_slots.push_back(dwords[start + 2u]);
    } else if (op == 107u && ins.length >= 3u) {                 // DCL_RESOURCE_RAW
      out.resource_slots.push_back(dwords[start + 2u]);
    } else if (op == 163u && ins.length >= 3u) {                 // DCL_RESOURCE_TYPED
      out.resource_slots.push_back(dwords[start + 2u]);
    }
    if (op == OP_RET) {
      out.has_ret = true;
      out.ret_offset = ins.offset;
    }
    // Walk operands to find max temp register. The walk is BOUNDED by the
    // instruction boundary: WalkOperand consumes a variable number of dwords
    // per operand, so the old fixed loop count (ins.length-1) over-ran into the
    // NEXT instruction's dwords and hallucinated phantom temps (e.g. cb0[65]'s
    // index read as temp 65). That inflated temp_base (block landed at r66-r77
    // on a shader that only uses r0-r5) and dcl_temps (5->78) on every patched
    // VS. Extended opcodes (first dword opcode field = 0xFF) use a 2-dword
    // header, so operands start at start+2 for those.
    uint32_t idx = start + 1u;
    if ((dwords[start] & 0xFFu) == 0xFFu) ++idx;  // extended opcode: 2-dword header
    while (idx < start + ins.length) {
      uint32_t first = 0u;
      const uint32_t otype = WalkOperand(dwords, idx, &first);
      if (otype == OPERAND_TEMP && first > out.max_temp) out.max_temp = first;
    }
    pos = start + ins.length;
    (void)pos;
    (void)dword_count;
  }
  return true;
}

// ── Generic skinned-VS patcher (Phase B core) ──
// Detects a skinned vertex shader (ISGN BLENDINDICES + BLENDWEIGHTS), injects a
// Results of a skinned-VS patch: the new hash plus the dynamically-chosen
// bind slots the addon must bind at draw time (prevVP cbuffer at cb#, prev-bone
// StructuredBuffer SRV at t#). Slots are per-shader because the game's used
// slots vary; the addon looks these up by the patched shader's (new) hash.
struct PatchInfo {
  uint32_t new_hash = 0u;          // CRC32 of the patched blob (what the pipeline holds)
  uint32_t prev_vp_cb_slot = 1u;   // free b-slot chosen for the prevVP cbuffer
  uint32_t prev_bone_t_slot = 1u;  // free t-slot chosen for the prev-bone SRV
  uint32_t bone_game_slot = 0u;    // the GAME's own bone StructuredBuffer slot (usually t0)
                                   // — where the addon reads the CURRENT bone SRV at draw
                                   // time to find/register its prev-frame twin (Phase D).
  uint32_t texcoord_index = 5u;    // TEXCOORD semantic index of the prevClip output
  uint32_t output_reg = 0u;        // new output register (oN)
  bool outline_applied = false;    // whether the GameEdgeParameters offset was replicated
  bool needs_no_binding = false;   // true: the patched VS reads only the game's own t0/b0
                                   // (no addon t#/b# binds needed — crash-bisection mode)
};

// Tunables for the generic patch (used for crash bisection).
struct PatchOptions {
  bool enable_outline = true;        // replicate the GameEdgeParameters offset in prevClip
  bool use_game_resources_only = false;  // read prev-bones from the game's t0 and prevVP
                                         // from the game's b0 ViewProjection (c10..c13) so the
                                         // patched VS needs NO addon binds (smoke test isolation)
  bool minimal_patch = false;        // emit ONLY the new prevClip output: prevClip =
                                     // game's cb0[10..13] x float4(v0,1). No re-skin, no
                                     // outline, NO new resource reads at all — isolates the
                                     // output register / OSGN / dcl_temps machinery.
  bool test_no_output = false;       // crash-bisection: run the whole injected block (reads
                                     // v0 + cb0[10..13]) but add NO prevClip output register.
                                     // PS's v7 goes undefined again. Isolates the output
                                     // register / OSGN append from everything else.
  bool test_constant_output = false; // crash-bisection: minimal, but o7 = (0,0,0,1) constant
                                     // — ZERO reads at all. Isolates the cb0 read / v7 data
                                     // from the mere existence of the output register.
};

// prev-bone re-skin block (t#) × addon prevVP cbuffer (b#) → new TEXCOORD
// output register, bumps dcl_temps, appends the OSGN entry, fixes chunk
// offsets, and rewrites the MD5. Returns the new CRC32 (via out_new_hash) and
// the chosen slots (via out_info).
inline bool PatchSkinnedVertexShader(std::vector<std::byte>& data, uint32_t* out_new_hash,
                                     PatchInfo* out_info = nullptr,
                                     const PatchOptions& options = {}) {
  DXBCHeader header;
  std::vector<ChunkInfo> chunks;
  if (!ParseDXBC(data, header, chunks)) return false;
  const ChunkInfo* isgn = FindChunk(chunks, "ISGN");
  const ChunkInfo* osgn = FindChunk(chunks, "OSGN");
  const ChunkInfo* shex = FindChunk(chunks, "SHEX");
  if (!shex) shex = FindChunk(chunks, "SHDR");
  if (!isgn || !osgn || !shex) return false;

  std::vector<SignatureEntry> is, os;
  if (!ParseSignature(data, *isgn, is) || !ParseSignature(data, *osgn, os)) return false;

  // Detect skinned VS.
  int blend_reg = -1, weight_reg = -1, pos_reg = -1, normal_reg = -1;
  for (const auto& e : is) {
    if (e.name == "BLENDINDICES") blend_reg = (int)e.register_index;
    else if (e.name == "BLENDWEIGHTS") weight_reg = (int)e.register_index;
    else if (e.name == "POSITION") pos_reg = (int)e.register_index;
    else if (e.name == "NORMAL") normal_reg = (int)e.register_index;
  }
  if (blend_reg < 0 || weight_reg < 0) return false;  // not skinned (e.g. World-matrix VS)
  if (pos_reg < 0) pos_reg = 0;

  // Scope: only G-buffer / forward-renderer VSs (they output TEXCOORD/COLOR).
  // Depth/shadow-caster VSs output only SV_POSITION and never feed the per-
  // object motion target; patching them is pure risk (stripped input layouts,
  // unusual passes) with no benefit.
  bool has_geometry_output = false;
  for (const auto& e : os)
    if (e.name == "TEXCOORD" || e.name == "COLOR") { has_geometry_output = true; break; }
  if (!has_geometry_output) return false;

  // Outline detection + layout via RDEF (GameEdgeParameters present => outline).
  // scene struct is fixed in senkiseki3: EyePosition at scene+0, ViewProjection at
  // scene+144 (c1/c10). The 4th VP column (col3) is what the outline clamp reads.
  uint32_t globals_slot = 0u;      // $Globals cbuffer slot
  uint32_t edge_elem = 0u;         // GameEdgeParameters cbuffer element
  uint32_t eye_elem = 1u;          // EyePosition cbuffer element (senkiseki3 c1 fallback)
  uint32_t vp_col3_elem = 13u;     // ViewProjection col3 element (senkiseki3 c13 fallback)
  bool outline = false;
  if (const ChunkInfo* rdef = FindChunk(chunks, "RDEF")) {
    const uint8_t* rdef_data =
        reinterpret_cast<const uint8_t*>(data.data()) + rdef->offset + kChunkHeaderSize;
    std::vector<RdefCbuffer> cbs;
    if (ParseRDEFCbuffers(rdef_data, rdef->size, cbs)) {
      for (const auto& cb : cbs) {
        for (const auto& v : cb.variables) {
          if (v.name != "GameEdgeParameters") continue;
          outline = true;
          edge_elem = v.start_offset / 16u;
          for (const auto& v2 : cb.variables)
            if (v2.name == "scene") {
              eye_elem = v2.start_offset / 16u;
              vp_col3_elem = (v2.start_offset + 144u) / 16u + 3u;
              break;
            }
          std::vector<ResourceBinding> bindings;
          if (ParseRDEFBindings(rdef_data, rdef->size, bindings))
            for (const auto& b : bindings)
              if (b.name == cb.name && b.input_type == 0u) globals_slot = b.bind_point;
          break;
        }
        if (outline) break;
      }
    }
  }

  // Choose prevClip TEXCOORD index (prefer 5) + new output register.
  std::array<bool, 32> used_tex = {};
  uint32_t max_out = 0;
  for (const auto& e : os) {
    if (e.name == "TEXCOORD" && e.semantic_index < 32u) used_tex[e.semantic_index] = true;
    if (e.register_index > max_out) max_out = e.register_index;
  }
  uint32_t tex_idx = 5u;
  if (used_tex[5u]) {
    tex_idx = 32u;
    for (uint32_t i = 5u; i < 32u; ++i)
      if (!used_tex[i]) { tex_idx = i; break; }
    if (tex_idx == 32u) return false;  // no free TEXCOORD slot
  }
  const uint32_t new_out_reg = max_out + 1u;

  // Scan SHEX.
  const uint8_t* shex_data = reinterpret_cast<const uint8_t*>(data.data()) + shex->offset + kChunkHeaderSize;
  ShexScan scan;
  if (!ScanShex(shex_data, shex->size, scan)) return false;
  if (!scan.has_ret) return false;  // can't find the tail

  // Pick free prevVP cb slot (skip used + 13=shader_injection) and prev-bone t slot.
  uint32_t cb_slot = 1u;
  while (std::find(scan.cb_slots.begin(), scan.cb_slots.end(), cb_slot) != scan.cb_slots.end() ||
         cb_slot == 13u)
    ++cb_slot;
  uint32_t t_slot = 1u;
  while (std::find(scan.resource_slots.begin(), scan.resource_slots.end(), t_slot) !=
         scan.resource_slots.end())
    ++t_slot;
  // vs_4_1 has only 14 cbuffer slots (b0..b13) — a slot at 14 would be invalid
  // bytecode and crash the driver. If every slot is taken, skip this shader.
  if (cb_slot > 13u) return false;

  // Crash-bisection mode (use_game_resources_only): the injected block reads the
  // GAME's own bone StructuredBuffer (first STRUCTURED t-slot, t0 for skinned
  // VSs) and the GAME's ViewProjection (b0 c10..c13) instead of an addon t#/b#
  // pair. The patched VS then needs NO addon binding — nothing can leak into
  // later draws and no extra SRV/CB is read. prevClip == current clip (motion
  // ~0), which is the correct smoke-test baseline. Phase D reverts to
  // prev-bone twins. We require the slot to be declared dcl_resource_structured
  // (never a texture) so ld_structured tN,64 can't be a descriptor-kind mismatch.
  const bool no_bind = options.use_game_resources_only || options.minimal_patch ||
                       options.test_no_output || options.test_constant_output;
  const bool minimal = options.minimal_patch || options.test_constant_output;
  const bool no_output = options.test_no_output;
  const bool constant = options.test_constant_output;
  const uint32_t bone_t_slot =
      (no_bind && !minimal && !scan.structured_slots.empty()) ? scan.structured_slots[0] : t_slot;
  // No-bind mode needs a confirmed structured bone buffer; without one, refuse
  // to patch rather than risk reading a texture as a StructuredBuffer.
  // (Minimal/constant/no-output modes read no structured buffer, so no check.)
  if (no_bind && !minimal && scan.structured_slots.empty()) return false;
  const uint32_t prev_vp_cb = no_bind ? globals_slot : cb_slot;
  const uint32_t prev_vp_elem = no_bind ? 10u : 0u;  // b0 c10 = ViewProjection (always declared)

  // Outline safety: never emit cb0 reads beyond the cbuffer's DECLARED register
  // count. The game binds buffers sized to the shader's declaration; reading
  // past the bound buffer is a GPU page fault (TDR). If the RDEF elements can't
  // be confirmed in-bounds, drop the outline offset rather than crash.
  uint32_t globals_declared = 0u;  // register count of the _Globals cbuffer
  for (const auto& [slot, count] : scan.cb_slot_sizes)
    if (slot == globals_slot) { globals_declared = count; break; }
  if (outline && !options.enable_outline) outline = false;
  // Only emit the outline block when we can CONFIRM every cb0 element it reads
  // is within the declared cbuffer. Reading past a bound cbuffer is a GPU page
  // fault (TDR) — a wrong RDEF offset or an unknown cbuffer size is not worth it.
  if (outline && globals_declared != 0u) {
    const uint32_t worst = std::max({edge_elem, eye_elem, vp_col3_elem});
    if (worst >= globals_declared) outline = false;
  } else if (outline) {
    outline = false;  // couldn't find the _Globals size — can't confirm safety
  }
  // Minimal / no-output / constant modes: never emit the outline block.
  if (minimal || no_output) outline = false;

  // Allocate temps: base = declared count (guaranteed unused) or max used + 1.
  // Use the MAX of the two: if the game's shader uses temp registers beyond its
  // dcl_temps declaration (legal in SM4+), picking dcl_temps alone would let the
  // injected block clobber the shader's live temps (wrong output / garbage
  // indices -> potential OOB reads).
  const uint32_t temp_base = std::max(scan.has_temps ? scan.dcl_temps : 0u,
                                      scan.max_temp + 1u);
  const uint32_t rP = temp_base;        // float4(v0,1)
  const uint32_t rC = temp_base + 1u;   // column scratch
  const uint32_t rB = temp_base + 2u;   // bone result / prevClip
  const uint32_t rAcc = temp_base + 3u; // accumulated skinned position
  const uint32_t rN = temp_base + 4u;   // skinned normal (outline)
  const uint32_t rW = temp_base + 5u;   // VP col3 / clipw (outline)
  const uint32_t rE = temp_base + 6u;   // GameEdgeParameters (outline)
  const uint32_t rT = temp_base + 7u;   // offset scratch (outline)
  const uint32_t rEye = temp_base + 8u; // eye position/dir (outline)
  const uint32_t kTempSlots = 12u;
  const uint32_t new_dcl_temps = temp_base + kTempSlots;

  // Build injected declaration tokens. In no_bind mode the game already declares
  // the bone StructuredBuffer (t0) and _Globals cbuffer (b0), so only the new
  // output register needs a declaration.
  std::vector<uint32_t> decls;
  if (!no_bind) {
    EmitDclConstantBuffer(decls, cb_slot, 4u);        // prevVP (16 floats)
    EmitDclResourceStructured(decls, t_slot, 64u);    // prev bones
  }
  if (!no_output) EmitDclOutput(decls, new_out_reg);  // prevClip output (skipped in no-output test)

  // Build injected body tokens.
  std::vector<uint32_t> body;
  // rP = float4(v0, 1)
  EmitMovInput(body, rP, 0x7u, (uint32_t)pos_reg, 0x7u);
  EmitMovImm1(body, rP, 0x8u, 0x3F800000u);  // 1.0f
  // 4-bone re-skin. Bone index comps and weight comps: y,x,z,w.
  // Matches the game's skinning: 3 columns (bytes 0/16/32) + w=1 (the 4th
  // translation column is dropped by the engine, so we drop it too for parity).
  // Minimal mode skips the re-skin entirely (no resource reads).
  static constexpr uint32_t kBoneComps[4] = {1u, 0u, 2u, 3u};
  static constexpr uint32_t kWeightSwizzles[4] = {kSwizzleYYYY, kSwizzleXXXX, kSwizzleZZZZ,
                                                  kSwizzleWWWW};
  if (!minimal) {
    for (uint32_t b = 0; b < 4u; ++b) {
      const uint32_t comp = kBoneComps[b];
      const uint32_t wswiz = kWeightSwizzles[b];
      EmitLdStructured(body, rC, 0xFu, (uint32_t)blend_reg, comp, 0u, bone_t_slot, kSwizzleXYZW);
      EmitDp4(body, rB, 0x1u, rC, kSwizzleXYZW, rP, kSwizzleXYZW);
      EmitLdStructured(body, rC, 0xFu, (uint32_t)blend_reg, comp, 16u, bone_t_slot, kSwizzleXYZW);
      EmitDp4(body, rB, 0x2u, rC, kSwizzleXYZW, rP, kSwizzleXYZW);
      EmitLdStructured(body, rC, 0xFu, (uint32_t)blend_reg, comp, 32u, bone_t_slot, kSwizzleXYZW);
      EmitDp4(body, rB, 0x4u, rC, kSwizzleXYZW, rP, kSwizzleXYZW);
      if (b == 0u)
        EmitMulInput(body, rAcc, 0x7u, (uint32_t)weight_reg, wswiz, rB, kSwizzleXYZW);
      else
        EmitMadInput(body, rAcc, 0x7u, (uint32_t)weight_reg, wswiz, rB, kSwizzleXYZW, rAcc,
                     kSwizzleXYZW);
    }
    EmitMovImm1(body, rAcc, 0x8u, 0x3F800000u);  // rAcc.w = 1
  }

  // Outline offset replication (prev-bone normal + GameEdgeParameters).
  // Skipped in minimal mode (outline forced false above).
  if (outline && normal_reg >= 0) {
    // Re-skin NORMAL with prev bones (4-bone, 3x3) into rN.
    for (uint32_t b = 0; b < 4u; ++b) {
      const uint32_t comp = kBoneComps[b];
      const uint32_t wswiz = kWeightSwizzles[b];
      EmitLdStructured(body, rC, 0x7u, (uint32_t)blend_reg, comp, 0u, bone_t_slot, kSwizzleXYZW);
      EmitDp3Input(body, rB, 0x1u, (uint32_t)normal_reg, 0x7u, rC, kSwizzleXYZW);
      EmitLdStructured(body, rC, 0x7u, (uint32_t)blend_reg, comp, 16u, bone_t_slot, kSwizzleXYZW);
      EmitDp3Input(body, rB, 0x2u, (uint32_t)normal_reg, 0x7u, rC, kSwizzleXYZW);
      EmitLdStructured(body, rC, 0x7u, (uint32_t)blend_reg, comp, 32u, bone_t_slot, kSwizzleXYZW);
      EmitDp3Input(body, rB, 0x4u, (uint32_t)normal_reg, 0x7u, rC, kSwizzleXYZW);
      if (b == 0u)
        EmitMulInput(body, rN, 0x7u, (uint32_t)weight_reg, wswiz, rB, kSwizzleXYZW);
      else
        EmitMadInput(body, rN, 0x7u, (uint32_t)weight_reg, wswiz, rB, kSwizzleXYZW, rN,
                     kSwizzleXYZW);
    }
    // normalize rN
    EmitDp3(body, rN, 0x8u, rN, kSwizzleXYZW, rN, kSwizzleXYZW);
    EmitRsq(body, rN, 0x8u, rN, kSwizzleWWWW);
    EmitMul(body, rN, 0x7u, rN, kSwizzleXYZW, rN, kSwizzleWWWW);
    // clipw = saturate(dot(rAcc, ViewProjection.col3)) clamped to [0.3, 1]
    EmitMovCb(body, rW, 0xFu, globals_slot, vp_col3_elem);
    EmitDp4(body, rW, 0x1u, rW, kSwizzleXYZW, rAcc, kSwizzleXYZW);
    EmitMaxImm1(body, rW, 0x1u, rW, kSwizzleXXXX, 0x3E99999Au);  // 0.3f
    EmitMinImm1(body, rW, 0x1u, rW, kSwizzleXXXX, 0x3F800000u);  // 1.0f
    // pos += edgeW * normal * clipw
    EmitMovCb(body, rE, 0xFu, globals_slot, edge_elem);
    EmitMul(body, rT, 0x7u, rE, kSwizzleWWWW, rN, kSwizzleXYZW);
    EmitMul(body, rT, 0x7u, rT, kSwizzleXYZW, rW, kSwizzleXXXX);
    EmitAdd(body, rAcc, 0x7u, rAcc, kSwizzleXYZW, rT, kSwizzleXYZW);
    // pos += normalize(pos - eye) * (0.001 + edgeW)
    EmitMovCb(body, rEye, 0xFu, globals_slot, eye_elem);
    EmitMulImm4(body, rEye, 0x7u, rEye, kSwizzleXYZW, 0xBF800000u, 0xBF800000u, 0xBF800000u,
                0xBF800000u);  // -eye
    EmitAdd(body, rEye, 0x7u, rAcc, kSwizzleXYZW, rEye, kSwizzleXYZW);
    EmitDp3(body, rEye, 0x8u, rEye, kSwizzleXYZW, rEye, kSwizzleXYZW);
    EmitRsq(body, rEye, 0x8u, rEye, kSwizzleWWWW);
    EmitMul(body, rEye, 0x7u, rEye, kSwizzleXYZW, rEye, kSwizzleWWWW);
    EmitAddImm1(body, rE, 0x8u, rE, kSwizzleWWWW, 0x3A83126Fu);  // rE.w = edgeW + 0.001
    EmitMul(body, rEye, 0x7u, rEye, kSwizzleXYZW, rE, kSwizzleWWWW);
    EmitAdd(body, rAcc, 0x7u, rAcc, kSwizzleXYZW, rEye, kSwizzleXYZW);
  }

  // prevClip into rB. constant mode: o7 = (0,0,0,1) with ZERO reads. minimal/
  // no-bind modes: game's own ViewProjection (b0 c10..c13); otherwise the
  // addon's prevVP cbuffer. no-output test: block runs but no output write.
  if (constant) {
    EmitMovImm4(body, rB, 0xFu, 0u, 0u, 0u, 0x3F800000u);  // rB = (0,0,0,1)
  } else {
    EmitDp4Cb(body, rB, 0x1u, prev_vp_cb, prev_vp_elem + 0u, minimal ? rP : rAcc, kSwizzleXYZW);
    EmitDp4Cb(body, rB, 0x2u, prev_vp_cb, prev_vp_elem + 1u, minimal ? rP : rAcc, kSwizzleXYZW);
    EmitDp4Cb(body, rB, 0x4u, prev_vp_cb, prev_vp_elem + 2u, minimal ? rP : rAcc, kSwizzleXYZW);
    EmitDp4Cb(body, rB, 0x8u, prev_vp_cb, prev_vp_elem + 3u, minimal ? rP : rAcc, kSwizzleXYZW);
  }
  if (!no_output) EmitMovOutput(body, new_out_reg, rB, kSwizzleXYZW);

  // ── Rebuild the SHEX chunk token stream ──
  // Token stream starts 8 bytes into the chunk data (version/type + dword count).
  const uint32_t* tokens = reinterpret_cast<const uint32_t*>(shex_data + 8u);
  const uint32_t old_token_dwords = (shex->size - 8u) / 4u;
  uint32_t first_non_decl_dw = scan.first_non_decl / 4u - 2u;  // rel. to token stream
  uint32_t ret_dw = scan.ret_offset / 4u - 2u;                 // rel. to token stream

  // Copy the original token stream (mutable) and patch dcl_temps if present.
  std::vector<uint32_t> new_tokens(tokens, tokens + old_token_dwords);
  bool patched_temps = false;
  if (scan.has_temps) {
    // Find the dcl_temps instruction inside the copied stream (it's a decl).
    for (uint32_t i = 0; i < first_non_decl_dw;) {
      const uint32_t op = new_tokens[i] & 0x7FFu;
      const uint32_t len = new_tokens[i] >> 24u;
      if (op == OP_DCL_TEMPS && len >= 2u) {
        new_tokens[i + 1u] = new_dcl_temps;
        patched_temps = true;
        break;
      }
      i += len;
    }
  }
  if (!patched_temps) {
    // Insert a dcl_temps declaration at the front of the decl region.
    std::vector<uint32_t> tmp;
    EmitDclTemps(tmp, new_dcl_temps);
    new_tokens.insert(new_tokens.begin(), tmp.begin(), tmp.end());
    first_non_decl_dw += (uint32_t)tmp.size();
    ret_dw += (uint32_t)tmp.size();
  }

  // Splice: decls (old) + new decls + body (old) + new body + ret..
  std::vector<uint32_t> assembled;
  assembled.reserve(new_tokens.size() + decls.size() + body.size());
  assembled.insert(assembled.end(), new_tokens.begin(), new_tokens.begin() + first_non_decl_dw);
  assembled.insert(assembled.end(), decls.begin(), decls.end());
  assembled.insert(assembled.end(), new_tokens.begin() + first_non_decl_dw, new_tokens.begin() + ret_dw);
  assembled.insert(assembled.end(), body.begin(), body.end());
  assembled.insert(assembled.end(), new_tokens.begin() + ret_dw, new_tokens.end());

  // ── Assemble a fresh blob with the grown SHEX chunk ──
  const uint32_t new_shex_size = 8u + (uint32_t)assembled.size() * 4u;
  const uint32_t delta = new_shex_size - shex->size;
  const uint32_t new_file_size = (uint32_t)data.size() + delta;

  std::vector<std::byte> out;
  out.reserve(new_file_size);
  // Header (magic..chunk_count, 32 bytes) then chunk offset array.
  out.insert(out.end(), data.begin(), data.begin() + 32);
  std::memcpy(out.data() + 24u, &new_file_size, 4);  // file_size
  for (const auto& c : chunks) {
    uint32_t off = c.offset;
    if (c.offset > shex->offset) off += delta;  // only chunks AFTER the grown SHEX move
    std::byte* p = reinterpret_cast<std::byte*>(&off);
    out.insert(out.end(), p, p + 4);
  }
  // Chunk bodies.
  for (const auto& c : chunks) {
    const uint8_t* src = reinterpret_cast<const uint8_t*>(data.data()) + c.offset;
    if (c.offset == shex->offset) {
      // Write chunk name + new size + new body.
      out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + 4);  // name
      std::byte* szp = reinterpret_cast<std::byte*>(const_cast<uint32_t*>(&new_shex_size));
      out.insert(out.end(), szp, szp + 4);
      // version/type (4 bytes) from old chunk body start, then new dword count.
      // The SHEX/SHDR length field counts ALL dwords of the chunk data INCLUDING
      // the version/type and the length dword itself (i.e. tokens + 2, == size/4).
      // Writing only the token count makes fxc/driver stop parsing early and miss
      // the trailing `ret` -> draw-time JIT TDR (GetDeviceRemovedReason 0x887A0005).
      out.insert(out.end(), data.begin() + c.offset + 8, data.begin() + c.offset + 12);
      const uint32_t new_count = (uint32_t)assembled.size() + 2u;
      std::byte* cp = reinterpret_cast<std::byte*>(const_cast<uint32_t*>(&new_count));
      out.insert(out.end(), cp, cp + 4);
      // new tokens.
      const std::byte* tb = reinterpret_cast<const std::byte*>(assembled.data());
      out.insert(out.end(), tb, tb + assembled.size() * 4u);
    } else {
      const uint32_t clen = 8u + c.size;
      out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + clen);
    }
  }

  // ── Append the OSGN output entry (with fresh offsets) ──
  DXBCHeader h2;
  std::vector<ChunkInfo> chunks2;
  if (!ParseDXBC(out, h2, chunks2)) return false;
  const ChunkInfo* osgn2 = FindChunk(chunks2, "OSGN");
  if (!osgn2) return false;
  if (!AppendSignatureEntry(out, *osgn2, "TEXCOORD", tex_idx, new_out_reg, 0xFu, 0x0u,
                            3u /* FLOAT32 */))
    return false;

  // ── Fix the STAT chunk (D3D11_SHADER_DESC mirror) ──
  // The STAT chunk is what the D3D11 runtime / NVIDIA driver validate a shader
  // against (temp register count, instruction count). We bumped dcl_temps and
  // injected instructions, so the ORIGINAL STAT is now stale (temp=5 while the
  // bytecode declares dcl_temps=17). fxc /dumpbin tolerates the mismatch; the
  // driver's compiler sizes its register file from STAT -> overflow -> TDR.
  // This is the SAME "fxc tolerates, driver rejects" class of bug as the chunk
  // alignment and OSGN read_write_mask issues. Layout (verified on-disk):
  //   [0] InstructionCount   [1] TempRegisterCount   [2] TempArrayCount
  //   [3] DefCount           [4] DclCount            ...
  // Both [0] and [1] matched our independent counts exactly (98/5 original,
  // 121/4 fresh) so this mapping is confirmed.
  // NOTE: must RE-PARSE after AppendSignatureEntry — the append shifted every
  // chunk after OSGN (incl. STAT) by the padded entry size, so the chunk
  // offsets from chunks2 are stale.
  {
    DXBCHeader h3;
    std::vector<ChunkInfo> chunks3;
    if (ParseDXBC(out, h3, chunks3)) {
      if (const ChunkInfo* stat = FindChunk(chunks3, "STAT")) {
        if (stat->size >= 8u) {
          uint8_t* stat_data =
              reinterpret_cast<uint8_t*>(out.data()) + stat->offset + kChunkHeaderSize;
          // Count non-declaration instructions in the final token stream.
          uint32_t new_instr = 0u;
          size_t i = 0u;
          while (i < assembled.size()) {
            const uint32_t t = assembled[i];
            const uint32_t len = t >> 24u;
            if (len == 0u) break;
            if (!IsDeclOpcode(t & 0x7FFu)) ++new_instr;
            i += len;
          }
          std::memcpy(stat_data + 0u, &new_instr, 4);      // InstructionCount
          std::memcpy(stat_data + 4u, &new_dcl_temps, 4);  // TempRegisterCount
        }
      }
    }
  }

  WriteDXBCHash(out);
  data = std::move(out);
  uint32_t new_hash = 0u;
  {
    const uint8_t* c = reinterpret_cast<const uint8_t*>(data.data());
    new_hash = ComputeCRC32(c, data.size());
  }
  if (out_new_hash != nullptr) *out_new_hash = new_hash;
  if (out_info != nullptr) {
    out_info->new_hash = new_hash;
    out_info->prev_vp_cb_slot = prev_vp_cb;
    out_info->prev_bone_t_slot = bone_t_slot;
    out_info->bone_game_slot = scan.structured_slots.empty() ? 0u : scan.structured_slots[0];
    out_info->texcoord_index = tex_idx;
    out_info->output_reg = new_out_reg;
    out_info->outline_applied = outline;  // true only if emitted + bounds-confirmed
    out_info->needs_no_binding = no_bind;
  }
  return true;
}

// ── Generic per-object-motion PS patcher (Phase E core) ──
// Detects a G-buffer pixel shader (>=3 SV_TARGET outputs), appends a TEXCOORD5
// input (prevClip from the paired patched char VS) + an SV_TARGET3 output (the
// appended 32-bit motion RTV), and injects
//   o3 = (prevNDC.x*0.5+0.5, prevNDC.y*0.5+0.5, 0, 1),  prevNDC = vN.xy / max(vN.w, 0.001)
// before the trailing ret. Written UNCONDITIONALLY (no b13 read): the motion
// RTV is only bound on char draws (MaybeAppendMotionRtv), so o3 is discarded
// whenever per-object motion is off. Mirrors the VS patch's chunk/STAT/hash
// fixups. No per-hash lists — this replaces the 22 hand-patched boot PSs.
inline bool PatchPerObjectPixelShader(std::vector<std::byte>& data, uint32_t* out_new_hash) {
  DXBCHeader header;
  std::vector<ChunkInfo> chunks;
  if (!ParseDXBC(data, header, chunks)) return false;
  const ChunkInfo* isgn = FindChunk(chunks, "ISGN");
  const ChunkInfo* osgn = FindChunk(chunks, "OSGN");
  const ChunkInfo* shex = FindChunk(chunks, "SHEX");
  if (!shex) shex = FindChunk(chunks, "SHDR");
  if (!isgn || !osgn || !shex) return false;

  std::vector<SignatureEntry> is, os;
  if (!ParseSignature(data, *isgn, is) || !ParseSignature(data, *osgn, os)) return false;

  // Scope gate: G-buffer PS with EXACTLY outputs at registers 0/1/2 (3
  // SV_TARGETs). Skip if SV_TARGET3 already exists (the appended motion RTV
  // lives at index 3) or if the PS already reads TEXCOORD5 (would collide with
  // our prevClip input). Requiring max_out == 2 keeps the appended output at
  // register 3 with semantic index 3 (the RTV index MaybeAppendMotionRtv uses).
  uint32_t max_out = 0;
  uint32_t sv_target_count = 0;
  for (const auto& e : os) {
    if (e.name == "SV_TARGET") {
      ++sv_target_count;
      if (e.semantic_index == 3u) return false;  // SV_TARGET3 already taken
    }
    if (e.register_index > max_out) max_out = e.register_index;
  }
  if (max_out != 2u || sv_target_count < 3u) return false;  // not a 3-RT G-buffer PS
  bool has_tex5 = false;
  uint32_t max_in = 0;
  for (const auto& e : is) {
    if (e.name == "TEXCOORD" && e.semantic_index == 5u) has_tex5 = true;
    if (e.register_index > max_in) max_in = e.register_index;
  }
  if (has_tex5) return false;

  const uint32_t new_in_reg = max_in + 1u;   // free input register (vN)
  const uint32_t new_out_reg = max_out + 1u; // = 3 for G-buffer PSs

  // Scan SHEX.
  const uint8_t* shex_data =
      reinterpret_cast<const uint8_t*>(data.data()) + shex->offset + kChunkHeaderSize;
  ShexScan scan;
  if (!ScanShex(shex_data, shex->size, scan)) return false;
  if (!scan.has_ret) return false;

  // Allocate temps: base = declared count or max used + 1.
  const uint32_t temp_base = std::max(scan.has_temps ? scan.dcl_temps : 0u, scan.max_temp + 1u);
  const uint32_t rT = temp_base;        // prevClip scratch
  const uint32_t rHalf = temp_base + 1u; // 0.5 constant
  const uint32_t new_dcl_temps = temp_base + 2u;

  // Build injected declaration tokens: dcl_input_ps vN.xyzw + dcl_output oM.
  std::vector<uint32_t> decls;
  EmitDclInputPs(decls, new_in_reg);
  EmitDclOutput(decls, new_out_reg);

  // Build injected body tokens (mirrors the hand-patched boot PS):
  //   rT = vN; rT.w = max(rT.w, 0.001); rT.xy = rT.xy / rT.w
  //   rHalf = (0.5,0.5); rT.xy = rT.xy*0.5 + 0.5
  //   rT.z = 0; rT.w = 1; o3 = rT
  std::vector<uint32_t> body;
  EmitMovInput(body, rT, 0xFu, new_in_reg, 0xFu);                  // mov rT.xyzw, vN.xyzw
  EmitMaxImm1(body, rT, 0x8u, rT, kSwizzleWWWW, 0x3A83126Fu);      // max rT.w, rT.w, l(0.001)
  EmitDiv(body, rT, 0x3u, rT, kSwizzleXYZW, rT, kSwizzleWWWW);     // div rT.xy, rT.xy, rT.ww
  EmitMovImm4(body, rHalf, 0x3u, 0x3F000000u, 0x3F000000u, 0u, 0u); // rHalf.xy = (0.5,0.5)
  EmitMad(body, rT, 0x3u, rT, kSwizzleXYZW, rHalf, kSwizzleXYZW, rHalf,
          kSwizzleXYZW);                                          // rT.xy = rT.xy*0.5+0.5
  EmitMovImm1(body, rT, 0x4u, 0u);                                 // rT.z = 0
  EmitMovImm1(body, rT, 0x8u, 0x3F800000u);                        // rT.w = 1.0 (valid flag)
  EmitMovOutput(body, new_out_reg, rT, kSwizzleXYZW);              // mov oM.xyzw, rT.xyzw

  // ── Rebuild the SHEX chunk token stream (same as the VS patch) ──
  const uint32_t* tokens = reinterpret_cast<const uint32_t*>(shex_data + 8u);
  const uint32_t old_token_dwords = (shex->size - 8u) / 4u;
  uint32_t first_non_decl_dw = scan.first_non_decl / 4u - 2u;  // rel. to token stream
  uint32_t ret_dw = scan.ret_offset / 4u - 2u;

  std::vector<uint32_t> new_tokens(tokens, tokens + old_token_dwords);
  bool patched_temps = false;
  if (scan.has_temps) {
    for (uint32_t i = 0; i < first_non_decl_dw;) {
      const uint32_t op = new_tokens[i] & 0x7FFu;
      const uint32_t len = new_tokens[i] >> 24u;
      if (op == OP_DCL_TEMPS && len >= 2u) {
        new_tokens[i + 1u] = new_dcl_temps;
        patched_temps = true;
        break;
      }
      i += len;
    }
  }
  if (!patched_temps) {
    std::vector<uint32_t> tmp;
    EmitDclTemps(tmp, new_dcl_temps);
    new_tokens.insert(new_tokens.begin(), tmp.begin(), tmp.end());
    first_non_decl_dw += (uint32_t)tmp.size();
    ret_dw += (uint32_t)tmp.size();
  }

  // Splice: decls (old) + new decls + body (old) + new body + ret..
  std::vector<uint32_t> assembled;
  assembled.reserve(new_tokens.size() + decls.size() + body.size());
  assembled.insert(assembled.end(), new_tokens.begin(), new_tokens.begin() + first_non_decl_dw);
  assembled.insert(assembled.end(), decls.begin(), decls.end());
  assembled.insert(assembled.end(), new_tokens.begin() + first_non_decl_dw,
                   new_tokens.begin() + ret_dw);
  assembled.insert(assembled.end(), body.begin(), body.end());
  assembled.insert(assembled.end(), new_tokens.begin() + ret_dw, new_tokens.end());

  // ── Assemble a fresh blob with the grown SHEX chunk ──
  const uint32_t new_shex_size = 8u + (uint32_t)assembled.size() * 4u;
  const uint32_t delta = new_shex_size - shex->size;
  const uint32_t new_file_size = (uint32_t)data.size() + delta;

  std::vector<std::byte> out;
  out.reserve(new_file_size);
  out.insert(out.end(), data.begin(), data.begin() + 32);
  std::memcpy(out.data() + 24u, &new_file_size, 4);
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
      out.insert(out.end(), data.begin() + c.offset + 8, data.begin() + c.offset + 12);
      const uint32_t new_count = (uint32_t)assembled.size() + 2u;
      std::byte* cp = reinterpret_cast<std::byte*>(const_cast<uint32_t*>(&new_count));
      out.insert(out.end(), cp, cp + 4);
      const std::byte* tb = reinterpret_cast<const std::byte*>(assembled.data());
      out.insert(out.end(), tb, tb + assembled.size() * 4u);
    } else {
      const uint32_t clen = 8u + c.size;
      out.insert(out.end(), data.begin() + c.offset, data.begin() + c.offset + clen);
    }
  }

  // ── Append the ISGN input entry (TEXCOORD5, free input register) ──
  {
    DXBCHeader h2;
    std::vector<ChunkInfo> chunks2;
    if (!ParseDXBC(out, h2, chunks2)) return false;
    const ChunkInfo* isgn2 = FindChunk(chunks2, "ISGN");
    if (!isgn2) return false;
    // rw_mask 0x0B mirrors the hand-patched PS ISGN entry (reads .xyw).
    if (!AppendSignatureEntry(out, *isgn2, "TEXCOORD", 5u, new_in_reg, 0xFu, 0x0Bu,
                              3u /* FLOAT32 */))
      return false;
  }
  // ── Append the OSGN output entry (SV_TARGET3) ──
  {
    DXBCHeader h2;
    std::vector<ChunkInfo> chunks2;
    if (!ParseDXBC(out, h2, chunks2)) return false;
    const ChunkInfo* osgn2 = FindChunk(chunks2, "OSGN");
    if (!osgn2) return false;
    if (!AppendSignatureEntry(out, *osgn2, "SV_TARGET", 3u, new_out_reg, 0xFu, 0x0u,
                              3u /* FLOAT32 */))
      return false;
  }

  // ── Fix the STAT chunk (instruction + temp counts) ──
  {
    DXBCHeader h2;
    std::vector<ChunkInfo> chunks2;
    if (ParseDXBC(out, h2, chunks2)) {
      if (const ChunkInfo* stat = FindChunk(chunks2, "STAT")) {
        if (stat->size >= 8u) {
          uint8_t* stat_data =
              reinterpret_cast<uint8_t*>(out.data()) + stat->offset + kChunkHeaderSize;
          uint32_t new_instr = 0u;
          size_t i = 0u;
          while (i < assembled.size()) {
            const uint32_t t = assembled[i];
            const uint32_t len = t >> 24u;
            if (len == 0u) break;
            if (!IsDeclOpcode(t & 0x7FFu)) ++new_instr;
            i += len;
          }
          std::memcpy(stat_data + 0u, &new_instr, 4);
          std::memcpy(stat_data + 4u, &new_dcl_temps, 4);
        }
      }
    }
  }

  WriteDXBCHash(out);
  data = std::move(out);
  uint32_t new_hash = 0u;
  {
    const uint8_t* c = reinterpret_cast<const uint8_t*>(data.data());
    new_hash = ComputeCRC32(c, data.size());
  }
  if (out_new_hash != nullptr) *out_new_hash = new_hash;
  return true;
}

}  // namespace senkiseki3::dxbc
