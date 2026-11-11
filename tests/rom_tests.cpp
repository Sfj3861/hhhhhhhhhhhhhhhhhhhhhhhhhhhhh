#include "retroport/rom.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "FALLO: " << description << '\n';
        ++failures;
    }
}

void write_u32_le(std::vector<std::uint8_t>& bytes, std::size_t offset,
                  std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::vector<std::uint8_t> make_game_boy_rom(std::uint8_t cgb_flag) {
    constexpr std::array<std::uint8_t, 48> logo{
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    };

    std::vector<std::uint8_t> bytes(0x8000, 0);
    for (std::size_t i = 0; i < logo.size(); ++i) {
        bytes[0x104 + i] = logo[i];
    }

    const std::string title = "RETRO TEST";
    for (std::size_t i = 0; i < title.size(); ++i) {
        bytes[0x134 + i] = static_cast<std::uint8_t>(title[i]);
    }
    bytes[0x143] = cgb_flag;
    bytes[0x147] = 0x00;
    bytes[0x148] = 0x00;
    bytes[0x149] = 0x00;

    std::uint8_t checksum = 0;
    for (std::size_t i = 0x134; i <= 0x14C; ++i) {
        checksum = static_cast<std::uint8_t>(checksum - bytes[i] - 1U);
    }
    bytes[0x14D] = checksum;
    return bytes;
}

std::vector<std::uint8_t> make_gba_rom() {
    std::vector<std::uint8_t> bytes(0xC0, 0);
    const std::string title = "RETRO TEST";
    const std::string game_code = "RTP1";
    const std::string maker_code = "01";

    for (std::size_t i = 0; i < title.size(); ++i) {
        bytes[0xA0 + i] = static_cast<std::uint8_t>(title[i]);
    }
    for (std::size_t i = 0; i < game_code.size(); ++i) {
        bytes[0xAC + i] = static_cast<std::uint8_t>(game_code[i]);
    }
    for (std::size_t i = 0; i < maker_code.size(); ++i) {
        bytes[0xB0 + i] = static_cast<std::uint8_t>(maker_code[i]);
    }

    bytes[0xB2] = 0x96;
    bytes[0xBC] = 0x02;

    std::uint32_t sum = 0;
    for (std::size_t i = 0xA0; i <= 0xBC; ++i) {
        sum += bytes[i];
    }
    bytes[0xBD] = static_cast<std::uint8_t>((0x19U - sum) & 0xFFU);
    return bytes;
}

std::vector<std::uint8_t> make_ds_rom() {
    std::vector<std::uint8_t> bytes(0x400, 0);
    const std::string title = "RETRO TEST";
    const std::string game_code = "RTP1";
    const std::string maker_code = "01";

    for (std::size_t i = 0; i < title.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(title[i]);
    }
    for (std::size_t i = 0; i < game_code.size(); ++i) {
        bytes[0x0C + i] = static_cast<std::uint8_t>(game_code[i]);
    }
    for (std::size_t i = 0; i < maker_code.size(); ++i) {
        bytes[0x10 + i] = static_cast<std::uint8_t>(maker_code[i]);
    }

    write_u32_le(bytes, 0x20, 0x200);
    write_u32_le(bytes, 0x2C, 0x100);
    write_u32_le(bytes, 0x30, 0x300);
    write_u32_le(bytes, 0x3C, 0x80);
    bytes[0x1E] = 0x01;
    return bytes;
}

} // namespace

int main() {
    const auto gb = retroport::inspect_rom_bytes(make_game_boy_rom(0x00), "gb-test");
    check(gb.success, "detectar una ROM Game Boy");
    if (gb.success) {
        check(gb.info.platform == retroport::Platform::GameBoy,
              "clasificar Game Boy correctamente");
        check(gb.info.title == "RETRO TEST", "extraer el titulo GB");
    }

    const auto gbc = retroport::inspect_rom_bytes(make_game_boy_rom(0x80), "gbc-test");
    check(gbc.success, "detectar una ROM Game Boy Color");
    if (gbc.success) {
        check(gbc.info.platform == retroport::Platform::GameBoyColor,
              "clasificar Game Boy Color correctamente");
    }

    const auto gba = retroport::inspect_rom_bytes(make_gba_rom(), "gba-test");
    check(gba.success, "detectar una ROM Game Boy Advance");
    if (gba.success) {
        check(gba.info.platform == retroport::Platform::GameBoyAdvance,
              "clasificar GBA correctamente");
        check(gba.info.game_code == "RTP1", "extraer el game code GBA");
        check(gba.info.version == 0x02, "extraer la version GBA");
    }

    const auto ds = retroport::inspect_rom_bytes(make_ds_rom(), "ds-test");
    check(ds.success, "detectar una ROM Nintendo DS");
    if (ds.success) {
        check(ds.info.platform == retroport::Platform::NintendoDS,
              "clasificar Nintendo DS correctamente");
        check(ds.info.game_code == "RTP1", "extraer el game code DS");
    }

    const std::vector<std::uint8_t> unknown(0x200, 0);
    const auto invalid = retroport::inspect_rom_bytes(unknown, "unknown-test");
    check(!invalid.success, "rechazar una ROM sin cabecera reconocible");

    const std::vector<std::uint8_t> empty;
    const auto empty_result = retroport::inspect_rom_bytes(empty, "empty-test");
    check(!empty_result.success, "rechazar una ROM vacia");

    if (failures != 0) {
        std::cerr << failures << " prueba(s) fallida(s).\n";
        return 1;
    }

    std::cout << "Todas las pruebas de RetroPort han pasado.\n";
    return 0;
}
