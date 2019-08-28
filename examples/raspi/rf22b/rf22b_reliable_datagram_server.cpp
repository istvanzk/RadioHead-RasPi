// rf22b_reliable_datagram_server.cpp
// -*- mode: C++ -*-
// Example program showing how to use RH_RF22B on Raspberry Pi
// Creates a RHReliableDatagram manager and listens to reliable datagrams sent to it from a client node
// Uses the bcm2835 library to access the GPIO pins to drive the RFM22B module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf22b
// make
// sudo ./rf22b_reliable_datagram_server
//
// Contributed by Istvan Z. Kovacs based on sample code RasPiRH.cpp by Mike Poublon, rf69_server.cpp by Charles-Henri Hallard and nrf24_reliable_datagram_server.pde by Mike McCauley


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
#include <RHReliableDatagram.h>

// RFM22B board
#define BOARD_RFM22B

// Now we include RasPi_Boards.h so this will expose defined
// constants with CS and IRQ pin definition
#include "../RasPiBoards.h"

// Our RFM22B Configuration
#define RF_FREQUENCY  434.00
#define RF_TX_DBM     RH_RF22_TXPOW_11DBM
#define RF_GROUP_ID   22 // All devices
#define RF_GATEWAY_ID 1  // Server ID (where to send packets)
#define RF_NODE_ID    10 // Client ID (device sending the packets)

// Create an instance of a driver and the manager
RH_RF22 rf22(RF_CS_PIN, RF_IRQ_PIN);
RHReliableDatagram manager(rf22, RF_GATEWAY_ID);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
  printf("\n%s Break received, exiting!\n", __BASEFILE__);
  force_exit=true;
}

void printbuffer(uint8_t buff[], int len)
{
  for (int i = 0; i< len; i++)
  {
    printf(" %2X", buff[i]);
  }
}


//Main Function
int main (int argc, const char* argv[] )
{
  unsigned long led_blink = 0;

  signal(SIGINT, sig_handler);
  printf( "%s\n", __BASEFILE__);

  if (!bcm2835_init()) {
    fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
    return 1;
  }

  printf( "RF22B CS=GPIO%d", RF_CS_PIN);

#ifdef RF_IRQ_PIN
  printf( ", IRQ=GPIO%d", RF_IRQ_PIN );
  // IRQ Pin input/pull up
  // When RX packet is available the pin is pulled down (IRQ is low!)
  pinMode(RF_IRQ_PIN, INPUT);
  bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_UP);
#endif


  if (!manager.init()) {
    fprintf( stderr, "\nRF22B module or Datagram manager init failed, Please verify wiring/module\n" );
  } else {
    printf( "\nRF22B module and Datagram manager seen OK!\r\n");


#ifdef RF_IRQ_PIN
    // Since we may check IRQ line with bcm_2835 Falling edge detection
    // In case radio already have a packet, IRQ is low and will never
    // go to high so never fire again
    // Except if we clear IRQ flags and discard one if any by checking
    manager.available();

    // Enable Falling Edge Detect Enable for the specified pin.
    // When a falling edge is detected, sets the appropriate pin in Event Detect Status.
    bcm2835_gpio_fen(RF_IRQ_PIN);
#endif


    // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36, 8dBm Tx power

    // High power version (RFM69HW or RFM69HCW) you need to set
    // transmitter power to at least 14 dBm up to 20dBm
    // NOT available in RHDatagram
    rf22.setTxPower(RF_TX_DBM);

    // set Network ID (by sync words)
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = RF_GROUP_ID;
    // NOT available in RHDatagram
    rf22.setSyncWords(syncwords, sizeof(syncwords));

    // Adjust Frequency
    // NOT available in RHDatagram
    rf22.setFrequency(RF_FREQUENCY);

    // This is our Gateway ID
    manager.setThisAddress(RF_GATEWAY_ID);
    //manager.setHeaderFrom(RF_GATEWAY_ID);

    // Where we're sending packet
    manager.setHeaderTo(RF_NODE_ID);

    printf("RF22B Group #%d, GW #%d to Node #%d init OK @ %3.2fMHz with %3.1dBm\n", RF_GROUP_ID, RF_GATEWAY_ID, RF_NODE_ID, RF_FREQUENCY, RF_TX_DBM);


    // Be sure to grab all node packet
    // we're sniffing to display, it's a demo
    // NOT available in RHDatagram
    rf22.setPromiscuous(true);

    // We're ready to listen for incoming message
    // NOT available in RHDatagram
    rf22.setModeRx();

    printf( "OK Node #%d @ %3.2fMHz\n", RF_NODE_ID, RF_FREQUENCY );
    printf( "Listening ...\n" );

    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

    //Begin the main body of code
    while (!force_exit) {

#ifdef RF_IRQ_PIN
      // We have a IRQ pin ,pool it instead reading
      // Modules IRQ registers from SPI in each loop

      // Rising edge fired ?
      if (bcm2835_gpio_eds(RF_IRQ_PIN)) {
        // Now clear the eds flag by setting it to 1
        bcm2835_gpio_set_eds(RF_IRQ_PIN);
        //printf("Packet Received, Falling edge event detect for pin GPIO%d\n", RF_IRQ_PIN);
#endif

        /* Begin Reliable Datagram Code */
        if (manager.available())
        {
            // Wait for a message addressed to us from the client
            uint8_t len = sizeof(buf);
            uint8_t from;
            if (manager.recvfromAck(buf, &len, &from))
            {
                printf("Datagram Packet [%02d] #%d => #%d %ddB: ", len, from, to);
                printbuffer(buf, len);
            }
            } else {
                printf("Packet receive failed");
            }
            printf("\n");
        }
        /* End Reliable Datagram Code */

#ifdef RF_IRQ_PIN
      }
#endif

      // Let OS doing other tasks
      // For timed critical appliation you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(5);
    }
  }

  printf( "\n%s Ending\n", __BASEFILE__ );
  bcm2835_close();
  return 0;
}

