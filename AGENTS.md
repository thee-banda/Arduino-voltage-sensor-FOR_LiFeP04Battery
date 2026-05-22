# Repository Guidelines

## Project Structure & Module Organization

This repository is an Arduino sketch for voltage-based LiFePO4 battery estimation.

- `Voltage-based-Battery-estimation.ino` contains the main sketch: pin setup, voltage-divider constants, ADC averaging, battery percentage calculation, status classification, and serial output.
- Keep additional Arduino source files in this sketch folder as `.ino`, `.h`, or `.cpp` files. Place reusable helpers in clearly named modules, for example `BatteryMath.h` or `DisplayOutput.cpp`.
- No automated test directory or asset directory exists yet.

## AGV Integration Context

This repository is a battery-estimation and LCD display component, not the complete AGV sensor controller. Use it as a reference prototype for the larger AGV sketch at `/Users/thee/Documents/Arduino/agv_sensor_v3T.ino/agv_sensor_v3T.ino.ino`.

The full AGV sketch currently uses its own active battery configuration and behavior:

- Battery analog input: `BAT_PIN A0`
- LCD: `LiquidCrystal_I2C` at address `0x27`
- Voltage divider constants: `R1 = 300000.0`, `R2 = 28800.0`
- Percent endpoints: `MIN_BAT_VOLT = 23.8`, `MAX_BAT_VOLT = 30.9`
- Low-battery output: `TW_Buzzer`
- Low-battery condition: `battery_display < 5`

Do not copy this component's voltage divider constants, full/empty calibration values, or LCD behavior directly into the AGV sketch without measuring and validating the real AGV hardware. The `26.65V` full/rest value and `24.00V` usable-empty policy in this repository are component-level calibration notes, not confirmed production calibration for `agv_sensor_v3T.ino.ino`.

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

Previously, the sketch displayed real-time percent values immediately. An older stabilization attempt used a majority filter in `calculateStableBatteryPercent()` with `PERCENT_HISTORY_COUNT`, and `handleDisplayPercentError()` was kept as experimental bounce-handling logic. Those older approaches are currently commented out and do not run.

The current implementation calculates the displayed percent from ADC counts in `calculateStableBatteryPercentFromADC()`. Small ADC movement near a percent edge is filtered with `ADC_HYSTERESIS_COUNTS = 2` and `PERCENT_CONFIRM_COUNT = 2`, so noise such as `734 -> 735 -> 734` should keep the same displayed percent. Larger real changes use `PERCENT_SNAP_THRESHOLD = 5`; when the raw ADC-based percent differs from the displayed percent by at least `5%`, the display snaps to the new value immediately so changing to another battery pack does not slowly count down one percent at a time. The `%` symbol is presentation text only; the filtered percent value is stored as an integer.

Problem 1.0 recorded on 20/05/2026 17:03: the initial estimated full-charge voltage range was `29.2V - 30V`, but after the adapter changed to green, the actual measured full battery voltage dropped to approximately `26.65V`.

Solution 1.0: calibrate `VOLTAGE_FULL` to the measured full-charge value. The current sketch sets `VOLTAGE_FULL = 26.65`.

TAICO specification note: TAICO lists charging voltage as `29.2V` and standard voltage as `24V`. This project does not use `29.2V` as `VOLTAGE_FULL` because the measured battery voltage after the adapter turns green is approximately `26.65V`. Therefore, `VOLTAGE_FULL = 26.65V` represents the measured full/rest display voltage, while `29.2V` represents the charger charge voltage.

TAICO end-of-discharge note: available TAICO 24V LiFePO4 reference data lists `29.2V` as maximum charge voltage and `20V` as end-of-discharge voltage. The project intentionally does not use `20V` as display-empty because it represents deep-discharge or protection territory. The intended display-empty / usable-empty policy is `VOLTAGE_EMPTY = 24.00V`, leaving reserve above true end-of-discharge. Do not change `VOLTAGE_EMPTY` down to `20V` unless the project intentionally wants to display usable charge into deep-discharge territory. Current implementation note: the sketch currently sets `VOLTAGE_EMPTY = 24.00V`.

Validation results:

- Step 1.1: Fully charged battery, adapter green, measured battery voltage `~26.65V`; Arduino displayed `Battery Percentage = 100%`.
- Step 1.2: Partially charged battery, adapter red, measured battery voltage `~24.77V`; Arduino displayed `Battery Percentage = 29%`.

Problem 1.2 recorded on 21/05/2026: ADC noise near a percent boundary, for example `734 -> 735 -> 734`, can make the displayed percent bounce if the sketch follows raw real-time values directly. However, forcing every percent change to move only one percent per loop is also wrong for real-time behavior because swapping to another battery pack would make the display slowly count down until it reaches the true value.

Solution 1.2: use ADC-based percent stabilization with two paths. Small changes below `5%` use hysteresis and repeated-direction confirmation to avoid display bounce. Large changes of `5%` or more snap to the new raw ADC-based percent immediately, treating the reading as a real battery state change rather than noise.

When changing `VOLTAGE_FULL`, `VOLTAGE_EMPTY`, `ADC_HYSTERESIS_COUNTS`, `PERCENT_CONFIRM_COUNT`, or `PERCENT_SNAP_THRESHOLD`, document the measured voltage, adapter color/state, observed displayed percent, and reason for the change.
