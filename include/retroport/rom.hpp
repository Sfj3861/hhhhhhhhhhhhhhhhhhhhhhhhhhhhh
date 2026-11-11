#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace retroport {

enum class Platform {
    GameBoy,
    GameBoyColor,
    GameBoyAdvance,
    NintendoDS,
    Unknown
};

struct RomInfo {
    Platform platform{Platform::Unknown};
    std::string source;
    std::uint64_t file_size{0};
    std::string title;
    std::string game_code;
    std::string maker_code;
    std::uint8_t version{0};
    int detection_score{0};
    std::vector<std::string> warnings;
};

struct InspectionResult {
    bool success{false};
    RomInfo info;
    std::string error;
};

[[nodiscard]] std::string to_string(Platform platform);

// Inspect a ROM already loaded in memory. This is useful for tests and for
// future frontends that do not read files directly.
[[nodiscard]] InspectionResult inspect_rom_bytes(
    std::span<const std::uint8_t> bytes,
    std::string source_name = "<memory>"
);

// Read a ROM file and inspect its platform header.
[[nodiscard]] InspectionResult inspect_rom(const std::filesystem::path& path);

} // namespace retroport
