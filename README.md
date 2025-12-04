Embedded Systems RTOS Smart Home Project ReadMe


This is a smart home project using an STM32 and an ESP32.

A hardware timer fires every 100 ms and cycles through 3 tasks:

1. Read current from the INA219
2. Read temperature from a thermistor
3. Turn fans on/off using a little state machine

The STM32 sends everything to an ESP32 over UART messages like:
T=71.2
I=123.4

What the STM32 Code Does

Reads thermistor voltage and converts it to °F using a basic linear formula
Reads relevant data from the INA219
Runs a fan-control state machine
Cold fan on pin D4 (active-low)
Hot fan on pin D3 (active-low)
Sends T= and I= values to the ESP
Uses a small task scheduler based on elapsed time inside a timer ISR

Task Periods:
INA219 task - every 1 second
Temp task - every 5 seconds
Fan control - every 200 ms

What the ESP32 Code Does

Opens a UART at 115200
Waits for lines from the STM32
If a line starts with "T=", it’s temperature
If a line starts with "I=", it’s current


Hardware Wiring 

STM32 to ESP32
PA9  (TX)  - ESP RX
PA10 (RX) - ESP TX

STM32
A0 - thermistor divider
D4 - cold fan relay (active-low)
D3 - hot fan relay (active-low)
I2C (SDA/SCL) - INA219

Build and Run (STM32)

1. Open the STM32 .ino file in Arduino IDE
2. Install STM32 core and Adafruit INA219 library
3. Select your STM32 board and COM port
4. Upload
5. Open Serial Monitor at 9600 baud
6. Fans will turn on/off based on temp, and UART will print I= and T= messages

Build and Run (ESP32)

1. Open the ESP code in Arduino IDE
2. Select your ESP32 board and COM port
3. Upload
4. Open Serial Monitor at 115200 baud
