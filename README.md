C++ library for communicating with 1-wire temperature sensors.

Should be compatible with:

- DS18S20
- DS1822
- DS18B20

which all use the same protocol.

I wrote this while working on a low-power project, after finding that other
libraries for these sensors didn't meet my requirements. Compared to other
libraries, this:

- Is not coupled to a particular platform; you should be able to port this to
  run anywhere without modifying it.

- Implements lots of error checking; this will generally return an error rather
  than an invalid reading.

- Supports useful things like mixed power modes, simultaneous conversion with
  multiple sensors, and going into sleep mode while conversion is running, but
  omits alarm functionality.

- Tries to minimise bus activity; if you already know how your sensor is
  configured auto-configuration can be skipped.

## Platform Notes

### Arduino

Requires [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL) (just for
`array`) and works with [OneWire](https://github.com/PaulStoffregen/OneWire).
To get these, add this line to `platformio.ini`:

    lib_deps = https://github.com/mike-matera/ArduinoSTL.git, https://github.com/PaulStoffregen/OneWire.git

## API

Documentation coming soon; for now, see the examples, tests and code.

## License

    Copyright 2020 Thomas Nixon

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
