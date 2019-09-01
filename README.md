RadioHead Packet Radio library for embedded microprocessors - Fork for RFM22B support on Raspberry PI
====================================================================================================

### Features added with this fork, August-September 2019. NOT TESTED YET!

This is a fork of the initial fork by [Charles-Henri Hallard][1] for Raspberry PI support of the original [RadioHead Packet Radio library][3] for embedded microprocessors by Mike McCauley. The [RadioHead Packet Radio library][3] provides a complete object-oriented library for sending and receiving packetized messages via a variety of common data radios and other transports on a range of embedded microprocessors.

**Please read the full documentation and licensing for the original code by [Mike McCauley][3]. This fork has been updated based on the orginal RadioHead Packet Radio library [version 1.92][3]**


**Compatible boards**

- Kept compatibility with RF69 and RF95 based radio modules on Raspberry PI - See original fork by [Charles-Henri Hallard][1].
- Added compatibility with RFM22B based radio modules on Raspberry PI. NOT TESTED YET!
  - Sample code for Raspberry Pi for HopeRF RFM22B based radio modules, e.g. [RFM22B][4], in a new [RadioHead/examples/raspi/rf22b][5] folder.
  - Based on sample code for RFM69HCW on Raspberry Pi by [Charles-Henri Hallard][1] and sample code for RF24 on Arduino by [Mike McCauley][3].


#### Installation and use on Raspberry PI

Clone repository
```shell
git clone https://github.com/istvanzk/RadioHead
```

**Connection and pins definition**

To connect a Raspberry Pi (V2) to a [RFM22B][4] module, connect the pins like this:

```cpp
          Raspberry PI      RFM22B
    P1_20,P1_25(GND)----------GND-\ (ground in)
                              SDN-/ (shutdown in)
                 VDD----------VCC   (3.3V in)
      P1_22 (GPIO25)----------NIRQ  (interrupt request out)
         P1_24 (CE0)----------NSEL  (chip select in)
         P1_23 (SCK)----------SCK   (SPI clock in)
        P1_19 (MOSI)----------SDI   (SPI Data in)
        P1_21 (MISO)----------SDO   (SPI data out)
                           /--GPIO0 (GPIO0 out to control transmitter antenna TX_ANT)
                           \--TX_ANT (TX antenna control in) RFM22B only
                           /--GPIO1 (GPIO1 out to control receiver antenna RX_ANT)
                           \--RX_ANT (RX antenna control in) RFM22B only
```

Boards pins (Chip Select, IRQ line, Reset and LED) definition for RFM22B are set in the [RadioHead/examples/raspi/RasPiBoards.h][7] file:

```cpp
// HopeRF RF22 based radio modules (no onboard led - reset pin not used)
// =========================================
// see http://www.sparkfun.com/products/10153
#elif defined (BOARD_RFM22B)
#define RF_CS_PIN  RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define RF_IRQ_PIN RPI_V2_GPIO_P1_22 // IRQ on GPIO25 so P1 connector pin #22
#define RF_LED_PIN NOT_A_PIN	     // No onboard led to drive
#define RF_RST_PIN NOT_A_PIN		 // No onboard reset
```

In your code, you need to define RFM22B board used and then, include the file definition like this:

```cpp
// RFM22B board
#define BOARD_RFM22B

// Now we include RasPi_Boards.h so this will expose defined
// constants with CS and IRQ pin definition
#include "../RasPiBoards.h"

// Your code starts here
// Blah blah ....
#ifdef RF_IRQ_PIN
// Blah blah ....
#endif

```
Go to example folder [RadioHead/examples/raspi/rf22b][5], compile rf22b_client and then run it:
```shell
$ cd RadioHead/examples/raspi/rf22b
$ make rf22b_client
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"rf22b_client\" -c -I../../.. rf22b_client.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RH_RF22\" -c -I../../.. ../../../RH_RF22.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RasPi\" -c ../../../RHutil/RasPi.cpp -I../../..
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHHardwareSPI\" -c -I../../.. ../../../RHHardwareSPI.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHGenericDriver\" -c -I../../.. ../../../RHGenericDriver.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHGenericSPI\" -c -I../../.. ../../../RHGenericSPI.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHSPIDriver\" -c -I../../.. ../../../RHSPIDriver.cpp
g++ rf22b_client.o RH_RF22.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o -lbcm2835 -o rf22b_client
$ sudo ./rf22b_client
```

In case of an intialisation error of the RF22B board:
```shell
rf22b_client

RF22B module init failed, Please verify wiring/module
RF22B CS=GPIO8, IRQ=GPIO25
rf22b_client Ending
```

When wiring/ module is OK, then:
```shell
rf22b_client

RF22B CS=GPIO8, IRQ=GPIO25
RF22B module seen OK!
RF22 Group ...
Sending...
Packet received ...

rf22b_client Ending
```

**The spi_scan example**

Go to example folder RadioHead/examples/raspi/spi_scan, compile spi_scan and then run it:
```shell
$ cd RadioHead/examples/raspi/spi_scan
$ make
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -c -I../../.. spi_scan.c
g++ spi_scan.o -lbcm2835  -o spi_scan
$ sudo ./spi_scan
Checking register(0x42) with CS=GPIO06 => Nothing!
Checking register(0x10) with CS=GPIO06 => Nothing!
Checking register(0x01) with CS=GPIO06 => Nothing!
Checking register(0x42) with CS=GPIO08 => Nothing!
Checking register(0x10) with CS=GPIO08 => Nothing!
Checking register(0x01) with CS=GPIO08 => RFM22B (V=0x06)
Checking register(0x42) with CS=GPIO07 => Nothing!
Checking register(0x10) with CS=GPIO07 => Nothing!
Checking register(0x01) with CS=GPIO07 => Nothing!
Checking register(0x42) with CS=GPIO26 => Nothing!
Checking register(0x10) with CS=GPIO26 => Nothing!
Checking register(0x01) with CS=GPIO26 => Nothing!
```


#### Packet Format for RF22B based radio modules

All messages sent and received by this Driver must conform to this packet format RadioHead/RF_RF22.h:
- 8 nibbles (4 octets) PREAMBLE
- 2 octets SYNC 0x2d, 0xd4
- 4 octets HEADER: (TO, FROM, ID, FLAGS)
- 1 octet LENGTH (0 to 255), number of octets in DATA
- 0 to 255 octets DATA
- 2 octets CRC computed with CRC16(IBM), computed on HEADER, LENGTH and DATA

For technical reasons, the message format is not protocol compatible with the
'HopeRF Radio Transceiver Message Library for Arduino' http://www.airspayce.com/mikem/arduino/HopeRF from the same author. Nor is it compatible with
'Virtual Wire' http://www.airspayce.com/mikem/arduino/VirtualWire.pdf also from the same author.



[1]: https://github.com/hallard/RadioHead
[2]: http://www.airspayce.com/mikem/arduino/RadioHead/
[3]: http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.92.zip

[4]: https://www.sparkfun.com/products/12030
[5]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/rf22b
[6]: https://github.com/istvanzk/RadioHead/tree/master/RH_RF22.h

[7]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/RasPiBoards.h



