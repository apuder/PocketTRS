
# PocketTRS

PocketTRS uses a modern microcontroller to emulate a TRS-80 personal computer. It can be connected to a VGA monitor and PS/2 keyboard for. PocketTRS has TRS-IO embedded for full FreHD support. PocketTRS supports the following features:

1. Emulates a Model III and Model 4
2. Grafyx support
3. Cassette input/game sound
4. Emulates the 50-pin expansion interface
6. Integrated TRS-IO

### Firmware

The heart of PocketTRS is an ESP32-based microcontroller. The firmware can be compiled using Espressif's ESP-IDF version 4.2. First install the
<a href="https://docs.espressif.com/projects/esp-idf/en/release-v4.2/esp32/get-started/index.html#step-1-install-prerequisites">ESP-IDF toolchain</a> for your platform. Next clone the PocketTRS git repository and compile the firmware via:

```
git clone --recursive https://github.com/apuder/PocketTRS.git
cd PocketTRS
idf.py build
```

The firmware can be flashed one the ESP as well as the necessary flashing circuitry is added to the PCB as explained in the next section.

### Hardware

This repository contains the KiCad and Gerber files for PocketTRS. The figure below shows the various connectors:

<img src="doc/ptr.png" width="60%" />

1. VGA connector
2. PS/2 connector
3. 3.5mm audio jack
4. Micro USB power supply
5. FTDI programmer interface
6. ESP reset button
7. Z80 reset button
8. 50-pin expansion interface

The BOM contains all required parts. Soldering some of the surface mounted components can be tricky. It is suggested to build up the PCB in a particular order and test the board along each step: TBD

### Configuration

After booting PocketTRS, the following hotkeys are available via the PS/2 keyboard:

* &lt;F1&gt;  - Enable lowercase
* &lt;F2&gt;  - TRS-80 &lt;CLEAR&gt; key
* &lt;F3&gt;  - PocketTRS Configuration
* &lt;F4&gt;  - Send screenshot to the printer
* &lt;F9&gt;  - Reset Z80
* &lt;ESC&gt; - TRS-80 &lt;BREAK&gt; key

&lt;F3&gt; allows the configuration of various aspects of PocketTRS, including setting up Wifi credentials and TRS-IO. Once Wifi is setup, the printer interface can be accessed via <a href="trs-io.local/printer">trs-io.local/printer</a>.


