#pragma once
#include <array>
#include <assert.h>
#include <stdint.h>

namespace onewiretemp {

using Address = std::array<uint8_t, 8>;

class OneWireDevice {
public:
  OneWireDevice() {}
  OneWireDevice(Address address) : address(address) {}
  Address address;
};

enum class PowerMode {
  UNKNOWN = 0,
  PARASITIC = 1,
  EXTERNAL = 2,
};

enum class Error {
  NO_ERROR = 0,
  ONE_WIRE = 1,
  CHECKSUM = 2,
  UKNNOWN_POWER_MODE = 3,
  UKNNOWN_RESOLUTION = 4,
  VERIFICATION_FAILED = 5,
  UNEXPECTED_RESOLUTION = 6,
};

/// commands
const uint8_t SEARCH_ROM = 0xF0;
const uint8_t READ_ROM = 0x33;
const uint8_t MATCH_ROM = 0x55;
const uint8_t SKIP_ROM = 0xCC;
const uint8_t ALARM_SEARCH = 0xEC;
const uint8_t CONVERT_T = 0x44;
const uint8_t WRITE_SCRATCHPAD = 0x4E;
const uint8_t READ_SCRATCHPAD = 0xBE;
const uint8_t COPY_SCRATCHPAD = 0x48;
const uint8_t RECALL = 0xB8;
const uint8_t READ_POWER_SUPPLY = 0xB4;

/// common properties and methods shared between single sensors and networks of
/// sensors
struct OneWireTempSensorCommon {
  PowerMode power_mode;
  uint8_t resolution;

  /// get the delay in ms required for the conversion
  Error get_delay(int &delay_ms) {
    if (resolution == 0)
      return Error::UKNNOWN_RESOLUTION;
    switch (resolution) {
    case 9:
      delay_ms = 94;
      break;
    case 10:
      delay_ms = 188;
      break;
    case 11:
      delay_ms = 375;
      break;
    case 12:
      delay_ms = 750;
      break;
    }
    return Error::NO_ERROR;
  }

  /// wait for conversion to complete; in parasitic mode this is a delay; in
  /// external power mode this polls the device
  template <typename OW, typename Delay>
  Error wait_for_conversion(OW ow, Delay delay) {
    if (power_mode == PowerMode::PARASITIC) {
      int delay_t;
      Error e = get_delay(delay_t);
      if (e != Error::NO_ERROR)
        return e;
      delay(delay_t);
    } else if (power_mode == PowerMode::EXTERNAL) {
      while (!ow.read_bit())
        ;
    } else
      return Error::UKNNOWN_POWER_MODE;

    return Error::NO_ERROR;
  }
};

/// Representation of a single temperature sensor.
class OneWireTempSensor : public OneWireDevice, public OneWireTempSensorCommon {
public:
  OneWireTempSensor() {}
  /// Construct for a given address.
  ///
  /// If the power mode and/or resolution is already known these can be provided
  /// to avoid having to probe or configure. If they are not provided, then
  /// call either probe() or probe_power_mode and configure() before conversion
  OneWireTempSensor(Address address, PowerMode power_mode = PowerMode::UNKNOWN,
                    uint8_t resolution = 0)
      : OneWireDevice(address), OneWireTempSensorCommon{power_mode,
                                                        resolution} {}

  /// is a device with the given address compatible with this class?
  static bool is_compatible(const Address &address) {
    uint8_t family = address[0];
    return family == 0x10 || family == 0x22 || family == 0x28;
  }

  /// select this device and send a command. If needs_power, the line will be
  /// held high if it's powered parasitically
  template <typename OW>
  Error send_addressed_command(OW ow, uint8_t command,
                               bool needs_power = false) {
    if (needs_power && power_mode == PowerMode::UNKNOWN)
      return Error::UKNNOWN_POWER_MODE;

    if (!ow.reset())
      return Error::ONE_WIRE;
    ow.select(address.data());
    ow.write(command, needs_power && power_mode == PowerMode::PARASITIC);

    return Error::NO_ERROR;
  }

  /// read the scratchpad register
  template <typename OW>
  Error read_scratchpad(OW ow, std::array<uint8_t, 8> &scratchpad) {
    send_addressed_command(ow, READ_SCRATCHPAD);

    for (size_t i = 0; i < scratchpad.size(); i++) {
      scratchpad[i] = ow.read();
    }

    uint8_t read_crc = ow.read();
    if (OW::crc8(scratchpad.data(), 8) != read_crc)
      return Error::CHECKSUM;

    return Error::NO_ERROR;
  }

  /// read the power mode
  template <typename OW> Error probe_power_mode(OW ow) {
    send_addressed_command(ow, READ_POWER_SUPPLY);
    power_mode = ow.read_bit() ? PowerMode::EXTERNAL : PowerMode::PARASITIC;
    return Error::NO_ERROR;
  }

