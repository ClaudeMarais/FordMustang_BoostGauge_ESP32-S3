# Boost Guage for Ford Mustang using ESP32-S3
 
An Arduino project boost gauge for a 2016 Ford Mustang Ecoboost. OBD2 data is used to display turbo boost pressure, current gear, vehicle speed and a shift indicator graph.

NOTE: It's recommended to disconnect the device if the car won't be driven for more than a week.

Car data like boost pressure, RPM, vehicle speed and current gear can be retrieved from the car's high speed CAN bus via the OBD2 port. The ESP32-S3 has a built-in CAN controller, so we connect a SN65HVD230 CAN bus transceiver to the ESP32-S3 which communicates on the high speed bus. Showing something on a display is a blocking operation, i.e. while the display is updating, you can't collect the latest car data. Luckily the ESP32-S3 has two cores, therefore we use one core to continuously read CAN frames with car data while updating the display on the other core.

The OBD2 connector has an always-on 12V pin where the device will be powered from. Since it's always on, it will unnecessarily draw power when the car is turned off. One option would be to just always unplug the device when not driving, another might be to add a button to the device to manually switch it on/off. In this project, we simply detect if the car is turned on and if not, we put the device into a lite sleep mode by reducing the processor clock speed to 40Mhz and switching off the display using a mosfet. For some reason switching in and out of actual deep sleep mode drains the car's battery, it's probably waking up some electronic components in the car? When we detect that the car is on, the processor is switched back to 240MHz and the dispay is switched on again.

Below is some power information when the device is connected together with a BAMA ECU tuner logger device, which also draws power and monitors when the car switches on.

| Microcontroller Setup                     | Power          | Notes  | 
| ------------------------------------------|----------------|--------| 
| 240MHz with the display on                | 125mA @ 5.12V  |        |
| 240MHz with display off                   | 38mA @ 5.1V    |        |
| 80Mhz with display off                    | 29mA @ 5.1V    |        | 
| 40Mhz with display off                    | 22mA @ 5.1V    | SN65HVD230 requires 80Mhz to operate reliably  | 
| 40/80MHz cycle (3s at 40MHz, 1s at 80MHz) | ~24mA avg @ 5V | Current idle mode |
| Deepsleep with display off                | ~7mA @ 5.12V   | Drains car battery 0.18V per hour | 

Estimated daily drain from the car's 12V battery (assuming 85% buck converter efficiency):

| Mode                      | 5V current | 12V current | Daily drain | Days to 50% (60Ah battery) |
|---------------------------|------------|-------------|-------------|----------------------------|
| 240 MHz display off       | 38 mA      | 18.6 mA     | 447 mAh     | 67 days                    |
| 80 MHz constant           | 29 mA      | 14.2 mA     | 341 mAh     | 88 days                    |
| 40 MHz constant           | 22 mA      | 10.8 mA     | 259 mAh     | 116 days                   |
| 40/80 MHz cycle (current) | ~24 mA     | 11.6 mA     | 279 mAh     | 107 days                   |

NOTE: It's fun to tinker with your car, but there is always a chance to mess things up. I won't be liable if for some reason you damage your car.

NOTE: The CAN IDs used in this project specifically work with a 2016 Ford Mustang Ecoboost. The same IDs might not work on other models, you'll have to research what PIDs work with your own car.

Some tips:
 - Diagrams of OBD2 pins are normally shown for the female connector in your car. Don't forget that those pins are in swapped/mirrored positions on the male connector.
 - The OBD2 connector has an "always on" 12V pin. Make sure the wire connecting to that pin on your male connector isn't exposed so that it cannot touch other wires!
 - I tried multiple pins on the ESP32-S3 to connect to the SN65HVD230, but only D4/D5 worked for me. Coincidentally these are also the SDA/SCL pins.
 
Hardware:
 - XIAO ESP32-S3
 - 12V to 5V Voltage regulator
 - SN65HVD230 CAN bus transceiver
 - ILI9341V SPI display, 2.8 inch 240x320 Capacitive Touch Screen
 - AO3401 A19T P-Channel Mosfet (30V 4.2A RDS 50mΩ)
 - 220 Ohm Resistor
 - 2.2K Ohm Resistor
 - Magnetic Pogo Pin Connector
 - OBD2 Connector
 - USB Data Cable
 
Arduino libraries used:
 - ESP32-TWAI-CAN: https://github.com/handmade0octopus/ESP32-TWAI-CAN
 - ArduinoGFX: https://github.com/moononournation/Arduino_GFX

If you are unfamiliar with Arduino, it’s pretty simple and there is a multitude of small tutorials on the internet. At a high level, you will need to:
-	Install the Arduino IDE - https://www.arduino.cc/en/software
-	Configure Arduino IDE for ESP32 - https://randomnerdtutorials.com/installing-esp32-arduino-ide-2-0/
-	Install the two libraries mentioned above - https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/
-	Try out the most simple ESP32-S3 app to switch on its onboard LED - https://youtu.be/JlnV3U3Rx7k?si=xF3rMG3LD-cYAaQe

--------------------------------

![alt text](Image1-1.JPG)

--------------------------------

![alt text](Image2.gif)

--------------------------------

![alt text](Image3.JPG)

--------------------------------

![alt text](Image4.JPG)

--------------------------------

![alt text](Image5.JPG)

--------------------------------

![alt text](Image6.JPG)

--------------------------------

![alt text](Image7.JPG)

--------------------------------

![alt text](Image8.JPG)

--------------------------------

![alt text](Image9.JPG)

--------------------------------

You will need about 1.5m of wiring to reach from the car's OBD2 connector up to the windshield of the car. Only four OBD2 wires are required to reach the device, two for power and two for the high speed CAN bus communication. I used three different cables for this. First, an OBD2 splitter so that another device, e.g. a tuner device, can be attached while this device is used. Then I soldered a long USB data cable to the OBD2 connector cable. Make sure that you specifically buy a data cable and not a charging cable, since a charging cable won’t have extra data wires.

![alt text](Image10.JPG)

--------------------------------

![alt text](Image11.JPG)

--------------------------------

![alt text](Image12.JPG)

--------------------------------

![alt text](Image13.JPG)

--------------------------------

![alt text](Image14.JPG)

--------------------------------

![alt text](Image15.JPG)

--------------------------------

![alt text](Image16.JPG)

--------------------------------

![alt text](Image17.JPG)

--------------------------------

![alt text](Image18.JPG)

--------------------------------

![alt text](Image19.JPG)

--------------------------------

![alt text](Image20.JPG)

--------------------------------

![alt text](Image21.JPG)

--------------------------------

![alt text](Image22.JPG)

--------------------------------
