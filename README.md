# HOTP32u4
HOTP generator for ATmega32u4

It works as an OTP token, injecting the HOTP code through the USB Keyboard interface.

The number of digits, terminator, counter and shared secret are configurable by the USB ACM Serial interface.

Allows HOTP keys between 6 and 9 characters long, optionally terminated with ENTER or TAB.

Initial counter configurable at any time.

Shared secret can be up to 32 bytes.

Originally written for Arduino Micro, it can be used on any PCB with 32U4 processor and USB interface.


Copyright (c) 2019 Alberto González Balaguer  https://github.com/albertogonb

Licensed under the EUPL-1.2-or-later  https://joinup.ec.europa.eu/collection/eupl/eupl-text-11-12
