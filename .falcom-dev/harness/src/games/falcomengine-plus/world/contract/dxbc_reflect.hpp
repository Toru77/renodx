#pragma once

// Minimal, bounds-checked DXBC (SM4/SM5) reflection reader.
//
// Reads only what the world shader classifier needs from a shader's own
// bytecode: RDEF resource bindings, constant-buffer variables (with their
// "used" flag), struct member layouts of structured-buffer elements, the
// input and output signatures, and whether the program contains a discard. There is no
// D3DCompiler dependency and every offset is validated, so an unexpected
// blob reports `valid = false` with an error instead of crashing.
//
// Layout references: the RDEF header carries an 'RD11' sub-header on SM5
// that states the descriptor sizes (cbuffer 24, binding 32, variable 40,
// type 36, member 12); SM4 uses the fixed sizes below. Name offsets are
// relative to the start of their chunk's data.

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace falcom_world::dxbc {

// D3D_SHADER_INPUT_TYPE values used by the classifier.
inline constexpr uint32_t kInputCBuffer = 0u;
inline constexpr uint32_t kInputTBuffer = 1u;
inline constexpr uint32_t kInputTexture = 2u;
inline constexpr uint32_t kInputSampler = 3u;
inline constexpr uint32_t kInputStructured = 5u;
inline constexpr uint32_t kInputByteAddress = 7u;

// D3D_CBUFFER_TYPE: the per-structured-buffer "resource bind info" entry.
inline constexpr uint32_t kCBufferResourceBindInfo = 3u;

// D3D_SHADER_VARIABLE_CLASS / TYPE values used by the layout check.
inline constexpr uint16_t kClassVector = 1u;
inline constexpr uint16_t kClassMatrixRows = 2u;
inline constexpr uint16_t kClassMatrixColumns = 3u;
inline constexpr uint16_t kTypeFloat = 3u;

inline constexpr uint32_t kVariableUsedFlag = 0x2u;  // D3D_SVF_USED
inline constexpr uint32_t kOpcodeDiscard = 0x0Du;
inline constexpr uint32_t kOpcodeCustomData = 0x35u;

struct ResourceBinding {
  std::string name;
  uint32_t type = 0u;
  uint32_t bind_point = 0u;
  uint32_t bind_count = 0u;
  uint32_t stride = 0u;  // NumSamples field: element stride of structured buffers
};

struct StructMember {
  std::string name;
  uint32_t offset = 0u;
  uint16_t var_class = 0u;
  uint16_t var_type = 0u;
  uint16_t rows = 0u;
  uint16_t columns = 0u;
};

struct Variable {
  std::string name;
  uint32_t offset = 0u;
  uint32_t size = 0u;
  bool used = false;
  std::vector<StructMember> members;  // direct members of struct-typed variables
};

struct ConstantBuffer {
  std::string name;
  uint32_t type = 0u;
  uint32_t size = 0u;
  std::vector<Variable> variables;
};

// D3D_REGISTER_COMPONENT_TYPE values of signature elements.
inline constexpr uint32_t kComponentUint32 = 1u;
inline constexpr uint32_t kComponentSint32 = 2u;
inline constexpr uint32_t kComponentFloat32 = 3u;
// D3D_NAME value of SV_Position.
inline constexpr uint32_t kSystemValuePosition = 1u;

struct SignatureElement {
  std::string semantic;
  uint32_t semantic_index = 0u;
  uint32_t system_value = 0u;    // D3D_NAME (0 = none, 1 = SV_Position, ...)
  uint32_t component_type = 0u;  // kComponent*
  uint32_t reg = 0u;             // register index
  uint8_t mask = 0u;             // components the element occupies (bit 0 = x)
};

struct Reflection {
  bool valid = false;
  bool has_rdef = false;
  uint16_t program_type = 0u;  // 0xFFFE vertex, 0xFFFF pixel
  uint8_t major = 0u;
  uint8_t minor = 0u;
  std::vector<ResourceBinding> resources;
  std::vector<ConstantBuffer> constant_buffers;
  std::vector<SignatureElement> inputs;
  std::vector<SignatureElement> outputs;
  bool has_discard = false;
  std::string error;

  [[nodiscard]] const ResourceBinding* FindResource(uint32_t type, uint32_t bind_point) const {
    for (const auto& resource : resources) {
      if (resource.type == type && resource.bind_point == bind_point) return &resource;
    }
    return nullptr;
  }

  [[nodiscard]] const ConstantBuffer* FindConstantBuffer(std::string_view name, uint32_t type) const {
    for (const auto& buffer : constant_buffers) {
      if (buffer.type == type && buffer.name == name) return &buffer;
    }
    return nullptr;
  }

  [[nodiscard]] bool HasInput(std::string_view semantic, uint32_t semantic_index) const {
    for (const auto& input : inputs) {
      if (input.semantic_index == semantic_index && input.semantic == semantic) return true;
    }
    return false;
  }

  [[nodiscard]] bool HasInput(std::string_view semantic) const {
    for (const auto& input : inputs) {
      if (input.semantic == semantic) return true;
    }
    return false;
  }

  // True when any constant buffer of the given D3D_CBUFFER_TYPE declares a
  // variable with this name that the program actually reads.
  [[nodiscard]] bool UsesVariable(std::string_view name, uint32_t cbuffer_type = 0u) const {
    for (const auto& buffer : constant_buffers) {
      if (buffer.type != cbuffer_type) continue;
      for (const auto& variable : buffer.variables) {
        if (variable.used && variable.name == name) return true;
      }
    }
    return false;
  }
};

namespace internal {

class Span {
 public:
  Span() = default;
  Span(const uint8_t* data, size_t size) : data_(data), size_(size) {}

  [[nodiscard]] bool Read32(size_t offset, uint32_t* out) const {
    if (offset > size_ || size_ - offset < 4u) return false;
    std::memcpy(out, data_ + offset, 4u);
    return true;
  }

  [[nodiscard]] bool Read16(size_t offset, uint16_t* out) const {
    if (offset > size_ || size_ - offset < 2u) return false;
    std::memcpy(out, data_ + offset, 2u);
    return true;
  }

  [[nodiscard]] bool Read8(size_t offset, uint8_t* out) const {
    if (offset >= size_) return false;
    *out = data_[offset];
    return true;
  }

  // Null-terminated string bounded by the chunk; names longer than 255
  // characters are treated as malformed.
  [[nodiscard]] bool ReadString(size_t offset, std::string* out) const {
    if (offset >= size_) return false;
    const size_t limit = (size_ - offset) < 256u ? (size_ - offset) : 256u;
    const auto* start = reinterpret_cast<const char*>(data_ + offset);
    const void* end = std::memchr(start, '\0', limit);
    if (end == nullptr) return false;
    out->assign(start, static_cast<const char*>(end));
    return true;
  }

  [[nodiscard]] size_t size() const { return size_; }

 private:
  const uint8_t* data_ = nullptr;
  size_t size_ = 0u;
};

// Returns the data span of the first chunk with this fourcc. Name offsets in
// RDEF/ISGN are relative to that span's start.
inline bool ChunkData(const uint8_t* base, size_t size, uint32_t fourcc, Span* out) {
  const Span container(base, size);
  uint32_t magic = 0u;
  uint32_t chunk_count = 0u;
  if (!container.Read32(0u, &magic) || magic != 0x43425844u) return false;  // 'DXBC'
  if (!container.Read32(28u, &chunk_count) || chunk_count > 64u) return false;
  for (uint32_t i = 0; i < chunk_count; ++i) {
    uint32_t chunk_offset = 0u;
    uint32_t chunk_fourcc = 0u;
    uint32_t chunk_size = 0u;
    if (!container.Read32(32u + static_cast<size_t>(i) * 4u, &chunk_offset)) return false;
    if (!container.Read32(chunk_offset, &chunk_fourcc)) return false;
    if (!container.Read32(static_cast<size_t>(chunk_offset) + 4u, &chunk_size)) return false;
    if (chunk_fourcc != fourcc) continue;
    const size_t data_offset = static_cast<size_t>(chunk_offset) + 8u;
    if (data_offset > size || size - data_offset < chunk_size) return false;
    *out = Span(base + data_offset, chunk_size);
    return true;
  }
  return false;
}

inline constexpr uint32_t FourCC(char a, char b, char c, char d) {
  return static_cast<uint32_t>(static_cast<uint8_t>(a))
         | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8u)
         | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16u)
         | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24u);
}

