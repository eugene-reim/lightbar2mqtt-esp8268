#include <PubSubClient.h>
#include <WiFi.h>

#include "constants.h"
#include "lightbar.h"
#include "remote.h"

#ifndef MQTT_H
#define MQTT_H

class Remote;
class Lightbar;

class MQTT
{
public:
    MQTT(WiFiClient *wifiClient, const char *mqttServer, int mqttPort, const char *mqttUser, const char *mqttPassword, const char *mqttRootTopic, bool homeAssistantAutoDiscovery, const char *homeAssistantAutoDiscoveryPrefix);
    ~MQTT();
    void setup();
    void loop();
    bool addLightbar(Lightbar *lightbar);
    bool removeLightbar(Lightbar *lightbar);
    bool addRemote(Remote *remote);
    bool removeRemote(Remote *remote);
    void onMessage(char *topic, byte *payload, unsigned int length);
    void sendAction(Remote *remote, byte command, byte options);
    const String getCombinedRootTopic();
    const String getClientId();

private:
    WiFiClient *wifiClient;
    PubSubClient *client;
    String clientId;
    Lightbar *lightbars[constants::MAX_LIGHTBARS];
    int lightbarCount = 0;
    Remote *remotes[constants::MAX_REMOTES];
    int remoteCount = 0;
    const char *mqttServer;
    int mqttPort = 1883;
    const char *mqttUser = "";
    const char *mqttPassword = "";
    String mqttRootTopic = "lightbar2mqtt";
    bool homeAssistantDiscovery = true;
    String homeAssistantDiscoveryPrefix = "homeassistant";

    String combinedRootTopic;
    std::function<void(Remote *, byte, byte)> remoteCommandHandler;

    void sendAllHomeAssistantDiscoveryMessages();
    void sendHomeAssistantLightbarDiscoveryMessages(Lightbar *lightbar);
    void sendHomeAssistantRemoteDiscoveryMessages(Remote *remote);
};

#endif