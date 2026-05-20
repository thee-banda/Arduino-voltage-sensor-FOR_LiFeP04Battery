# Arduino Voltage Sensor for LiFePO4 Battery

This Arduino project monitors a 24V LiFePO4 battery using an Arduino UNO and a resistor voltage divider. It reads the divided battery voltage on analog pin `A0`, calculates the estimated battery voltage and charge percentage, then prints the results to the Serial Monitor.

## Hardware

- Arduino UNO
- 24V LiFePO4 battery pack
- Voltage divider:
  - `R1 = 56k ohm`
  - `R2 = 10k ohm`
- Battery positive connected through the divider to `A0`
- Common ground between the battery divider and Arduino

## How It Works

The sketch averages multiple ADC samples from `A0`, converts the ADC value to the measured Arduino input voltage, then multiplies by the voltage divider ratio to estimate the real battery voltage.

It also calculates a simple linear battery percentage using these thresholds:

| Status | Voltage |
| --- | --- |
| Full | `30.0V` |
| Low | `< 25.6V` |
| Warning low | `< 24.8V` |
| Critical low | `<= 24.0V` |
| Over voltage | `> 29.2V` |

## Files

- `Voltage-based-Battery-estimation.ino` - main Arduino sketch
- `LEDDisplay.ino` - reserved for future LED or display output code
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
