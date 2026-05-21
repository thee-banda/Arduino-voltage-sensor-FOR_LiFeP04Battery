# Arduino Voltage Sensor for LiFePO4 Battery

This Arduino project monitors a 24V LiFePO4 battery using an Arduino UNO and a resistor voltage divider. It reads the divided battery voltage on analog pin `A0`, calculates the estimated battery voltage and charge percentage, then prints the results to the Serial Monitor.

## Hardware

- Arduino UNO
- 24V LiFePO4 battery pack
- Resistor `R1 = 56k ohm`
- Resistor `R2 = 10k ohm`
- Capacitor `104` / `0.1 uF` for analog input filtering
- In my lab, I use a `1 uF` capacitor, but I suggest using a `0.1 uF` capacitor for this filter.
- Battery positive connected through the divider to `A0`
- Common ground between the battery divider and Arduino

## Board Compatibility

In my lab setup, I use an Arduino UNO. Other MCU boards may also work, such as ESP32 or other Arduino-compatible boards, but you must adjust the analog input pin, ADC reference voltage, and safe maximum input voltage for the board you use.

## How It Works

The sketch averages multiple ADC samples from `A0`, converts the ADC value to the measured Arduino input voltage, then multiplies by the voltage divider ratio to estimate the real battery voltage.

It also calculates a simple linear battery percentage using these thresholds:

| Status | Voltage |
| --- | --- |
| Full | `26.65V` |
| Low | `< 25.6V` |
| Warning low | `< 24.8V` |
| Critical low | `<= 24.0V` |
| Over voltage | `> 29.2V` |

`26.65V` is the calibrated full-charge value from my lab test when the charger adapter status changed to green.

## Important Functions

- `readAverageADC(pin)` reads analog pin `A0` multiple times and returns the average ADC value. This reduces noise before voltage conversion.
- `adcToVoltage(adc)` converts the raw Arduino ADC value into the measured voltage at `A0`.
- `calculateBatteryPercent(voltage)` converts battery voltage into a simple percent value using `VOLTAGE_EMPTY` and `VOLTAGE_FULL`, then limits the result between `0%` and `100%`.
- `addBatteryPercentReading(percent)` saves the latest rounded percent into a history buffer. The buffer size is controlled by `PERCENT_HISTORY_COUNT`.
- `calculateStableBatteryPercent()` checks the recent percent history and displays the most common integer value. For example, if the values are `18, 18, 17, 18, 19`, the display stays at `18`.
- `getBatteryStatus(voltage)` returns a text status such as `NORMAL`, `LOW BATTERY`, `WARNING LOW`, `CRITICAL LOW`, or `OVER VOLTAGE`.
- `displayBatteryData()` prints the current ADC value, measured `A0` voltage, estimated battery voltage, and stable battery percentage to Serial Monitor.

The experimental `handleDisplayPercentError()` logic is currently commented out in the sketch. It is kept as a reference for detecting one-sample display bounces such as `18 -> 19 -> 18`.

## Files

- `Voltage-based-Battery-estimation.ino` - main Arduino sketch
- `AGENTS.md` - contributor guidelines for this repository

## Usage

Open the folder in Arduino IDE, select **Arduino UNO**, and upload the sketch.

Or use Arduino CLI:

```sh
arduino-cli compile --fqbn arduino:avr:uno .
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn arduino:avr:uno .
arduino-cli monitor -p /dev/cu.usbmodemXXXX -c baudrate=9600
```

Replace `/dev/cu.usbmodemXXXX` with your board's serial port.

## Serial Output

The sketch prints:

- Raw ADC value
- Measured voltage at `A0`
- Estimated battery voltage
- Estimated battery percentage

## Safety Note

Always confirm the voltage divider output stays below `5V` before connecting it to the Arduino analog input. Measure the divider output with a multimeter before uploading or testing with a real battery.
