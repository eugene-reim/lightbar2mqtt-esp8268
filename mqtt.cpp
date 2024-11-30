#include "mqtt.h"

#include <Arduino_JSON.h>
#include <esp_mac.h>

MQTT::MQTT(Lightbar *lightbar, const char *wifiSsid, const char *wifiPassword, const char *mqttServer, int mqttPort, const char *mqttUser, const char *mqttPassword, const char *mqttRootTopic, bool homeAssistantAutoDiscovery, const char *homeAssistantAutoDiscoveryPrefix, const char *homeAssistantDeviceName)
{
    this->wifiSsid = wifiSsid;
    this->wifiPassword = wifiPassword;
    this->mqttServer = mqttServer;
    this->mqttPort = mqttPort;
    this->mqttUser = mqttUser;
    this->mqttPassword = mqttPassword;
    this->mqttRootTopic = String(mqttRootTopic);
    this->homeAssistantDiscovery = homeAssistantAutoDiscovery;
    this->homeAssistantDiscoveryPrefix = String(homeAssistantAutoDiscoveryPrefix);
    this->homeAssistantDeviceName = String(homeAssistantDeviceName);

    this->wifiClient = new WiFiClient();
    this->client = new PubSubClient(*this->wifiClient);
    this->lightbar = lightbar;
    this->lightbar->registerCommandListener(std::bind(&MQTT::sendAction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    String mac = "";
    unsigned char mac_base[6] = {0};
    if (esp_efuse_mac_get_default(mac_base) == ESP_OK)
    {
        char buffer[13]; // 6*2 characters for hex + 5 characters for colons + 1 character for null terminator
        sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
        mac = buffer;
    }
    this->clientId = "l2m_" + mac;
}

MQTT::~MQTT()
{
    delete this->client;
    delete this->wifiClient;
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

    if (!strcmp(topic, String(this->mqttRootTopic + "/" + this->clientId + "/pair").c_str()))
    {
        this->lightbar->pair();
        return;
    }

    JSONVar command = JSON.parse(String(payload, length));

    if (JSON.typeof(command) != "object")
        return;

    if (command.hasOwnProperty("state"))
    {
        const char *state = command["state"];
        this->lightbar->setOnOff(strcmp(state, "ON"));
    }

    if (command.hasOwnProperty("brightness"))
    {
        this->lightbar->setBrightness((uint8_t)command["brightness"]);
    }

    if (command.hasOwnProperty("color_temp"))
    {
        this->lightbar->setMiredTemperature((uint)command["color_temp"]);
    }
}

void MQTT::setupWifi()
{
    Serial.print("[WiFi] Connecting to network \"");
    Serial.print(this->wifiSsid);
    Serial.print("\"...");

    WiFi.begin(this->wifiSsid, this->wifiPassword);
    WiFi.setHostname(this->clientId.c_str());

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
}

void MQTT::setupMqtt()
{
    this->client->setServer(this->mqttServer, this->mqttPort);
    this->client->setCallback(std::bind(&MQTT::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    while (!this->client->connected())
    {
        Serial.println("[MQTT] Connecting to MQTT broker...");
        uint retries = 0;
        if (this->client->connect(this->clientId.c_str(), this->mqttUser, this->mqttPassword, String(this->mqttRootTopic + "/" + this->clientId + "/availability").c_str(), 1, true, "offline"))
        {
            Serial.println("[MQTT] connected!");
            this->client->publish(String(this->mqttRootTopic + "/" + this->clientId + "/availability").c_str(), "online", true);
            this->client->subscribe(String(this->mqttRootTopic + "/" + this->clientId + "/lightbar/command").c_str());
            this->client->subscribe(String(this->mqttRootTopic + "/" + this->clientId + "/pair").c_str());
        }
        else
        {
            Serial.print("[MQTT] Connection failed! rc=");
            Serial.print(this->client->state());
            Serial.println(" try again in 1 second");
            while (WiFi.status() != WL_CONNECTED)
            {
                this->setupWifi();
            }
            delay(1000);
            retries++;
            if (retries > 60)
                ESP.restart();
        }
    }

    if (homeAssistantDiscovery)
        this->sendHomeAssistantDiscoveryMessages();
}

void MQTT::setup()
{
    Serial.print("[MQTT] Device ID: ");
    Serial.println(this->clientId);
    Serial.print("[MQTT] Root Topic: ");
    Serial.println(this->mqttRootTopic + "/" + this->clientId);

    this->setupWifi();
    this->setupMqtt();
}

void MQTT::sendHomeAssistantDiscoveryMessages()
{
    const String baseConfig = R"json(
    "schema": "json",
    "o": {
        "name": "lightbar2mqtt",
        "sw_version": "0.1",
        "support_url": "https://github.com/ebinf/lightbar2mqtt"
    },
    "~": ")json" + this->mqttRootTopic +
                              "/" +
                              this->clientId +
                              R"json(",
    "availability_topic": "~/availability",
    "dev": {
        "ids": ")json" + this->clientId +
                              R"json(",
        "name": ")json" + homeAssistantDeviceName +
                              R"json(",
        "mdl": "MJGJD01YL",
        "mf": "Xiaomi"
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
    "cmd_t": "~/lightbar/command",
    "uniq_id": ")json" + this->clientId +
                           R"json(_lightbar",
    "max_mireds": 370,
    "min_mireds":153,
    "p": "light"
    )json" + "}";

    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/light/" + this->clientId + "/lightbar/config").c_str(), rendevous_str.length(), true);
    this->client->print(rendevous_str);
    this->client->endPublish();

    rendevous_str = "{" +
                    baseConfig +
                    R"json(
    "name": "Remote",
    "state_topic": "~/remote/state",
    "uniq_id": ")json" +
                    this->clientId + R"json(_remote",
    "value_template": "{{ value }}",
    "p": "sensor"
    )json" + "}";

    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/sensor/" + this->clientId + "/remote/config").c_str(), rendevous_str.length(), true);
    this->client->print(rendevous_str);
    this->client->endPublish();

    rendevous_str = "{" +
                    baseConfig +
                    R"json(
    "name": "Pair",
    "cmd_t": "~/pair",
    "uniq_id": ")json" +
                    this->clientId + R"json(_pair",
    "p": "button"
    )json" + "}";
    this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/button/" + this->clientId + "/pair/config").c_str(), rendevous_str.length(), true);
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
    "topic": "~/remote/state",
    "p": "device_automation"
    )json" + "}";

        this->client->beginPublish(String(homeAssistantDiscoveryPrefix + "/device_automation/" + this->clientId + "/action_" + cmd + "/config").c_str(), rendevous_str.length(), true);
        this->client->print(rendevous_str);
        this->client->endPublish();
    }
}

void MQTT::loop()
{
    if (!this->client->connected())
    {
        Serial.println("[MQTT] connection lost!");
        this->setupMqtt();
    }
    this->client->loop();
}

void MQTT::sendAction(uint32_t serial, byte command, byte options)
{
    const char *action;
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
    this->client->publish(String(this->mqttRootTopic + "/" + this->clientId + "/remote/state").c_str(), action);
}