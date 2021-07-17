#include <Arduino.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
} pm1006k_measurement_t;

typedef enum {
    PM1006K_HEADER,
    PM1006K_LENGTH,
    PM1006K_DATA,
    PM1006K_CHECK
} pm1006k_state_t;

class PM1006K {

private:
    Stream *_serial;
    bool _debug;

    pm1006k_state_t _state;
    size_t _rxlen;
    size_t _index;
    uint8_t _txbuf[16];
    uint8_t _rxbuf[16];
    uint8_t _checksum;
    
    bool send_command(size_t cmd_len, const uint8_t *cmd_data);
    int build_tx(size_t cmd_len, const uint8_t *cmd_data);
    bool process_rx(uint8_t c);

public:
    static const int BIT_RATE = 9600;

    /**
     * Constructor.
     *
     * @param serial the serial port, NOTE: the serial port has to be configured for a bit rate of PM1006K::BIT_RATE !
     */
    explicit PM1006K(Stream *serial, bool debug = false);
    
    /**
     * Attempts to retrieve a PM measurement.
     * 
     * @param the measurement.
     * @return true if a measurement was taken
     */
    bool read_pm(pm1006k_measurement_t * measurement);

};

