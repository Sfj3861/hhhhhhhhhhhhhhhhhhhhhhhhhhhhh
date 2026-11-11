# RetroPort architecture

## Design boundary

RetroPort analyses executable data and creates representations suitable for
human review and later porting. It does not execute the original game and is
not intended to be an emulator.

The pipeline is designed as separate stages:

1. Container and header recognition.
2. Platform-specific memory-map and executable-region discovery.
3. Instruction decoding.
4. Control-flow and code/data analysis.
5. Platform-neutral intermediate representation.
6. Pseudocode and C-like code generation.
7. Asset extraction and hardware abstraction.
8. A separate target backend, initially planned for Nintendo Wii.

Each stage should expose testable input/output structures. Platform-specific
decoding must not leak into the platform-neutral representation.

## Supported source architectures

| System | Main CPU family | Initial decoder target |
| --- | --- | --- |
| Game Boy | Sharp SM83 | SM83 instruction decoder |
| Game Boy Color | Sharp SM83 | SM83 instruction decoder plus banking context |
| Game Boy Advance | ARM7TDMI | ARM and Thumb decoder |
| Nintendo DS | ARM946E-S / ARM7TDMI | ARM9 and ARM7 decoders |

The Nintendo DS loader will eventually need to understand the NDS header,
ARM9 and ARM7 binaries, overlays, and the Nitro filesystem tables.

## Current implementation

Version 0.1.0 contains only the safe file reader and initial header recognisers.
The detector uses several structural signals and checksum checks. Its score is
an internal ranking heuristic, not a statistical confidence percentage.

A detected header does not prove that every byte in the image is valid or that
the ROM is unmodified. Patched, homebrew, overdumped or unusual images may
produce warnings or require future detector improvements.

## Intermediate representation

The planned IR should retain:

- Source platform and address-space identity.
- Original address and instruction width.
- Decoded operation and typed operands.
- Control-flow edges and references.
- Memory-mapped I/O annotations.
- Confidence and provenance for inferred functions and data.

The IR must distinguish facts decoded directly from the binary from guesses
made by static analysis. This is essential because data may resemble code and
not every function boundary can be recovered automatically.

## Wii porting boundary

The Wii backend is a later, separate component. It will compile generated or
reconstructed C/C++ using a Wii-compatible toolchain and provide replacements
for source-platform services such as graphics, audio, input, timers and save
data. Reconstructing instructions alone does not produce a working native Wii
port.
