A test for the library on real hardware.

To run this, you'll need to connect two compatible sensors and tweak the
definitions at the top of `src/main.cpp`.

- Their ground pins should be tied together, and to ground.
- Their data pins should be tied together, and to pin `pin_data`.
- A 4.7kohm resistor should be connected between `pin_data` and `pin_pull_up`.
- The power pin of sensor with address `addr_1` should be connected to
  `pin_power_1`, and `pin_power_2` to `addr_2`.

The addresses of all 1-wire devices on the bus are printed at start-up.
