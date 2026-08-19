# Deimos-4 | Analog, DIY rhythm machine

Welcome to the official repository for the **Deimos-4**, a DIY, analog 4-voice drum machine with step-sequencer powered by an Arduino Nano.

Designed for both live jamming and having lots of fun, the Deimos-4 combines classic analog circuitry based on the original 606 and 808 circuits, a 16-step sequencer, an intuitive interface, basic sync out connectivity, and an expansion header for future expansions.

---

## Overview & Features

* **4 analog drum voices:** Dedicated circuits for Kick, Snare, Closed Hi-Hat (CHH), and Open Hi-Hat (OHH) with choke logic for the hihats. The decay and pitch for kick and snappy and pitch for snare are changeable. The levels of each individual drum voice can also be changed with trimpots.
* **16-step sequencer:** Divided across 4 pages (4 steps per page) with per-step pattern memory. There are controls for tempo, swing and sequencer length. Individual drum voices are mutable while the sequence is running.
* **Dual operating modes:**
  * **Live mode:** Finger-drum live or over a running background sequence.
  * **Sequencer mode:** Full step-editing control per voice and per step.
* **EEPROM memory:** Automatic pattern saving using the internal EEPROM memory from the Arduino Nano.
* **Sync out:** Jumper configurable sync out with **opto-isolated S-trig** (short-to-ground triggers used for some vintage synths) or **direct 5V V-trig** (for most modern gear, eurorack).

---

## Test points & Expansion header

For DIY builders, modders, and audio hackers, the Deimos-4 PCB includes dedicated test points for diagnostics as well as an onboard expansion header for custom hardware modifications.

### Onboard test points
* **Noise generators:** Dedicated test/tap points for both **White Noise (noice noize)** and **Metallic Noise (Beehive(ing)_Noizzze)** (used for snare snappy and hihat circuits respectively) 
* **Hidden tom circuit:** Test/tap point for the snare without snappy
* **Voltage rails:** Testpoints for 9V, digital 5V, 4.5V and GND rails
* **Individual voice tap points:** Test/tap points for the raw audio signal of each drum voice (Kick, Snare, CHH, OHH) before it reaches the master summing stage.
* **Triggers** 4 testpoints for triggering drum voices: Kick trigger (BD_TR), Snare trigger (SN_TR), Hihat trigger (to trigger open hihat; HH_TR) and a gate testpoint (HH_GT) that together with the Hihat trigger, triggers the closed hihat.

### Hardware expansion header
The PCB design also features an 8 pin header designed for hardware expansions and modifications. This connector includes:
* **Separate outs:** Tap each drum voice individually to route them to dedicated external audio jacks, outboard effects, or separate mixer channels.
* **Power rails:** Exposes internal power rails (9V, 4.5V and GND) to power future expansion boards or external mod circuits.
* **Master out & To jack input:** Access to master out and jack input. These two pins should be connected by default.

## Manual & Documentation

For detailed information on drummachine history, kit assembly or operating manual, please refer to the temporary user manual included in the repository. For now there only is a version Dutch, but the English version is coming soon.

* **[Handleiding-ned.pdf]** 

---

## Terms of use & Licensing

If you made it until here, I thank you for your interest in the Deimos-4 drummachine project! Please take a moment to review the guidelines regarding the hardware, software, and documentation shared in this repository:

* **Firmware & Documentation:** All source code, schematics, and documentation provided here are free for personal, educational, and non-commercial use. You are welcome to study, modify, and flash the code for your own personal DIY builds.
* **PCB design files:** Please note that the KiCad PCB layout files and Gerbers aren't included in this repository and will not be released publicly. The Deimos-4 hardware design is proprietary and will be manufactured and sold as a commercial DIY kit in the future.
* **Commercial restriction:** You may not use the documentation, code, or hardware schematics from this repository for any commercial purposes, product manufacturing, or resale without explicit permission.
