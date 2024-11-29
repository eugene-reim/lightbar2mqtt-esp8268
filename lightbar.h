#ifndef LIGHTBAR_H
#define LIGHTBAR_H

#include "radio.h";

class Lightbar
{
public:
    Lightbar(uint32_t incoming_serial, uint32_t outgoing_serial, uint8_t ce, uint8_t csn);
    ~Lightbar();
    void setup();

    enum Command
    {
        ON_OFF = 0x01,
        COOLER = 0x02,
        WARMER = 0x03,
        BRIGHTER = 0x04,
        DIMMER = 0x05,
        RESET = 0x06
    };

    void sendRawCommand(Command command, byte options);
    void sendRawCommand(Command command);
    void onOff();
    void brighter();
    void dimmer();
    void warmer();
    void cooler();
    void reset();
    void pair();
    void setOnOff(bool on);
    void setTemperature(uint8_t value);
    void setMiredTemperature(uint mireds);
    void setBrightness(uint8_t value);

    void loop();

    bool registerCommandListener(std::function<void(uint32_t, byte, byte)> callback);

    static void callback(uint32_t serial, byte command, byte options);

private:
    Radio *radio;
    bool onState = false;
    uint32_t incoming_serial;
};

#endif