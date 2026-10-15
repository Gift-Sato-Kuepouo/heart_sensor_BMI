# Wiring & Assembly

This document describes how the hardware is connected, the I2C bus details,
and the bill of materials for the heart-rate monitor.

- Board: **Arduino Uno (Rev3)**
- Reference files: [`hardware/poi.png`](../hardware/poi.png) (breadboard view),
  [`hardware/Schematic_poi.png`](../hardware/Schematic_poi.png) (schematic view),
  [`hardware/PCB_poi.png`](../hardware/PCB_poi.png) (PCB view),
  [`hardware/gy-max30102-module.fzpz`](../hardware/gy-max30102-module.fzpz)
  (Fritzing part — double-click to install into Fritzing), and the video
  walkthrough [`hardware/SetupVideo.mov`](../hardware/SetupVideo.mov).

> **Verify the images yourself.** The exported PNGs and video show the intended
> layout; always double-check each pin against the tables below before powering
> the board, because the tables are the authoritative source.

---

## 1. Bill of materials

| Qty | Component                                   | Fritzing search term        |
| :-: | ------------------------------------------- | --------------------------- |
|  1  | Arduino Uno R3                              | `Arduino Uno R3`            |
|  1  | MAX30102 heart-rate / pulse-oximetry sensor | `MAX30102`                  |
|  1  | OLED display, I2C, 0.96-inch (SSD1306)      | `SSD1306 OLED I2C`          |
|  1  | LED (any colour; red works well)            | `LED`                       |
|  1  | 220 Ω resistor                              | `220 ohm resistor`          |
|  1  | Breadboard                                  | `Breadboard`                |
|  —  | Jumper wires (M-M)                          | `Jumper Wire`               |
|  1  | USB cable (A-to-B) or suitable power supply | Appropriate USB / power component |

---

## 2. Pin map used by the firmware

### 2.1 MAX30102 heart-rate sensor

| Sensor pin | Arduino Uno | Notes                                    |
| :--------: | :---------: | ---------------------------------------- |
| **VIN**    | **3.3V**    | 5 V only if the breakout explicitly supports it |
| **GND**    | **GND**     | Ground                                   |
| **SDA**    | **A4**      | I2C data (shared with the OLED)          |
| **SCL**    | **A5**      | I2C clock (shared with the OLED)         |
| **INT**    | *(not connected)* | Interrupt output — unused in this firmware |

> The gy-max30102 breakout pin order is **VIN – GND – SCL – SDA – INT**.
> Check the silkscreen on your own module; some clones reorder the pins.

### 2.2 SSD1306 OLED display

| OLED pin | Arduino Uno  | Notes                                         |
| :------: | :----------: | --------------------------------------------- |
| **VCC**  | **3.3V**     | Voltage supported by your OLED module (many accept 3.3–5 V) |
| **GND**  | **GND**      | Ground                                        |
| **SDA**  | **A4**       | Same I2C data line as the MAX30102            |
| **SCL**  | **A5**       | Same I2C clock line as the MAX30102           |

> Many 0.96" SSD1306 modules have a solder jumper on the back selecting
> **3.3 V** or **5 V** logic. If yours has it, set it to match your VCC.

### 2.3 Heartbeat indicator LED

| Connection         | Destination           |
| ------------------ | --------------------- |
| Arduino **D9**     | 220 Ω resistor        |
| Other end of resistor | LED **anode (+)**  |
| LED **cathode (−)**   | **GND**            |

The 220 Ω resistor limits the LED current to roughly 15 mA from a 5 V pin —
safe for both the LED and the ATmega328P. The firmware blinks this LED on
every detected beat, so it doubles as a live pulse indicator.

---

## 3. Notes on the shared I2C bus

Both the MAX30102 and the SSD1306 sit on the **same two-wire bus**:

```
Arduino A4 (SDA) ----+--------------------+
                     |                    |
                 MAX30102 SDA         OLED SDA

Arduino A5 (SCL) ----+--------------------+
                     |                    |
                 MAX30102 SCL         OLED SCL
```

- The MAX30102 answers at address **0x57**; the SSD1306 at **0x3C**
  (some modules use **0x3D** — check the back of the display).
- Because the addresses differ, both devices can coexist without conflict.
- Keep the jumper wires short (under ~15 cm). The Uno's internal I2C
  pull-ups are weak (~20–50 kΩ); most SSD1306 and MAX30102 breakouts add
  their own 4.7 kΩ–10 kΩ pull-ups, which is normally enough for a breadboard.
- If you later add a third I2C device, run an `i2c_scanner` sketch first to
  confirm every address on the bus.

---

## 4. Power budget

The Arduino's **3.3 V** pin supplies up to ~50 mA. The MAX30102 draws
roughly 10–15 mA with the default LED currents used in the firmware, and the
OLED another ~20 mA — well within budget. If you decide to power the OLED
from **5 V** instead (module-dependent), only the display moves rails; SDA/SCL
stay on the shared bus, and the SSD1306 breakout's on-board regulator and
level shifters handle the translation.

---

## 5. Assembly checklist

1. **Do not plug in USB yet.**
2. Seat the MAX30102 and the OLED on the breadboard; confirm each module's
   pin labels match §2.
3. Run jumper wires: `VIN→3.3V`, `GND→GND`, `SDA→A4`, `SCL→A5` for **both**
   modules (power and ground may be daisy-chained on the breadboard rails).
4. Place the LED: `D9 → 220 Ω → anode`, `cathode → GND`.
5. Compare your breadboard against [`hardware/poi.png`](../hardware/poi.png)
   and [`hardware/Schematic_poi.png`](../hardware/Schematic_poi.png), or watch
   [`hardware/SetupVideo.mov`](../hardware/SetupVideo.mov).
6. Plug in USB, upload the sketch (see the [README](../README.md)), then place
   a fingertip gently over the MAX30102's two optical windows.
