.. cpp:namespace:: onewiretemp

User Guide
----------

The main class to look at is :class:`OneWireTempSensor`, which
represents a single sensor on a 1-wire bus, with an address and some
configuration. You should have one of these per sensor which you want to read,
initialised with the address, e.g.:

.. code-block:: cpp

    using owt = onewiretemp;
    owt::Address address = {0x22, 0xB, 0xEA, 0x61, 0x0, 0x0, 0x0, 0x15};
    owt::OneWireTempSensor sensor(address);

Initialisation
~~~~~~~~~~~~~~
.. cpp:namespace-push:: OneWireTempSensor

To initialise this you should call :func:`probe` to figure out the current
power mode and resolution, or perhaps call :func:`probe_power_mode` and
:func:`configure` to configure the resolution.

.. code-block:: cpp

    owt::Error e = owts.probe(ow);
    if (e != owt::Error::NO_ERROR)
        abort();

Note that:

- methods generally return :enum:`Error`, indicating if an error
  occurred; make sure to check this!

- methods generally take (templated) parameters ``ow``, which refers to an
  ``OneWire`` instance (or something else with the same methods) and ``delay``
  which should refer to a function which delays for a given number of ms, like
  ``delay`` in Arduino.

Reading
~~~~~~~

To read the temperature, call
:func:`convert_and_read_temperature`:

.. code-block:: cpp

    float temp_c;
    owt::Error e = owts.convert_and_read_temperature(ow, delay, temp_c);
    if (e != owt::Error::NO_ERROR)
        abort();

.. cpp:namespace-pop::

Advanced Stuff
~~~~~~~~~~~~~~

You can use :class:`OneWireTempSensors` to read from multiple
temperature sensors simultaneously.

.. cpp:namespace-push:: OneWireTempSensors

To initialise, construct, and probe or configure all of your temperature
sensors, as in `Initialisation`_, then call :func:`configure` on each of them:

.. code-block:: cpp

    std::vector<owt::OneWireTempSensor> sensors;
    // populate sensors here

    owt::OneWireTempSensors all_sensors;
    for (auto &sensor : sensors)
        all_sensors.configure(sensor);

To read from all sensors, call :func:`start_conversion` and
:func:`wait_for_conversion() <OneWireTempSensorCommon::wait_for_conversion>` on
the :class:`OneWireTempSensors` object, then
:func:`read_temperature() <OneWireTempSensor::read_temperature>` on each of the sensors. This saves
time because the address of each sensor does not have to be sent, and all
sensors convert at once.

.. code-block:: cpp

    if (bus.start_conversion(ow) != owt::Error::NO_ERROR)
        abort();
    if (bus.wait_for_conversion(ow, delay) != owt::Error::NO_ERROR)
        abort();

    for (auto &sensor : sensors) {
        float temp_c;
        if (sensor.read_temperature(ow, temp_c) != owt::Error::NO_ERROR)
            abort();
        // do something with temp_c
    }

.. cpp:namespace-pop::
