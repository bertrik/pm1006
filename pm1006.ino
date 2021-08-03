#include <Arduino.h>
#include <SoftwareSerial.h>

#include "pm1006.h"

#define PIN_PM1006_RX  D2
#define PIN_PM1006_TX  D3

#define printf Serial.printf

static SoftwareSerial pmSerial(PIN_PM1006_RX, PIN_PM1006_TX);
static PM1006 pm1006(&pmSerial);

void setup(void)
{
    Serial.begin(115200);
    printf("Hello this is PM600!\n");

    pmSerial.begin(PM1006::BIT_RATE);
}

void loop(void)
{
    static int last_tick = -1;

    // try to perform a measurement every 3 seconds
    int tick = millis() / 3000;
    if (tick != last_tick) {
        last_tick = tick;

        printf("Attempting measurement:\n");
        uint16_t pm2_5;
        if (pm1006.read_pm25(&pm2_5)) {
            printf("PM2.5 = %u\n", pm2_5);
        } else {
            printf("Measurement failed!\n");
        }
    }
}
