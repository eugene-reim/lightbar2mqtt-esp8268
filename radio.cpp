#include "radio.h"

/*
 * Package structure:
 *  0 –  7: Preamble (see constant above)
 *  8 – 10: Remote ID
 * 11 – 11: Separator (0xFF)
 * 12 – 12: Sequence counter
 * 13 – 14: Command ID + options
 * 15 – 16: CRC16 checksum
 */

Radio::Radio(uint8_t ce, uint8_t csn)
{
    this->radio = RF24(ce, csn);
}

Radio::~Radio()
{
    this->radio.stopListening();
    this->radio.powerDown();
}

void Radio::setOutgoingSerial(uint32_t serial)
{
    this->outgoing_serial = serial;
}

bool Radio::addIncomingSerial(uint32_t serial, std::function<void(uint32_t, byte, byte)> callback)
{
    if (this->num_remotes >= MAX_REMOTES)
        return false;
    this->remotes[this->num_remotes].serial = serial;
    this->remotes[this->num_remotes].last_received_package_id = 0;
    this->remotes[this->num_remotes].callback = callback;
    this->num_remotes++;
}

bool Radio::removeIncomingSerial(uint32_t serial)
{
    for (int i = 0; i < this->num_remotes; i++)
    {
        if (this->remotes[i].serial == serial)
        {
            for (int j = i; j < this->num_remotes - 1; j++)
            {
                this->remotes[j] = this->remotes[j + 1];
            }
            this->num_remotes--;
            return true;
        }
    }
    return false;
}

void Radio::sendCommand(byte command, byte options)
{
    byte data[17] = {0};
    memcpy(data, Radio::preamble, sizeof(Radio::preamble));
    data[8] = (this->outgoing_serial & 0xFF0000) >> 16;
    data[9] = (this->outgoing_serial & 0x00FF00) >> 8;
    data[10] = this->outgoing_serial & 0x0000FF;
    data[11] = 0xFF;
    data[12] = this->last_sent_package_id;
    data[13] = command;
    data[14] = options;

    this->crc.restart();
    this->crc.add(data, sizeof(data) - 2);
    uint16_t checksum = this->crc.calc();
    data[15] = (checksum & 0xFF00) >> 8;
    data[16] = checksum & 0x00FF;

    this->last_sent_package_id += 1;

    Serial.print("[Radio] Sending command: 0x");
    for (int i = 0; i < 17; i++)
    {
        Serial.print(data[i], HEX);
    }
    Serial.println();

    this->radio.stopListening();
    for (int i = 0; i < 20; i++)
    {
        this->radio.write(&data, sizeof(data), true);
        delay(10);
    }
    this->radio.startListening();
}

void Radio::sendCommand(byte command)
{
    return this->sendCommand(command, 0x0);
}

void Radio::setup()
{
    uint retries = 0;
    while (!this->radio.begin())
    {
        Serial.println("[Radio] nRF24 not responding! Is it wired correctly?");
        delay(1000);
        retries++;
        if (retries > 60)
            ESP.restart();
    }

    Serial.println("[Radio] Setting up radio...");
    this->radio.failureDetected = false;

    this->radio.openReadingPipe(0, Radio::address);

    this->radio.setChannel(68);
    this->radio.setDataRate(RF24_2MBPS);
    this->radio.disableCRC();
    this->radio.disableDynamicPayloads();
    this->radio.setPayloadSize(17);
    this->radio.setAutoAck(false);
    this->radio.setRetries(15, 15);

    this->radio.openWritingPipe(Radio::address);

    this->radio.startListening();
    Serial.println("[Radio] done!");
}

void Radio::loop()
{
    if (this->radio.failureDetected)
    {
        Serial.println("[Radio] Failure detected!");
        delay(1000);
        this->setup();
        delay(1000);
    }

    // Only continue if there is a package available.
    if (!this->radio.available())
        return;

    // Read raw data, append a 5 and shift it. See
    // https://github.com/lamperez/xiaomi-lightbar-nrf24?tab=readme-ov-file#baseband-packet-format
    // on why that is necessary.
    byte raw_data[18] = {0};
    this->radio.read(&raw_data, sizeof(raw_data));
    byte data[17] = {0x5};
    for (int i = 0; i < 17; i++)
    {
        if (i == 0)
            data[i] = 0x50 | raw_data[i] >> 5;
        else
            data[i] = ((raw_data[i - 1] >> 1) & 0x0F) << 4 | ((raw_data[i - 1] & 0x01) << 3) | raw_data[i] >> 5;
    }

    // Check if preamble matches. Ignore package otherwise.
    if (memcmp(data, Radio::preamble, sizeof(Radio::preamble)))
        return;

    // Make sure the checksum of the package is correct.
    this->crc.restart();
    this->crc.add(data, sizeof(data) - 2);
    uint16_t calculated_checksum = this->crc.calc();
    uint16_t package_checksum = data[15] << 8 | data[16];
    if (calculated_checksum != package_checksum)
    {
        Serial.println("[Radio] Ignoring pacakge with wrong checksum!");
        return;
    }

    // Check if package is coming from a observed remote.
    bool found = false;
    uint32_t serial = data[8] << 16 | data[9] << 8 | data[10];
    for (int i = 0; i < this->num_remotes; i++)
    {
        Remote remote = this->remotes[i];
        if (serial != remote.serial)
            continue;

        found = true;

        // Make sure the same package was not handled before.
        uint8_t package_id = data[12];
        if (package_id <= remote.last_received_package_id && package_id > remote.last_received_package_id - 64)
        {
            Serial.println("[Radio] Ignoring package with too low package number!");
            continue;
        }
        remote.last_received_package_id = package_id;

        Serial.println("[Radio] Package received!");
        remote.callback(remote.serial, data[13], data[14]);
    }

    if (!found)
    {
        Serial.print("[Radio] Ignoring package with not matching serial: 0x");
        Serial.print(serial, HEX);
        Serial.println("");
    }
}