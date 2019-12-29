# HOTP32u4
HOTP generator for ATmega32u4

It acts as an OTP token by injecting the HOTP code through the USB Keyboard interface.

The number of digits, terminator, counter and shared secret are configurable by the USB ACM serial interface.

Allows HOTP keys between 6 and 9 characters long, optionally terminated with ENTER or TAB.

Initial counter configurable at any time

Shared secret can be up to 32 bytes.
