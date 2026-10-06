<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="logo/movens-logo-dark.svg">
    <img src="logo/movens-logo.svg" alt="Movens" width="387">
  </picture>
</p>

<p align="center">
  A 3D-printed, 5-axis robotic arm with a gripper, driven by an ESP32 and controlled from your browser.
</p>

---

Movens is a heavily modified fork of the [BCN3D Moveo](https://github.com/BCN3D/BCN3D-Moveo), the open-source educational arm. The mechanical design keeps Moveo's printed structure but replaces several parts. The electronics and firmware are new. Instead of Marlin on an Arduino Mega + RAMPS with G-code over USB, Movens runs its own firmware on an ESP32 and serves a web control panel over WiFi. There's nothing to install on the computer or phone you control it from.

*Movens* is Latin for "moving", from the same verb as *Moveo* ("I move").

## Features

- **Browser control panel** served by the arm itself, with light and dark themes and no internet access needed
- **Joint control:** jog or type a target angle for each of the five joints, plus the gripper servo
- **Cartesian control:** jog the tool in X / Y / Z and pitch, or go straight to a pose. Inverse kinematics picks the closest valid solution within the joint limits.
- **Synchronized moves:** all joints start and finish together, so the tool moves smoothly
- **Guided calibration** per joint: steps per degree, home angle, speed, acceleration, and soft limits. It's saved on the board and survives reboots.
- **WiFi setup from the browser:** join your network (reachable at `http://movens.local`), or fall back to the arm's own access point
- **Emergency stop** in every page, also bound to the <kbd>Esc</kbd> key
- **Over-the-air firmware updates** from the Arduino IDE once the arm is on your network
- **REST API**, so you can drive the arm from scripts as well as the web UI

## Hardware

| | |
|---|---|
| Controller | ESP32 development board (classic ESP32, e.g. *ESP32 Dev Module*) |
| Joints J1–J5 | Stepper motors, each on a STEP/DIR driver |
| Gripper | Hobby servo, driven at 700–2300 µs |
| Structure | 3D-printed parts. See [Repository layout](#repository-layout). |

### Wiring

Default pins, set in [`movens_config.h`](FIRMWARE/movens/movens_config.h):

| Joint | Role | STEP | DIR |
|---|---|---|---|
| J1 | Base yaw | 12 | 13 |
| J2 | Shoulder | 27 | 14 |
| J3 | Elbow | 25 | 26 |
| J4 | Wrist roll | 32 | 33 |
| J5 | Wrist pitch | 2 | 15 |
| Gripper | Servo signal | 4 | |

Stepper drivers need their own motor supply. Tie the drivers' logic ground to the ESP32 ground.

## Firmware

The firmware lives in [`FIRMWARE/movens`](FIRMWARE/movens) and builds with the Arduino IDE or `arduino-cli`.

### Requirements

- [Arduino-ESP32 core](https://github.com/espressif/arduino-esp32) (Boards Manager: *esp32 by Espressif Systems*)
- Libraries, from the Library Manager:
  - [FastAccelStepper](https://github.com/gin66/FastAccelStepper)
  - [ESP32Servo](https://github.com/madhephaestus/ESP32Servo)
  - [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) and [AsyncTCP](https://github.com/ESP32Async/AsyncTCP)

`WiFi`, `Preferences` and `ArduinoOTA` come with the ESP32 core.

### Configure and flash

1. Open `FIRMWARE/movens/movens.ino`.
2. Review [`movens_config.h`](FIRMWARE/movens/movens_config.h): pins, stepper speed and acceleration, gripper servo range, and the access-point name and password. **Change the default AP password (`movens_1234`).**
3. Measure your arm and update the `KIN_*` link lengths in the same file. They're used by the Cartesian control.
4. Select your ESP32 board and port, then upload.

Once the arm is on your WiFi network, later updates can go over the air. Pick the `movens` network port in the Arduino IDE.

### Module layout

| File | Purpose |
|---|---|
| `movens.ino` | Setup and the main loop (Core 1) |
| `movens_config.h` | Pins, servo range, WiFi defaults, arm geometry |
| `joints.*` | Stepper engine, J1–J5 steppers, gripper servo |
| `calibration.*` | Steps ↔ degrees, soft limits, saving to flash (NVS) |
| `kinematics.*` | Forward and inverse kinematics (tool pose ↔ joint angles) |
| `commands.*` | FreeRTOS queue from web handlers (Core 0) to the motion loop (Core 1) |
| `connectivity.*` | WiFi station / access-point fallback, saved network, OTA |
| `web_server.*` | HTTP routes and REST API |
| `web_*.h` | Embedded HTML, CSS, JS and favicon for the web UI |
| `json_util.*` | Minimal JSON number parsing |

## Getting started

1. **Power on in the alignment pose.** Each joint's step counter starts at 0 at power-on, so put every joint on its alignment marks before switching on.
2. **Connect.** On first boot the arm starts an access point named `movens`. Join it and open `http://192.168.4.1`.
3. **Join your network (optional).** On the **Network** page, pick your WiFi and save. The arm restarts and joins it. Open `http://movens.local` or the IP your router assigned. If it can't connect, it falls back to the `movens` access point.
4. **Calibrate each joint** on the **Calibrate** page:
   1. Zero the counter at the alignment pose.
   2. Jog to two poses whose angles you can measure, as far apart as the joint allows, and record each one. Approach both from the same direction so backlash doesn't skew the result.
   3. Check the computed steps per degree, set speed, acceleration and soft limits, then save.
   4. Verify by commanding a few angles.
5. **Move the arm** from the **Control** page, by joint or in Cartesian space. Cartesian control needs all five joints calibrated.

Joint angle conventions, like where zero is and which way is positive, must match those described in [`movens_config.h`](FIRMWARE/movens/movens_config.h) for the kinematics to be correct.

## REST API

Every page in the web UI uses this API, and you can call it directly. POST bodies are JSON.

| Method | Path | Body / reply |
|---|---|---|
| GET | `/status` | Steps (`jN`), angles (`aN`), moving flags (`mN`) and tool pose (`x y z pitch yaw`) |
| POST | `/move` | `{joint, steps}`: relative move |
| POST | `/moveto` | `{joint, pos}`: absolute move in steps |
| POST | `/moveangle` | `{joint, deg}`: absolute move in degrees |
| POST | `/movejoints` | `{a1..a5 \| j1..j5, speed?}`: synchronized multi-joint move |
| POST | `/movepose` | `{x, y, z, pitch, yaw?, rel?, dry?, speed?}`: IK move to a tool pose |
| POST | `/setpos` | `{joint, deg}` or `{joint, steps}`: redefine current position, no motion |
| POST | `/servo` | `{us}`: gripper servo pulse width |
| POST | `/stop` | Stop all motors immediately |
| POST | `/home` | Move all joints to step 0 |
| GET / POST | `/config` | Speed and acceleration per joint |
| GET / POST | `/calib` | Calibration and saved motion profile per joint |
| GET / POST | `/wifi` | Current connection and saved network. POST `{ssid, password?}` |
| GET | `/wifiscan` | Nearby networks |

Example: move the tool 20 mm up from where it is now.

```bash
curl -X POST http://movens.local/movepose -d '{"z":20,"pitch":0,"rel":1}'
```

## Repository layout

| Path | Contents |
|---|---|
| [`FIRMWARE/movens`](FIRMWARE/movens) | Movens ESP32 firmware and web UI |
| [`cad`](cad) | Movens parts: new and modified designs (STEP / STL / 3MF) and the Fusion 360 model |
| [`CAD files/movens.f3d`](CAD%20files) | Fusion 360 assembly |
| [`CAD files/uno box`](CAD%20files/uno%20box) | Controller enclosure from an earlier Arduino Uno build |
| [`logo`](logo) | Movens logo, icon and favicon (SVG, light and dark) |
| [`CAD files/Moveo`](CAD%20files/Moveo) | Original BCN3D Moveo SolidWorks parts and assemblies |
| [`FIRMWARE/Marlin_BCN3D_Moveo`](FIRMWARE/Marlin_BCN3D_Moveo) | Original BCN3D Moveo Marlin firmware (not used by Movens) |
| [`BOM`](BOM) | Original BCN3D Moveo bill of materials |
| [`USER MANUAL`](USER%20MANUAL) | Original BCN3D Moveo user and assembly manual |

The original Moveo manual is still the best guide to printing and assembling the shared structural parts.

## Credits

- [**BCN3D Moveo**](https://github.com/BCN3D/BCN3D-Moveo) by BCN3D Technologies, developed with the Departament d'Ensenyament of the Generalitat de Catalunya. Movens is a fork of this project.
- The Moveo is itself based on the [BetaBots Robot Arm](https://hackaday.io/project/3800-3d-printable-robot-arm) by [Andreas Hölldorfer](http://chaozlabs.blogspot.de/) ([GitHub](https://github.com/4ndreas/BetaBots-Robot-Arm-Project)).

## License

[MIT](LICENSE). The original BCN3D Moveo copyright notice is kept, as the license requires.
