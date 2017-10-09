
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <Wire.h>
#include <Timezone.h>
#include <TM1637Display.h>
#include <Adafruit_LEDBackpack.h>
#include <SoftwareSerial.h>


TimeChangeRule myDST = {"MESZ", Last, Sun, Mar, 2, +120};
TimeChangeRule mySDT = {"MEZ", Last, Sun, Oct, 3, +60};
Timezone DE(myDST, mySDT);


TimeChangeRule *tcr;
time_t utc, local;

#define TM_CLK 2
#define TM_DIO 3

int pinAM = 4;
int pinBM = 5;
int pinBMLast;
int pinBMval;

int pinAH = 6;
int pinBH = 7;
int pinAHLast;
int pinAHval;

int ahour;
int amin;

unsigned long alarmMillis;
unsigned long actualAlarmMillis = 0;
unsigned long oldMillis;

int i = 0;

uint16_t dfCMD[10] {0x7E, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};

//initialize the DFPlayer (10 RX, 11 TX)
SoftwareSerial dfPlayer(10, 11);

//initialize the alarm display
TM1637Display alarmDisplay(TM_CLK, TM_DIO);

//initilize a variable for the clock display
Adafruit_7segment clockDisplay = Adafruit_7segment();

void setup() {

  Serial.begin(9600);
  Serial.setTimeout(0);

  //initilize the DFPlayer
  dfPlayer.begin(9600);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (dfPlayer.isListening())
    Serial.println("DFPlayer is online!");

  //initilize the clock display
  clockDisplay.begin(0x70);

  //set to max brightness
  alarmDisplay.setBrightness(5, true);
  clockDisplay.setBrightness(16);

  //set the pin configuration of the rotary encoder
  pinMode(pinAM, INPUT);
  pinMode(pinBM, INPUT);
  pinMode(pinAH, INPUT);
  pinMode(pinBH, INPUT);
  pinBMLast = digitalRead(pinBM);
  pinAHLast = digitalRead(pinAH);

  //Sync the internal clock
  setSyncProvider(RTC.get);
  setSyncInterval(3600); //sync every hour
  if (timeStatus() != timeSet)
    Serial.println("unable to sync with the RTC!");
  else
    Serial.println("synchronized with the RTC!");

  displayPrintAlarm();

  DFsendCommand(0x09, 0x00, 0x02);
  DFsendCommand(0x06, 0x00, 0x12);
  DFsendCommand(0x07, 0x00, 0x00);
  DFsendCommand(0x0D, 0x00, 0x00);
}

void loop() {

  if (Serial.available()) {
    //input the Alarm-Time (Serial)
    String serialString = Serial.readString();
    processSyncMessage(serialString);
  }

  pinBMval = digitalRead(pinBM);
  if (pinBMval != pinBMLast) { //the encoder is rotating
    if (digitalRead(pinAM) == pinBMval)//clockwise, because A changed first
      amin++;
    else
      amin--;
    pinBMLast = pinBMval;
    alarmMillis = millis();
    //print ALARM time (Display)
    displayPrintAlarm();
  }

  pinAHval = digitalRead(pinAH);
  if (pinAHval != pinAHLast) {
    //the encoder is rotating
    if (digitalRead(pinBH) != pinAHval) {
      //clockwise, because A changed first
      i++;
    }
    else {
      //counter clockwise, because B changed first
      i--;
    }
    pinAHLast = pinAHval;
    alarmMillis = millis();

    if (i%2 == 0) {
      ahour = ahour + (i/2);
      i = 0;
    }
    //print ALARM time (Display)
    displayPrintAlarm();
  }

  if ((millis() - oldMillis) >= 5000) {
    utc = now();
    local = DE.toLocal(utc, &tcr);
    //print clock time (Display)
    clockDisplayPrint();
    if ((millis() - actualAlarmMillis) >= 60000)
      testAlarm();
    oldMillis = millis();

    DFsendCommand(0x0D, 0x00, 0x00);
  }
}

//display the time!
void clockDisplayPrint() {
  //make out of hours and minutes one variable
  int timeValue = hour(local)*100 + minute(local);
  //display the given time with dots
  clockDisplay.print(timeValue, DEC);
  //print zeros when given hour is <10 || <1
  if (timeValue <= 1000)
    clockDisplay.writeDigitNum(0, 0);
  if (timeValue <= 100)
    clockDisplay.writeDigitNum(1, 0);
  clockDisplay.drawColon(true);
  clockDisplay.writeDisplay();
}


/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  'T'   // Header tag for serial time sync message

void processSyncMessage(String serialString) {
  unsigned long pctime;

  if (serialString[0] == TIME_HEADER) {
     serialString.remove(0, 1);
     pctime = serialString.toInt();
     //Serial.println(pctime);
     RTC.set(pctime);
     setTime(pctime);
     Serial.println("Time Set!");
  }
}

void alarmConversion() {
  if (ahour >= 24)
    ahour = 0;
  if (ahour <= -1)
    ahour = 23;

  if (amin >= 60) {
    amin = 0;
    ahour++;
  }
  if (amin <= -1) {
    amin = 59;
    ahour--;
  }
}

//check if the alarm should ring
void testAlarm() {
  //Serial.println("Check for Alarm");
  if (ahour == hour(local) && amin == minute(local) && (millis() - alarmMillis) >= 5000) {
    Serial.println("ALARM!");
    actualAlarmMillis = millis();
  }
}

//display the alarm time
void displayPrintAlarm() {
  alarmConversion();
  alarmDisplay.showNumberDecEx(ahour, 0x10, true, 2, 0); //show alarm hours with dot on the alarm display
  alarmDisplay.showNumberDecEx(amin, 0x10, true, 2, 2); //show alarm minutes with dot on the alarm display
}

//calculate the checksum
void DFchecksum() {
  uint16_t sum = 0;
  for (int j = 1; j <= 6; j++) {
    sum += dfCMD[j];
    //Serial.println(dfCMD[j]);
  }
  //Serial.print("vorher: ");
  //Serial.println(sum);
  sum = (0-sum);

  //Serial.println();
  //Serial.println(sum);
  dfCMD[7] = (uint8_t) (sum >> 8);
  dfCMD[8] = (uint8_t) (sum);
  //Serial.println(dfCMD[7]);
  //Serial.println(dfCMD[8]);
}

void DFsendCommand(int cmd, int para1, int para2) {
  dfCMD[3] = cmd;
  dfCMD[5] = para1;
  dfCMD[6] = para2;
  DFchecksum();
}
