#include <Arduino.h>
#include <SoftwareSerial.h>

#include "editline.h"
#include "cmdproc.h"

#include "pm1006.h"

#define PIN_PM1006_RX  D2
#define PIN_PM1006_TX  D1
#define PIN_LDR        A0
#define PIN_FAN        D0
#define PIN_LED_G      D5
#define PIN_LED_R      D3

#define printf Serial.printf

static SoftwareSerial pmSerial(PIN_PM1006_RX, PIN_PM1006_TX);
static PM1006 pm1006(&pmSerial);
static char editline[80];

static void show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        printf("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[]);

static int do_fan(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    bool on = (atoi(argv[1]) != 0);
    printf("Turning fan %s\n", on ? "on" : "off");
    digitalWrite(PIN_FAN, on);
    return 0;
}

static int do_ldr(int argc, char *argv[])
{
    int v = analogRead(PIN_LDR);
    printf("LDR value = %d\n", v);
    return 0;
}

static int do_ledg(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    bool on = (atoi(argv[1]) != 0);
    printf("Turning green LED %s\n", on ? "on" : "off");
    digitalWrite(PIN_LED_G, !on);
    return 0;
}

static int do_ledr(int argc, char *argv[])
{
    if (argc < 2) {
        return -1;
    }
    bool on = (atoi(argv[1]) != 0);
    printf("Turning red LED %s\n", on ? "on" : "off");
    digitalWrite(PIN_LED_R, !on);
    return 0;
}

static int do_measure(int argc, char *argv[])
{
    uint16_t pm2_5;
    printf("Measuring... ");
    if (pm1006.read_pm25(&pm2_5)) {
        printf("PM2.5 = %u\n", pm2_5);
    } else {
        printf("FAIL!\n");
    }
    return 0;
}

static void printhex(const char *header, size_t len, const uint8_t *data)
{
    printf("%s", header);
    for (size_t i = 0; i < len; i++) {
        printf(" %02X", data[i]);
    }
    printf("\n");
}

static int do_command(int argc, char *argv[])
{
    uint8_t cmd_data[32];
    uint8_t rsp_data[32];

    if (argc < 2) {
        return -1;
    }

    // convert command buffer
    char *str = argv[1];
    char buf[3];
    size_t cmd_len = 0;
    for (size_t i = 0; i < strlen(str); i += 2) {
        strlcpy(buf, str + i, 3);
        cmd_data[cmd_len++] = strtoul(buf, NULL, 16);
    }

    // perform command/response exchange
    printhex("CMD:", cmd_len, cmd_data);
    if (pm1006.send_command(cmd_len, cmd_data)) {
        size_t rsp_len = pm1006.get_response(rsp_data);
        printhex("RSP:", rsp_len, rsp_data);
        return 0;
    } else {
        printf("FAIL!\n");
        return -1;
    }
}

const cmd_t commands[] = {
    { "help", do_help, "Show help" },
    { "fan", do_fan, "<0|1> Turn fan on or off" },
    { "ldr", do_ldr, "Read LDR" },
    { "g", do_ledg, "<0|1> Control green LED" },
    { "r", do_ledr, "<0|1> Control red/orange LED" },
    { "m", do_measure, "Perform a PM2.5 measurement" },
    { "c", do_command, "<hex> Send a custom command" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return CMD_OK;
}

void setup(void)
{
    Serial.begin(115200);
    printf("Hello this is PM600!\n");

    pinMode(PIN_FAN, OUTPUT);
    digitalWrite(PIN_FAN, 0);

    pinMode(PIN_LED_G, OUTPUT);
    digitalWrite(PIN_LED_G, 1);
    pinMode(PIN_LED_R, OUTPUT);
    digitalWrite(PIN_LED_R, 1);

    EditInit(editline, sizeof(editline));

    pmSerial.begin(PM1006::BIT_RATE);
}

void loop(void)
{
    // parse command line
    bool haveLine = false;
    if (Serial.available()) {
        char c;
        haveLine = EditLine(Serial.read(), &c);
        Serial.write(c);
    }
    if (haveLine) {
        int result = cmd_process(commands, editline);
        switch (result) {
        case CMD_OK:
            printf("OK\n");
            break;
        case CMD_NO_CMD:
            break;
        case CMD_UNKNOWN:
            printf("Unknown command, available commands:\n");
            show_help(commands);
            break;
        default:
            printf("%d\n", result);
            break;
        }
        printf(">");
    }
}
