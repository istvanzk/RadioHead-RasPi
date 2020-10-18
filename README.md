RadioHead Packet Radio library for embedded microprocessors - Fork for RFM22B and RFM69 support on Raspberry Pi, October 2020
================================================================================================================

### Features added with this fork

This is a modification of the initial fork by [Charles-Henri Hallard][1] for Raspberry PI support of the original [RadioHead Packet Radio library][2]. The [RadioHead Packet Radio library][2] for embedded microprocessors by Mike McCauley provides a complete object-oriented library for sending and receiving packetized messages via a variety of common data radios and other transports on a range of embedded microprocessors.

**Please read the full documentation and licensing for the original code by [Mike McCauley][2]. This fork has been merged with the orginal RadioHead Packet Radio library [version 1.108][3]**


**Compatible boards**

- Kept compatibility with RF69 and RF95 based radio modules on Raspberry Pi from the [original fork by Charles-Henri Hallard][1]. 
  - RF95 NOT Tested! 
  - Custom RHutil code from [original fork][1] moved to RadioHead/RHutil_rf69_rf95.
- Added compatibility with [HopeRF RFM22B][4] and [HopeRF RFM69HCW][12] based radio modules on Raspberry Pi 
  - Based on sample code for RFM69HCW on Raspberry Pi by [Charles-Henri Hallard][1] and sample code for RF24 on Arduino by [Mike McCauley][3]
  - Custom RHutil code available in RadioHead/RHutil_izk  
  - Code modifications: 
    - RadioHead.h (include RHutil_izk/RasPi.h when bcm2835.h is used)
    - RHSPIDriver.cpp/h (define and use RPI_CE0_CE1_FIX)
    - RHReliableDatagram.cpp and RH_RF22.cpp/h (several modifications) 
    - RH_RF69.cpp/h (small modifications in addition to [original fork][1])
  - Sample code can be found in the [RadioHead/examples/raspi/rf22b_izk][5] and [RadioHead/examples/raspi/rf69_izk][13] folders
 

### Installation and use on Raspberry PI

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

To connect a Raspberry Pi (V2) to a [RFM69HCW][12] module, connect the pins like this:

```cpp
          Raspberry PI     RFM69HCW
      P1_20,P1_25(GND)----------GND   (ground in)
                   VDD----------VCC   (3.3V in)
        P1_22 (GPIO25)----------DIO0  (interrupt request out)
           P1_24 (CE0)----------NSS   (chip select in)
           P1_23 (SCK)----------SCK   (SPI clock in)
          P1_19 (MOSI)----------SDI   (SPI Data in)
          P1_21 (MISO)----------SDO   (SPI data out)
```


Boards pins Chip Select and IRQ line (Reset and LED not used) definition for RFM22B and RFM69HCW can be set in your code like:

```cpp
#define RF_CS_PIN  RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define RF_IRQ_PIN RPI_V2_GPIO_P1_22 // IRQ on GPIO25 so P1 connector pin #22
#define RF_LED_PIN NOT_A_PIN	       // No onboard led to drive
#define RF_RST_PIN NOT_A_PIN		     // No onboard reset
```

[1]: https://github.com/hallard/RadioHead
[2]: http://www.airspayce.com/mikem/arduino/RadioHead/
[3]: http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.108.zip

[4]: https://www.hoperf.com/modules/rf_transceiver/RFM22BW.html
[5]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/rf22b_izk
[6]: https://github.com/istvanzk/RadioHead/tree/master/RH_RF22.h

[7]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/RasPiBoards.h

[8]: https://github.com/raspberrypi/linux/issues/2550
[9]: https://groups.google.com/forum/#!topic/bcm2835/Y3D1mmp6vew
[10]: https://groups.google.com/forum/#!topic/bcm2835/1QWkdCZWlpE
[12]: https://www.hoperf.com/modules/rf_transceiver/RFM69HCW.html
[13]: https://github.com/istvanzk/RadioHead/tree/master/examples/raspi/rf69_izk


