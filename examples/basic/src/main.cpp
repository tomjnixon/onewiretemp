#include "onewiretemp.hpp"
#include <OneWire.h>

namespace owt = onewiretemp;

// pins and addresses; these need changing to replicate this!
const int pin_data = 8;
owt::Address address = {0x22, 0xB, 0xEA, 0x61, 0x0, 0x0, 0x0, 0x15};

OneWire ow(pin_data);
owt::OneWireTempSensor owts(address);

void show_addresses() {
  ow.reset_search();
  owt::Address addr;
  while (ow.search(addr.data())) {
    Serial.print("found {");
    for (size_t i = 0; i < addr.size(); i++) {
      if (i)
        Serial.print(", ");
      Serial.print("0x");
      Serial.print(addr[i], HEX);
    }
    Serial.println("}");
  }
}

void setup() {
  Serial.begin(115200);

  { // power on the sensors
    const int pin_power_1 = 9;
    const int pin_power_2 = 7;
    const int pin_pull_up = 10;

    pinMode(pin_power_1, OUTPUT);
    digitalWrite(pin_power_1, 1);
    pinMode(pin_power_2, OUTPUT);
    digitalWrite(pin_power_2, 1);

    pinMode(pin_pull_up, OUTPUT);
    digitalWrite(pin_pull_up, 1);

    delay(10);
  }

  show_addresses();

  // probe the sensor to figure out power mode and resolution
  owt::Error e = owts.probe(ow);
  if (e != owt::Error::NO_ERROR) {
    Serial.println("error probing");
    while (1) ;
  }
}

void loop() {
  // read and check for errors
  float temp_c;
  owt::Error e = owts.convert_and_read_temperature(ow, delay, temp_c);

  if (e != owt::Error::NO_ERROR)
    Serial.println("error reading");
  else {
    Serial.print("temp = ");
    Serial.print(temp_c);
    Serial.println("C");
  }

  delay(2000);
}
