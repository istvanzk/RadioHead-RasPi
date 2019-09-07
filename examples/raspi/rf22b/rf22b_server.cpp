// rf22b_server.cpp
// -*- mode: C++ -*-
// Example program showing how to use RH_RF22B on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM22B module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf22b
// make
// sudo ./rf22b_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon
// Contributed by Istvan Z. Kovacs based on sample code rf69_server.cpp by Charles-Henri Hallard

// Packet Format - excerpt from RF_RF22.h
// All messages sent and received by this Driver must conform to this packet format:
// - 8 nibbles (4 octets) PREAMBLE
// - 2 octets SYNC 0x2d, 0xd4
// - 4 octets HEADER: (TO, FROM, ID, FLAGS)
// - 1 octet LENGTH (0 to 255), number of octets in DATA
// - 0 to 255 octets DATA
// - 2 octets CRC computed with CRC16(IBM), computed on HEADER, LENGTH and DATA
//
// For technical reasons, the message format is not protocol compatible with the
// 'HopeRF Radio Transceiver Message Library for Arduino' http://www.airspayce.com/mikem/arduino/HopeRF from the same author.
// Nor is it compatible with 'Virtual Wire' http://www.airspayce.com/mikem/arduino/VirtualWire.pdf also from the same author.

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <RH_RF22.h>

// RFM22B board
#define BOARD_RFM22B

// Now we include RasPi_Boards.h so this will expose defined
// constants with CS and IRQ pin definition
#include "../RasPiBoards.h"

// Our RFM22B Configuration
#define RF_FREQUENCY  434.00
#define RF_TXPOW      RH_RF22_TXPOW_11DBM
#define RF_GROUP_ID   22 // All devices
#define RF_GATEWAY_ID 1  // Server ID (where to send packets)
#define RF_NODE_ID    10 // Client ID (device sending the packets)

// Create an instance of a driver
RH_RF22 rf22(RF_CS_PIN, RF_IRQ_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
  printf("\n%s Break received, exiting!\n", __BASEFILE__);
  force_exit=true;
}

//Main Function
int main (int argc, const char* argv[] )
{

  signal(SIGINT, sig_handler);
  printf( "%s\n", __BASEFILE__);

  if (!bcm2835_init()) {
    fprintf( stderr, "\n%s BCM2835: init() failed\n", __BASEFILE__ );
    return 1;
  }

  printf( "RF22B CS=GPIO%d", RF_CS_PIN);

#ifdef RF_IRQ_PIN
  // IRQ Pin input/pull up
  // When RX packet is available the pin is pulled down (IRQ is low!)
  bcm2835_gpio_fsel(RF_IRQ_PIN, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_UP);
#endif


  if (!rf22.init()) {
    fprintf( stderr, "\nRF22B: Module init() failed. Please verify wiring/module\n" );
  } else {
    printf( "RF22B: Module seen OK. CS=GPIO%d, IRQ=GPIO%d\n", RF_CS_PIN, RF_IRQ_PIN);


#ifdef RF_IRQ_PIN
    // Since we may check IRQ line with bcm_2835 Falling edge detection
    // In case radio already have a packet, IRQ is low and will never
    // go to high so never fire again
    // Except if we clear IRQ flags and discard one if any by checking
    rf22.available();

    // Enable Falling Edge Detect Enable for the specified pin.
    // When a falling edge is detected, sets the appropriate pin in Event Detect Status.
    bcm2835_gpio_fen(RF_IRQ_PIN);

    printf("BCM2835: Falling edge detect enabled on GPIO%d\n", RF_IRQ_PIN);
#endif


    // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36, 8dBm Tx power

    // High power version (RFM69HW or RFM69HCW) you need to set
    // transmitter power to at least 14 dBm up to 20dBm
    rf22.setTxPower(RF_TXPOW);

    // set Network ID (by sync words)
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = RF_GROUP_ID;
    rf22.setSyncWords(syncwords, sizeof(syncwords));

    // Adjust Frequency
    rf22.setFrequency(RF_FREQUENCY);

    // This is our Gateway ID
    rf22.setThisAddress(RF_GATEWAY_ID);
    rf22.setHeaderFrom(RF_GATEWAY_ID);

    // Where we're sending packet
    rf22.setHeaderTo(RF_NODE_ID);

    printf("RF22B: Group #%d, GW #%d to Node #%d init OK @ %3.2fMHz with 0x%02X TxPw\n", RF_GROUP_ID, RF_GATEWAY_ID, RF_NODE_ID, RF_FREQUENCY, RF_TXPOW);


    // Be sure to grab all node packet
    // we're sniffing to display, it's a demo
    rf22.setPromiscuous(true);

    // We're ready to listen for incoming message
    rf22.setModeRx();

    printf( "\tListening ...\n" );

    //Begin the main body of code
    while (!force_exit)
    {

#ifdef RF_IRQ_PIN
      // We have a IRQ pin ,pool it instead reading
      // Modules IRQ registers from SPI in each loop

      // Falling edge fired ?
      if (bcm2835_gpio_eds(RF_IRQ_PIN))
      {
        // Now clear the eds flag by setting it to 1
        bcm2835_gpio_set_eds(RF_IRQ_PIN);
        printf("BCM2835: Packet Received. Falling edge event detected for pin GPIO%d\n", RF_IRQ_PIN);
#endif

        if (rf22.available())
        {
          // Should be a message for us now

          printf("RF22B: Received packet available\n");

          uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];
          uint8_t len  = sizeof(buf);
          uint8_t from = rf22.headerFrom();
          uint8_t to   = rf22.headerTo();
          //uint8_t id   = rf22.headerId();
          //uint8_t flags= rf22.headerFlags();
          int8_t rssi  = rf22.lastRssi();

          if (rf22.recv(buf, &len))
          {
            printf("RF22B: Packet received [%02d] #%d => #%d %ddB: ", len, from, to, rssi);
            printbuffer(buf, len);
          } else
                printf("RF22B: Packet receive failed");

          printf("\n");
        }

#ifdef RF_IRQ_PIN
      }
#endif

      // Let OS doing other tasks
      // For timed critical appliation you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(100);
    }
  }

  printf( "\n%s Ending\n", __BASEFILE__ );
  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}

