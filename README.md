# Ehrenpreis — electronic chime module

**A compact, programmable audio module for embedded projects, built around the NXP LPC802 microcontroller.**

Ehrenpreis explores a practical middle ground between a simple piezo buzzer and a full digital audio subsystem. It provides embedded projects with distinctive, programmable sound effects without a single board computer, an I²S audio interface or a dedicated high-performance audio processor.

The design uses a low-cost microcontroller to generate audio through PWM. A passive second-order RC low-pass filter reconstructs the signal, which is then AC-coupled and amplified by a dedicated audio amplifier. The firmware is written in C, with a lookup-table-based waveform generator and configurable sound envelopes.

The hardware and firmware are developed as an open project.

The repository is organised into two parts:

- hardware/ – Hardware design files for the electronic chime module.
- firmware/ – Embedded firmware for the LPC802 microcontroller.

A blog post on wedge.at documents the development process and the decisions behind the project.

License: BSD 3-Clause. See LICENSE for details.
