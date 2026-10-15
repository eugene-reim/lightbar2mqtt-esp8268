# lightbar2mqtt — ESP8266 (original 0.3)

Это сборка [оригинального lightbar2mqtt 0.3](https://github.com/ebinf/lightbar2mqtt) с адаптацией под ESP8266 из [форка eugene-reim](https://github.com/eugene-reim/lightbar2mqtt-esp8268).

Форк отделился от оригинала в декабре 2024 (после reboot-on-MQTT-fail). В оригинале после этого появились: версия 0.3, поддержка «новых» ламп (другие nRF-адреса), публикация состояния в MQTT/HA, кнопка `toggle_internal`, настраиваемые SPI-пины и NTP. Логика форка почти не менялась — другие библиотеки и системные вызовы ESP8266.

Control your [Xiaomi Mi Computer Monitor Light Bar](https://www.mi.com/global/product/mi-computer-monitor-light-bar/) with MQTT and add it to Home Assistant! All you need is a ESP8266, a nRF24 module and a light bar of course.

## Acknowledgements

This project is heavily based on the amazing reverse engineering work done by [lamperez](https://github.com/lamperez). Take a look at their [repository](https://github.com/lamperez/xiaomi-lightbar-nrf24) for more information on the protocol used, the reverse engineering process and a Python version running on Raspberry Pi. It is licensed under the GNU General Public License v3.0.

I also took some inspiration from [Ben Allen](https://github.com/benallen-dev) and their [repository](https://github.com/benallen-dev/xiaomi-lightbar), licensed under the MIT license.

ESP8266 port: [eugene-reim/lightbar2mqtt-esp8268](https://github.com/eugene-reim/lightbar2mqtt-esp8268).

This project would not have been possible without the work of these amazing people! Thank you!

## Features

- Control power state, brightness and color temperature via simple MQTT messages
- Publish assumed on/off state to MQTT (`…/state`) so Home Assistant can follow it
- Button to toggle the internal on/off assumption without sending RF (`…/toggle_internal`)
- Receive state updates of the remote via MQTT as well
- Either use the controller additionally or decouple light bar and remote to gain full control
- Integrates with Home Assistant – either manually or with zero effort using the automatic discovery feature
  - The light bar is represented as a `light` entity
  - The remote is represented as a `sensor` entity
  - Actions taken on the remote also trigger `device_automation`s. This allows you to trigger automations in Home Assistant based on actions taken on the remote
- Use multiple light bars or remotes with ease! Each one can be controlled/monitored individually and will also have separate entities in Home Assistant.
- Optional NTP time sync (helps some routers list the ESP in the client table)

## Requirements

- a Xiaomi Mi Computer Monitor Light Bar, Model MJGJD**01**YL (without BLE/WiFi). The MJGJD**02**YL will not work!
- an ESP8266 (tested with NodeMCU v1.0)
- a nRF24 module (tested with nRF24L01)

## Installation

### 1. Hardware

Connect the nRF24 module to the ESP8266 as follows. At least these pins work for the NodeMCU v1.0 + nRF24 combination from the ESP8266 fork. CE and CSN can be changed in `config.h`. Hardware SPI pins (SCK, MOSI, MISO) are fixed on ESP8266.

| nRF24 | ESP8266 (NodeMCU) | GPIO |
| :---- | ----------------: | ---: |
| VCC   |               3V3 |      |
| GND   |               GND |      |
| CE    |                D2 |    4 |
| CSN   |                D1 |    5 |
| SCK   |                D5 |   14 |
| MOSI  |                D7 |   13 |
| MISO  |                D6 |   12 |

### 2. Software

1. Copy the `config-example.h` file to `config.h` and adjust the settings to your needs.
2. Connect your ESP8266 to your computer.
3. Open the Arduino IDE and install the required libraries:
   - [Arduino_JSON](https://github.com/arduino-libraries/Arduino_JSON) by Arduino, _Version 0.2.0_
   - [CRC](https://github.com/RobTillaart/CRC) by Rob Tillaart, _Version 1.0.3_
   - [PubSubClient](https://pubsubclient.knolleary.net/) by Nick O'Leary, _Version 2.8_
   - [RF24](https://nrf24.github.io/RF24/) by TMRh20, _Version 1.4.10_
4. Board: **NodeMCU 1.0 (ESP-12E Module)** (or your ESP8266 board). Upload the sketch.
5. Inspect the serial monitor (115200 baud). If everything is set up correctly, the ESP8266 should connect to your WiFi and MQTT broker.
6. At this point, you probably don't know the serial of your remote. Just press/turn the remote and you should see the serial in the serial monitor. (E.g. `[Radio] Ignoring package with not matching serial: 0x7B7E12`) Copy it and paste it into the `config.h` file in the "Remotes" section.
7. Upload the sketch again.
8. If your Home Assistant has the MQTT integration set up, the light bar should be discovered automatically.
9. Enjoy controlling your light bar via MQTT!

### 3. Pairing Light bar and ESP8266 (optional)

If you opt to use different values for the serial and remote in the `config.h` file, you need to pair the light bar with the ESP8266. To do this, power-cycle the light bar and within 10 seconds either press the "Pair" button in Home Assistant or send a message to the pair topic (see below). The light bar should blink a few times if the pairing was successful.

## Usage

### MQTT Topics

All MQTT topics are prefixed with a root topic. You can set this root topic in the `config.h` file. The default root topic is `lightbar2mqtt`. The root topic is followed by the MAC address of your ESP8266 in the format `l2m_<MAC of your ESP8266>`, e.g. `l2m_1234567890AB`. Just take a look at the serial monitor of your ESP8266 to find out the used root topic and MAC address, it will be printed right after the startup message:

```text
...
[MQTT] Device ID: l2m_1234567890AB
[MQTT] Root Topic: lightbar2mqtt/l2m_12345679890AB
...
```

In order to be able to control multiple light bars or remotes, each one has their own sub-topics, starting with the chosen serial (all lower case).

#### Light Bar

To control the light bar, you can send messages to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP8266>/0x<Serial of the light bar>/command` e.g. `lightbar2mqtt/l2m_1234567890AB/0xabcdef/command`. The payload should be a JSON object with the following keys:

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

Assumed on/off state is published (retained) to: `<MQTT_ROOT_TOPIC>/l2m_<MAC>/0x<Serial of the light bar>/state`

```json
{ "state": "ON" }
```

#### Toggle internal state

The lamp does not report whether it is on or off. The firmware keeps an internal assumption (defaults to on after boot). If Home Assistant and the lamp get out of sync, publish anything to:

`<MQTT_ROOT_TOPIC>/l2m_<MAC>/0x<Serial of the light bar>/toggle_internal`

This flips the internal flag **without** sending an RF on/off command. In Home Assistant this is the "Toggle Internal State" button.

#### Remote

The remote sends its state to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP8266>/0x<Serial of the remote>/state` e.g. `lightbar2mqtt/l2m_1234567890AB/0x123456/state`. The payload is a plain string with one the following values:

- `press`
- `turn_clockwise`
- `turn_counterclockwise`
- `hold`
- `press_turn_clockwise`
- `press_turn_counterclockwise`

#### Pairing

To pair the light bar with the ESP8266, send a message to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP8266>/0x<Serial of the light bar>/pair` e.g. `lightbar2mqtt/l2m_1234567890AB/0xabcdef/pair`. The payload can be anything, it will be ignored.

Please note that the light bar needs to be power-cycled within 10 seconds _before_ sending the pairing message.

#### Availability

The ESP8266 sends its availability to the following topic: `<MQTT_ROOT_TOPIC>/l2m_<MAC of your ESP8266>/availability` e.g. `lightbar2mqtt/l2m_1234567890AB/availability`. The payload is either `online` or `offline`.

### Home Assistant

If your Home Assistant has the MQTT integration set up, the light bar(s) and remote(s) should be discovered automatically.

Additionally, the above mentioned events from the remote are also available as `device_automation` triggers. You can use these triggers to create automations in Home Assistant based on the actions taken on the remote. To do so, create a new automation in Home Assistant and select "Device" as the trigger type. Select the corresponding remote entity and the desired trigger, e.g. `"press" action`.

### Known Issues / Limitations

- The light bar does not send its state to the ESP8266. This means that if you change the state of the light bar via the controller, the ESP8266 will not know about it. This is a limitation of the protocol used by the light bar.
- There is no way of knowing whether the light bar is currently on or off. Therefore this project assumes that the light bar is on when the ESP8266 starts. If you turn off the light bar (e.g. via the remote), this might lead to an inverted state in Home Assistant. Use **Toggle Internal State**, or turn the device on in Home Assistant and power-cycle the light bar to fix this.
- Sometimes actions taken on the remote are not recognized by the ESP8266. When building automations in Home Assistant, don't rely on the remote events to be 100% accurate. Normally, the second or third try should work. It is therefore also recommended to decouple the light bar and original remote, as otherwise some actions on the remote might change the state of the light bar but not trigger anything in Home Assistant.

## What was merged from original 0.3

- New nRF send/receive addresses (`0x5555555555` / `0xAAAAAAAAAA`) and duplicate-packet handling for newer monitor bars
- MQTT/HA assumed state + `toggle_internal`
- Bugfix: `setOnOff(strcmp(state, "ON") == 0)` (previously inverted)
- Bugfix: `unregisterCommandListener` when removing a remote
- Optional NTP after Wi‑Fi connect
- Version string `0.3`

ESP8266-specific (from the fork, adapted to 0.3):

- `ESP8266WiFi` instead of `WiFi` / `esp_mac`
- `WiFi.macAddress()` for the MQTT client id
- `SPI.begin()` without ESP32-style pin arguments (hardware SPI pins are fixed)
- NTP via `configTime` + `time()` (`getLocalTime` is ESP32-only)
- JSON payload parsing without `String(payload, length)` (not available the same way on ESP8266)
- `std::function` listener unregister without `.target<Signature>()`

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
