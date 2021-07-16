#include <Arduino.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pm1006k.h"

#define DEFAULT_TIMEOUT 1000

PM1006K::PM1006K(Stream * serial, bool debug)
{
    _serial = serial;
    _debug = debug;

    _state = PM1006K_HEADER;
    _length = 0;
    _index = 0;
    memset(_rxbuf, 0, sizeof(_rxbuf));
    _checksum = 0;
}

bool PM1006K::read(pm1006k_measurement_t * measurement)
{
    uint8_t cmd[4];

    // send the command
    int len = 0;
    cmd[len++] = 0x11;
    cmd[len++] = 0x01;          // command code length?
    cmd[len++] = 0x02;          // command code?
    cmd[len++] = 0xEC;          // checksum?
    _serial->write(cmd, len);

    // wait for response
    unsigned long start = millis();
    while ((millis() - start) < DEFAULT_TIMEOUT) {
        while (_serial->available()) {
            char c = _serial->read();
            if (process_rx(c) && (_length > 12)) {
                // got frame, decode it
                // rxbuf[0] probably echoes the command code from the command frame (2)
                // rxbuf[1], rxbuf[2] has yet unknown content
                measurement->pm2_5 = (_rxbuf[3] << 8) + _rxbuf[4];
                // rxbuf[5], rxbuf[6] has yet unknown content
                measurement->pm1_0 = (_rxbuf[7] << 8) + _rxbuf[8];
                // rxbuf[9], rxbuf[10] has yet unknown content
                measurement->pm10 = (_rxbuf[11] << 8) + _rxbuf[12];
                return true;
            }
        }
        yield();
    }

    // timeout
    return false;
}

bool PM1006K::process_rx(uint8_t c)
{
    switch (_state) {
    case PM1006K_HEADER:
        _checksum = c;
        if (c == 0x16) {
            _state = PM1006K_LENGTH;
        }
        break;

    case PM1006K_LENGTH:
        _checksum += c;
        if (c < sizeof(_rxbuf)) {
            _length = c;
            _index = 0;
            _state = (_length > 0) ? PM1006K_DATA : PM1006K_CHECK;
        } else {
            _state = PM1006K_HEADER;
        }
        break;

    case PM1006K_DATA:
        _checksum += c;
        _rxbuf[_index++] = c;
        if (_index == _length) {
            _state = PM1006K_CHECK;
        }
        break;

    case PM1006K_CHECK:
        _checksum += c;
        // checksum is probably 0 now, not completely sure yet about checksum verification, skip it for now
        _state = PM1006K_HEADER;
        return (c == 0);

    default:
        _state = PM1006K_HEADER;
        break;
    }
    return false;
}

