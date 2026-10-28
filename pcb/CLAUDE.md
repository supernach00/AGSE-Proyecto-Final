# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`pcb/` holds the KiCad 10 hardware design for the final project (`proyecto_final`, git root is the parent directory). The board carries the signal generator that is already running as a prototype: an STM32F411 "Black Pill" firmware (`../Blackpill_codec_V2/`) driving a PCM3060 audio codec, plus a desktop control app (`../comandos.py`, Tkinter + pyserial over USB CDC at 115200). The project, its commit messages and its code comments are in Spanish. Keep using Spanish for commits and for names in schematics and labels.

`pcb_preliminar/` is the current KiCad project (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`). Custom parts live in project-local libraries registered through `sym-lib-table`/`fp-lib-table` as `${KIPRJMOD}/pcb_preliminar.kicad_sym` and `${KIPRJMOD}/pcb_preliminar.pretty`.

The user's KiCad UI is in English, so give menu, dialog and option names in English even when the conversation is in Spanish.

## Fabrication constraints

The board is **single-sided and homemade**: it is etched with acid, the holes are drilled by hand, and there is no plating. Every design choice has to fit that:

- Route only on `B.Cu`, with components on the top side. KiCad keeps a minimum of 2 copper layers, so leave `F.Cu` empty. Don't use vias; when a crossing can't be avoided, use a wire jumper.
- Use wide tracks and clearances: signals ≥ 0.8 mm, power ≥ 1.2 mm, clearance ≥ 0.5 mm.
- Use big through-hole pads: ~2 mm pads with a 1.0 mm drill on a 2.54 mm pitch (oval pads, wider across the row, give more annular ring).
- The Black Pill and the PCM3060 adapter are **modules on female pin sockets**, not chips. Each one is a custom module symbol plus a footprint with the socket pads and the module outline on `F.Fab`/`F.CrtYd`. Their pin numbers are the physical header positions.

## Commands

KiCad files are S-expression text. Use `kicad-cli` (10.0.x is installed) for checks and exports instead of hand-parsing:

```sh
kicad-cli sch erc pcb_preliminar/pcb_preliminar.kicad_sch            # electrical rules check
kicad-cli pcb drc pcb_preliminar/pcb_preliminar.kicad_pcb            # design rules check
kicad-cli sch export netlist pcb_preliminar/pcb_preliminar.kicad_sch
kicad-cli sch export pdf pcb_preliminar/pcb_preliminar.kicad_sch
kicad-cli pcb export gerbers pcb_preliminar/pcb_preliminar.kicad_pcb -o gerbers/
```

If a `~*.lck` file exists next to a design file, KiCad has that file open. Edit the file only when the user asks, and warn them that KiCad will overwrite your changes, or show a reload conflict, the next time it saves.

## Hardware the board must match

The firmware's CubeMX config (`../Blackpill_codec_V2/Version V2.ioc`) is the source of truth for pin assignments. If the schematic and the `.ioc` disagree, flag it rather than silently changing either one.

The crystal, USB and SWD are on the Black Pill module itself, so the base board only needs to route I2S, I2C, the keypad and power.

- **MCU**: STM32F411CEUx (UFQFPN48), HSE crystal on PH0/PH1, SWD on PA13/PA14, USB FS device on PA11/PA12 (used as the CDC link to `comandos.py`).
- **Codec**: a PCM3060 on I2S2 in half-duplex master transmit mode (MCK on PA3, CK on PB10, WS on PB12, SD on PB15). It runs Philips I2S with 24-bit data, MCLK output enabled, and a nominal 96 kHz sample rate (the actual rate is 97656.25 Hz). Control is over I2C1 at address `0x46`.
- **OLED**: an SSD1306 on the same I2C1 bus (PB6 SCL, PB7 SDA, 100 kHz) at address `0x3C`. The codec and the OLED share this bus.
- **4×4 keypad** (`../Blackpill_codec_V2/Core/Src/keypad.c`): outputs on PA0, PA1, PB0 and PB1; inputs on PB8, PB9, PB13 and PB14.
