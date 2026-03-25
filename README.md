# License

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenAquatix Firmware
Firmware for OpenAquatix: a JANUS compatible software defined underwater acoustic modem for research purposes

# Description
The firmware in this repository is for the underwater acoustic modem found [here](https://github.com/ericvoi/OpenAquatix-Hardware/tree/main).

# Key Features
- 6 types of error correction (CRC-8, CRC-16, CRC-32, checksum-8, checksum-16, checksum-32)
- FSK and FHBFSK modulation/demodulation schemes
- K=9 convolutional code
- Fully JANUS compliant
- Feedback networks for both the input and output networks to ensure that the system is calibrated

# Application Overview
The firmware for this project consists of a seven-task FreeRTOS application that manages modulation, demodulation, external communication over USB or UART, system monitoring, medium access control, and storing configuration data.

## Message Processing (MESS)
This task handles all of the signal processing for both the input and output as well as handling the feedback networks which ensure calibration of the device. Task functions:
- Listening to the input ADC to determine when a message starts
- Decoding received messages
- Preparing packets with a sender id, message type, and message length
- Adding error correction to packets and determining if errors occurred during demodulation
- Printing raw data over USB
- Calibrating the input hardware and the output hardware to ensure responsivity over frequency (TODO)
- Sending received messages to the COMM task

## Communication (COMM)
This task serves as the communication link for users and hosts a HMI over USB and UART. Task functions:
- Hosting a HMI that lets the user change internal parameters, invoke functions, and view parameters
- Printing received messages
- Outputting HMI over USB and UART and listening for commands from either interface

## System (SYS)
This task is the central task and its primary purpose is to ensure that the system is operating as expected. Task functions:
- Track power consumption (TODO)
- Track temperature
- Track errors
- Check misc input GPIO pins (TODO)
- Update status LED according to system state
- Determine overall system state and relay that to other tasks (TODO)
- Act as the sole task in low-power modes

## Configuration (CFG)
This task facilitates the storage of all configuration parameters in flash memory. Task functions:
- Load parameters from flash on boot
- Update changed parameters to flash

## Medium Access Control (MAC)
This task facilitates access to the transmission medium with either no MAC method or CSMA/CA with BEB:
- Processes channel reports to determine the background noise level
- Forwards received messages to the COMM task
- Immediately forwards emergency messages
- Sends messages to send to the MESS task if medium is available

## DAC (DAC)
This task's only purpose is to fill the DAC DMA buffers when notified by the DMA callback. Task functions:
- Modulating the DAC with DMA to generate an input signal for the power amplifier

## Filter (FILT)
Filters raw ADC data, converts to floating point, and removes the DC offset for further processing. Task functions
- Removing DC offset from raw ADC data
- Converting fixed point values to floating point and normalizing
- Managing input and output DMA streams for FMAC (TODO)

## Building

### Requirements
- [CMake 3.20+](https://cmake.org/download/)
- [Ninja](https://ninja-build.org/)
- [Arm GNU Toolchain 15.2.rel1](https://developer.arm.com/downloads/-arm-gnu-toolchain-downloads)
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
