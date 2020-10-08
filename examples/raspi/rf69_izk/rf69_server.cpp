// rf69_server.cpp
// -*- mode: C++ -*-
// Example program showing how to use RH_RF69 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM69HCW module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf69_izk
// make
// sudo ./rf69_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon
// Contributed by Istvan Z. Kovacs based on code rf22b_server.cpp for Raspberry Pi

// Packet Format - excerpt from RH_RF69.h
// All messages sent and received by this RH_RF69 Driver conform to this packet format:
// - 4 octets PREAMBLE
// - 2 octets SYNC 0x2d, 0xd4 (configurable, so you can use this as a network filter)
// - 1 octet RH_RF69 payload length
// - 4 octets HEADER: (TO, FROM, ID, FLAGS)
// - 0 to 60 octets DATA 
// - 2 octets CRC computed with CRC16(IBM), computed on HEADER and DATA

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

// Our RF69 Configuration
#define RF_FREQUENCY  434.00
#define RF_TXPOW      14 // since the radio is the high power HCW model, the Tx power is set in the range 14 to 20
#define RF_GROUP_ID   22 // All devices
#define RF_GATEWAY_ID 1  // Server ID (where to send packets)
#define RF_NODE_ID    10 // Client ID (device sending the packets)

// Create an instance of a driver
RH_RF69 rf69(RF_CS_PIN, RF_IRQ_PIN);

void end_sig_handler(int sig)
{
    fprintf(stdout, "\n%s Interrupt signal (%d) received. Exiting!\n", __BASEFILE__, sig);

    bcm2835_gpio_clr_len(RF_IRQ_PIN);
    bcm2835_spi_end();
    bcm2835_close();

    if(sig>0)
        exit(sig);
}

//Main Function
int main (int argc, const char* argv[] )
{

    fprintf(stdout, "%s started\n", __BASEFILE__);

    // Signal handler
    signal(SIGABRT, end_sig_handler);
    signal(SIGTERM, end_sig_handler);
    signal(SIGINT, end_sig_handler);

    // BCM library init
    if (!bcm2835_init()) {
        fprintf( stderr, "\n%s BCM2835: init() failed\n", __BASEFILE__ );
        return 1;
    }

    // IRQ Pin input/pull up
    // When RX packet is available the pin is pulled down (IRQ is low!)
    bcm2835_gpio_fsel(RF_IRQ_PIN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_UP);


    if (!rf69.init()) {
        fprintf(stderr, "\nRF69: Module init() failed. Please verify wiring/module\n");
        end_sig_handler(1);
    } else {
        fprintf(stdout, "RF69: Module seen OK. Using: CS=GPIO%d, IRQ=GPIO%d\n", RF_CS_PIN, RF_IRQ_PIN);
    }

    // Since we may check IRQ line with bcm_2835 Falling edge (or level) detection
    // in case radio already have a packet, IRQ is low and will never
    // go to high so never fire again
    // Except if we clear IRQ flags and discard one if any by checking
    rf69.available();

    // Enable Low Detect Enable for the specified pin.
    // When a low level detected, sets the appropriate pin in Event Detect Status.
    bcm2835_gpio_len(RF_IRQ_PIN);
    fprintf(stdout, "BCM2835: Low detect enabled on GPIO%d\n", RF_IRQ_PIN);


    // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36, 8dBm Tx power

    // High power version (RFM69HW or RF69) you need to set
    // transmitter power to at least 14 dBm up to 20dBm
    rf69.setTxPower(RF_TXPOW);

    // Set Network ID (by sync words)
    // Use this for any non-default setup, else leave commented out
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = 0xd4; //RF_GROUP_ID;
    rf69.setSyncWords(syncwords, sizeof(syncwords));

    // Adjust Frequency
    //rf69.setFrequency(RF_FREQUENCY,0.05);

    // This is our Gateway ID
    rf69.setThisAddress(RF_GATEWAY_ID);
    rf69.setHeaderFrom(RF_GATEWAY_ID);

    // Where we're sending packet
    rf69.setHeaderTo(RF_NODE_ID);

    fprintf(stdout, "RF69: Group #%d, GW 0x%02X to Node 0x%02X init OK @ %3.2fMHz with 0x%02X TxPw\n", RF_GROUP_ID, RF_GATEWAY_ID, RF_NODE_ID, RF_FREQUENCY, RF_TXPOW);


    // Be sure to grab all node packet
    // we're sniffing to display, it's a demo
    rf69.setPromiscuous(false);

    // We're ready to listen for incoming message
    rf69.setModeRx();


    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from; //= rf69.headerFrom();
    uint8_t to;   //= rf69.headerTo();
    uint8_t id;   //= rf69.headerId();
    uint8_t flags;//= rf69.headerFlags();

    fprintf(stdout, "\tListening ...\n" );

    uint8_t last_id = 0;

    //Begin the main body of code
    while (true) {

      // Low Detect ?
      if (bcm2835_gpio_eds(RF_IRQ_PIN)) {

        // Now clear the eds flag by setting it to 1
        bcm2835_gpio_set_eds(RF_IRQ_PIN);

        fprintf(stdout, "BCM2835: LOW detected for pin GPIO%d\n", RF_IRQ_PIN);

        if (rf69.recvfrom(buf, &len, &from, &to, &id, &flags)) {

            int8_t rssi  = rf69.lastRssi();
            fprintf(stdout, "RF69: Packet received, %02d bytes, from 0x%02X to 0x%02X, ID: 0x%02X, F: 0x%02X, with %ddBm => '", len, from, to, id, flags, rssi);
            fprintbuffer(stdout, buf, len);
            fprintf(stdout, "'\n");

            // Send back an ACK if not done already
            if (id != last_id) {
                uint8_t ack = '!';
                rf69.setHeaderId(id);
                rf69.setHeaderFlags(0x80);
                rf69.sendto(&ack, sizeof(ack), from);
                rf69.setModeRx();
                last_id = id;
            }

        } else
            fprintf(stdout, "RF69: Packet receive failed\n");

      }
      fflush(stdout);

      // Let OS doing other tasks
      // For timed critical application you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(100);

    }

    fprintf(stdout, "\n%s Ending\n", __BASEFILE__ );
    end_sig_handler(0);

    return 0;
}

