#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>
#include <Arduino.h>

namespace constants
{
    // The version number of lightbar2mqtt.
    const String VERSION = "0.2";

    // The maximum number of light bars that can be connected to the controller.
    const uint8_t MAX_LIGHTBARS = 10;

    // The maximum number of remotes that can be connected to the controller.
    const uint8_t MAX_REMOTES = 10;

    // The maximum number of serials, the controller will be able to save latest package ids for.
    // This should always >= MAX_REMOTES + MAX_LIGHTBARS.
    const uint8_t MAX_SERIALS = 32;

    // The maximum number of command listeners that can be registered for a remote.
    const uint8_t MAX_COMMAND_LISTENERS = 10;
};

struct SerialWithName
{
    uint32_t serial;
    const char *name;
};

#endif