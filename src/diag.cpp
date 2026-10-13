// ---------------------------------------------------------------------------
// Raw SPI bring-up diagnostic (interactive serial console)
// ---------------------------------------------------------------------------
// This file only compiles when DIAGNOSTIC_MODE is defined, which is done by the
// dedicated PlatformIO env in platformio.ini:
//
//     [env:nano_every_diag]
//
// Build + flash it with:
//
//     pio run -e nano_every_diag -t upload
//     pio device monitor -b 115200
//
// It deliberately does NOT use the Adafruit drivers: everything is raw
// SPI.transfer() so you can see exactly what is on the wire. Type `help` for
// the command list.
// ---------------------------------------------------------------------------

#ifdef DIAGNOSTIC_MODE

#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <stdlib.h>

#include "globals.hpp"

// -- Bus configuration ------------------------------------------------------
static uint32_t g_hz = 1000000;  // clock for subsequent transactions
static uint8_t g_modeIdx = 0;    // 0..3 -> SPI_MODE0..3

static uint8_t modeConst(uint8_t idx) {
    switch (idx & 3) {
        case 0:  return SPI_MODE0;
        case 1:  return SPI_MODE1;
        case 2:  return SPI_MODE2;
        default: return SPI_MODE3;
    }
}

// -- Console input buffer ---------------------------------------------------
static char inbuf[128];
static uint8_t inlen = 0;

// -- Small helpers ----------------------------------------------------------
static bool ieq(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return false;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

static long parseNum(const char *s, long def = 0) {
    if (!s) return def;
    char *end = NULL;
    long v = strtol(s, &end, 0);  // supports 123 and 0x7B
    if (end == s) return def;
    return v;
}

static int parseHexByte(const char *s, int def = -1) {
    if (!s) return def;
    const char *q = s;
    if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) q += 2;
    char *end = NULL;
    long v = strtol(q, &end, 16);  // forced hex, e.g. "8D" or "0x8D"
    if (end == q) return def;
    return (int)(v & 0xFF);
}

// Accepts a pin number or one of the named aliases.
static int resolvePin(const char *s, int def = -1) {
    if (!s) return def;
    if (ieq(s, "bno") || ieq(s, "bno_cs")) return BNO08X_CS;
    if (ieq(s, "dps") || ieq(s, "dps_cs")) return DPS310_CS;
    if (ieq(s, "sd")  || ieq(s, "sd_cs"))  return SD_CS;
    if (ieq(s, "int")) return BNO08X_INT;
    if (ieq(s, "rst")) return BNO08X_RESET;
    long v = parseNum(s, def);
    return (int)v;
}

static void printByte(uint8_t b) {
    if (b < 0x10) Serial.print('0');
    Serial.print(b, HEX);
}

static void parkAllCS() {
    pinMode(BNO08X_CS, OUTPUT); digitalWrite(BNO08X_CS, HIGH);
    pinMode(DPS310_CS, OUTPUT); digitalWrite(DPS310_CS, HIGH);
    pinMode(SD_CS, OUTPUT);     digitalWrite(SD_CS, HIGH);
}

// One raw CS-asserted transaction. tx may be NULL (sends 0x00).
static void busTransfer(int cs, const uint8_t *tx, uint8_t *rx, int n) {
    if (cs < 0) { Serial.println(F("bad CS pin")); return; }
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    delayMicroseconds(5);
    SPI.beginTransaction(SPISettings(g_hz, MSBFIRST, modeConst(g_modeIdx)));
    digitalWrite(cs, LOW);
    delayMicroseconds(5);
    for (int i = 0; i < n; i++) rx[i] = SPI.transfer(tx ? tx[i] : 0x00);
    digitalWrite(cs, HIGH);
    delayMicroseconds(5);
    SPI.endTransaction();
}

