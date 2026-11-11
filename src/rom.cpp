#include "retroport/rom.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

namespace retroport {
namespace {

constexpr std::uint64_t kMaximumRomSize = 512ULL * 1024ULL * 1024ULL;

constexpr std::array<std::uint8_t, 48> kGameBoyLogo{
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

struct Candidate {
    Platform platform{Platform::Unknown};
    int score{0};
    std::string title;
    std::string game_code;
    std::string maker_code;
    std::uint8_t version{0};
    std::vector<std::string> warnings;
};

std::string extract_text(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::size_t length
) {
    if (offset > bytes.size() || length > bytes.size() - offset) {
        return {};
    }

    std::string value;
    value.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
        const auto character = bytes[offset + i];
        if (character == 0) {
            break;
        }
        if (character >= 0x20 && character <= 0x7E) {
            value.push_back(static_cast<char>(character));
        }
    }

    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }

    return value;
}

bool is_alphanumeric_code(std::string_view value, std::size_t expected_length) {
    if (value.size() != expected_length) {
        return false;
    }

    return std::all_of(value.begin(), value.end(), [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0;
    });
}

std::uint32_t read_u32_le(
    std::span<const std::uint8_t> bytes,
    std::size_t offset
) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        return 0;
    }

    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

bool range_is_inside_rom(
    std::span<const std::uint8_t> bytes,
    std::uint32_t offset,
    std::uint32_t length,
    std::uint32_t minimum_offset
) {
    if (offset < minimum_offset || length == 0) {
        return false;
    }

    const auto start = static_cast<std::uint64_t>(offset);
    const auto size = static_cast<std::uint64_t>(length);
    return start <= bytes.size() && size <= bytes.size() - start;
}

std::optional<Candidate> detect_game_boy(
    std::span<const std::uint8_t> bytes
) {
    if (bytes.size() < 0x150) {
        return std::nullopt;
    }

    const bool logo_valid = std::equal(
        kGameBoyLogo.begin(),
        kGameBoyLogo.end(),
        bytes.begin() + 0x104
    );

    std::uint8_t calculated_checksum = 0;
    for (std::size_t i = 0x134; i <= 0x14C; ++i) {
        calculated_checksum = static_cast<std::uint8_t>(
            calculated_checksum - bytes[i] - 1U
        );
    }

    const bool checksum_valid = calculated_checksum == bytes[0x14D];
    if (!logo_valid && !checksum_valid) {
        return std::nullopt;
    }

    Candidate candidate;
    const auto cgb_flag = bytes[0x143];
    const bool cgb_flag_valid = cgb_flag == 0x80 || cgb_flag == 0xC0;
    candidate.platform = cgb_flag_valid ? Platform::GameBoyColor : Platform::GameBoy;
    candidate.title = extract_text(bytes, 0x134, 16);
    candidate.score = 1 + (logo_valid ? 3 : 0) +
                      (checksum_valid ? 2 : 0) +
                      (!candidate.title.empty() ? 1 : 0) +
                      (cgb_flag_valid ? 1 : 0);

    if (!logo_valid) {
        candidate.warnings.emplace_back(
            "La firma gráfica de Nintendo de la cabecera GB no coincide."
        );
    }
    if (!checksum_valid) {
        candidate.warnings.emplace_back(
            "La suma de comprobación de la cabecera GB no coincide."
        );
    }
    if (cgb_flag != 0x00 && !cgb_flag_valid) {
        candidate.warnings.emplace_back(
            "La marca de compatibilidad GB/GBC contiene un valor poco habitual."
        );
    }

    return candidate;
}

std::optional<Candidate> detect_game_boy_advance(
    std::span<const std::uint8_t> bytes
) {
    if (bytes.size() < 0xC0 || bytes[0xB2] != 0x96) {
        return std::nullopt;
    }

    Candidate candidate;
    candidate.platform = Platform::GameBoyAdvance;
    candidate.title = extract_text(bytes, 0xA0, 12);
    candidate.game_code = extract_text(bytes, 0xAC, 4);
    candidate.maker_code = extract_text(bytes, 0xB0, 2);
    candidate.version = bytes[0xBC];

    std::uint32_t sum = 0;
    for (std::size_t i = 0xA0; i <= 0xBC; ++i) {
        sum += bytes[i];
    }
    const auto calculated_checksum = static_cast<std::uint8_t>(
        (0x19U - sum) & 0xFFU
    );
    const bool checksum_valid = calculated_checksum == bytes[0xBD];
    const bool game_code_valid = is_alphanumeric_code(candidate.game_code, 4);

    if (!game_code_valid && !checksum_valid) {
        return std::nullopt;
    }

    candidate.score = 1 + 3 + (checksum_valid ? 2 : 0) +
                      (game_code_valid ? 1 : 0) +
                      (!candidate.title.empty() ? 1 : 0);

    if (!checksum_valid) {
        candidate.warnings.emplace_back(
            "La suma de comprobación de la cabecera GBA no coincide."
        );
    }
    if (candidate.title.empty()) {
        candidate.warnings.emplace_back(
            "La cabecera GBA no contiene un título legible."
        );
    }

    return candidate;
}

