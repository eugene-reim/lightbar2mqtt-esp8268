/* -- nRF24 --------------------------------------------------------------------------------------------------- */
// The pin number to which the nRF24's Chip Enable (CE) pin is connected.
#define RADIO_PIN_CE 4

// The pin number to which the nRF24's Chip Select Null (CSN) pin is connected.
#define RADIO_PIN_CSN 5

/* -- Light Bar ----------------------------------------------------------------------------------------------- */
// The serial of the remote to listen to.
// If you don't know the serial of your remote, just set this to any value and flash your controller. Once
// the controller is running, the serial of your remote will be printed to the console.
#define INCOMING_SERIAL 0x123456

// The serial, the controller should emulate.
// If set to the same value as INCOMING_SERIAL, the original remote will still control the ligtbar directly.
// To separate the light bar from the original remote, set this to a different value, e.g. 0xABCDEF.
#define OUTGOING_SERIAL 0xABCDEF

/* -- WiFi ---------------------------------------------------------------------------------------------------- */
// The SSID of the WiFi network to connect to.
#define WIFI_SSID "<Your WiFi>"

// The password of the WiFi network to connect to.
#define WIFI_PASSWORD "<Your Password>"

/* -- MQTT ---------------------------------------------------------------------------------------------------- */
// The IP address of the MQTT broker to connect to.
#define MQTT_SERVER "192.168.1.1"

// The port of the MQTT broker to connect to.
#define MQTT_PORT 1883

// The user to use to connect to the MQTT broker.
// Set this to NULL if no user is required.
#define MQTT_USER "<Your User>"

// The password to use to connect to the MQTT broker.
// Set this to NULL if no password is required.
#define MQTT_PASSWORD "<Your Password>"

// The root topic used for communicating with this controller.
// Normally, you don't need to change this. Only if you want to archive a specific structure in your MQTT broker.
// This has nothing to do with Home Assistant's discovery feature.
// Each controller will use its own unique subtopic consisting of the controllers MAC address. Therefore, you can
// have multiple controllers in your network without any conflicts.
#define MQTT_ROOT_TOPIC "lightbar2mqtt"

/* -- Home Assistant Device Discovery ------------------------------------------------------------------------- */
// Whether to send Home Assistant discovery messages.
#define HOME_ASSISTANT_DISCOVERY true

// The prefix to use for Home Assistant discovery messages.
// This must match the prefix used in your Home Assistant configuration. The default is "homeassistant".
#define HOME_ASSISTANT_DISCOVERY_PREFIX "homeassistant"

// The name of the device to use in Home Assistant.
// This is the name that will be displayed in the Home Assistant UI. Of course, you can change this in the UI
// later on. But if you want to have a specific name from the beginning or make it easier to identify the device,
// you can set it here.
#define HOME_ASSISTANT_DEVICE_NAME "Mi Computer Monitor Light Bar"