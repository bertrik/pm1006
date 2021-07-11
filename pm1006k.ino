#include <Arduino.h>
#include <SoftwareSerial.h>

#include "pm1006k.h"

#define PIN_PM1006K_RX  D2
#define PIN_PM1006K_TX  D3

#define printf Serial.printf

static SoftwareSerial pmSerial(PIN_PM1006K_RX, PIN_PM1006K_TX);
static PM1006K pm1006k(&pmSerial);

void setup(void)
{
    Serial.begin(115200);
    printf("Hello this is PM600K!\n");

    pmSerial.begin(PM1006K::BIT_RATE);
}

void loop(void)
{
    static int last_tick = -1;

    // try to perform a measurement every 3 seconds
    int tick = millis() / 3000;
    if (tick != last_tick) {
        last_tick = tick;

        printf("Attempting measurement:\n");
        pm1006k_measurement_t measurement;
        if (pm1006k.read(&measurement)) {
            printf("PM1.0 = %d\n", measurement.pm1_0);
            printf("PM2.5 = %d\n", measurement.pm2_5);
            printf("PM10  = %d\n", measurement.pm10);
        } else {
            printf("Measurement failed!\n");
        }
    }
}
