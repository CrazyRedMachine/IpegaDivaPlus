# IpegaDivaPlus

Turn your Ipega Diva controller into a mini HORI ASC

## Acknowledgments

This project uses [pico_i2c_sniffer by Juan Schiavoni](https://github.com/jjsch-dev/pico_i2c_sniffer)

Keyboard/gamepad USB code is based off [Pico Game Controller by Speedy Potato](https://github.com/speedypotato/Pico-Game-Controller)

## About

This project is designed for the Ipega Diva mini controller (sometimes called "Future Tone DX Mini Controller", "PEGA Angel", "hatsune ANGEL", and various other names... )

Model number is PG-SW056 (nintendo switch version with XYBA buttons) or PG-P4016 (PS4 version with triangle/square/cross/circle buttons).

While it is a very cute controller, the slider behavior is a huge letdown, the "normal/arcade" mode switch gave me false hopes that it could behave like the HORI Diva controller but it doesn't.

**This turns the ipega controller into what we all thought it was at first, a cool little controller**

(note that **you lose the analog sticks** from the controller but you gain so much more instead :))

## Foreword & disclaimer

This project requires a bit of soldering on the controller itself, and while most of the soldering is done on test pads and doesn't require high soldering skill,
I'd still advise against doing this as your first ever soldering project. Please buy practice boards or practice on electronic boards you're throwing away anyways.

**YOU MIGHT BREAK YOUR CONTROLLER IF YOU MESS UP**

## Features

### Firmware update

- **hold HOME+CIRCLE (home+A on PG-SW056) while plugging controller to put the controller in bootloader mode**

- drag&drop IpegaDivaPlus.uf2 to the RP2040 drive (either compile it yourself [or download it from the release page](https://github.com/CrazyRedMachine/IpegaDivaPlus/releases/latest))

### Mod Compatibility

My controller is the PG-P4016 (PS4) model, so that's what most of the guide is based on, but the mod has also been tested on the PG-SW056 (switch) version.

I provide extra diagrams for this model as well as an appendix to the guide.

### Controller compatibility

The controller has been tested working on:
- Chunithm and Project Diva AFT (using [redboard dlls](https://github.com/CrazyRedMachine/RedBoard/releases/latest))
- Project Diva Megamix (on Switch AND on Switch 2)
- Project Diva Megamix+ (on PC)

**Note:** In PC Megamix+ you need to remap button inputs in the settings, because it doesn't follow the XYBA button order.. this is not a firmware bug,
this is the expected behavior with switch diva controllers...

### Keyboard and Gamepad modes

The normal/arcade switch acts as a keyboard/gamepad mode switch (it can be changed on-the-fly)

- Keyboard mode is more of a PoC and was only done to quickly test the controller at first (TODO: adapt to umiguri mapping?)

- Gamepad mode acts like the HORI controller, with touch data encoded on the analog axis values

### Slider mapping

Ipega touch slider is comprised of 18 zones whereas the Project Diva arcade panel has 32. Therefore the mapping is as follows:

| Arcade | 1 | 2 | 3 4 | 5 6 | 7 8 | 9 10 | 11 12 | 13 14 | 15 16 | 17 18 | 19 20 | 21 22 | 23 24 | 25 26 | 27 28 | 29 30 | 31 | 32 |
| ------ |:-:|:-:|:---:|:---:|:---:|:----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:--:|:--:|
| Ipega  | a | b | c   | d   | e   | f    | g     | h     | i     | j     | k     | l     | m     | n     | o     | p     | q  | r  |

## GUIDE

### Flash the firmware

Before anything else, start by flashing the firmware by either recompiling it **[or download it from the release page](https://github.com/CrazyRedMachine/IpegaDivaPlus/releases/latest)**

### Naming scheme

The controller consists of 7 different pcb as shown on this picture, we'll focus on 6 of them.

![boards](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/boards.png?raw=true)
![pcb_name](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/pcb_name.png?raw=true)

### General principle

Main board powers the Touch board and communicates with it over I2C protocol.

The touch board itself has two separate chips which communicate over I2C as well.

Unfortunately, the easily accessible line between main and slider touch contains nothing but the analog axis final values.

Rather than communicating directly with the touch board, the trick here is to let the main board deal with initialization and led modes,
and just sniff the internal touch board communication to extract full slider data in real time.

### USB

![usb](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/usb.png?raw=true)

The first thing to deal with is USB connection. As you can see, USB1 is what handles the power and data lines,
and should be redirected to our rpi pico instead.

We still need to power the main board therefore we are going to use VBUS and GND pins for this purpose.

### USB: Dock connector for Nintendo Switch

With our usb wiring, the USB2 board is now disconnected. We are going to restore functionality and also, as an added bonus,
make it work with your nintendo switch if needed (it originally only had power lines going through it, now data lines will work too)

![dock](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/dock.png?raw=true)

### I2C

This is the core part, soldering on the internal i2c pads.

**Note:** Rather than the pads, you can locate the resistors R8 and R9, it's easier to solder on them directly.

Make sure to put something insulating between the rpi pico and the touch board, you don't want them in direct contact
 
![i2c](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/i2c.png?raw=true)

### Buttons

#### Wiring diagram

Here's how to wire every other button on your pico so that you retain (almost) full controller functionality

![buttons](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/buttons.png?raw=true)

#### MAIN board

The dome buttons are directly wired on the main board so you'll have to solder on the test pads. Make sure to use the **top pad**,
with an X/Y/B/A label nearby. The bottom one is the common ground, shared by all buttons so that won't work.

#### BUTTON board

The button board is wired to the main board using a ZIF 0.5mm pitch 10-pin connector.

You can either solder to the test pads on the main board, or use a ZIF breakout board.

**MAKE SURE IT'S 0.5mm PITCH AND 10 PIN**

![10pin](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/10pin.png?raw=true)

If you take one with dupont header soldered on (as on this picture), make sure to take right angled ones

**Note:** Similarly to the button domes, if you want to retain the "LED" button functionality, you'll have to solder on the test pad 
labeled "CLEAR" directly on the main board

#### DPAD board

Similar principle, but it uses a 12-pin connector. I also recommend using a ZIF breakout board.

**MAKE SURE IT'S 0.5mm PITCH AND 12 PIN**

![12pin](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/12pin.png?raw=true)

The analog sticks are not wired anywhere. The rpi pico has only 3 exposed ADC pins so it's not 
possible to interface the 2 analog sticks (4 axis in total). Also the firmware code is already running on a very tight loop 
in order not to drop i2c packets, so I'm not even sure everything would still work well if I added analog reads.

### APPENDIX: Switch version

On the whole the switch version controller is very similar, just the USB boards are not the same,
and some connectors have changed too.

#### USB

USB is slightly different as here the "dock" connector is the main one.

![swusb](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/swusb.png?raw=true)

#### I2C

There's foam under the touch slider so you'll have to lift it in order to reveal the pads

#### Buttons

The "clear" pad is not at the same place but it still retains the same label so no biggie :)

![swbuttons](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/swbuttons.png?raw=true)

### FINAL OVERVIEW

#### Diagram

Here's the full diagram

![final](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/final.png?raw=true)

#### Photo

![ps4_wiring_photo](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/ps4_wiring_photo.jpg?raw=true)

- There's a layer of double stick foam under the pico so it's not in direct contact with the touch board
- I didn't have headers soldered on for the 12-pin connector, but it still made my life easier
- use hot glue on critical parts you don't want moving (e.g. the I2C solder points)

#### Switch version photo

![switch_wiring_photo](https://github.com/CrazyRedMachine/IpegaDivaPlus/blob/main/assets/switch_wiring_photo.jpg?raw=true)

- Here no zif breakout boards have been used, also the clear pad is not soldered for LED button

## How to build

```
export PICO_SDK_PATH=<path to your pico-sdk>
mkdir build
cd build
cmake ..
make
```