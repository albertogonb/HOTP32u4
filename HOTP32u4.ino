/*
  HOTP32u4  1.0

  Copyright (c) 2019 Alberto González Balaguer  https://github.com/albertogonb
  Licensed under the EUPL v1.2  https://joinup.ec.europa.eu/collection/eupl/eupl-text-11-12

  Hardware: ATmega32u4 badusb, Arduino Micro
  
*/
#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>
#include <avr/wdt.h>
#include <DirectIO.h>
#include <Keyboard.h>
#include <SimpleHOTP.h>
#include <EEPROMWearLevel.h>

Output<LED_BUILTIN> led;

Scheduler ts;

void  tReadMenu();
void  tLedOn();

Task tRead(1, TASK_FOREVER, tReadMenu, &ts, true);                    // 1ms
Task tLed(TASK_IMMEDIATE, TASK_FOREVER, tLedOn, &ts, true);           // 500ms

#define EEwl_VERSION  1
#define EEwl_NUM      1
#define EEwl_MAX      1024
#define EEwl_SIZE     (EEwl_MAX - sizeof(const_secret) - 3)

const uint8_t PROGMEM const_secret[32] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
const uint32_t baud = 500000;
int32_t   hotp_counter;
uint8_t   hotp_secret[sizeof(const_secret)];
uint32_t  hotp_decimal;
uint8_t   hotp_final;
uint8_t   hotp_digits;
uint8_t   long_secret;
char      text[64];
uint8_t   c, n;

void setup() {

  EEPROMwl.begin(EEwl_VERSION, EEwl_NUM, EEwl_SIZE);
  if (EEPROM.read(1) == 0xff) {
    EEPROM.write(EEwl_SIZE, '\n');
    EEPROM.write(EEwl_SIZE + 1, 6);
    EEPROM.write(EEwl_SIZE + 2, sizeof(const_secret));
    for (n = 0; n < sizeof(const_secret); ++n) {
      EEPROM.update(EEwl_SIZE + 3 + n, pgm_read_byte_near(const_secret + n));
    }
    EEPROMwl.putToNext(0, (int32_t)-1);
    PowerDown();
  }
  EEPROMwl.get(0, hotp_counter);
  hotp_final = EEPROM.read(EEwl_SIZE);
  hotp_digits = EEPROM.read(EEwl_SIZE + 1);
  long_secret = EEPROM.read(EEwl_SIZE + 2);
  for (n = 0; n < long_secret; ++n) {
    hotp_secret[n] = EEPROM.read(EEwl_SIZE + 3 + n);
  }

  Serial.begin(baud);
  Keyboard.begin();
  delay(1500);
  led = HIGH;
  Key hotp_key(hotp_secret, long_secret);
  SimpleHOTP hotp(hotp_key, ++hotp_counter);
  hotp.setDigits(hotp_digits);
  hotp_decimal = hotp.generateHOTP();
  switch (hotp_digits) {
  case 6:
    sprintf(text, "%06lu%c", hotp_decimal, hotp_final);
    break;
  case 7:
    sprintf(text, "%07lu%c", hotp_decimal, hotp_final);
    break;
  case 8:
    sprintf(text, "%08lu%c", hotp_decimal, hotp_final);
    break;
  case 9:
    sprintf(text, "%09lu%c", hotp_decimal, hotp_final);
    break;
  }
  Keyboard.print(text);
  Keyboard.end();
  EEPROMwl.putToNext(0, hotp_counter);
  led = LOW;

  while (! Serial);                 // wait for /dev/ttyACM0 init
  Serial.print(F("HOTP32u4 1.0\r\n\r\n"));
  Serial.print(F("Copyright (c) 2019 Alberto Gonzalez Balaguer  https://github.com/albertogonb\r\n"));
  Serial.print(F("Licensed under the EUPL v1.2  https://joinup.ec.europa.eu/collection/eupl/eupl-text-11-12\r\n\r\n"));
  sprintf(text, "BAUD = %lu, F_CPU = %lu, DIGI = %d, COUN = %lu", baud, F_CPU, hotp_digits, hotp_counter); Serial.write(text);
  Serial.print(F("\r\n\nCommands: r Reset  fX Final  dN Digits  cNNNNNN Counter  sXXX.XXX Secret\r\n"));

  wdt_enable(WDTO_120MS);
  
}

void loop() {

  ts.execute();
  wdt_reset();
  
}

boolean minus_sign;
boolean nibble_low;

