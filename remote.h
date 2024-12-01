#ifndef REMOTE_H
#define REMOTE_H

#include "constants.h"
#include "radio.h"

class Radio;

class Remote
{
public:
    Remote(Radio *radio, uint32_t serial, const char *name);
    ~Remote();

    uint32_t getSerial();
    String getSerialString();
    const char *getName();

    bool registerCommandListener(std::function<void(Remote *, byte, byte)> callback);
    bool unregisterCommandListener(std::function<void(Remote *, byte, byte)> callback);

    void callback(byte command, byte options);

private:
    Radio *radio;
    uint32_t serial;
    const char *name;
    String serialString;

    std::function<void(Remote *, byte, byte)> commandListeners[constants::MAX_COMMAND_LISTENERS];
    uint8_t numCommandListeners = 0;
};

#endif