  uint8_t get_resolution(std::array<uint8_t, 8> &scratchpad) {
    return 9 + ((scratchpad[4] >> 5) & 3);
  }

  /// read the power mode and configured resolution
  template <typename OW> Error probe(OW ow) {
    Error e;
    e = probe_power_mode(ow);
    if (e != Error::NO_ERROR)
      return e;

    std::array<uint8_t, 8> scratchpad;
    e = read_scratchpad(ow, scratchpad);
    if (e != Error::NO_ERROR)
      return e;

    resolution = get_resolution(scratchpad);
    return Error::NO_ERROR;
  }

  /// set the resolution, and write it to the eeprom
  template <typename OW, typename Delay>
  Error configure(OW ow, Delay delay, uint8_t new_resolution) {
    if (power_mode == PowerMode::UNKNOWN)
      return Error::UKNNOWN_POWER_MODE;

    uint8_t config = ((new_resolution - 9) << 5) | 0x1f;
    uint8_t data[3] = {75, 70, config};

    Error e;
    e = send_addressed_command(ow, WRITE_SCRATCHPAD);
    if (e != Error::NO_ERROR)
      return e;
    ow.write_bytes(data, 3);

    e = send_addressed_command(ow, COPY_SCRATCHPAD, true);
    if (e != Error::NO_ERROR)
      return e;
    if (power_mode == PowerMode::PARASITIC) {
      delay(10);
      ow.depower();
    } else
      while (!ow.read_bit())
        ;

    std::array<uint8_t, 8> scratchpad;
    e = read_scratchpad(ow, scratchpad);
    if (e != Error::NO_ERROR)
      return e;

    for (size_t i = 0; i < 3; i++)
      if (scratchpad[i + 2] != data[i])
        return Error::VERIFICATION_FAILED;

    resolution = new_resolution;

    return Error::NO_ERROR;
  }

  /// start the conversion
  template <typename OW> Error start_conversion(OW ow) {
    if (power_mode == PowerMode::UNKNOWN)
      return Error::UKNNOWN_POWER_MODE;

    return send_addressed_command(ow, CONVERT_T, true);
  }

  /// read the scratchpad and extract the temperature
  template <typename OW> Error read_temperature(OW ow, float &temp_c) {
    Error e;
    std::array<uint8_t, 8> scratchpad;
    e = read_scratchpad(ow, scratchpad);
    if (e != Error::NO_ERROR)
      return e;

    if (resolution != 0 && get_resolution(scratchpad) != resolution)
      return Error::UNEXPECTED_RESOLUTION;

    uint8_t masks[4] = {0x7, 0x3, 0x1, 0x0};
    uint8_t mask = masks[(scratchpad[4] >> 5) & 3];

    uint16_t temp_2c = ((uint16_t)scratchpad[1] << 8) + (scratchpad[0] & ~mask);

    uint16_t msb = 1 << 15;
    int16_t temp = -(int16_t)(temp_2c & msb) + (temp_2c & ~msb);

    temp_c = 0.0625f * temp;
    return Error::NO_ERROR;
  }

  /// start a conversion, wait for it to complete, and read the result
  template <typename OW, typename Delay>
  Error convert_and_read_temperature(OW ow, Delay delay, float &temp_c) {
    Error e;

    e = start_conversion(ow);
    if (e != Error::NO_ERROR)
      return e;

    e = wait_for_conversion(ow, delay);
    if (e != Error::NO_ERROR)
      return e;

    return read_temperature(ow, temp_c);
  }
};

/// Utility class to perform simultaneous conversion of multiple sensors
class OneWireTempSensors : public OneWireTempSensorCommon {
public:
  /// If provided, power_mode must be PARASITIC if any devices are parasitic,
  /// and resolution must be greater than or equal to the resolution of all
  /// devices on the bus. Otherwise, just use call configure() on all probed
  /// devices.
  OneWireTempSensors(PowerMode power_mode = PowerMode::UNKNOWN,
                     uint8_t resolution = 0)
      : OneWireTempSensorCommon{power_mode, resolution} {}

  /// set up to be compatible with a device of a given power mode and resolution
  void configure(PowerMode device_power_mode, uint8_t device_resolution) {
    if (power_mode == PowerMode::UNKNOWN ||
        device_power_mode == PowerMode::PARASITIC)
      power_mode = PowerMode::PARASITIC;
    if (resolution == 0 || device_resolution >= resolution)
      resolution = device_resolution;
  }

  /// set up to be compatible with a device
  void configure(const OneWireTempSensor &sensor) {
    configure(sensor.power_mode, sensor.resolution);
  }

  /// start conversion on all devices
  template <typename OW> Error start_conversion(OW ow) {
    if (!ow.reset())
      return Error::ONE_WIRE;

    ow.write(SKIP_ROM);
    ow.write(CONVERT_T, power_mode == PowerMode::PARASITIC);
    return Error::NO_ERROR;
  }
};

} // namespace onewiretemp