void  tReadMenu() {

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.write(c);
    switch (c) {
    case 'r':
      Reset();
      break;
    case 'f':
      tRead.setCallback(&tReadFinal);
      break;
    case 'c':
      hotp_counter = 0;
      minus_sign = false;
      tRead.setCallback(&tReadCounter);
      break;
    case 'd':
      tRead.setCallback(&tReadDigits);
      break;
    case 's':
      long_secret = 0;
      nibble_low = false;
      tRead.setCallback(&tReadSecret);
      break;
    default:
      Serial.print(F("\r \r"));
      break;
    }
  }

}

void  tReadFinal() {

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.write(c);
    tRead.setCallback(&tReadMenu);
    if ((c != 'e') && (c != 't') && (c != 'n')) {
      Serial.print(F(" Invalid character\r\n"));
      return;
    }
    switch (c) {
    case 'e':
      hotp_final = '\n';
      Serial.print(F("\rFinal = <ENTER>\r\n"));
      break;
    case 't':
      hotp_final = '\t';
      Serial.print(F("\rFinal = <TAB>\r\n"));
      break;
    case 'n':
      hotp_final = 0;
      Serial.print(F("\rFinal = NONE\r\n"));
      break;
    }
    EEPROM.update(EEwl_SIZE, hotp_final);
  }

}
void  tReadCounter() {

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.write(c);
    if (c == '\r') {
      if  (minus_sign) {
        hotp_counter *= -1;
      }
      sprintf(text, "Counter = %d\r\n", hotp_counter); Serial.write(text);
      EEPROMwl.put(0, hotp_counter);
      tRead.setCallback(&tReadMenu);
      return;
    }
    if (c == '-') {
      minus_sign = true;
    }
    else {
    if ((c < '0') || (c > '9')) {
      Serial.print(F(" Invalid character\r\n"));
      tRead.setCallback(&tReadMenu);
      return;
    }
    if (hotp_counter > 99999999) {
      Serial.print(F(" Greater than 999999999\r\n"));
      tRead.setCallback(&tReadMenu);
      return;
    }
    hotp_counter *= 10;
    hotp_counter += c - '0';
    }
  }

}

void  tReadDigits() {

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.write(c);
    tRead.setCallback(&tReadMenu);
    if ((c < '6') || (c > '9')) {
      Serial.print(F(" Invalid character\r\n"));
      return;
    }
    hotp_digits = c - '0';
    sprintf(text, "\rDigits = %d\r\n", hotp_digits); Serial.write(text);
    EEPROM.update(EEwl_SIZE + 1, hotp_digits);
  }

}

void  tReadSecret() {

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.write(c);
    if (c == '\r') {
      Serial.print(F("Secret = "));
      EEPROM.update(EEwl_SIZE + 2, long_secret);
      for (n = 0; n < long_secret; ++n) {
        sprintf(text, "%02x", hotp_secret[n]); Serial.write(text);
        EEPROM.update(EEwl_SIZE + 3 + n, hotp_secret[n]);
      }
      Serial.print(F("\r\n"));
      tRead.setCallback(&tReadMenu);
      return;
    }
    if (((c < '0') || (c > '9')) && ((c < 'a') || (c > 'f')) && ((c < 'A') || (c > 'F'))) {
      Serial.print(F(" Invalid character\r\n"));
      tRead.setCallback(&tReadMenu);
      return;
    }
    if ((c >= '0') && (c <= '9')) {
      c -= '0';
    }
    else {
    if ((c >= 'a') && (c <= 'f')) {
      c -= 'a' - 10;
    }
    else {
    if ((c >= 'A') && (c <= 'F')) {
      c -= 'A' - 10;
    }
    }
    }
    if (nibble_low) {
      nibble_low = false;
      hotp_secret[long_secret++] |= c;
    }
    else {
      nibble_low = true;
      if (long_secret >= sizeof(const_secret)) {
        sprintf(text, " More than %d characteres\r\n", sizeof(const_secret) * 2); Serial.write(text);
        tRead.setCallback(&tReadMenu);
        return;
      }
      hotp_secret[long_secret] = c << 4;
    }
  }
  
}

void  tLedOn() {

  led = HIGH;
  tLed.setCallback(&tLedOff);
  tLed.delay(50);

}

void  tLedOff() {

  led = LOW;
  tLed.setCallback(&tLedOn);
  tLed.delay(500 - 50);

}

void PowerDown() {

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();

}

void Reset() {

  Serial.print(F("\rRebooting ...\r\n\n"));
  wdt_enable(WDTO_15MS);     // Reset by WatchDog
  while (1);
  
}
