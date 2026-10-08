#pragma once
// Harness stub of src/utils/path.hpp: output goes to a temp directory.
#include <filesystem>
#include <fstream>
#include <string>

namespace renodx::utils::path {

static std::filesystem::path GetOutputPath() {
  return std::filesystem::temp_directory_path() / "pooltest";
}

static void WriteTextFile(const std::filesystem::path& path, std::string& string) {
  std::ofstream file(path);
  file << string;
}

}  // namespace renodx::utils::path
