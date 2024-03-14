# Golioth partner demo with NXP

This repository demonstrates a very bare-bones sensor data reporting
application. It highlights how to use [Zephyr
RTOS](https://www.zephyrproject.org/) with the [Golioth Firmware
SDK](https://github.com/golioth/golioth-firmware-sdk) to report environment
sensor readings to the cloud.

![Golioth weather sensor using an NXP i.MX RT1024 Evaluation
Kit](img/mimxrt_1024_bme280.jpg)

## Golioth

[Golioth](https://golioth.io/) is an instant IoT cloud for firmware devices.
With Golioth, your project will have OTA, Time Series and Stateful data handling,
command and control, remote logging, and many other useful services from day
one.

[Try Golioth now](https://console.golioth.io/), the service is free for
individuals use.

## Setup

Do not clone this repo. Instead, use `west init` to set up a workspace with the
correct dependencies.

### Initial Setup

```
west init -m https://github.com/golioth/iot_weather_fleet.git --mf west-zephyr.yml
west update
```

## Building

Build and flash the project:

```
# NXP i.MX RT1024 EVK (uses west-zephyr.yml)
west build -b mimxrt1024_evk .
west flash
```

## Save Device Credentials to Non-Volatile Storage

Each device needs to be assigned its own PSK-ID and PSK to authenticate with
Golioth. Devices that use WiFi also need an SSID and PSK. These are assigned
(once) via the serial console after the firmware has been flashed and will
persist across future firmware upgrades.

```
uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
uart:~$ settings set golioth/psk <my-psk>
uart:~$ kernel reboot cold
```

## Hardware

- [i.MX RT1024 Evaluation
  Kit](https://www.nxp.com/design/design-center/development-boards/i-mx-evaluation-and-development-boards/i-mx-rt1024-evaluation-kit:MIMXRT1024-EVK)
- [MIKROE Arduino UNO click
  shield](https://www.mikroe.com/arduino-uno-click-shield)
- Bosch BME280 sensor ([MIKROE Weather Click](https://www.mikroe.com/weather-click))

## Features

### Streaming sensor data

Temperature, pressure, and humidity data is collected by this demo and
available on the cloud on with the timestamp when it was received. Here is an
example:

![Temperate, pressure, and humidity data streamed to
Golioth](img/golioth-stream-data.png)

### Device Settings

The Golioth Settings service is used to configure how often the sensor is read
and reported to the cloud. Use the `LOOP_DELAY_S` key to remotely configure the
number of seconds between readings.
