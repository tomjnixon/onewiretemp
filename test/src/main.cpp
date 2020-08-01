#include "onewiretemp.hpp"
#include <OneWire.h>

namespace owt = onewiretemp;

// pins and addresses; these need changing to replicate this!
const int pin_power_1 = 9;
const int pin_power_2 = 7;
const int pin_data = 8;
const int pin_pull_up = 10;
owt::Address addr_1 = {0x22, 0xB, 0xEA, 0x61, 0x0, 0x0, 0x0, 0x15};
owt::Address addr_2 = {0x22, 0xE, 0xA6, 0x61, 0x0, 0x0, 0x0, 0x36};

OneWire ow(pin_data);

void do_check(bool x, const char *expr, int line) {
  if (!x) {
    Serial.print("check(");
    Serial.print(expr);
    Serial.print(") failed at line ");
    Serial.println(line);
    while (1)
      ;
  }
}

#define check(expr) do_check((expr), #expr, __LINE__)

bool power_1 = true;
bool power_2 = true;

void reset() {
  digitalWrite(pin_power_1, 0);
  digitalWrite(pin_power_2, 0);
  digitalWrite(pin_pull_up, 0);
  ow.depower();
  delay(10);
  digitalWrite(pin_power_1, power_1);
  digitalWrite(pin_power_2, power_2);
  digitalWrite(pin_pull_up, 1);
  delay(10);
}

void check_read(const owt::Address &address) {
  reset();
  owt::OneWireTempSensor owts(address);
  check(owts.probe(ow) == owt::Error::NO_ERROR);
  float temp_c;
  auto e = owts.convert_and_read_temperature(ow, delay, temp_c);
  check(e == owt::Error::NO_ERROR);

  check(temp_c < 40 && temp_c > 10);
}

void check_configure(const owt::Address &address, uint8_t resolution) {
  reset();
  {
    owt::OneWireTempSensor owts(address);
    check(owts.probe_power_mode(ow) == owt::Error::NO_ERROR);
    check(owts.configure(ow, delay, resolution) == owt::Error::NO_ERROR);
    float temp_c;
    check(owts.convert_and_read_temperature(ow, delay, temp_c) ==
          owt::Error::NO_ERROR);
    check(temp_c < 40 && temp_c > 10);
    check(owts.resolution == resolution);
  }

  reset();
  {
    owt::OneWireTempSensor owts(address);
    check(owts.probe(ow) == owt::Error::NO_ERROR);
    check(owts.resolution == resolution);
  }
}

void check_multiple() {
  reset();

  std::array<owt::OneWireTempSensor, 2> sensors{addr_1, addr_2};

  for (auto &sensor : sensors)
    check(sensor.probe(ow) == owt::Error::NO_ERROR);

  owt::OneWireTempSensors bus;
  for (auto &sensor : sensors)
    bus.configure(sensor);

  check(bus.start_conversion(ow) == owt::Error::NO_ERROR);
  check(bus.wait_for_conversion(ow, delay) == owt::Error::NO_ERROR);

  for (auto &sensor : sensors) {
    float temp_c;
    check(sensor.read_temperature(ow, temp_c) == owt::Error::NO_ERROR);
    check(temp_c < 40 && temp_c > 10);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(pin_power_1, OUTPUT);
  pinMode(pin_power_2, OUTPUT);
  pinMode(pin_pull_up, OUTPUT);

  reset();

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

  check_read(addr_1);
  check_configure(addr_1, 9);
  check_configure(addr_1, 12);
  check_multiple();

  Serial.println("done external power");

  power_1 = false;
  power_2 = false;

  check_read(addr_1);
  check_configure(addr_1, 9);
  check_configure(addr_1, 12);
  check_multiple();

  Serial.println("done parasitic power");

  power_1 = true;
  power_2 = false;
  check_multiple();

  Serial.println("done mixed power");

  Serial.println("tests passed");
}

void loop() {}