// -- Commands ---------------------------------------------------------------
static void cmdHelp() {
    Serial.println(F("commands:"));
    Serial.println(F("  help                      this list"));
    Serial.println(F("  pins                      CS/INT/MISO pin states"));
    Serial.println(F("  spi <hz> <mode>           set clock + mode (mode 0..3)"));
    Serial.println(F("  cs <pin> <0|1|z>          drive a CS/pin low/high/hi-Z"));
    Serial.println(F("  t <cs> [hh hh ...]        raw transfer, print every RX byte"));
    Serial.println(F("  r <cs> <reg> [count]      read: send reg|0x80 then zeros"));
    Serial.println(F("  w <cs> <reg> <val>        write byte val to reg"));
    Serial.println(F("  scan <cs> <start> <end>   read a register range (hex)"));
    Serial.println(F("  dps                       DPS310 ID (0x0D) in mode 0 and 3"));
    Serial.println(F("  dpsdump                   dump DPS310 regs 0x00..0x1F"));
    Serial.println(F("  int                       read BNO INT (D6) level"));
    Serial.println(F("  bnoreset                  pulse BNO RST, time the INT assert"));
    Serial.println(F("  loop                      D11<->D12 loopback (needs jumper)"));
    Serial.println(F("  <cs> = bno|dps|sd or a pin number   |  bytes are hex (8D)"));
}

static void printPinEq(int pin) {
    Serial.print(')');
    Serial.print('=');
    Serial.print(digitalRead(pin));
}

static void cmdPins() {
    Serial.print(F("CS: BNO(D")); Serial.print(BNO08X_CS); printPinEq(BNO08X_CS);
    Serial.print(F("  DPS(D")); Serial.print(DPS310_CS); printPinEq(DPS310_CS);
    Serial.print(F("  SD(D")); Serial.print(SD_CS); printPinEq(SD_CS);
    Serial.print(F("  | INT(D")); Serial.print(BNO08X_INT); printPinEq(BNO08X_INT);
    Serial.print(F("  MISO(D12)=")); Serial.println(digitalRead(12));
}

static void cmdTransfer(char **tok, int n) {
    if (n < 2) { Serial.println(F("usage: t <cs> [hh ...]")); return; }
    int cs = resolvePin(tok[1]);
    uint8_t tx[32];
    uint8_t rx[32];
    int cnt = 0;
    for (int i = 2; i < n && cnt < 32; i++) {
        int b = parseHexByte(tok[i]);
        if (b < 0) { Serial.println(F("bad hex byte")); return; }
        tx[cnt++] = (uint8_t)b;
    }
    if (cnt == 0) tx[cnt++] = 0x00;

    busTransfer(cs, tx, rx, cnt);

    Serial.print(F("TX:"));
    for (int i = 0; i < cnt; i++) { Serial.print(' '); printByte(tx[i]); }
    Serial.println();
    Serial.print(F("RX:"));
    for (int i = 0; i < cnt; i++) { Serial.print(' '); printByte(rx[i]); }
    Serial.println();
}

static void cmdRead(char **tok, int n) {
    if (n < 3) { Serial.println(F("usage: r <cs> <reg> [count]")); return; }
    int cs = resolvePin(tok[1]);
    int reg = parseHexByte(tok[2]);
    int count = (n >= 4) ? (int)parseNum(tok[3], 1) : 1;
    if (reg < 0 || count < 1) { Serial.println(F("bad args")); return; }
    if (count > 31) count = 31;

    uint8_t tx[32];
    uint8_t rx[32];
    tx[0] = (uint8_t)(reg | 0x80);           // read flag
    for (int i = 1; i <= count; i++) tx[i] = 0x00;
    busTransfer(cs, tx, rx, count + 1);

    Serial.print(F("RX:"));
    for (int i = 0; i <= count; i++) { Serial.print(' '); printByte(rx[i]); }
    Serial.println();
    Serial.print(F("data[1..]="));
    for (int i = 1; i <= count; i++) { Serial.print(' '); printByte(rx[i]); }
    Serial.println(F("   <- register value(s)"));
}

static void cmdWrite(char **tok, int n) {
    if (n < 4) { Serial.println(F("usage: w <cs> <reg> <val>")); return; }
    int cs = resolvePin(tok[1]);
    int reg = parseHexByte(tok[2]);
    int val = parseHexByte(tok[3]);
    if (reg < 0 || val < 0) { Serial.println(F("bad args")); return; }
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), (uint8_t)val };
    uint8_t rx[2];
    busTransfer(cs, tx, rx, 2);
    Serial.print(F("wrote ")); printByte((uint8_t)val);
    Serial.print(F(" to ")); printByte((uint8_t)reg);
    Serial.print(F("  RX:")); Serial.print(' '); printByte(rx[0]);
    Serial.print(' '); printByte(rx[1]); Serial.println();
}

