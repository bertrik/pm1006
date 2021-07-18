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
    _rxlen = 0;
    _index = 0;
    memset(_rxbuf, 0, sizeof(_rxbuf));
    _checksum = 0;
}

bool PM1006K::read_pm(pm1006k_measurement_t * measurement)
{
    uint8_t cmd = 0x02;
    if (send_command(1, &cmd) && (_rxlen > 12) && (_rxbuf[0] == cmd)) {
        // rxbuf[1], rxbuf[2] has yet unknown content
        measurement->pm2_5 = (_rxbuf[3] << 8) + _rxbuf[4];
        // rxbuf[5], rxbuf[6] has yet unknown content
        measurement->pm1_0 = (_rxbuf[7] << 8) + _rxbuf[8];
        // rxbuf[9], rxbuf[10] has yet unknown content
        measurement->pm10 = (_rxbuf[11] << 8) + _rxbuf[12];
        return true;
    }
    return false;
}

bool PM1006K::read_pm_25(uint16_t *pm)
{
    uint8_t cmd[] = {0x0B, 0x01};
    if (send_command(2, cmd) && (_rxlen > 4) && (_rxbuf[0] == cmd[0])) {
        *pm = (_rxbuf[3] << 8) + _rxbuf[4];
        return true;
    }
    return false;
}

// sends a command and waits for response, returns length of response
bool PM1006K::send_command(size_t cmd_len, const uint8_t *cmd_data)
{
    // build and send command
    int txlen = build_tx(cmd_len, cmd_data);
    _serial->write(_txbuf, txlen);

    // wait for response
    unsigned long start = millis();
    while ((millis() - start) < DEFAULT_TIMEOUT) {
        while (_serial->available()) {
            char c = _serial->read();
            if (process_rx(c)) {
                return true;
            }
        }
        yield();
    }

    // timeout
    return false;
}

// builds a tx buffer, returns length of tx data
int PM1006K::build_tx(size_t cmd_len, const uint8_t *cmd_data)
{
    int len = 0;
    _txbuf[len++] = 0x11;
    _txbuf[len++] = cmd_len;
    for (size_t i = 0; i < cmd_len; i++) {
        _txbuf[len++] = cmd_data[i];
    }
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += _txbuf[i];
    }
    _txbuf[len++] = (256 - sum) & 0xFF;
    return len;
}

// processes one rx character, returns true if a valid frame was found
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
            _rxlen = c;
            _index = 0;
            _state = (_rxlen > 0) ? PM1006K_DATA : PM1006K_CHECK;
        } else {
            _state = PM1006K_HEADER;
        }
        break;

    case PM1006K_DATA:
        _checksum += c;
        _rxbuf[_index++] = c;
        if (_index == _rxlen) {
            _state = PM1006K_CHECK;
        }
        break;

    case PM1006K_CHECK:
        _checksum += c;
        _state = PM1006K_HEADER;
        return (c == 0);

    default:
        _state = PM1006K_HEADER;
        break;
    }
    return false;
}

