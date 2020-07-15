RadioHead Packet Radio library for embedded microprocessors - Fork for RFM22B support on Raspberry PI, July 2020
================================================================================================================

### Features added with this fork

This is a modification of the initial fork by [Charles-Henri Hallard][1] for Raspberry PI support of the original [RadioHead Packet Radio library][2]. The [RadioHead Packet Radio library][2] for embedded microprocessors by Mike McCauley provides a complete object-oriented library for sending and receiving packetized messages via a variety of common data radios and other transports on a range of embedded microprocessors.

**Please read the full documentation and licensing for the original code by [Mike McCauley][2]. This fork has been merged with the orginal RadioHead Packet Radio library [version 1.108][3]**


**Compatible boards**

- Kept compatibility with RF69 and RF95 based radio modules on Raspberry PI. *NOT Tested!* - Custom RHutil code moved to RadioHead/RHutil_rf69_rf95. See original fork by [Charles-Henri Hallard][1].
- Added compatibility with RFM22B based radio modules on Raspberry PI - Custom RHutil code available in RadioHead/RHutil_rf22b:
  - Sample code for Raspberry Pi for HopeRF RFM22B based radio modules, e.g. [RFM22B][4], in a new [RadioHead/examples/raspi/rf22b][5] folder.
  - Based on sample code for RFM69HCW on Raspberry Pi by [Charles-Henri Hallard][1] and sample code for RF24 on Arduino by [Mike McCauley][3].


#### Installation and use on Raspberry PI

Clone repository
```shell
git clone https://github.com/istvanzk/RadioHead
```

Note: the example scripts using the bcm2835_gpio_len/eds combination, work only if the boot/config.txt contains the line
```shell
dtoverlay=gpio-no-irq
```
For the explanations see [conclusion][8], [discussion][9], [discussion][10].

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
```

**The rf22b_client example**

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

In case of intialisation error of the RF22B board:
```shell
rf22b_client started

RF22B module init failed, Please verify wiring/module
RF22B CS=GPIO8, IRQ=GPIO25
rf22b_client Ending
```

When wiring/ module is OK, then:
```shell
rf22b_client started
RF22B: Module seen OK. Using: CS=GPIO8, IRQ=GPIO25
RF22B: Group #22, Node #10 to GW #1 init OK @ 434.00MHz with 0x04 TxPw
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!
RF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'  : Sent OK!


^CRF22B: Sending 10 bytes to GW #1 => 'Hi Raspi!'
rf22b_client Interrupt signal (2) received. Exiting!
```

**The rf22b_server example**

Go to example folder [RadioHead/examples/raspi/rf22b][5], compile rf22b_server and then run it:
```shell
$ cd RadioHead/examples/raspi/rf22b
$ make rf22b_server
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"rf22b_server\" -c -I../../.. rf22b_server.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RH_RF22\" -c -I../../.. ../../../RH_RF22.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RasPi\" -c ../../../RHutil/RasPi.cpp -I../../..
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHHardwareSPI\" -c -I../../.. ../../../RHHardwareSPI.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHGenericDriver\" -c -I../../.. ../../../RHGenericDriver.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHGenericSPI\" -c -I../../.. ../../../RHGenericSPI.cpp
g++ -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"RHSPIDriver\" -c -I../../.. ../../../RHSPIDriver.cpp
g++ rf22b_server.o RH_RF22.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o -lbcm2835 -o rf22b_server
$ sudo ./rf22b_server
```

With the default RH22 client running (examples/rf22/rf22_client.pde)
When wiring/ module is OK, then:
```shell
rf22b_server started
RF22B: Module seen OK. Using: CS=GPIO8, IRQ=GPIO25
 - RXBUF NOT VALID 0 -
RF22B: Group #22, GW #1 to Node #10 init OK @ 434.00MHz with 0x00 TxPw
	Listening ...
BCM2835: LOW detected for pin GPIO25
 - RXBUF VALID 0xFF => 0xFF -  - RSSI -126dBm -  - RXBUF VALID 1 -
RF22B: Received packet available
RF22B: Packet received, 13 bytes, from #255 to #255, with -126dBm => 'Hello World!'
BCM2835: LOW detected for pin GPIO25
 - RXBUF VALID 0xFF => 0xFF -  - RSSI -126dBm -  - RXBUF VALID 2 -
RF22B: Received packet available
RF22B: Packet received, 13 bytes, from #255 to #255, with -126dBm => 'Hello World!'
BCM2835: LOW detected for pin GPIO25
 - RXBUF VALID 0xFF => 0xFF -  - RSSI -126dBm -  - RXBUF VALID 3 -
RF22B: Received packet available
RF22B: Packet received, 13 bytes, from #255 to #255, with -126dBm => 'Hello World!'

^C
rf22b_server Interrupt signal (2) received. Exiting!
```

**The rf22b_reliable_datagram_server example**

Go to example folder [RadioHead/examples/raspi/rf22b][5], compile rf22b_reliable_datagram_server.
The server does not reply with a new msg as in rf22_reliable_datagram_server.pde, only the ACK is be transmitted to the client.
Using a simplified rf22_reliable_datagram_client.pde at the client node without manager.recvfromAckTimeout(...) sequence.

*The current version rf22b_reliable_datagram_server.cpp is fairly unstable*
- sometimes it does not start, or
- received packet lenght is incorrect and has the max length configured in RH_RF22.h (actual packet is read twice from the buffer), and/or
- the ACK transmission seems is never received at the client (the client manager.sendToWait(...) returns false), even if the debug msg shows it has been transmitted


#### Packet Format for RF22B based radio modules

All messages sent and received by this Driver must conform to this packet format RadioHead/RF_RF22.h:
- 8 nibbles (4 octets) PREAMBLE
- 2 octets SYNC 0x2d, 0xd4
- 4 octets HEADER: (TO, FROM, ID, FLAGS)
- 1 octet LENGTH (0 to 255), number of octets in DATA
- 0 to 255 octets DATA
- 2 octets CRC computed with CRC16(IBM), computed on HEADER, LENGTH and DATA
On reception the, TO address is checked for validity against RH_RF22_REG_3F_CHECK_HEADER3 or the broadcast address of 0xff.
In the default setup the transmitted TO address will be 0xff, the FROM address will be 0xff.
and all such messages will be accepted.
These setting permit the out-of the box RH_RF22 config to act as an unaddresed, unreliable datagram service.

For technical reasons, the message format is not protocol compatible with the
'HopeRF Radio Transceiver Message Library for Arduino' http://www.airspayce.com/mikem/arduino/HopeRF from the same author. Nor is it compatible with
'Virtual Wire' http://www.airspayce.com/mikem/arduino/VirtualWire.pdf also from the same author.



[1]: https://github.com/hallard/RadioHead
[2]: http://www.airspayce.com/mikem/arduino/RadioHead/
[3]: http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.108.zip

[4]: https://www.sparkfun.com/products/12030
[5]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/rf22b
[6]: https://github.com/istvanzk/RadioHead/tree/master/RH_RF22.h

[7]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/RasPiBoards.h

[8]: https://github.com/raspberrypi/linux/issues/2550
[9]: https://groups.google.com/forum/#!topic/bcm2835/Y3D1mmp6vew
[10]: https://groups.google.com/forum/#!topic/bcm2835/1QWkdCZWlpE



