## License

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenAquatix Firmware
Firmware for OpenAquatix: a JANUS compatible software defined underwater acoustic modem for research purposes

## Description
The firmware in this repository is for the underwater acoustic modem found [here](https://github.com/ericvoi/OpenAquatix-Hardware/tree/main). OpenAquatix is intended as a modem used for both research and educational purposes, with a rich set of features and customizability as a core design principle. Contributions are welcome, and the lead developer can be reached at <ericvoisin1@gmail.com> for any inquiries, feature requests, feedback, etc.

## Key Features
- Customizable packet preambles and cargo
- Two-way ranging support with +/- 0.1m precision
- FSK and FHBFSK modulation/demodulation schemes
- Fully JANUS compliant
- 2 ECC options and 6 error detection options applied individually to cargo and preambles
- Integrated MAC layer with GA CSMA with CA via BEB and interface for others
- Integrated power, temperature, and pressure monitoring
- Comprehensive error management
- Text-based HMI for parameter configuration and message sending
- On-the-go updateable parameters that get saved to flash for long-term use
- HMI available over USB or UART for daughter/carrier boards to translate into a protocol of your choice
- Feedback networks for both the input and output networks to ensure that the system is calibrated
- Integration with HIL simulator [OpenCREST](https://github.com/ericvoi/OpenCREST) for transducer-drive simulations of complex channels and network topologies

## Building

### Requirements
- [CMake 3.20+](https://cmake.org/download/)
- [Ninja](https://ninja-build.org/)
- [Arm GNU Toolchain 15.2.rel1](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
  - Select: `arm-none-eabi`

### Build

Set the toolchain path (once, or add to your system PATH):
```cmd
setx OPENAQUATIX_TOOLCHAIN_DIR "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/15.2 rel1/bin"
```

Configure and build:
```cmd
cmake --preset debug
cmake --build --preset debug
```

Output files will be in `build/`:
- `UAM.elf` — debug/flash with GDB
- `UAM.bin` — raw binary flashed over USB

If using VS Code as the IDE, press f5 to compile and debug after installing the Cortex-Debug extension. Additional key combinations can be used to build, flash (no debug), and build + flash.

## Third-Party Libraries

| Library | License | Version/Commit |
|---------|---------|----------------|
| TinyUSB | MIT | v0.20.0 |
| STM32 HAL | BSD-3 | v1.11.5 |
| CMSIS RTOS | Apache-2.0 | v2.1.0 |
| FreeRTOS Kernel | MIT | v10.6.2 |
| CMSIS DSP | Apache-2.0 | v1.6.0 |

## Citations

If you use this code, please cite
```bibtex
@inproceedings{openaquatix,
  author={Voisin, Eric and Cockrall, Cameron and Huang, Letian and Wang, Zhaohui and Elezzabi, Abdulhakem},
  booktitle={OCEANS 2025 - Great Lakes}, 
  title={Development of a Low-Cost Reconfigurable Underwater Acoustic Modem for AUV Applications}, 
  year={2025},
  volume={},
  number={},
  pages={1-8},
  keywords={Water;Protocols;Frequency shift keying;Lakes;Forward error correction;Modems;Reliability;Underwater acoustics;Standards;Wireless fidelity},
  doi={10.23919/OCEANS59106.2025.11245092}}
```
