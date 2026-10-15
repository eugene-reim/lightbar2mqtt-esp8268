#include <ESP8266WiFi.h>
#include <SPI.h>
#include <time.h>

#include "constants.h"
#include "config.h"
#include "config_defaults.h"
#include "radio.h"
#include "lightbar.h"
#include "mqtt.h"

WiFiClient wifiClient;
Radio radio(RADIO_PIN_CE, RADIO_PIN_CSN);
MQTT mqtt(&wifiClient, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_ROOT_TOPIC, HOME_ASSISTANT_DISCOVERY, HOME_ASSISTANT_DISCOVERY_PREFIX);

void printLocalTime()
{
  time_t now = time(nullptr);
  struct tm timeinfo;
  if (now < 8 * 3600 * 2 || !localtime_r(&now, &timeinfo))
  {
    Serial.println("[Time] Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "[Time] %A, %B %d %Y %H:%M:%S");
}

void setupWifi()
{
  Serial.print("[WiFi] Connecting to network \"");
  Serial.print(WIFI_SSID);
  Serial.print("\"...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setHostname(mqtt.getClientId().c_str());

  uint retries = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    retries++;
    if (retries > 60)
      ESP.restart();
  }
  Serial.println();
  Serial.println("[WiFi] connected!");

  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());

  // init and get the time
#if defined(GMT_OFFSET_SEC) && defined(DST_OFFSET_SEC) && defined(NTP_SERVER)
  Serial.println("[Time] Syncing time with NTP server...");
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  retries = 0;
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2 && retries < 20)
  {
    delay(500);
    now = time(nullptr);
    retries++;
  }
  printLocalTime();
#else
  Serial.println("[Time] NTP server not configured, skipping time sync.");
#endif
}

void setup()
{
  Serial.begin(115200);
  Serial.println("##########################################");
  Serial.println("# LIGHTBAR2MQTT            (Version " + constants::VERSION + ") #");
  Serial.println("# https://github.com/ebinf/lightbar2mqtt #");
  Serial.println("##########################################");

  // ESP8266 hardware SPI pins are fixed: SCK=GPIO14 (D5), MISO=GPIO12 (D6), MOSI=GPIO13 (D7)
  SPI.begin();

  radio.setup();

  setupWifi();

  for (int i = 0; i < sizeof(REMOTES) / sizeof(SerialWithName); i++)
  {
    Remote *remote = new Remote(&radio, REMOTES[i].serial, REMOTES[i].name);
    mqtt.addRemote(remote);
  }

  for (int i = 0; i < sizeof(LIGHTBARS) / sizeof(SerialWithName); i++)
  {
    Lightbar *lightbar = new Lightbar(&radio, LIGHTBARS[i].serial, LIGHTBARS[i].name);
    mqtt.addLightbar(lightbar);
  }

  mqtt.setup();
}

void loop()
{
  if (!WiFi.isConnected())
  {
    Serial.println("[WiFi] connection lost!");
    setupWifi();
  }

  mqtt.loop();
  radio.loop();
}
