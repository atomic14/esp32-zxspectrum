# OpenVegaPlus
ZX Spectrum Emulator for ESP32 with SPI TFT Screen

## Hardware parts:
Board TT-GO-T8 V1.7

TFT 3.2inch_SPI_Module_ILI9341_SKU:MSP3218

Inputs: PCF8575 I2C Multiplexer 16 I/O

Sound: PAM8403 3W stereo amplifier

Some diodes and pushbuttons for inputs

Some resistor to adapt sound no normal level

## ESP32 I/O Configuration
### TFT: (On VSPI port)

05 - LCD DC

18 - LCD SCK

19 - LCD MISO (Optional)

23 - LCD MOSI

32 - LCD CS

33 - LCD RESET


### Sound:

25 - Sound Out L

26 - Sound Out R


### Inputs:

21 - I2C SDA 

22 - I2C SCL 

### SD Card: (On HSPI port)

14 - SD SCK

02 - SD MISO

15 - SD MOSI

13 - SD CS


### Tape:

27 - Tape IN

### Free:

00

02

04

12

34 (only DI)

35 (only DI)

RXD

TXD

RST

VN (39)

VP (36)

 
