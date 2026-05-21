# Repository Guidelines

## Project Structure & Module Organization

This repository is an Arduino sketch for voltage-based LiFePO4 battery estimation.

- `Voltage-based-Battery-estimation.ino` contains the main sketch: pin setup, voltage-divider constants, ADC averaging, battery percentage calculation, status classification, and serial output.
- Keep additional Arduino source files in this sketch folder as `.ino`, `.h`, or `.cpp` files. Place reusable helpers in clearly named modules, for example `BatteryMath.h` or `DisplayOutput.cpp`.
- No automated test directory or asset directory exists yet.

## Build, Test, and Development Commands

Use Arduino IDE or Arduino CLI. From this directory:

```sh
arduino-cli compile --fqbn arduino:avr:uno .
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn arduino:avr:uno .
arduino-cli monitor -p /dev/cu.usbmodemXXXX -c baudrate=9600
```

`compile` checks the sketch for Arduino UNO compatibility. `upload` flashes the board; replace the serial port with the local board port. `monitor` verifies runtime serial output at the sketch's configured `9600` baud.

## Coding Style & Naming Conventions

Use Arduino/C++ style with two-space indentation, braces on their own logical blocks as already used, and descriptive function names such as `readAverageADC()` and `calculateBatteryPercent()`. Constants should remain uppercase with underscores, for example `VOLTAGE_WARNING` and `SAMPLE_COUNT`. Use `const byte`, `const int`, or `const float` for fixed configuration values. Prefer `F("...")` for repeated serial strings to conserve SRAM.

Keep comments useful and hardware-specific: document voltage divider values, board assumptions, pin assignments, and threshold rationale. Avoid duplicating section headers.

When changing, fixing, or adding Arduino logic, add a short `//` comment on or directly above the changed line explaining what the line does. Keep these comments practical, for example `displayErrors++; // Count one unstable percent bounce`.

## Testing Guidelines

There is no automated test framework configured. Validate changes by compiling for `arduino:avr:uno`, uploading to an Arduino UNO, and checking serial output for stable ADC value, A0 voltage, calculated battery voltage, percentage, and status. For threshold changes, test representative voltages around `24.0`, `24.8`, `25.6`, `29.2`, and `30.0` volts.

## Commit & Pull Request Guidelines

This directory does not contain Git history, so no existing commit convention can be inferred. Use concise imperative commit messages, for example `Calibrate battery voltage thresholds` or `Add LED display output`. Pull requests should describe hardware tested, board type, resistor values, measured input voltage, serial output changes, and any safety-relevant threshold updates.

## Safety & Configuration Tips

Confirm the voltage divider keeps `A0` below `5V` for the maximum battery voltage. Do not upload threshold or divider-ratio changes without rechecking the actual resistor values and measured multimeter readings.

## NOTE Report Update 1.0

Previously, the sketch displayed real-time percent values immediately. The current code stabilizes the final displayed percent with a majority filter in `calculateStableBatteryPercent()`. `handleDisplayPercentError()` is present in the sketch as experimental error-handling logic, but it is currently commented out and does not run.

The percent filter uses a sample size of `n`, where `n` is the number of recent loop cycles stored in `PERCENT_HISTORY_COUNT`. In the latest sketch, `PERCENT_HISTORY_COUNT = 10`. When the sensor collects recent percent samples, the most common rounded integer value wins. For example, if `n = 4` and the sampled values are `[x, x, x, y]`, `x` wins and is displayed. The `%` symbol is presentation text only; the filtered percent value is stored as an integer.

Problem 1.0 recorded on 20/05/2026 17:03: the initial estimated full-charge voltage range was `29.2V - 30V`, but after the adapter changed to green, the actual measured full battery voltage dropped to approximately `26.65V`.

Solution 1.0: calibrate `VOLTAGE_FULL` to the measured full-charge value. The current sketch sets `VOLTAGE_FULL = 26.65`.

Validation results:

- Step 1.1: Fully charged battery, adapter green, measured battery voltage `~26.65V`; Arduino displayed `Battery Percentage = 100%`.
- Step 1.2: Partially charged battery, adapter red, measured battery voltage `~24.77V`; Arduino displayed `Battery Percentage = 29%`.

When changing `VOLTAGE_FULL` or `PERCENT_HISTORY_COUNT`, document the measured voltage, adapter color/state, observed displayed percent, and reason for the change.
