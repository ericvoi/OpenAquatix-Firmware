# License

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# OpenAquatix Firmware
Firmware for the OpenAquatix: a JANUS compatible software defined underwater acoustic modem for research purposes

# Description
The firmware in this repository is for the underwater acoustic modem found [here](https://github.com/ericvoi/UAM_PCB/tree/main).

# Key Features
- 6 types of error correction (CRC-8, CRC-16, CRC-32, checksum-8, checksum-16, checksum-32)
- FSK and FHBFSK modulation/demodulation schemes
- K=9 convolutional code
- Fully JANUS compliant
- Feedback networks for both the input and output networks to ensure that the system is calibrated

# Application Overview
The firmware for this project consists of a six-task FreeRTOS application that manages modulation, demodulation, external communication over USB or UART, system monitoring, medium access control, and storing configuration data.

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
- Track errors (TODO)
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

# Third-Party Libraries

| Library | License | Version/Commit |
|---------|---------|----------------|
| TinyUSB | MIT | v0.20.0 |
| STM32 HAL | BSD-3 | v1.11.5 |
| CMSIS RTOS | Apache-2.0 | v2.1.0 |
| FreeRTOS Kernel | MIT | v10.3.1 |
| CMSIS DSP | Apache-2.0 | v1.6.0 |
