# CH4100_SIM7000
Using Seeed CAN_BUS Shield and Botletics SIM7000A Shield simultaneously on an Arduino UNO enabling CAN_BUS control of CH4100 charger
for my Zero Motorcycle and also internet access for the Arduino with Verizon's cell towers allowing me to monitor the charging
remotely through Adafruit IO.

Note: The Seeed CAN_BUS Shield and Botletics SIM7000A Shield both use pins 9, 10, and 11 on the Arduino.  The Seeed Shield uses them
for SPI and the Botletics Shield uses them for Software Serial.  To make these stackable, I broke off the male pins for 9,10, and 11
from the Botletics Shield and then simply used male-male jumpers to connect pins 10 and 11 to 4 and 5 on the female side.  I stacked
the Seeed on top of the Arduino and then the Botletics on top of the Seeed (with the 10,11 - 4,5 jumpers above that.  I also have
Vin jumpered to 5V which allows me to power everything through the DB9 port on the Seeed.

Also had to break off the reset pin from the SIM7000 shield because it kept resetting after I powered down the Arduino.
*Update:This ^ is still an issue.
Issues to work out:
1. If there's a comm issue, it stops sending messages to the charger (so it turns off).

2. The battery on the SIM7000A shield means it will stay on after the 5v power is removed. (I've been powering this from the 240v in from
the charging station so this isn't ideal.

3. I always had issues with the CANBUS shield not turning on properly (I always had to power cycle the arduino after charger was powered
to get it to work properly. I'm guessing this may be more problematic with the stacked shields but I don't know.  So far I've only tested
this with arduino usb in power (from startup).
