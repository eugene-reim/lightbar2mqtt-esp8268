#include "remote.h"

Remote::Remote(Radio *radio, uint32_t serial, const char *name)
{
    this->radio = radio;
    this->serial = serial;
    this->name = name;

    this->radio->addRemote(this);

    this->serialString = "0x" + String(this->serial, HEX);
}

Remote::~Remote()
{
}

uint32_t Remote::getSerial()
{
    return this->serial;
}

String Remote::getSerialString()
{
    return this->serialString;
}

const char *Remote::getName()
{
    return this->name;
}

void Remote::callback(byte command, byte options)
{
    for (int i = 0; i < this->numCommandListeners; i++)
    {
        this->commandListeners[i](this, command, options);
    }
}

bool Remote::registerCommandListener(std::function<void(Remote *, byte, byte)> callback)
{
    if (this->numCommandListeners >= constants::MAX_COMMAND_LISTENERS)
    {
        Serial.println("[Remote] Could not add command listener to remote, because too many are saved!");
        Serial.println("[Remote] Please check if you actually want to save more than " + String(constants::MAX_COMMAND_LISTENERS, DEC) + " command listeners.");
        Serial.println("[Remote] If you do, increase MAX_COMMAND_LISTENERS in constants.h and recompile.");
        return false;
    }
    this->commandListeners[this->numCommandListeners] = callback;
    this->numCommandListeners++;
    return true;
}

bool Remote::unregisterCommandListener(std::function<void(Remote *, byte, byte)> callback)
{
    for (int i = 0; i < this->numCommandListeners; i++)
    {
        if (this->commandListeners[i].target<void(Remote *, byte, byte)>() == callback.target<void(Remote *, byte, byte)>())
        {
            for (int j = i; j < this->numCommandListeners - 1; j++)
            {
                this->commandListeners[j] = this->commandListeners[j + 1];
            }
            this->numCommandListeners--;
            return true;
        }
    }
    return false;
}