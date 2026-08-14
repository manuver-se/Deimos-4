# Deimos-4 | Analog Drum Machine & Sequencer

Welcome to the official repository for the **Deimos-4**, a DIY, analog 4-voice drum machine with step-sequencer powered by an Arduino Nano.

Designed for both live jamming and having lots of fun, the Deimos-4 combines classic analog circuits based on the original 606 and 808 circuits, an intuitive tactile interface, custom LED visual feedback, and hardware sync out.

---

## Overview & Features

* **4 Analog Drum Voices:** Dedicated circuits for Kick, Snare, Closed Hi-Hat (CHH), and Open Hi-Hat (OHH) with choke logic for the hihats. The decay and pitch for kick and snappy and pitch for snare are changeable. 
* **16-Step Sequencer:** Divided across 4 pages (4 steps per page) with per-step pattern memory. There are controls for tempo, swing and sequencer length.
* **Dual Operating Modes:**
  * **Live Mode:** Finger-drum live or over a running background sequence.
  * **Sequencer Mode:** Full step-editing control per voice and per page.
* **EEPROM memory:** Automatic pattern saving using the internal EEPROM memory from the Arduino Nano.
* **Sync Out:** Hardware jumper configurable for **Opto-Isolated S-Trig** (short-to-ground triggers used for some vintage synths) or **Direct 5V V-Trig** (for most modern gear, eurorack).

---

## Test Points & Expansion Header

For DIY builders, modders, and audio hackers, the Deimos-4 PCB includes dedicated **Test Points** for diagnostics as well as an onboard **Expansion Breakout Header** for custom hardware modifications.

### Onboard Test Points
* **Noise Generators:** Dedicated test/tap points for both **White Noise (noice noize)** and **Metallic Noise (Beehive(ing)_Noizzze)** (used for snare snappy and hihat circuits respectively) 
* **Tom Voice Circuits:** Test/tap point for the snare without snappy
* **Voltage Rails:** Testpoints for 9V, digital 5V, 4.5V and GND rails
* **Individual Voice Tap Points:** Test/tap points for the raw audio signal of each drum voice (Kick, Snare, CHH, OHH) before it reaches the master summing stage.

### Hardware Expansion Connector
The PCB design also features an 8 pin header designed for hardware expansions and modifications. This connector features:
* **Separate outs:** Tap each drum sound individually to route them to dedicated external audio jacks, outboard effects, or separate mixer channels.
* **Power Rails:** Exposes internal power rails (9V, 4.5V and GND) to power expansion boards, active filters, or external mod circuits.
* **Master Out & To Jack Input:** Access to master out and jack input. These two pins are connected by default.

## Manual & Documentation

For detailed information on operating instructions, control layouts, feature guides, and hardware setup, please refer to the temporary user manual included in the repository. For now there only is a version Dutch, but the English version is coming soon.

* **[Handleiding-ned.pdf]** 

---

## Terms of Use & Licensing

Thank you for your interest in the Deimos-4 project! Please take a moment to review the guidelines regarding the hardware, software, and documentation shared in this repository:

* **Firmware & Documentation:** All source code, schematics, and documentation provided here are free for personal, educational, and non-commercial use. You are welcome to study, modify, and flash the code for your own personal DIY builds.
* **PCB Design Files:** Please note that the KiCad PCB layout files and Gerbers are **not included** in this repository and will not be released publicly. The Deimos-4 hardware design is proprietary and will be manufactured and sold as a commercial DIY kit in the future.
* **Commercial Restriction:** You may **not** use the documentation, code, or hardware schematics from this repository for any commercial purposes, product manufacturing, or resale without explicit permission.
