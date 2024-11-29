#include "lightbar.h"

Lightbar::Lightbar(uint32_t incoming_serial, uint32_t outgoing_serial, uint8_t ce, uint8_t csn)
{
    this->radio = new Radio(ce, csn);
    this->radio->setOutgoingSerial(outgoing_serial);
    this->incoming_serial = incoming_serial;
}

Lightbar::~Lightbar()
{
    delete this->radio;
}

void Lightbar::setup()
{
    this->radio->setup();
}

void Lightbar::sendRawCommand(Command command, byte options)
{
    this->radio->sendCommand(command, options);
}

void Lightbar::sendRawCommand(Command command)
{
    this->radio->sendCommand(command);
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

void Lightbar::loop()
{
    this->radio->loop();
}

bool Lightbar::registerCommandListener(std::function<void(uint32_t, byte, byte)> callback)
{
    return this->radio->addIncomingSerial(this->incoming_serial, callback);
}