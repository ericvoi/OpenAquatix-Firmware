# UAM_Firmware
Firmware for the Underwater Acoustic Modem capstone project

# Description
The firmware in this repository is for the underwater acoustic modem found [here](https://github.com/ericvoi/UAM_PCB/tree/main). Both the hardware and the software were developed for a final year engineering capstone project to develop and build an underwater acoustic modem.

# Key Features
- 6 types of error correction (CRC-8, CRC-16, CRC-32, checksum-8, checksum-16, checksum-32)
- FSK and FHBFSK modulation/demodulation schemes
- Feedback networks for both the input and output networks to ensure that the system is calibrated

# Application Overview
The firmware for this project consists of a five-task FreeRTOS application that manages modulation, demodulation, external communication over USB or UART, system monitoring, and storing configuration data.

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
- Act as the sole task in low-power modes (TODO)

## Configuration (CFG)
This task facilitates the storage of all configuration parameters in flash memory. Task functions:
- Load parameters from flash on boot
- Update changed parameters to flash

## DAC (DAC)
This task's only purpose is to fill the DAC DMA buffers when notified by the DMA callback. Task functions:
- Modulating the DAC with DMA to generate an input signal for the power amplifier