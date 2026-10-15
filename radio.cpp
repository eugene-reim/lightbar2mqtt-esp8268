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

bool Radio::addRemote(Remote *remote)
{
    if (this->num_remotes >= constants::MAX_REMOTES)
    {
        Serial.println("[Radio] Could not add remote, because too many remotes are saved!");
        Serial.println("[Radio] Please check if you actually want to save more than " + String(constants::MAX_REMOTES, DEC) + " remotes.");
        Serial.println("[Radio] If you do, increase MAX_REMOTES in constants.h and recompile.");
        return false;
    }
    if (this->num_package_ids >= constants::MAX_SERIALS)
    {
        Serial.println("[Radio] Could not add remote, because too many serials are saved!");
        Serial.println("[Radio] Please check if you actually want to save more than " + String(constants::MAX_SERIALS, DEC) + " serials.");
        Serial.println("[Radio] If you do, increase MAX_SERIALS in constants.h and recompile.");
        return false;
    }
    this->remotes[this->num_remotes] = remote;
    this->num_remotes++;
    this->package_ids[this->num_package_ids].serial = remote->getSerial();
    this->package_ids[this->num_package_ids].package_id = 0;
    this->num_package_ids++;
    Serial.print("[Radio] Remote ");
    Serial.print(remote->getSerialString());
    Serial.println(" added!");
    return true;
}

bool Radio::removeRemote(Remote *remote)
{
    for (int i = 0; i < this->num_remotes; i++)
    {
        if (this->remotes[i] == remote)
        {
            for (int j = 0; j < this->num_package_ids; j++)
            {
                if (this->package_ids[j].serial == remote->getSerial())
                {
                    for (int k = j; k < this->num_package_ids - 1; k++)
                    {
                        this->package_ids[k] = this->package_ids[k + 1];
                    }
                    this->num_package_ids--;
                    break;
                }
            }

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

void Radio::sendCommand(uint32_t serial, byte command, byte options)
{
    PackageIdForSerial *package_id = nullptr;
    for (int i = 0; i < this->num_package_ids; i++)
    {
        if (this->package_ids[i].serial == serial)
        {
            package_id = &this->package_ids[i];
            break;
        }
    }
    if (package_id == nullptr)
    {
        if (this->num_package_ids >= constants::MAX_SERIALS)
        {
            Serial.println("[Radio] Could not send command, because too many serials are saved!");
            Serial.println("[Radio] Please check if you actually want to save more than " + String(constants::MAX_SERIALS, DEC) + " serials.");
            Serial.println("[Radio] If you do, increase MAX_SERIALS in constants.h and recompile.");
            return;
        }
        package_id = &this->package_ids[this->num_package_ids];
        package_id->serial = serial;
        package_id->package_id = 0;
        this->num_package_ids++;
    }

    byte data[17] = {0};
    memcpy(data, Radio::preamble, sizeof(Radio::preamble));
    data[8] = (serial & 0xFF0000) >> 16;
    data[9] = (serial & 0x00FF00) >> 8;
    data[10] = serial & 0x0000FF;
    data[11] = 0xFF;
    data[12] = ++package_id->package_id;
    data[13] = command;
    data[14] = options;

    this->crc.restart();
    this->crc.add(data, sizeof(data) - 2);
    uint16_t checksum = this->crc.calc();
    data[15] = (checksum & 0xFF00) >> 8;
    data[16] = checksum & 0x00FF;

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

void Radio::sendCommand(uint32_t serial, byte command)
{
    return this->sendCommand(serial, command, 0x0);
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

    this->radio.openReadingPipe(0, Radio::receive_address);

    this->radio.setChannel(68);
    this->radio.setDataRate(RF24_2MBPS);
    this->radio.disableCRC();
    this->radio.disableDynamicPayloads();
    this->radio.setPayloadSize(17);
    this->radio.setAutoAck(false);
    this->radio.setRetries(15, 15);

    this->radio.openWritingPipe(Radio::send_address);

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

    if (this->radio.available())
        this->handlePackage();
}

void Radio::handlePackage()
{
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
    Remote *remote = nullptr;
    uint32_t serial = data[8] << 16 | data[9] << 8 | data[10];
    for (int i = 0; i < this->num_remotes; i++)
    {

        if (serial == this->remotes[i]->getSerial())
        {
            remote = this->remotes[i];
            break;
        }
    }

    if (remote == nullptr)
    {
        Serial.print("[Radio] Ignoring package with unknown serial: 0x");
        Serial.print(serial, HEX);
        Serial.println("");
        return;
    }

    // Make sure the same package was not handled before.
    uint8_t package_id = data[12];
    PackageIdForSerial *package_id_for_serial = nullptr;
    for (int i = 0; i < this->num_package_ids; i++)
    {
        if (this->package_ids[i].serial == serial)
        {
            package_id_for_serial = &this->package_ids[i];
            break;
        }
    }
    if (package_id_for_serial == nullptr)
    {
        Serial.print("[Radio] Could not find latest package id for serial 0x");
        Serial.print(serial, HEX);
        Serial.println("!");
        return;
    }
    if (package_id <= package_id_for_serial->serial && package_id > package_id_for_serial->serial - 64)
    {
        Serial.println("[Radio] Ignoring package with too low package number!");
        return;
    }

    if (package_id_for_serial->never_read) {
        package_id_for_serial->never_read = false;
    }
    else if (package_id_for_serial->package_id == package_id){
        return; 
    }

    package_id_for_serial->package_id = package_id;

    Serial.println("[Radio] Package received!");
    remote->callback(data[13], data[14]);
}