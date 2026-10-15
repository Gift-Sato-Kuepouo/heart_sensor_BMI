# Investigating Heart-Rate Sensors

A student lab guide for the `heart_sensor_BMI` project. It explains **how the
MAX30102 measures a heartbeat**, walks through the experimental procedure, and
gives you observation tables and discussion questions to complete.

- Level: introductory ECE lab (Arduino Uno + I2C peripherals)
- Time: ~2 hours build + ~30 minutes of experiments
- Prerequisites: basic Ohm's law, familiarity with the Arduino IDE

---

## 1. Learning objectives

By the end of this investigation you should be able to:

1. Explain the principle of **photoplethysmography (PPG)** and why an optical
   sensor can detect a heartbeat.
2. Wire two I2C devices (MAX30102 + SSD1306) on one shared bus and justify
   each connection.
3. Distinguish **raw PPG signal**, **beat detection**, and **BPM averaging**
   in the firmware.
4. Measure your own resting and post-exercise heart rate and evaluate the
   sensor's accuracy against a manual pulse count.
5. Identify sources of error (motion artefact, ambient light, perfusion) and
   propose improvements.

---

## 2. Background: how an optical heart-rate sensor works

### 2.1 Photoplethysmography (PPG)

Your finger is not a static piece of tissue — it is a volume of blood that
**pulses** with every heartbeat. When the left ventricle contracts
(systole), a bolus of arterial blood arrives in the capillary bed of the
fingertip; during diastole the volume falls slightly. Blood absorbs light,
so the amount of light that passes through (or reflects from) the finger
**rises and falls at your heart rate**. This technique is called
photoplethysmography — "writing with light" (Greek *plethysmos*, increase).

### 2.2 Why two wavelengths?

The MAX30102 contains two LEDs and a photodetector:

| LED    | Wavelength | Absorbed mainly by        |
| ------ | ---------- | ------------------------- |
| Red    | ~660 nm    | **Deoxygenated** haemoglobin |
| Infrared | ~880 nm  | **Oxygenated** haemoglobin   |

Haemoglobin's absorption of red vs. infrared light depends on how much
oxygen it carries. By taking the **ratio of the red and infrared PPG
amplitudes** you can estimate **SpO₂** (blood-oxygen saturation) — the same
principle as a hospital finger clip. This project uses the **infrared** LED
for beat detection (it penetrates deeper and gives a cleaner pulse), but the
raw values of both are available in the serial output for you to explore.

### 2.3 From photons to beats to BPM

The signal chain in the firmware is:

```
finger
  └─► MAX30102 photodiode  ─►  IR sample (irValue, ~50 000+ with finger)
        └─► checkForBeat()     ─►  "a beat happened at millisecond T"
              └─► inter-beat interval (IBI = T_now − T_last)
                    └─► BPM = 60 000 / IBI
                          └─► 8-beat moving average  ─► OLED + LED
```

Key idea: **BPM is derived from time, not from the sample value.** The sensor
samples at 400 Hz (a new IR value every 2.5 ms); the firmware looks for a
sharp rise in that waveform (a peak in the PPG pulse), timestamps it, and
divides 60 seconds by the average interval between timestamps.

### 2.4 Normal values and context

| Condition             | Typical heart rate |
| --------------------- | ------------------ |
| Resting adult         | 60–100 bpm         |
| Trained athlete at rest | 40–60 bpm        |
| Light exercise        | 100–140 bpm        |
| Max effort (age-pred.) | ≈ 220 − age bpm   |

Resting heart rate varies with caffeine, stress, temperature and time of
day — keep these constant when you compare measurements.

---

## 3. Hardware to investigate

See [`wiring.md`](wiring.md) for the exact pin map and
[`../hardware/poi.png`](../hardware/poi.png) for the visual layout. The three
subsystems you are investigating:

| Subsystem | Part                   | What it contributes to the investigation |
| --------- | ---------------------- | ---------------------------------------- |
| Sensing   | MAX30102 (I2C, 0x57)   | Produces the raw PPG waveform and beat events |
| Output    | SSD1306 OLED (I2C, 0x3C) | Displays live BPM — your measurement instrument |
| Indicator | LED on D9 + 220 Ω      | Visualises each beat; lets you cross-check BPM by eye |

---

## 4. Experimental procedure

### Part A — Verify the bus (10 min)

1. Upload the firmware and open **Tools → Serial Monitor at 115200 baud**.
2. **Without a finger** on the sensor, record the steady `IR=` value:
   - `IR ≈ 0–2 000` is normal (photodiode sees only ambient light).
   - A much higher idle value suggests the sensor is saturated or dirty.
3. If the OLED prints `MAX30102 not found`, run an I2C scanner sketch and
   confirm devices at **0x57** (sensor) and **0x3C** (display).

### Part B — Resting heart rate (15 min)

1. Sit quietly for two minutes. Place your fingertip **lightly and still**
   over the sensor's optical windows — pressing too hard blanches the
   capillaries and weakens the pulse.
