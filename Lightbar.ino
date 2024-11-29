#include "config.h"
#include "lightbar.h"
#include "mqtt.h"

Lightbar lightbar(INCOMING_SERIAL, OUTGOING_SERIAL, RADIO_PIN_CE, RADIO_PIN_CSN);
MQTT mqtt(&lightbar, WIFI_SSID, WIFI_PASSWORD, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_ROOT_TOPIC, HOME_ASSISTANT_DISCOVERY, HOME_ASSISTANT_DISCOVERY_PREFIX, HOME_ASSISTANT_DEVICE_NAME);

void setup()
{
  Serial.begin(115200);
  Serial.println("##########################################");
  Serial.println("# LIGHTBAR2MQTT            (Version 0.1) #");
  Serial.println("# https://github.com/ebinf/lightbar2mqtt #");
  Serial.println("##########################################");

  lightbar.setup();
  mqtt.setup();
}

void loop()
{
  mqtt.loop();
  lightbar.loop();
}