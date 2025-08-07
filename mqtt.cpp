#include <Arduino_JSON.h>

#include "mqtt.h"

MQTT::MQTT(WiFiClient *wifiClient, const char *mqttServer, int mqttPort, const char *mqttUser, const char *mqttPassword, const char *mqttRootTopic, bool homeAssistantAutoDiscovery, const char *homeAssistantAutoDiscoveryPrefix)
{
    this->mqttServer = mqttServer;
    this->mqttPort = mqttPort;
    this->mqttUser = mqttUser;
    this->mqttPassword = mqttPassword;
    this->mqttRootTopic = String(mqttRootTopic);
    this->homeAssistantDiscovery = homeAssistantAutoDiscovery;
    this->homeAssistantDiscoveryPrefix = String(homeAssistantAutoDiscoveryPrefix);

    this->remoteCommandHandler = std::bind(&MQTT::sendAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    this->client = new PubSubClient(*wifiClient);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buffer[13]; // 6*2 characters for hex + 1 character for null terminator
    sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    this->clientId = "l2m_" + String(buffer);
    this->combinedRootTopic = this->mqttRootTopic + "/" + this->clientId;
}

MQTT::~MQTT()
{
    delete this->client;
}

const String MQTT::getCombinedRootTopic()
{
    return this->combinedRootTopic;
}

const String MQTT::getClientId()
{
    return this->clientId;
}

void MQTT::onMessage(char *topic, byte *payload, unsigned int length)
{
    Serial.print("[MQTT] New Message (");
    Serial.print(topic);
    Serial.print("): ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    JSONVar command = JSON.parse(String((char*)payload).substring(0, length));

    Lightbar *lightbar = nullptr;
    for (int i = 0; i < this->lightbarCount; i++)
    {
        lightbar = this->lightbars[i];
        if (!strcmp(topic, String(this->getCombinedRootTopic() + "/" + lightbar->getSerialString() + "/pair").c_str()))
        {
            lightbar->pair();
            return;
        }

        if (strcmp(topic, String(this->getCombinedRootTopic() + "/" + lightbar->getSerialString() + "/command").c_str()))
            continue;

        if (JSON.typeof(command) != "object")
            continue;

        if (command.hasOwnProperty("state"))
        {
            const char *state = command["state"];
            lightbar->setOnOff(strcmp(state, "ON"));
        }

        if (command.hasOwnProperty("brightness"))
        {
            lightbar->setBrightness((uint8_t)command["brightness"]);
        }

        if (command.hasOwnProperty("color_temp"))
        {
            lightbar->setMiredTemperature((uint)command["color_temp"]);
        }
    }
}

void MQTT::setup()
{
    Serial.print("[MQTT] Device ID: ");
    Serial.println(this->clientId);
    Serial.print("[MQTT] Root Topic: ");
    Serial.println(this->getCombinedRootTopic());

    this->client->setServer(this->mqttServer, this->mqttPort);
    this->client->setCallback(std::bind(&MQTT::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    uint retries = 0;
    Serial.println("[MQTT] Connecting to MQTT broker...");
    while (!this->client->connected())
    {
        if (!this->client->connect(this->clientId.c_str(), this->mqttUser, this->mqttPassword, String(this->getCombinedRootTopic() + "/availability").c_str(), 1, true, "offline"))
        {
            Serial.print("[MQTT] Connection failed! rc=");
            Serial.print(this->client->state());
            Serial.println(" trying again in 1 second.");
            delay(1000);
            retries++;
            if (retries > 60)
                ESP.restart();
        }
    }

    Serial.println("[MQTT] connected!");
    this->client->publish(String(this->getCombinedRootTopic() + "/availability").c_str(), "online", true);
    this->client->subscribe(String(this->getCombinedRootTopic() + "/+/command").c_str());
    this->client->subscribe(String(this->getCombinedRootTopic() + "/+/pair").c_str());

    this->sendAllHomeAssistantDiscoveryMessages();
}

bool MQTT::addLightbar(Lightbar *lightbar)
{
    if (this->lightbarCount >= constants::MAX_LIGHTBARS)
    {
        Serial.println("[MQTT] Could not add light bar, because too many light bars are saved!");
        Serial.println("[MQTT] Please check if you actually want to save more than " + String(constants::MAX_LIGHTBARS, DEC) + " light bars.");
        Serial.println("[MQTT] If you do, increase MAX_LIGHTBARS in constants.h and recompile.");
        return false;
    }
    this->lightbars[this->lightbarCount] = lightbar;
    this->lightbarCount++;
    this->sendHomeAssistantLightbarDiscoveryMessages(lightbar);
    return true;
}

bool MQTT::removeLightbar(Lightbar *lightbar)
{
    for (int i = 0; i < this->lightbarCount; i++)
    {
        if (this->lightbars[i] == lightbar)
        {
            for (int j = i; j < this->lightbarCount - 1; j++)
            {
                this->lightbars[j] = this->lightbars[j + 1];
            }
            this->lightbarCount--;
            return true;
        }
    }
    return false;
}

bool MQTT::addRemote(Remote *remote)
{
    if (this->remoteCount >= constants::MAX_REMOTES)
    {
        Serial.println("[MQTT] Could not add remote, because too many remotes are saved!");
        Serial.println("[MQTT] Please check if you actually want to save more than " + String(constants::MAX_REMOTES, DEC) + " remotes.");
        Serial.println("[MQTT] If you do, increase MAX_REMOTES in constants.h and recompile.");
        return false;
    }
    this->remotes[this->remoteCount] = remote;
    this->remoteCount++;
    remote->registerCommandListener(this->remoteCommandHandler);
    this->sendHomeAssistantRemoteDiscoveryMessages(remote);
    return true;
}

bool MQTT::removeRemote(Remote *remote)
{
    for (int i = 0; i < this->remoteCount; i++)
    {
        if (this->remotes[i] == remote)
        {
            this->remotes[i]->registerCommandListener(this->remoteCommandHandler);
            for (int j = i; j < this->remoteCount - 1; j++)
            {
                this->remotes[j] = this->remotes[j + 1];
            }
            this->remoteCount--;
            return true;
        }
    }
    return false;
}

void MQTT::sendAllHomeAssistantDiscoveryMessages()
{
    if (!this->homeAssistantDiscovery)
        return;
    for (int i = 0; i < this->lightbarCount; i++)
    {
        this->sendHomeAssistantLightbarDiscoveryMessages(this->lightbars[i]);
    }
    for (int i = 0; i < this->remoteCount; i++)
    {
        this->sendHomeAssistantRemoteDiscoveryMessages(this->remotes[i]);
    }
}

void MQTT::sendHomeAssistantLightbarDiscoveryMessages(Lightbar *lightbar)
{
    if (!this->homeAssistantDiscovery)
        return;

    Serial.print("[MQTT] Sending lightbar discovery messages for ");
    Serial.println(lightbar->getSerialString());

    const String topicClient = this->clientId + "_" + lightbar->getSerialString();
    const String baseConfig = R"json(
    "schema": "json",
    "o": {
        "name": "lightbar2mqtt",
        "sw_version": ")json" +
                              constants::VERSION +
                              R"json(",
        "support_url": "https://github.com/ebinf/lightbar2mqtt"
    },
    "~": ")json" + this->getCombinedRootTopic() +
                              "/" +
                              lightbar->getSerialString() +
                              R"json(",
    "availability_topic": ")json" +
                              this->getCombinedRootTopic() + R"json(/availability",
    "dev":
    {
        "ids" : ")json" + topicClient +
                              R"json(",
        "name": ")json" +
                              lightbar->getName() +
                              R"json(",
        "mdl": "Mi Computer Monitor Light Bar (MJGJD01YL)",
        "mf": "Xiaomi",
        "sw": "lightbar2mqtt )json" +
                              constants::VERSION +
                              R"json(",
        "sn": ")json" +
                              lightbar->getSerialString() +
                              R"json("
    },)json";

    String rendevous_str = "{" +
                           baseConfig +
                           R"json(
    "supported_color_modes": [
        "color_temp"
    ],
    "brightness": true,
    "brightness_scale": 15,
    "name": "Light bar",
    "cmd_t": "~/command",
    "uniq_id": ")json" + topicClient +
                           R"json(_lightbar",
    "max_mireds": 370,
    "min_mireds":153,
    "p": "light",
    "icon": "mdi:wall-sconce-flat"
    )json" + "}";

    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/light/" + topicClient + "/lightbar/config").c_str(), rendevous_str.length(), true);
    this->client->print(rendevous_str);
    this->client->endPublish();

    rendevous_str = "{" +
                    baseConfig +
                    R"json(
    "name": "Pair",
    "cmd_t": "~/pair",
    "uniq_id": ")json" +
                    topicClient + R"json(_pair",
    "p": "button"
    )json" + "}";
    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/button/" + topicClient + "/pair/config").c_str(), rendevous_str.length(), true);
    this->client->print(rendevous_str);
    this->client->endPublish();
}

