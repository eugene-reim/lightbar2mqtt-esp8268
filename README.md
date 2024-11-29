# lightbar2mqtt

Control your [Xiaomi Mi Computer Monitor Light Bar](https://www.mi.com/global/product/mi-computer-monitor-light-bar/) with MQTT and add it to Home Assistant! All you need is a ESP32, a nRF24 module and a light bar of course.

## Acknowledgements

This project is heavily based on the amazing reverse engineering work done by [lamperez](https://github.com/lamperez). Take a look at their [repository](https://github.com/lamperez/xiaomi-lightbar-nrf24) for more information on the protocol used, the reverse engineering process and a Python version running on Raspberry Pi. It is licensed under the GNU General Public License v3.0.

I also took some inspiration from [Ben Allen](https://github.com/benallen-dev) and their [repository](https://github.com/benallen-dev/xiaomi-lightbar), licensed under the MIT license.

This project would not have been possible without the work of these amazing people! Thank you!

## Features

- Control power state, brightness and color temperature via simple MQTT messages
- Receive state updates of the remote via MQTT as well
- Either use the controller aditionally or decouple light bar and remote to gain full control
- Integrates with Home Assistant – either manually or with zero effort using the automatic discovery feature
  - The light bar is represented as a `light` entity
  - The remote is represented as a `sensor` entity
  - Actions taken on the remote also trigger `device_automation`s. This allows you to trigger automations in Home Assistant based on actions taken on the remote

## Requirements

- a Xiaomi Mi Computer Monitor Light Bar, Model MJGJD**01**YL (without BLE/WiFi). The MJGJD**02**YL will not work!
- an ESP32
- a nRF24 module (tested with nRF24L01)

## Installation

### 1. Hardware

Connect the nRF24 module to the ESP32 as follows. At least these pins work for my combination of ESP32 and nRF24 module. You can change the CE & CSN pins in the `config.h` file if you need to. The SPI pins (SCK, MOSI, MISO) might be different on your ESP32 – check the pinout of your ESP32!

| nRF24 |  ESP32 |
| :---- | -----: |
| VCC   |    3V3 |
| GND   |    GND |
| CE    |  Pin 4 |
| CSN   |  Pin 5 |
| SCK   | Pin 18 |
| MOSI  | Pin 23 |
| MISO  | Pin 19 |

### 2. Software

1. Clone this repository
2. Copy the `config-example.h` file to `config.h` and adjust the settings to your needs.
3. Connect your ESP32 to your computer.
4. Open the Arduino IDE and install the required libraries:
   - [Arduino_JSON](https://github.com/arduino-libraries/Arduino_JSON) by Arduino, _Version 0.2.0_
   - [CRC](https://github.com/RobTillaart/CRC) by Rob Tillaart, _Version 1.0.3_
   - [PubSubClient](https://pubsubclient.knolleary.net/) by Nick O'Leary, _Version 2.8_
   - [RF24](https://nrf24.github.io/RF24/) by TMRh20, _Version 1.4.10_
5. Select your serial port and board. Upload the sketch to your ESP32.
6. Inspect the serial monitor (115200 baud). If everything is set up correctly, the ESP32 should connect to your WiFi and MQTT broker.
7. At this point, you probably don't know the serial of your remote. Just press/turn the remote and you should see the serial in the serial monitor. (E.g. `[Radio] Ignoring package with not matching serial: 0x7B7E12`) Copy it and paste it into the `config.h` file.
8. Upload the sketch again.
9. If your Home Assistant has the MQTT integration set up, the light bar should be discovered automatically.
10. Enjoy controlling your light bar via MQTT!

### 3. Pairing Light bar and ESP32 (optional)

If you opt to use different values for `INCOMING_SERIAL` and `OUTGOING_SERIAL` in the `config.h` file, you need to pair the light bar with the ESP32. To do this, power-cycle the light bar and within 10 seconds either press the "Pair" button in Home Assistant or send a message to the pair topic (see below). The light bar should blink a few times if the pairing was successful.

## Usage

### MQTT Topics

All MQTT topics are prefixed with a root topic. You can set this root topic in the `config.h` file. The default root topic is `lightbar2mqtt`. The root topic is followed by the MAC address of your ESP32 in the format `l2m_<MAC of your ESP32>`, e.g. `l2m_1234567890AB`. Just take a look at the serial monitor of your ESP32 to find out the used root topic and MAC address, it will be printed right after the startup message:

```text
...
[MQTT] Device ID: l2m_1234567890AB
[MQTT] Root Topic: lightbar2mqtt/l2m_12345679890AB
...
```

#### Light Bar

To control the light bar, you can send messages to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP32>/lightbar/command` e.g. `lightbar2mqtt/l2m_1234567890AB/lightbar/command`. The payload should be a JSON object with the following keys:

- `state`: `"ON"` or `"OFF"`
- `brightness`: `0` (off) to `15` (full brightness)
- `color_temp`: Mireds – `153` (cold) to `370` (warm)

Example:

```json
{
  "state": "ON",
  "brightness": 15,
  "color_temp": 153
}
```

#### Remote

The remote sends its state to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP32>/remote/state` e.g. `lightbar2mqtt/l2m_1234567890AB/remote/state`. The payload is a plain string with one the following values:

- `press`
- `turn_clockwise`
- `turn_counterclockwise`
- `hold`
- `press_turn_clockwise`
- `press_turn_counterclockwise`

#### Pairing

To pair the light bar with the ESP32, send a message to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP32>/pair` e.g. `lightbar2mqtt/l2m_1234567890AB/pair`. The payload can be anything, it will be ignored.

Please note that the light bar needs to be power-cycled within 10 seconds _before_ sending the pairing message.

#### Availability

The ESP32 sends its availability to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP32>/availability` e.g. `lightbar2mqtt/l2m_1234567890AB/availability`. The payload is either `online` or `offline`.

### Home Assistant

If your Home Assistant has the MQTT integration set up, the light bar should be discovered automatically.

Additionally, the above mentioned events from the remote are also available as `device_automation` triggers. You can use these triggers to create automations in Home Assistant based on the actions taken on the remote. To do so, create a new automation in Home Assistant and select "Device" as the trigger type. Select the light bar entity and the desired trigger, e.g. `"press" action`.

### Known Issues / Limitations

- The light bar does not send its state to the ESP32. This means that if you change the state of the light bar via the controller, the ESP32 will not know about it. This is a limitation of the protocol used by the light bar.
- There is no way of knowing whether the light bar is currently on or off. Therefore this project assumes that the light bar is on when the ESP32 starts. If you turn off the light bar (e.g. via the remote), this might lead to an inverted state in Home Assistant. Just turn the device on in Home Assistant and power-cycle the light bar to fix this.
- Sometimes actions taken on the remote are not recognized by the ESP32. When building automations in Home Assistant, don't rely on the remote events to be 100% accurate. Normally, the second or third try should work.

## Contributing

If you find a bug or have an idea for a new feature, feel free to open an issue or create a pull request. I'm happy to see this project grow and improve!

I've designed the code to be easily™ extendable. For example, it should be relatively easy to add support for multiple light bars or multiple remotes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