2. Wait for the OLED to leave `Measuring...` and show a BPM value.
3. Simultaneously count your pulse at your wrist or neck for **30 seconds**
   and double it. Record both numbers.
4. Repeat three times; compute the mean absolute error between the sensor and
   the manual count.

| Trial | Sensor BPM | Manual count BPM | Difference |
| ----- | ---------- | ---------------- | ---------- |
| 1     |            |                  |            |
| 2     |            |                  |            |
| 3     |            |                  |            |
| Mean  |            |                  |            |

### Part C — Response to exercise (15 min)

1. Perform 2 minutes of vigorous exercise (jumping jacks, stair climbing).
2. Immediately sit down and start Part B's measurement again, recording BPM
   every 30 s for 5 minutes.
3. Note how quickly the reading rises and how long recovery takes.

| Time after exercise (s) | Sensor BPM | Manual count BPM |
| ----------------------- | ---------- | ---------------- |
| 0–30                    |            |                  |
| 30–60                   |            |                  |
| 60–90                   |            |                  |
| 90–120                  |            |                  |
| 120–150                 |            |                  |
| 150–180                 |            |                  |
| 180–210                 |            |                  |
| 210–240                 |            |                  |
| 240–270                 |            |                  |
| 270–300                 |            |                  |

### Part D — Signal-quality experiments (20 min)

Each of these deliberately breaks an assumption. Predict the outcome **before**
trying it, then record what actually happens.

| Manipulation                    | Predicted effect on BPM reading | Observed |
| ------------------------------- | ------------------------------- | -------- |
| Lift finger halfway off the sensor |                             |          |
| Press very hard on the sensor   |                                 |          |
| Wave the other hand near the sensor (ambient light) |               |          |
| Talk / move the measurement arm |                                |          |
| Wrap the sensor in a thin cloth (simulate dark skin / poor perfusion) |  |          |
| Measure after drinking coffee   |                                 |          |

---

## 5. Analysis questions

Answer these in your lab report. They map directly to the sections above.

1. **Principle** — In §2.1 you learned that blood volume pulses with each
   heartbeat. Explain why the *infrared* LED gives a stronger, cleaner
   waveform than the red LED for beat detection. *(Hint: think about
   penetration depth.)*
2. **Signal chain** — The firmware uses `irValue > 50000` as its finger-presence
   test. What physical quantity does that number represent, and what would
   happen to your readings in a very brightly lit room if you used a
   transparent film instead of your finger?
3. **Time vs. amplitude** — BPM is computed from the *interval* between beats,
   not from the waveform's height. Explain why this design choice makes the
   measurement robust to the amplitude changes you observed in Part D.
4. **Averaging** — The firmware averages the last 8 beats. What is the
   trade-off between a larger window (smoother BPM, slower response) and a
   smaller one (noisier, faster)? Justify your answer with data from Part C.
5. **Error sources** — Rank the Part D manipulations from most to least
   damaging to accuracy and explain each in terms of the physics of PPG.
6. **Extension: SpO₂** — The MAX30102 also reports a red-LED signal. Describe
   (you do not need to implement it) how you would compute an SpO₂ estimate
   from the red and infrared AC/DC components. What calibration would a real
   device need?
7. **Extension: BMI** — This repository is named `heart_sensor_BMI` but the
   current hardware cannot measure body mass. Look up the BMI formula
   (kg / m²). Propose a complete system addition — a weight sensor (e.g. HX711
   + load cell), a way to enter height, and where the computation would live
   (firmware vs. host PC) — and discuss the clinical limitations of BMI.

---

## 6. Safety and ethics

- The MAX30102 is a **Class 1 LED product** — eye-safe in normal use; never
  stare into the lit emitter at point-blank range anyway.
- The sensor and firmware are **not a medical device**. Do not use the
  readings for clinical decisions; a medical-grade pulse oximeter uses
  calibrated optics, motion artefact rejection and clinical validation.
- If you feel faint during the exercise portion, stop and rest. Stay hydrated.

---

## 7. Troubleshooting

| Symptom | Likely cause | Fix |
| ------- | ------------ | --- |
| OLED dark, serial says `SSD1306 not found` | SDA/SCL swapped, or VCC not connected | Re-check §2.2 of wiring.md; run an I2C scanner |
| `MAX30102 not found` | Sensor on wrong I2C address or power | Confirm VIN = 3.3 V; scan for 0x57 |
| BPM stuck at 0, OLED says `Measuring...` | No finger / too little contact | Rest the fingertip flat over both LEDs |
| Wildly fluctuating BPM | Motion artefact | Brace the hand on the table; keep still |
| Plausible but wrong BPM | Ambient light leaking in | Shield the sensor; repeat in dimmer room |
| LED never blinks but OLED shows BPM | LED wired backwards / missing GND | Flip the LED: anode toward the resistor |
