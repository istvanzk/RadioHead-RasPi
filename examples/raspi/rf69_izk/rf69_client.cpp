// rf69_client.cpp
// -*- mode: C++ -*-
// Example program showing how to use RH_RF69 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM69HCW module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf69_izk
// make
// sudo ./rf69_client
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon
// Contributed by Istvan Z. Kovacs based on code rf22b_client.cpp for Raspberry Pi

// Packet Format - excerpt from RF_rf69.h
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

#include <RH_RF69.h>

// Constants with CS and IRQ pin definition
// HopeRF RFM69HCW based radio modules (no onboard led - reset pin not used)
// =========================================
// https://www.sparkfun.com/products/13910
#define RF_CS_PIN  RPI_V2_GPIO_P1_24 // Slave Select on CE0 so P1 connector pin #24
#define RF_IRQ_PIN RPI_V2_GPIO_P1_22 // IRQ on GPIO25 so P1 connector pin #22
#define RF_LED_PIN NOT_A_PIN	     // No onboard led to drive
#define RF_RST_PIN NOT_A_PIN		 // No onboard reset

// Our RFM69HCW Configuration
#define RF_FREQUENCY  434.00
#define RF_TXPOW      RH_RF69_TXPOW_11DBM
#define RF_GROUP_ID   22 // All devices
#define RF_GATEWAY_ID 1  // Server ID (where to send packets)
#define RF_NODE_ID    10 // Client ID (device sending the packets)

// Create an instance of a driver
RH_rf69 rf69(RF_CS_PIN, RF_IRQ_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
  printf("\n%s Interrupt signal (%d) received. Exiting!\n", __BASEFILE__, sig);
  //force_exit=true;

  bcm2835_spi_end();
  bcm2835_close();

  exit(sig);
}

//Main Function
int main (int argc, const char* argv[] )
{
  static unsigned long last_millis;

  signal(SIGINT, sig_handler);
  printf( "%s started\n", __BASEFILE__);

  if (!bcm2835_init()) {
    fprintf( stderr, "\n%s BCM2835: init() failed\n", __BASEFILE__ );
    return 1;
  }

  // IRQ Pin input/pull up
  // When RX packet is available the pin is pulled down (IRQ is low!)
  bcm2835_gpio_fsel(RF_IRQ_PIN, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_UP);

  if (!rf69.init()) {
    fprintf( stderr, "\nRF69: Module init() failed. Please verify wiring/module\n" );
  } else {
    printf( "RF69: Module seen OK. Using: CS=GPIO%d, IRQ=GPIO%d\n", RF_CS_PIN, RF_IRQ_PIN);

    // Since we may check IRQ line with bcm_2835 Falling edge (or level) detection
    // in case radio already have a packet, IRQ is low and will never
    // go to high so never fire again
    // Except if we clear IRQ flags and discard one if any by checking
    rf69.available();

    // Enable Falling Edge Detect Enable for the specified pin.
    // When a falling edge is detected, sets the appropriate pin in Event Detect Status.
    //bcm2835_gpio_fen(RF_IRQ_PIN);
    //printf("BCM2835: Falling edge detect enabled on GPIO%d\n", RF_IRQ_PIN);


    // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36, 8dBm Tx power

    // High power version (RFM69HW or RFM69HCW) you need to set
    // transmitter power to at least 14 dBm up to 20dBm
    rf69.setTxPower(RF_TXPOW);

    // Set Network ID (by sync words)
    // Use this for any non-default setup, else leave commented out
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = 0xd4; //RF_GROUP_ID;
    rf69.setSyncWords(syncwords, sizeof(syncwords));

    // Adjust Frequency
    //rf69.setFrequency(RF_FREQUENCY, 0.05);

    // This is our Node ID
    rf69.setThisAddress(RF_NODE_ID);
    rf69.setHeaderFrom(RF_NODE_ID);

    // Where we're sending packet
    rf69.setHeaderTo(RF_GATEWAY_ID);

    printf("RF69: Group #%d, Node #%d to GW #%d init OK @ %3.2fMHz with 0x%02X TxPw\n", RF_GROUP_ID, RF_NODE_ID, RF_GATEWAY_ID, RF_FREQUENCY, RF_TXPOW);

    last_millis = millis();

    //Begin the main body of code
    while (!force_exit)
    {

      //printf( "\tmillis()=%ld last=%ld diff=%ld\n", millis() , last_millis,  millis() - last_millis );

      // Send every 5 seconds
      if ( millis() - last_millis > 5000 )
      {
        last_millis = millis();

        // Send a message to RF69_server
        uint8_t data[] = "Hi Raspi!";
        uint8_t len = sizeof(data);

        printf("RF69: Sending %02d bytes to GW #%d => '", len, RF_GATEWAY_ID );
        printbuffer(data, len);
        printf("' ");
        if(rf69.send(data, len)) // calls waitPacketSent()
            printf(" : Sent OK!\n" );
        else
            printf(" : NOT Sent!\n" );
        //bcm2835_gpio_set_eds(RF_IRQ_PIN);
      }

      // Now wait for a reply

      // We have a IRQ pin, pool it instead reading
      // Modules IRQ registers from SPI in each loop

      // Falling edge fired ?
      //if (bcm2835_gpio_eds(RF_IRQ_PIN))
      if (bcm2835_gpio_lev(RF_IRQ_PIN) == LOW)
      {
        // Now clear the eds flag by setting it to 1
        //bcm2835_gpio_set_eds(RF_IRQ_PIN);
        //printf("BCM2835: Falling edge event detected for pin GPIO%d\n", RF_IRQ_PIN);
        printf("BCM2835: LOW detected for pin GPIO%d\n", RF_IRQ_PIN);

        uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        uint8_t from; //= rf69.headerFrom();
        uint8_t to;   //= rf69.headerTo();
        uint8_t id;   //= rf69.headerId();
        uint8_t flags;//= rf69.headerFlags();

        if (rf69.recvfrom(buf, &len, &from, &to, &id, &flags))
        {
            // Should be a message for us now
            int8_t rssi  = rf69.lastRssi();
            printf("RF69: Packet received, %02d bytes, from 0x%02X to 0x%02X, ID: 0x%02X, F: 0x%02X, with %ddBm => '", len, from, to, id, flags, rssi);
            printbuffer(buf, len);
            printf("'\n");
        } else
            printf("RF69: Packet receive failed\n");

      }

      // Let OS doing other tasks
      // Since we do nothing until each 5 sec
      delay(200);
    }
  }

  printf( "\n%s Ending\n", __BASEFILE__ );
  //bcm2835_gpio_clr_fen(RF_IRQ_PIN);
  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}

