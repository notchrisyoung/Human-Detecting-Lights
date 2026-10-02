# Human-Detecting Lights

Room lights that turn themselves on when someone is actually *in* the room, and off when they leave. It uses a **24 GHz mmWave presence sensor (HLK-LD2410C)** rather than a PIR, so it keeps the lights on while you're sitting still. When presence is detected, an ESP32 fades up a 12 V LED strip and switches on a **Kasa HS200 smart light switch** over Wi-Fi. When the room empties, everything fades back off.

<!-- PHOTOS: add photos of the install and the printed enclosure here, e.g. ![Install](docs/install.jpg) -->

## How it works

- The LD2410C's presence output pin is read once a second.
- **Someone present →** the LED strip fades up (PWM through a MOSFET) and the Kasa switch is turned on.
- **Room empty →** the Kasa switch turns off and the strip fades down to off.
- **Night mode (22:00–06:00):** the strip comes on at a dimmer level (`LED_NIGHT_BRIGHTNESS`) and the Kasa switch is left **off**, so you get a soft night light instead of the main room lights.
- Time comes from NTP, with a POSIX time-zone string (`PST8PDT,...` by default).
- If the Kasa switch isn't found at boot, the sketch keeps re-scanning the network for it.

## Hardware

- ESP32 dev board (plus a GPIO breakout board)
- HLK-LD2410C mmWave radar presence sensor
- Kasa Smart Light Switch HS200
- 12 V LED light strip and a 12 V power supply
- Logic-level MOSFET and resistors to drive the strip
- 3D-printed parts in `STL Files/`: an ESP holder with lid, and a mounting bar

### Pins

| Signal | ESP32 GPIO |
|---|---|
| LED strip PWM (to MOSFET gate) | 27 |
| LD2410C presence output | 12 |

## Setup

1. Arduino IDE with the **ESP32 board package**.
2. Libraries: `KasaSmartPlug` and `Arduino_JSON`.
3. In `LightController/LightController.ino`, set:
   - `ssid` / `password`: your Wi-Fi
   - `SWITCH_NAME`: the name of your Kasa switch exactly as it appears in the Kasa app
   - `time_zone`: your POSIX TZ string
   - Optional: `LED_DAY_BRIGHTNESS`, `LED_NIGHT_BRIGHTNESS`, `START_NIGHT`, `END_NIGHT`
4. Upload. The serial monitor (115200) lists every Kasa device found on the network, which helps if the switch name doesn't match.

> Don't commit your real Wi-Fi credentials. Keep them only in your local copy.

## Files

| Path | Purpose |
|---|---|
| `LightController/LightController.ino` | ESP32 firmware |
| `STL Files/Big ESP holder.STL`, `Big ESP holder lid.STL` | Printable enclosure for the ESP32 |
| `STL Files/bar.STL` | Mounting bar |