struct RdefSizes {
  uint32_t cbuffer = 24u;
  uint32_t binding = 32u;
  uint32_t variable = 24u;
  uint32_t member = 12u;
};

inline bool ParseMembers(const Span& rdef, uint32_t type_offset, std::vector<StructMember>* out) {
  uint16_t member_count = 0u;
  uint32_t member_offset = 0u;
  if (!rdef.Read16(type_offset + 10u, &member_count)) return false;
  if (member_count == 0u) return true;
  if (member_count > 64u) return false;
  if (!rdef.Read32(type_offset + 12u, &member_offset)) return false;
  out->reserve(member_count);
  for (uint32_t m = 0; m < member_count; ++m) {
    const size_t entry = static_cast<size_t>(member_offset) + m * 12u;
    uint32_t name_offset = 0u;
    uint32_t member_type_offset = 0u;
    StructMember member;
    if (!rdef.Read32(entry + 0u, &name_offset)) return false;
    if (!rdef.Read32(entry + 4u, &member_type_offset)) return false;
    if (!rdef.Read32(entry + 8u, &member.offset)) return false;
    if (!rdef.ReadString(name_offset, &member.name)) return false;
    if (!rdef.Read16(member_type_offset + 0u, &member.var_class)) return false;
    if (!rdef.Read16(member_type_offset + 2u, &member.var_type)) return false;
    if (!rdef.Read16(member_type_offset + 4u, &member.rows)) return false;
    if (!rdef.Read16(member_type_offset + 6u, &member.columns)) return false;
    out->push_back(std::move(member));
  }
  return true;
}

