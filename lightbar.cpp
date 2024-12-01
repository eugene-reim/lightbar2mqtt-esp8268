#include "lightbar.h"

Lightbar::Lightbar(Radio *radio, uint32_t serial, const char *name)
{
    this->radio = radio;
    this->serial = serial;
    this->name = name;

    this->serialString = "0x" + String(this->serial, HEX);
}

Lightbar::~Lightbar()
{
}

uint32_t Lightbar::getSerial()
{
    return this->serial;
}

const String Lightbar::getSerialString()
{
    return this->serialString;
}

const char *Lightbar::getName()
{
    return this->name;
}

void Lightbar::sendRawCommand(Command command, byte options)
{
    this->radio->sendCommand(serial, command, options);
}

void Lightbar::sendRawCommand(Command command)
{
    this->radio->sendCommand(serial, command);
}

void Lightbar::onOff()
{
    this->sendRawCommand(Lightbar::Command::ON_OFF);
    onState = !onState;
}

void Lightbar::setOnOff(bool on)
{
    if (onState != on)
        this->onOff();
}

void Lightbar::brighter()
{
    this->sendRawCommand(Lightbar::Command::BRIGHTER);
}

void Lightbar::dimmer()
{
    this->sendRawCommand(Lightbar::Command::DIMMER);
}

void Lightbar::warmer()
{
    this->sendRawCommand(Lightbar::Command::WARMER);
}

void Lightbar::cooler()
{
    this->sendRawCommand(Lightbar::Command::COOLER);
}

void Lightbar::reset()
{
    this->sendRawCommand(Lightbar::Command::RESET);
}

void Lightbar::pair()
{
    this->sendRawCommand(Lightbar::Command::RESET);
}

void Lightbar::setTemperature(uint8_t value)
{
    // Send max value first, then set to the desired value. See
    // https://github.com/lamperez/xiaomi-lightbar-nrf24?tab=readme-ov-file#command-codes
    // for details.
    this->sendRawCommand(Lightbar::Command::COOLER, 0x0 - 16);
    this->sendRawCommand(Lightbar::Command::WARMER, (byte)value);
}

void Lightbar::setMiredTemperature(uint mireds)
{
    mireds = max(mireds, (uint)153);
    mireds = min(mireds, (uint)370);
    float amount = ((1 - ((mireds - 153) * 1.0 / (370 - 153) * 1.0)) * 15) + 0.5;
    this->setTemperature((uint8_t)amount);
}

void Lightbar::setBrightness(uint8_t value)
{
    // Send max value first, then set to the desired value. See
    // https://github.com/lamperez/xiaomi-lightbar-nrf24?tab=readme-ov-file#command-codes
    // for details.
    this->sendRawCommand(Lightbar::Command::DIMMER, 0x0 - 16);
    this->sendRawCommand(Lightbar::Command::BRIGHTER, (byte)value);
}