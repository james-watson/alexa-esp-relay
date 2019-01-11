#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include "fauxmoESP.h"

fauxmoESP fauxmo;

class Relay
{
public:
    Relay(const char* name, const uint8_t pin)
        : mName(name)
        , mPin(pin)
    {
        set(false);
    };
    
    void set(bool state) const
    {
        if (state)
        {
            pinMode(mPin, OUTPUT);
            digitalWrite(mPin, LOW);
        }
        else
        {
            pinMode(mPin,INPUT_PULLUP);
        }
    };
    const char* mName;
private:
    const uint8_t mPin;    
};

// device names can be renamed in the alexa app
Relay devices[] = {
    {"device one", 4},
    {"device two", 5}
};

inline String getUID()
{
    unsigned char mac[6];
    WiFi.macAddress(mac);
    String result;
    for (int i = 3; i < 6; ++i)
        result += String(mac[i], 16);
    return result;
}

void setupWifi()
{
    WiFiManager wifiManager;
    //  wifiManager.resetSettings();

    // using arduino String because I cbf looking at the c docs for the strn functions.
    // I'm a cpp dev damnit, give me objects
    String uid = getUID();
    String name = "Fauxmo" + uid;
    wifiManager.autoConnect(name.c_str(), "Fauxmo");
}

void onAction(unsigned char device_id, const char * device_name, bool state, unsigned char value)
{
    // State is a boolean (ON/OFF) 
    // value range 0 to 255 eg. "set kitchen light to 50%" == 128

    for(uint8_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i)
    {
        if (strcmp(device_name, devices[i].mName) == 0)
            devices[i].set(state);
    }
}

void setup()
{
    setupWifi();

    fauxmo.createServer(true);
    fauxmo.setPort(80);
    fauxmo.enable(true);

    for(uint8_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i)
        fauxmo.addDevice(devices[i].mName);

    fauxmo.onSetState(onAction);
}

void loop()
{
    // fauxmoESP uses an async TCP server but a sync UDP server
    // Therefore, we have to manually poll for UDP packets
    // who's idea was this???
    fauxmo.handle();

    // For some reason the library example code had this memory cleaning hack :(
    // This makes me deeply uncomfortable, however, I'm not going to search for 
    // the memory leak in the library, I really can't be bothered
    static unsigned long last = millis();
    if (millis() - last > 5000)
    {
        last = millis();
        ESP.getFreeHeap();
    }
}