inline bool ParseRdef(const Span& rdef, Reflection* out) {
  uint32_t cbuffer_count = 0u;
  uint32_t cbuffer_offset = 0u;
  uint32_t binding_count = 0u;
  uint32_t binding_offset = 0u;
  if (!rdef.Read32(0u, &cbuffer_count) || !rdef.Read32(4u, &cbuffer_offset)) return false;
  if (!rdef.Read32(8u, &binding_count) || !rdef.Read32(12u, &binding_offset)) return false;
  if (!rdef.Read8(16u, &out->minor) || !rdef.Read8(17u, &out->major)) return false;
  if (!rdef.Read16(18u, &out->program_type)) return false;
  if (cbuffer_count > 64u || binding_count > 256u) return false;

  RdefSizes sizes;
  uint32_t rd11 = 0u;
  if (out->major >= 5u && rdef.Read32(28u, &rd11) && rd11 == FourCC('R', 'D', '1', '1')) {
    uint32_t value = 0u;
    if (rdef.Read32(36u, &value)) sizes.cbuffer = value;
    if (rdef.Read32(40u, &value)) sizes.binding = value;
    if (rdef.Read32(44u, &value)) sizes.variable = value;
    if (rdef.Read32(52u, &value)) sizes.member = value;
  }
  if (sizes.cbuffer < 24u || sizes.binding < 32u || sizes.variable < 24u || sizes.member < 12u) return false;

  out->resources.reserve(binding_count);
  for (uint32_t i = 0; i < binding_count; ++i) {
    const size_t entry = static_cast<size_t>(binding_offset) + i * sizes.binding;
    uint32_t name_offset = 0u;
    ResourceBinding binding;
    if (!rdef.Read32(entry + 0u, &name_offset)) return false;
    if (!rdef.Read32(entry + 4u, &binding.type)) return false;
    if (!rdef.Read32(entry + 16u, &binding.stride)) return false;
    if (!rdef.Read32(entry + 20u, &binding.bind_point)) return false;
    if (!rdef.Read32(entry + 24u, &binding.bind_count)) return false;
    if (!rdef.ReadString(name_offset, &binding.name)) return false;
    out->resources.push_back(std::move(binding));
  }

  out->constant_buffers.reserve(cbuffer_count);
  for (uint32_t i = 0; i < cbuffer_count; ++i) {
    const size_t entry = static_cast<size_t>(cbuffer_offset) + i * sizes.cbuffer;
    uint32_t name_offset = 0u;
    uint32_t variable_count = 0u;
    uint32_t variable_offset = 0u;
    ConstantBuffer buffer;
    if (!rdef.Read32(entry + 0u, &name_offset)) return false;
    if (!rdef.Read32(entry + 4u, &variable_count)) return false;
    if (!rdef.Read32(entry + 8u, &variable_offset)) return false;
    if (!rdef.Read32(entry + 12u, &buffer.size)) return false;
    if (!rdef.Read32(entry + 20u, &buffer.type)) return false;
    if (!rdef.ReadString(name_offset, &buffer.name)) return false;
    if (variable_count > 512u) return false;
    buffer.variables.reserve(variable_count);
    for (uint32_t v = 0; v < variable_count; ++v) {
      const size_t variable_entry = static_cast<size_t>(variable_offset) + v * sizes.variable;
      uint32_t variable_name_offset = 0u;
      uint32_t flags = 0u;
      uint32_t type_offset = 0u;
      Variable variable;
      if (!rdef.Read32(variable_entry + 0u, &variable_name_offset)) return false;
      if (!rdef.Read32(variable_entry + 4u, &variable.offset)) return false;
      if (!rdef.Read32(variable_entry + 8u, &variable.size)) return false;
      if (!rdef.Read32(variable_entry + 12u, &flags)) return false;
      if (!rdef.Read32(variable_entry + 16u, &type_offset)) return false;
      if (!rdef.ReadString(variable_name_offset, &variable.name)) return false;
      variable.used = (flags & kVariableUsedFlag) != 0u;
      // Only structured-buffer element layouts are needed one level deep.
      if (buffer.type == kCBufferResourceBindInfo && !ParseMembers(rdef, type_offset, &variable.members)) return false;
      buffer.variables.push_back(std::move(variable));
    }
    out->constant_buffers.push_back(std::move(buffer));
  }
  return true;
}

