#include "retroport/rom.hpp"

#include <iomanip>
#include <iostream>
#include <string_view>

namespace {

void print_usage() {
    std::cout
        << "RetroPort - ROM analysis toolkit\n\n"
        << "Uso:\n"
        << "  retroport --help\n"
        << "  retroport --version\n"
        << "  retroport inspect <archivo-ROM>\n\n"
        << "Plataformas detectadas en esta version:\n"
        << "  Game Boy, Game Boy Color, Game Boy Advance y Nintendo DS\n";
}

void print_inspection(const retroport::RomInfo& info) {
    std::cout << "Resultado del analisis\n"
              << "----------------------\n"
              << "Archivo: " << info.source << '\n'
              << "Plataforma: " << retroport::to_string(info.platform) << '\n'
              << "Tamano: " << info.file_size << " bytes\n";

    if (!info.title.empty()) {
        std::cout << "Titulo: " << info.title << '\n';
    }
    if (!info.game_code.empty()) {
        std::cout << "Game code: " << info.game_code << '\n';
    }
    if (!info.maker_code.empty()) {
        std::cout << "Maker code: " << info.maker_code << '\n';
    }

    if (info.platform == retroport::Platform::GameBoyAdvance ||
        info.platform == retroport::Platform::NintendoDS) {
        std::cout << "Version de cabecera: 0x"
                  << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned int>(info.version)
                  << std::dec << std::nouppercase << std::setfill(' ')
                  << '\n';
    }

    std::cout << "Puntuacion de deteccion: " << info.detection_score
              << " (heuristica interna, no es un porcentaje)\n";

    if (!info.warnings.empty()) {
        std::cout << "\nAvisos:\n";
        for (const auto& warning : info.warnings) {
            std::cout << "  - " << warning << '\n';
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2) {
        const std::string_view argument(argv[1]);
        if (argument == "--help" || argument == "-h") {
            print_usage();
            return 0;
        }
        if (argument == "--version") {
            std::cout << "RetroPort 0.1.0\n";
            return 0;
        }
    }

    if (argc != 3 || std::string_view(argv[1]) != "inspect") {
        print_usage();
        return 2;
    }

    const auto result = retroport::inspect_rom(argv[2]);
    if (!result.success) {
        std::cerr << "Error: " << result.error << '\n';
        return 1;
    }

    print_inspection(result.info);
    return 0;
}
