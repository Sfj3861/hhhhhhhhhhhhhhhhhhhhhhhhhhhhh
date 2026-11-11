# RetroPort

RetroPort is an open development project for analysing Game Boy, Game Boy Color,
Game Boy Advance and Nintendo DS ROMs, reconstructing useful high-level
representations of their programs, and eventually helping create native ports
for platforms such as the Nintendo Wii.

**RetroPort is not an emulator.** The goal is to analyse program code and assets,
produce documented disassembly and C-like output, and provide a separate
porting layer for platform-specific behaviour.

## Current status: 0.1.0

The first milestone is a small, dependency-free C++20 command-line tool. It can:

- Read a ROM file without modifying it.
- Identify likely GB, GBC, GBA and Nintendo DS images using their headers.
- Extract basic title and game-code metadata where available.
- Report suspicious or incomplete headers.
- Run synthetic header tests through CTest.

This release does not yet disassemble instructions, recover functions, rebuild
original source code, extract all assets, or create Wii executables.

## Build

Requirements: CMake 3.20 or newer and a C++20 compiler (GCC, Clang or recent
MSVC).

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ctest --test-dir build --output-on-failure

## Usage

    retroport --help
    retroport --version
    retroport inspect path/to/your-own-or-authorised-rom.gba

The inspect command also handles GB, GBC and NDS images.

## Roadmap

1. ROM inspection and metadata.
2. SM83, ARM and Thumb disassemblers.
3. Function discovery, control-flow graphs and code/data separation.
4. Platform-neutral IR plus readable pseudocode and C-like output.
5. Symbol recovery, asset extraction, annotations and optional C++ organisation.
6. A Nintendo Wii backend with replacements for original hardware services.

## Important limitations

A ROM does not normally contain the original project files, variable names,
comments or build configuration. Static analysis can recover useful program
structure, but it cannot guarantee restoration of the exact original source.
A working native port also requires replacing the original console's graphics,
audio, input, timing and hardware interfaces.

Only analyse ROM images that you own or are authorised to use. RetroPort does
not provide copyrighted game files.

## Project structure

- include/retroport/ — public C++ API.
- src/ — command-line application and analysis implementation.
- tests/ — dependency-free automated tests.
- docs/ — architecture notes.
