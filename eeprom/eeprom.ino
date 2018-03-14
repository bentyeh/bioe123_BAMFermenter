#include <EEPROM.h>

/** the current address in the EEPROM (i.e. which byte we're going to read from next) **/
int lastaddr;

unsigned int time_min;
byte heat_set;
byte stir_set;
byte air_set;
byte fan_set;
int od;
int purple;
int temp;

String START_FLAG = "start";
String END_FLAG = "end";

const int totalsize = sizeof(time_min)
                    + sizeof(heat_set)
                    + sizeof(stir_set)
                    + sizeof(air_set)
                    + sizeof(fan_set)
                    + sizeof(od)
                    + sizeof(purple)
                    + sizeof(temp);

void setup() {
  // initialize serial
  Serial.begin(9600);

  // wait for serial port to connect. Needed for native USB port only
  while (!Serial) {
    ;
  }

  // write test data to EEPROM
  /*for(int i = 2; i < EEPROM.length(); i++) {
    EEPROM.write(i, i & int_mask);
    EEPROM.update(0, i);
  }*/

  EEPROM.get(0, lastaddr);
  lastaddr = min(lastaddr, EEPROM.length());
}

void loop() {
  // read a byte from the current address of the EEPROM
  int addr = sizeof(lastaddr);
  Serial.println(lastaddr);

  float real_temp;

  // print start flag
  Serial.println(START_FLAG);

  // print header
  Serial.print("time (min), ");
  Serial.print("heat, ");
  Serial.print("stir, ");
  Serial.print("air, ");
  Serial.print("fan, ");
  Serial.print("od, ");
  Serial.print("purple, ");
  Serial.println("temp");


  // loop until reading last value
  while(addr < lastaddr - totalsize) {
    // read values
    EEPROM.get(addr, time_min);
    addr += sizeof(time_min);
    EEPROM.get(addr, heat_set);
    addr += sizeof(heat_set);
    EEPROM.get(addr, stir_set);
    addr += sizeof(stir_set);
    EEPROM.get(addr, air_set);
    addr += sizeof(air_set);
    EEPROM.get(addr, fan_set);
    addr += sizeof(fan_set);
    EEPROM.get(addr, od);
    addr += sizeof(od);
    EEPROM.get(addr, purple);
    addr += sizeof(purple);
    EEPROM.get(addr, temp);
    addr += sizeof(temp);

    // print values
    Serial.print(time_min); Serial.print(" ");
    Serial.print(heat_set); Serial.print(" ");
    Serial.print(stir_set); Serial.print(" ");
    Serial.print(air_set); Serial.print(" ");
    Serial.print(fan_set); Serial.print(" ");
    Serial.print(od); Serial.print(" ");
    Serial.print(purple); Serial.print(" ");
    Serial.print(realtemp(temp)); Serial.println(" ");
  }

  // print end flag
  Serial.println(END_FLAG);
}

double realtemp(int raw_temp) {
  return ((double) raw_temp - 18.2) / 9.15;
}
