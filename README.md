# CH4100_SIM7000
Using Seeed CAN_BUS Shield and Botletics SIM7000A Shield simultaneously on an Arduino UNO enabling CAN_BUS control of CH4100 charger
for my Zero Motorcycle and also internet access for the Arduino with Verizon's cell towers allowing me to monitor the charging
remotely through Adafruit IO.

Note: The Seeed CAN_BUS Shield and Botletics SIM7000A Shield both use pins 9, 10, and 11 on the Arduino.  The Seeed Shield uses them
for SPI and the Botletics Shield uses them for Software Serial.  To make these stackable, I broke off the male pins for 9,10, and 11
from the Botletics Shield and then simply used male-male jumpers to connect pins 10 and 11 to 4 and 5 on the female side.  I stacked
the Seeed on top of the Arduino and then the Botletics on top of the Seeed (with the 10,11 - 4,5 jumpers above that.  I also have
Vin jumpered to 5V which allows me to power everything through the DB9 port on the Seeed.
