#ifndef RADIO_H
#define RADIO_H

#include <RF24.h>
#include <CRC16.h>

#define MAX_REMOTES 10

struct Remote
{
    uint32_t serial;
    uint8_t last_received_package_id;
    std::function<void(uint32_t, byte command, byte options)> callback;
};

class Radio
{
public:
    Radio(uint8_t ce, uint8_t csn);
    ~Radio();
    void setup();
    void sendCommand(byte command, byte options);
    void sendCommand(byte command);
    void loop();
    void setOutgoingSerial(uint32_t serial);
    bool addIncomingSerial(uint32_t serial, std::function<void(uint32_t, byte, byte)> callback);
    bool removeIncomingSerial(uint32_t serial);

private:
    RF24 radio;
    uint8_t last_sent_package_id = 0;
    uint32_t outgoing_serial;

    Remote remotes[MAX_REMOTES];
    uint8_t num_remotes = 0;

    static const uint64_t address = 0xAAAAAAAAAAAA;
    static constexpr byte preamble[8] = {0x53, 0x39, 0x14, 0xDD, 0x1C, 0x49, 0x34, 0x12};

    // For details on how these parameters were chosen, see
    // https://github.com/lamperez/xiaomi-lightbar-nrf24?tab=readme-ov-file#crc-checksum
    CRC16 crc = CRC16(0x1021, 0xfffe, 0x0000, false, false);
};

#endif