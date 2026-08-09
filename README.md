# Maple RailRoad Sound Decoder

WiFi controller, sound decoder, and sound projects for model trains.

Project includes library for programming the sounds with set of state machines.

There are several ready-to-use sample sound projects.

![Web interface for locomotive operation](/docs/interface.png)

![Render of the PCB](/docs/sounddecoder-esp32-s3-wroom-2-ext.png)

# Project structure

* esp32 - directory where building of the firmware starts
* hardware - several versions of PCBs for manufacturing the decoder
* projects - ready-to-use sound projects for different locomotives
* compiler - compiler for the sound projects
* emulator - cab emulator for listening sounds with virtual locomotive control
* sample - dummy player which generates wav file by interpreting the sound project
* other directories include the source code of the firmware

# Bringing locomotive to life

* Get the hardware (order PCB, solder the components)
* Build firmware
  * Install ESP32 toolkit (esp idf)
  * Run esp32/build32M.sh (or other version)
* Upload esp32/build32M/esp32sounddecoder.bin into the ESP32 module
* Build selected sound project with compiler/compiler.py
* Upload built project through the web interface into the controller
* Install the controller into the locomotive

# Software features

Controller provides web interface that may be accessed through
WiFi (default address 192.168.4.1).

Sound module plays 8- or 16-bit sounds in 8 channels.

Output functions and sound schemes are programmed with
custom language. The scripts may include conditions,
loops, and finite state machines. These features
may be used for creating complex sound projects.

# Hardware

Controller is intended to be used with G-scale locomotives
with lithium batteries installed, but it may also be used
with track power (some power capacitors are probably needed).

Controller includes the following hardware components:
* ESP32 CPU with embedded WiFi module and flash memory
* Motor driver
* I2S audio power amplifier (MAX98357A)
* 7 programmable outputs
* BEMF for adjusting the motor speed (work in progress)

# Links

* Radio-controlled engines with sound
  - https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32
* Cost effective DIY DCC Decoder for model railroad locomotives
  - https://github.com/gab-k/RP2040-Decoder
* NIMRS: open-source NIMRS-21Pin-Decoder, based on the ESP32-S3 microcontroller
  - https://github.com/CDFER/NIMRS-21Pin-Decoder
  - https://github.com/ProjectInitiative/NIMRS-Firmware
