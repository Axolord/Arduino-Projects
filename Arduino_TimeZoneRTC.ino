
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <Wire.h>
#include <Timezone.h>


TimeChangeRule myDST = {"MESZ", Last, Sun, Mar, 2, +120};
TimeChangeRule mySDT = {"MEZ", Last, Sun, Oct, 3, +60};
Timezone DE(myDST, mySDT);

TimeChangeRule *tcr;
time_t utc, local;


void setup() {
  Serial.begin(9600); 
  Serial.setTimeout(0);
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet)
    Serial.println("unable to sync with the RTC!");
  else
    Serial.println("synchronized with the RTC!");
}

void loop() {
  if (Serial.available()) {
    String serialString = Serial.readString();
    
    Serial.println(serialString);
    
    testAlarm(serialString);
    processSyncMessage(serialString);
  }
  Serial.println();
  utc = now();
  printTime(utc, "UTC");
  local = DE.toLocal(utc, &tcr);
  printTime(local, (*tcr).abbrev);
  delay(1000);
}

//Function to print time with time zone
void printTime(time_t t, char *tz) {
  printDigits(hour(t));
  Serial.print(":");
  printDigits(minute(t));
  Serial.print(":");
  printDigits(second(t));
  Serial.print(" ");
  Serial.print(day(t));
  Serial.print(" ");
  Serial.print(month(t));
  Serial.print(" ");
  Serial.print(year(t));
  Serial.print(" ");
  Serial.print(tz);
  Serial.println();
}

void printDigits(int digits) {
  // function to print in format "01:01" instead of "1:1"
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*  code to process time sync messages from the serial port   */
#define TIME_HEADER  "T"   // Header tag for serial time sync message

void processSyncMessage(String serialString) {
  unsigned long pctime;

  if (serialString.indexOf(TIME_HEADER) == 0) {
     pctime = serialString.toInt();
     RTC.set(pctime);
     setTime(pctime);
     Serial.println("Time Set!");
  }
}


#define ALARM_HEADER "A" //Header tag for serial alarm Message

void testAlarm(String serialString) {
  int ahour;
  int amin;
   
  if (serialString.indexOf(ALARM_HEADER) == 0) {
    serialString.remove(0, 1);
    ahour = serialString.toInt();
    amin = serialString.toInt();
    Serial.println(ahour);
    Serial.println(amin);
  }
  //alarm1(ahour, amin);
}

void alarm1(int ahour, int amin) {
  if (ahour == hour(local) && amin == minute(local)) {
    Serial.println("ALARM!");
  }
}