std::optional<Candidate> detect_nintendo_ds(
    std::span<const std::uint8_t> bytes
) {
    if (bytes.size() < 0x200) {
        return std::nullopt;
    }

    const auto arm9_offset = read_u32_le(bytes, 0x20);
    const auto arm9_size = read_u32_le(bytes, 0x2C);
    const auto arm7_offset = read_u32_le(bytes, 0x30);
    const auto arm7_size = read_u32_le(bytes, 0x3C);

    const bool arm9_valid = range_is_inside_rom(bytes, arm9_offset, arm9_size, 0x200);
    const bool arm7_valid = range_is_inside_rom(bytes, arm7_offset, arm7_size, 0x200);
    if (!arm9_valid || !arm7_valid) {
        return std::nullopt;
    }

    Candidate candidate;
    candidate.platform = Platform::NintendoDS;
    candidate.title = extract_text(bytes, 0x00, 12);
    candidate.game_code = extract_text(bytes, 0x0C, 4);
    candidate.maker_code = extract_text(bytes, 0x10, 2);
    candidate.version = bytes[0x1E];

    const bool game_code_valid = is_alphanumeric_code(candidate.game_code, 4);
    if (!game_code_valid) {
        return std::nullopt;
    }

    candidate.score = 1 + 2 + 2 + 1 +
                      (!candidate.title.empty() ? 1 : 0);

    if (candidate.title.empty()) {
        candidate.warnings.emplace_back(
            "La cabecera Nintendo DS no contiene un título legible."
        );
    }

    return candidate;
}

} // namespace

std::string to_string(Platform platform) {
    switch (platform) {
    case Platform::GameBoy:
        return "Game Boy";
    case Platform::GameBoyColor:
        return "Game Boy Color";
    case Platform::GameBoyAdvance:
        return "Game Boy Advance";
    case Platform::NintendoDS:
        return "Nintendo DS";
    case Platform::Unknown:
    default:
        return "Desconocida";
    }
}

InspectionResult inspect_rom_bytes(
    std::span<const std::uint8_t> bytes,
    std::string source_name
) {
    InspectionResult result;

    if (bytes.empty()) {
        result.error = "El archivo está vacío.";
        return result;
    }

    std::vector<Candidate> candidates;
    if (auto candidate = detect_game_boy(bytes)) {
        candidates.push_back(std::move(*candidate));
    }
    if (auto candidate = detect_game_boy_advance(bytes)) {
        candidates.push_back(std::move(*candidate));
    }
    if (auto candidate = detect_nintendo_ds(bytes)) {
        candidates.push_back(std::move(*candidate));
    }

    if (candidates.empty()) {
        result.error =
            "No se reconoce una cabecera válida de GB, GBC, GBA o Nintendo DS.";
        return result;
    }

    const auto best = std::max_element(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& left, const Candidate& right) {
            return left.score < right.score;
        }
    );

    result.success = true;
    result.info.platform = best->platform;
    result.info.source = std::move(source_name);
    result.info.file_size = static_cast<std::uint64_t>(bytes.size());
    result.info.title = best->title;
    result.info.game_code = best->game_code;
    result.info.maker_code = best->maker_code;
    result.info.version = best->version;
    result.info.detection_score = best->score;
    result.info.warnings = best->warnings;

    if (candidates.size() > 1) {
        result.info.warnings.emplace_back(
            "Más de una cabecera parece plausible; se eligió la de mayor puntuación."
        );
    }

    return result;
}

InspectionResult inspect_rom(const std::filesystem::path& path) {
    InspectionResult result;
    std::error_code filesystem_error;

    if (!std::filesystem::exists(path, filesystem_error) || filesystem_error) {
        result.error = "No se encuentra el archivo: " + path.string();
        return result;
    }

    const auto file_size = std::filesystem::file_size(path, filesystem_error);
    if (filesystem_error) {
        result.error = "No se pudo consultar el tamaño del archivo: " + path.string();
        return result;
    }
    if (file_size == 0) {
        result.error = "El archivo está vacío.";
        return result;
    }
    if (file_size > kMaximumRomSize) {
        result.error = "El archivo supera el límite de seguridad de 512 MiB.";
        return result;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        result.error = "No se pudo abrir el archivo: " + path.string();
        return result;
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file_size));
    input.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );

    if (!input || static_cast<std::uint64_t>(input.gcount()) != file_size) {
        result.error = "No se pudo leer el archivo completo: " + path.string();
        return result;
    }

    return inspect_rom_bytes(bytes, path.string());
}

} // namespace retroport