// ISGN/OSGN entries are 24 bytes (name, index, system value, component
// type, register, mask, read/write mask); ISG1/OSG1 prefix a stream field
// and append a min-precision field (32 bytes), so `name_field` is 4 there.
inline bool ParseSignature(const Span& signature, uint32_t element_size, uint32_t name_field,
                           std::vector<SignatureElement>* out) {
  uint32_t count = 0u;
  if (!signature.Read32(0u, &count) || count > 64u) return false;
  out->reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const size_t entry = 8u + static_cast<size_t>(i) * element_size + name_field;
    uint32_t name_offset = 0u;
    SignatureElement element;
    if (!signature.Read32(entry, &name_offset)) return false;
    if (!signature.Read32(entry + 4u, &element.semantic_index)) return false;
    if (!signature.Read32(entry + 8u, &element.system_value)) return false;
    if (!signature.Read32(entry + 12u, &element.component_type)) return false;
    if (!signature.Read32(entry + 16u, &element.reg)) return false;
    if (!signature.Read8(entry + 20u, &element.mask)) return false;
    if (!signature.ReadString(name_offset, &element.semantic)) return false;
    out->push_back(std::move(element));
  }
  return true;
}

// Walks the instruction stream only far enough to find a discard; operand
// decoding is not needed because every instruction token states its length.
inline bool ScanForDiscard(const Span& shader, bool* out_discard) {
  uint32_t length = 0u;
  if (!shader.Read32(4u, &length)) return false;
  const size_t token_count = shader.size() / 4u;
  if (length > token_count) length = static_cast<uint32_t>(token_count);
  size_t token = 2u;
  while (token < length) {
    uint32_t opcode_token = 0u;
    if (!shader.Read32(token * 4u, &opcode_token)) return false;
    const uint32_t opcode = opcode_token & 0x7FFu;
    uint32_t instruction_length = (opcode_token >> 24u) & 0x7Fu;
    if (opcode == kOpcodeCustomData) {
      if (!shader.Read32((token + 1u) * 4u, &instruction_length)) return false;
    }
    if (instruction_length == 0u) return false;
    if (opcode == kOpcodeDiscard) {
      *out_discard = true;
      return true;
    }
    token += instruction_length;
  }
  return true;
}

}  // namespace internal

inline Reflection Parse(const void* code, size_t size) {
  Reflection reflection;
  if (code == nullptr || size < 32u) {
    reflection.error = "no bytecode";
    return reflection;
  }
  const auto* base = static_cast<const uint8_t*>(code);

  internal::Span rdef;
  if (!internal::ChunkData(base, size, internal::FourCC('R', 'D', 'E', 'F'), &rdef)) {
    reflection.error = "no RDEF chunk (reflection stripped)";
    return reflection;
  }
  reflection.has_rdef = true;
  if (!internal::ParseRdef(rdef, &reflection)) {
    reflection.error = "malformed RDEF";
    return reflection;
  }

  internal::Span signature;
  if (internal::ChunkData(base, size, internal::FourCC('I', 'S', 'G', '1'), &signature)) {
    if (!internal::ParseSignature(signature, 32u, 4u, &reflection.inputs)) {
      reflection.error = "malformed ISG1";
      return reflection;
    }
  } else if (internal::ChunkData(base, size, internal::FourCC('I', 'S', 'G', 'N'), &signature)) {
    if (!internal::ParseSignature(signature, 24u, 0u, &reflection.inputs)) {
      reflection.error = "malformed ISGN";
      return reflection;
    }
  }
  if (internal::ChunkData(base, size, internal::FourCC('O', 'S', 'G', '1'), &signature)) {
    if (!internal::ParseSignature(signature, 32u, 4u, &reflection.outputs)) {
      reflection.error = "malformed OSG1";
      return reflection;
    }
  } else if (internal::ChunkData(base, size, internal::FourCC('O', 'S', 'G', 'N'), &signature)) {
    if (!internal::ParseSignature(signature, 24u, 0u, &reflection.outputs)) {
      reflection.error = "malformed OSGN";
      return reflection;
    }
  }

  internal::Span program;
  if (internal::ChunkData(base, size, internal::FourCC('S', 'H', 'E', 'X'), &program)
      || internal::ChunkData(base, size, internal::FourCC('S', 'H', 'D', 'R'), &program)) {
    if (!internal::ScanForDiscard(program, &reflection.has_discard)) {
      reflection.error = "malformed shader program";
      return reflection;
    }
  }

  reflection.valid = true;
  return reflection;
}

}  // namespace falcom_world::dxbc