static void cmdScan(char **tok, int n) {
    if (n < 4) { Serial.println(F("usage: scan <cs> <start> <end>")); return; }
    int cs = resolvePin(tok[1]);
    int start = parseHexByte(tok[2]);
    int end = parseHexByte(tok[3]);
    if (start < 0 || end < 0 || end < start) { Serial.println(F("bad range")); return; }
    for (int reg = start; reg <= end; reg++) {
        uint8_t tx[2] = { (uint8_t)(reg | 0x80), 0x00 };
        uint8_t rx[2];
        busTransfer(cs, tx, rx, 2);
        Serial.print(F("  ")); printByte((uint8_t)reg);
        Serial.print(F(": ")); printByte(rx[1]);
        Serial.println();
    }
}

static void cmdDps() {
    Serial.println(F("DPS310 PROD_ID reg 0x0D (expect 10):"));
    for (int m = 0; m <= 3; m++) {
        uint8_t tx[2] = { 0x8D, 0x00 };
        uint8_t rx[2];
        uint8_t save = g_modeIdx;
        g_modeIdx = m;
        busTransfer(DPS310_CS, tx, rx, 2);
        g_modeIdx = save;
        Serial.print(F("  mode ")); Serial.print(m); Serial.print(F(": "));
        printByte(rx[1]);
        if (rx[1] == 0x10) Serial.print(F("  <-- OK"));
        Serial.println();
    }
}

static void cmdDpsDump() {
    Serial.println(F("DPS310 regs 0x00..0x1F:"));
    for (int reg = 0x00; reg <= 0x1F; reg++) {
        uint8_t tx[2] = { (uint8_t)(reg | 0x80), 0x00 };
        uint8_t rx[2];
        busTransfer(DPS310_CS, tx, rx, 2);
        Serial.print(F("  ")); printByte((uint8_t)reg);
        Serial.print(F(": ")); printByte(rx[1]);
        if ((reg & 0x07) == 0x07) Serial.println(); else Serial.print(F("   "));
    }
    Serial.println();
}

static void cmdBnoReset() {
    pinMode(BNO08X_INT, INPUT_PULLUP);
    Serial.print(F("BNO INT idle = ")); Serial.println(digitalRead(BNO08X_INT));

    pinMode(BNO08X_RESET, OUTPUT);
    digitalWrite(BNO08X_RESET, HIGH);
    delay(10);
    digitalWrite(BNO08X_RESET, LOW);
    delay(10);
    digitalWrite(BNO08X_RESET, HIGH);

    unsigned long t0 = millis();
    while (digitalRead(BNO08X_INT) == HIGH && (millis() - t0) < 2000) { /* spin */ }

    if (digitalRead(BNO08X_INT) == LOW) {
        Serial.print(F("INT asserted after "));
        Serial.print(millis() - t0);
        Serial.println(F(" ms  -> chip alive & in SPI mode"));
    } else {
        Serial.println(F("INT never asserted -> not in SPI mode / not running / INT-RST not wired"));
    }
}

static void cmdLoop() {
    pinMode(11, OUTPUT);
    pinMode(12, INPUT);
    digitalWrite(11, HIGH); delayMicroseconds(10);
    int hi = digitalRead(12);
    digitalWrite(11, LOW);  delayMicroseconds(10);
    int lo = digitalRead(12);
    Serial.print(F("D11->D12 loopback: driven high read "));
    Serial.print(hi);
    Serial.print(F(", driven low read "));
    Serial.println(lo);
    Serial.println(F("  expect 1 then 0 when a jumper ties D11-D12 together"));
}

