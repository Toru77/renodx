/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <Windows.h>
#include <dxgi1_6.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace farcry6_dlss::streamline {

// ReShade's Ubisoft compatibility mode patches the native factory vtable.
// Recover its original method address from the matching system DXGI image so a
// hook can sit below ReShade and the overlay, preserving their call order.
inline void* FindNativeFactoryMethod(IDXGIFactory2* factory, uint32_t slot) {
  if (factory == nullptr || slot != 15u) return nullptr;
  auto** table = *reinterpret_cast<void***>(factory);
  HMODULE module = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(table), &module)) return nullptr;
  std::wstring path(32768u, L'\0');
  std::wstring system_path(32768u, L'\0');
  const DWORD path_length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
  const UINT system_length = GetSystemDirectoryW(system_path.data(), static_cast<UINT>(system_path.size()));
  if (path_length == 0u || path_length >= path.size() || system_length == 0u || system_length >= system_path.size()) return nullptr;
  std::wstring expected(system_path.data(), system_length);
  expected += L"\\dxgi.dll";
  if (_wcsicmp(path.data(), expected.c_str()) != 0) return nullptr;

  HANDLE file = CreateFileW(path.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return nullptr;
  LARGE_INTEGER length = {};
  if (!GetFileSizeEx(file, &length) || length.QuadPart < static_cast<LONGLONG>(sizeof(IMAGE_DOS_HEADER))) {
    CloseHandle(file);
    return nullptr;
  }
  HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
  const auto* bytes = mapping == nullptr ? nullptr : static_cast<const uint8_t*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
  void* result = nullptr;
  if (bytes != nullptr) {
    const uint64_t file_size = static_cast<uint64_t>(length.QuadPart);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes);
    const auto* loaded_dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew >= 0
        && static_cast<uint64_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) <= file_size
        && loaded_dos->e_magic == IMAGE_DOS_SIGNATURE && loaded_dos->e_lfanew >= 0) {
      const auto* image = reinterpret_cast<const IMAGE_NT_HEADERS64*>(bytes + dos->e_lfanew);
      const auto* loaded = reinterpret_cast<const IMAGE_NT_HEADERS64*>(reinterpret_cast<const uint8_t*>(module) + loaded_dos->e_lfanew);
      const uint64_t section_offset = static_cast<uint64_t>(dos->e_lfanew) + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader)
                                    + image->FileHeader.SizeOfOptionalHeader;
      if (image->Signature == IMAGE_NT_SIGNATURE && loaded->Signature == IMAGE_NT_SIGNATURE
          && image->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC
          && image->FileHeader.TimeDateStamp == loaded->FileHeader.TimeDateStamp
          && image->OptionalHeader.SizeOfImage == loaded->OptionalHeader.SizeOfImage
          && section_offset + static_cast<uint64_t>(image->FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER) <= file_size) {
        const auto* sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(bytes + section_offset);
        const uintptr_t table_address = reinterpret_cast<uintptr_t>(&table[slot]);
        const uintptr_t base = reinterpret_cast<uintptr_t>(module);
        if (table_address >= base && table_address - base < image->OptionalHeader.SizeOfImage) {
          const uint64_t rva = table_address - base;
          for (uint16_t i = 0; i < image->FileHeader.NumberOfSections; ++i) {
            const auto& section = sections[i];
            if (rva < section.VirtualAddress || rva - section.VirtualAddress + sizeof(uint64_t) > section.SizeOfRawData) continue;
            const uint64_t offset = section.PointerToRawData + rva - section.VirtualAddress;
            if (offset + sizeof(uint64_t) > file_size) break;
            uint64_t original = 0u;
            std::memcpy(&original, bytes + offset, sizeof(original));
            if (original < image->OptionalHeader.ImageBase) break;
            const uint64_t method_rva = original - image->OptionalHeader.ImageBase;
            for (uint16_t j = 0; j < image->FileHeader.NumberOfSections; ++j) {
              const auto& code = sections[j];
              if ((code.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0u && method_rva >= code.VirtualAddress
                  && method_rva - code.VirtualAddress < code.Misc.VirtualSize && method_rva < image->OptionalHeader.SizeOfImage) {
                result = reinterpret_cast<void*>(base + method_rva);
                break;
              }
            }
            break;
          }
        }
      }
    }
    UnmapViewOfFile(bytes);
  }
  if (mapping != nullptr) CloseHandle(mapping);
  CloseHandle(file);
  return result;
}

}  // namespace farcry6_dlss::streamline
