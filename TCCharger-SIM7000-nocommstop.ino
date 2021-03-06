
#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"

#define SIMCOM_7000

/************************* PIN DEFINITIONS *********************************/
#define FONA_PWRKEY 6
//SIM7000A uses 9,10,11 for RI, TX and RX but CANBUS Shield uses 9 and 11 for SPI comm so...
#define FONA_TX 4 // Microcontroller RX  (Pins 9,10,11 cut on SIM7000A and 10,11 jumpered to 4,5)
#define FONA_RX 5 // Microcontroller TX

#define samplingRate 60 // The time we want to delay after each post (in seconds)

#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

/************************* MQTT SETUP *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "yourusernamehere"
#define AIO_KEY         "yourkeyhere"

Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish feed_charging = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/charging");

#include <Wire.h>

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char imei[16] = {0}; // Use this for device ID
uint8_t type;
uint16_t battLevel = 0; // Battery voltage
uint8_t counter = 0;
uint8_t trycounter = 0;
char battBuff[5] = "3123";


#include <SPI.h>

#define CAN_2515
#include "mcp2515_can.h"
#define SPI_CS_PIN 9 //CS Pin

#define outputvoltage 1160
#define outputcurrent 300
#define sendId 0x18E54024  // This is the send to header/name for CH4100
#define fonatries 4
unsigned int chargewatts;

unsigned char len = 0; //Length of received CAN message
unsigned char buf[8]; //Buffer for CAN message
unsigned long int receiveId; //ID of Charger
unsigned int errorct = 0; //tracks number of cycles chargers not charging. Charger starts at around 17
byte chargeron = 0xFB; //yes(on) = FC, no(off) = FB or seemingly anything else - start charger at FB(off)

unsigned long lastCanTime = 0;
unsigned long lastAdaTime = 0;
#define minAdaTime 60000
#define minCanTime 800

mcp2515_can CAN(SPI_CS_PIN);

//Functions
/************************************************
** Function name:           canRead
** Descriptions:            read CAN message
*************************************************/

void canRead(){

  if(CAN_MSGAVAIL == CAN.checkReceive()){ //Check for messages

    CAN.readMsgBuf(&len, buf); // read data, len: data length, buf: data buffer

    receiveId = CAN.getCanId(); //CAN-ID lesen

    if(receiveId == 0x18EB2440){ 
      if (bitRead(buf[1],0) == 1) errorct++;
      Serial.println(errorct);
      if (chargeron == 0xFC & errorct > 512) chargeron = 0xFB; // if 'commanded' on but found off a couple times, force off to prevent cycling.  Will be 17 after 1st on, ~21 after 2nd on
            
      float pv_voltage = (((float)buf[3]*256.0) + ((float)buf[2]))/10.0; //highByte/lowByte + offset
      Serial.println(pv_voltage);
      float pv_current = (3200-(((float)buf[5]*256.0) + ((float)buf[4])))/10.0; //highByte/lowByte + offset
      chargewatts = pv_voltage * pv_current;

      if (millis() > 5000 & millis() < 240000) chargeron = 0xFC; // wait 5-ish seconds before turning on charger
      if (pv_voltage > 116.4 || pv_voltage < 80) chargeron = 0xFB; // if voltage reaches this level turn off charger - changed from 116 to 116.4
      if (pv_current == 0.4 & millis() > 240000) chargeron = 0xFB; // if charge current drops to .4 amps, turn off charging.
    }
  }
}




/************************************************
** Function name:           canWrite
** Descriptions:            write CAN message
*************************************************/

String canWrite(unsigned char data[8], unsigned long int id){

  byte sndStat = CAN.sendMsgBuf(id, 1, 8, data); //Send message (ID, extended Frame, Datalength, Data)
  if(sndStat == CAN_OK) //Status byte for transmission
    return "Snt";
  else
    return "Er";
}


void commCharger() { 

  unsigned char voltamp[8] = {chargeron, lowByte(outputvoltage), highByte(outputvoltage), lowByte(3200-outputcurrent), highByte(3200-outputcurrent),0xFF,0xFF,0xFF};  //Generate the message
  Serial.println(canWrite(voltamp, sendId)); //Send message and output results
  canRead(); //Read output from charger
  
  Serial.println(); //Carriage Return

}

/************************************************
** Function name:           setup
** Descriptions:            Arduino setup
*************************************************/

void setup() {

  pinMode(2, INPUT);
  
  Serial.begin(9600); //Serial Port Initialize

  fona.powerOn(FONA_PWRKEY); // Power on the module
  moduleSetup(); // Establishes first-time serial comm and prints IMEI
  fona.setFunctionality(1); // AT+CFUN=1
  fona.setNetworkSettings(F("vzwinternet")); // For Verizon SIM card

  if (!fona.enableGPRS(false)) Serial.println(F("Er1"));
  while (!fona.enableGPRS(true) && (millis() < 30000)) {
    Serial.println(F("Er0"));
    delay(2000); // Retry every 2s but only try for 30 secs.
  }
  Serial.println(F("FONAok"));

  while(CAN_OK != CAN.begin(CAN_500KBPS)){ //CAN Bus initialisieren
    delay(1000);
  }
 
  Serial.print(F("CANok "));
  Serial.println(digitalRead(2));  
//  if (digitalRead(2) == 1) {
//    Serial.println(F("NResetng"));
//    delay(1000); //Placeholder for CANBUS fix
//  }

}

/************************************************
** Function name:           loop
** Descriptions:            Arduino loop
*************************************************/

void loop() {

// Transmit keep charging message to Charger.
  if(millis() > (lastCanTime + minCanTime)) {

    lastCanTime = millis();
    commCharger();
    Serial.print("W");Serial.println(chargewatts);
  }

// Do FONA(SIM7000) Adafruit transmission
  if(millis() > (lastAdaTime + minAdaTime)) {
    lastAdaTime = millis();
    Serial.println(F("ChNet"));
    trycounter = 0;
    while (!netStatus() && trycounter < 1) {  //// Let's just try once.
      trycounter++;
      Serial.println(trycounter);
      delay(2000); // Retry in 2s
    }

    battLevel = chargewatts;
    dtostrf(battLevel,4,0,battBuff);
    
    MQTT_connect();

    MQTT_publish_checkSuccess(feed_charging, battBuff);
  }


}

//FONA FUNCTIONS

void moduleSetup() {
  fonaSS.begin(115200); // Default SIM7000 shield baud rate
  fonaSS.println("AT+IPR=9600"); // Set baud rate
  Serial.println(F("baud"));
  delay(500); // Short pause to let the command run
  fonaSS.begin(9600);
  
  while (!fona.begin(fonaSS) && (errorct < fonatries)) {
    Serial.println(F("StrtFona"));
    errorct++;
  }
  
  type = fona.type();
}

bool netStatus() {
  int n = fona.getNetworkStatus();
  if (!(n == 1 || n == 5)) return false;
  else return true;
}

void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) {
    Serial.println(F("StConn"));
    return;
  }
  Serial.println(F("Ming"));

  int retrycon = 0;
  while ((ret = mqtt.connect()) != 0 && (retrycon < 1)) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("Dis"));
    retrycon++;
    mqtt.disconnect();
    Serial.print(F("ReConn")); Serial.println(retrycon);
    delay(5000);  // wait 5 seconds
  }
  
  Serial.println(F("MCon"));
}

void MQTT_publish_checkSuccess(Adafruit_MQTT_Publish &feed, const char *feedContent) {
  Serial.println(F("TrPub"));
  if (!feed.publish(feedContent)) {
    Serial.println(F("NoPub"));
  }
}