void MQTT::sendHomeAssistantRemoteDiscoveryMessages(Remote *remote)
{
    if (!this->homeAssistantDiscovery)
        return;

    Serial.print("[MQTT] Sending remote discovery messages for ");
    Serial.println(remote->getSerialString());

    const String topicClient = this->clientId + "_" + remote->getSerialString();
    const String baseConfig = R"json(
    "schema": "json",
    "o": {
        "name": "lightbar2mqtt",
        "sw_version": ")json" +
                              constants::VERSION +
                              R"json(",
        "support_url": "https://github.com/ebinf/lightbar2mqtt"
    },
    "~": ")json" + this->getCombinedRootTopic() +
                              "/" +
                              remote->getSerialString() +
                              R"json(",
    "availability_topic": ")json" +
                              this->getCombinedRootTopic() + R"json(/availability",
    "dev": {
        "ids": ")json" + topicClient +
                              R"json(",
        "name": ")json" + remote->getName() +
                              R"json(",
        "mdl": "Mi Computer Monitor Light Bar Remote Control (MJGJD01YL)",
        "mf": "Xiaomi",
        "sw": "lightbar2mqtt )json" +
                              constants::VERSION +
                              R"json(",
        "sn": ")json" + remote->getSerialString() +
                              R"json("
    },)json";

    String rendevous_str = "{" +
                           baseConfig +
                           R"json(
    "name": "Remote",
    "state_topic": "~/state",
    "uniq_id": ")json" +
                           topicClient + R"json(_remote",
    "value_template": "{{ value }}",
    "enabled_by_default": true,
    "entity_category": "diagnostic",
    "icon": "mdi:gesture-double-tap"
    )json" + "}";

    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/sensor/" + topicClient + "/remote/config").c_str(), rendevous_str.length(), true);
    this->client->print(rendevous_str);
    this->client->endPublish();

    const char *commands[] = {
        "press",
        "turn_clockwise",
        "turn_counterclockwise",
        "press_turn_clockwise",
        "press_turn_counterclockwise",
        "hold"};
    String cmd;
    for (int i = 0; i < 6; i++)
    {
        cmd = commands[i];
        rendevous_str = "{" +
                        baseConfig +
                        R"json(
    "automation_type": "trigger",
    "payload": ")json" + cmd +
                        R"json(",
    "subtype": ")json" + cmd +
                        R"json(",
    "type": "action",
    "topic": "~/state",
    "p": "device_automation"
    )json" + "}";

        this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/device_automation/" + topicClient + "/action_" + cmd + "/config").c_str(), rendevous_str.length(), true);
        this->client->print(rendevous_str);
        this->client->endPublish();
    }
}

void MQTT::loop()
{
    if (!this->client->connected())
    {
        Serial.println("[MQTT] connection lost!");
        this->setup();
    }
    this->client->loop();
}

void MQTT::sendAction(Remote *remote, byte command, byte options)
{
    String action;
    switch ((uint8_t)command)
    {
    case Lightbar::Command::ON_OFF:
        action = "press";
        break;

    case Lightbar::Command::BRIGHTER:
        action = "turn_clockwise";
        break;

    case Lightbar::Command::DIMMER:
        action = "turn_counterclockwise";
        break;

    case Lightbar::Command::WARMER:
        action = "press_turn_counterclockwise";
        break;

    case Lightbar::Command::COOLER:
        action = "press_turn_clockwise";
        break;

    case Lightbar::Command::RESET:
        action = "hold";
        break;

    default:
        return;
    }

    String topic = String(this->getCombinedRootTopic() + "/" + remote->getSerialString() + "/state");
    Serial.print("[MQTT] Sending message (");
    Serial.print(topic);
    Serial.print("): ");
    Serial.println(action);
    this->client->publish(topic.c_str(), action.c_str());
    delay(200);
    this->client->publish(topic.c_str(), NULL);
}