static void handle(char *s) {
    char *tok[24];
    int n = 0;
    char *p = strtok(s, " \t\r\n");
    while (p && n < 24) { tok[n++] = p; p = strtok(NULL, " \t\r\n"); }
    if (n == 0) return;

    if (ieq(tok[0], "help") || ieq(tok[0], "?")) {
        cmdHelp();
    } else if (ieq(tok[0], "pins") || ieq(tok[0], "p")) {
        cmdPins();
    } else if (ieq(tok[0], "spi")) {
        if (n >= 2) g_hz = (uint32_t)parseNum(tok[1], g_hz);
        if (n >= 3) g_modeIdx = (uint8_t)(parseNum(tok[2], g_modeIdx) & 3);
        Serial.print(F("spi hz=")); Serial.print(g_hz);
        Serial.print(F(" mode=")); Serial.println(g_modeIdx);
    } else if (ieq(tok[0], "cs")) {
        if (n < 3) { Serial.println(F("usage: cs <pin> <0|1|z>")); return; }
        int pin = resolvePin(tok[1]);
        if (pin < 0) { Serial.println(F("bad pin")); return; }
        if (ieq(tok[2], "z")) { pinMode(pin, INPUT); }
        else if (parseNum(tok[2], 0) != 0) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
        else { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
        Serial.print(F("pin D")); Serial.print(pin);
        Serial.print(F(" -> ")); Serial.println(tok[2]);
    } else if (ieq(tok[0], "t")) {
        cmdTransfer(tok, n);
    } else if (ieq(tok[0], "r")) {
        cmdRead(tok, n);
    } else if (ieq(tok[0], "w")) {
        cmdWrite(tok, n);
    } else if (ieq(tok[0], "scan")) {
        cmdScan(tok, n);
    } else if (ieq(tok[0], "dps")) {
        cmdDps();
    } else if (ieq(tok[0], "dpsdump")) {
        cmdDpsDump();
    } else if (ieq(tok[0], "int")) {
        Serial.print(F("BNO INT (D")); Serial.print(BNO08X_INT);
        Serial.print(F(") = ")); Serial.println(digitalRead(BNO08X_INT));
    } else if (ieq(tok[0], "bnoreset")) {
        cmdBnoReset();
    } else if (ieq(tok[0], "loop")) {
        cmdLoop();
    } else {
        Serial.println(F("? unknown command (type help)"));
    }
}

// Runs the key probes automatically on boot so no typing is required. The
// interactive console still works afterwards.
static void runAutoDiagnostics() {
    Serial.println();
    Serial.println(F("================ AUTO DIAGNOSTIC ================"));

    Serial.println(F("[1] pin states"));
    cmdPins();

    Serial.println(F("[2] Nano SPI loopback (needs D11-D12 jumper, sensors unplugged)"));
    cmdLoop();

    Serial.println(F("[3] DPS310 ID (known-good bus reference)"));
    cmdDps();

    Serial.println(F("[4] DPS310 register dump 0x00..0x1F"));
    cmdDpsDump();

    Serial.println(F("[5] BNO085 reset + INT timing"));
    cmdBnoReset();

    pinMode(BNO08X_INT, INPUT_PULLUP);
    Serial.print(F("[6] BNO INT level after reset test = "));
    Serial.println(digitalRead(BNO08X_INT));

    Serial.println(F("[7] BNO raw transfer (send 00 00 00 00), each mode:"));
    for (int m = 0; m <= 3; m++) {
        g_modeIdx = (uint8_t)m;
        uint8_t tx[4] = {0, 0, 0, 0};
        uint8_t rx[4];
        busTransfer(BNO08X_CS, tx, rx, 4);
        Serial.print(F("    mode ")); Serial.print(m); Serial.print(F(":"));
        for (int i = 0; i < 4; i++) { Serial.print(' '); printByte(rx[i]); }
        Serial.println();
    }

    Serial.println(F("================ END ('help' for commands) ====="));
}

// -- Arduino entry points ---------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    parkAllCS();
    pinMode(BNO08X_INT, INPUT_PULLUP);

    SPI.begin();  // applies PORTMUX_SPI0_ALT2 -> D11/D12/D13

    Serial.println();
    Serial.println(F("================ RAW SPI DIAGNOSTIC ================"));
    Serial.println(F("Type 'help' for the command list."));
    Serial.println(F("CS aliases: bno=D5 dps=D3 sd=D2. Bytes are hex (e.g. 8D)."));
    Serial.println(F("==================================================="));

    runAutoDiagnostics();
}

void loop() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            inbuf[inlen] = 0;
            if (inlen > 0) handle(inbuf);
            inlen = 0;
        } else if ((c == 8 || c == 127) && inlen > 0) {
            inlen--;  // backspace
        } else if (inlen < sizeof(inbuf) - 1) {
            inbuf[inlen++] = c;
        }
    }
}

#endif  // DIAGNOSTIC_MODE
