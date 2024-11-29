#include <PubSubClient.h>
#include <WiFi.h>
#include "lightbar.h"

#ifndef MQTT_H
#define MQTT_H

class MQTT
{
public:
    MQTT(Lightbar *lightbar, const char *wifiSsid, const char *wifiPassword, const char *mqttServer, int mqttPort, const char *mqttUser, const char *mqttPassword, const char *mqttRootTopic, bool homeAssistantAutoDiscovery, const char *homeAssistantAutoDiscoveryPrefix, const char *homeAssistantDeviceName);
    ~MQTT();
    void setup();
    void loop();
    void onMessage(char *topic, byte *payload, unsigned int length);
    void sendAction(uint32_t serial, byte command, byte options);

private:
    WiFiClient *wifiClient;
    PubSubClient *client;
    String clientId;
    Lightbar *lightbar;
    const char *wifiSsid;
    const char *wifiPassword;
    const char *mqttServer;
    int mqttPort = 1883;
    const char *mqttUser = "";
    const char *mqttPassword = "";
    String mqttRootTopic = "lightbar2mqtt";
    bool homeAssistantDiscovery = true;
    String homeAssistantDiscoveryPrefix = "homeassistant";
    String homeAssistantDeviceName = "Mi Computer Monitor Light Bar";

    void setupWifi();
    void setupMqtt();
    void sendHomeAssistantDiscoveryMessages();
};

#endif