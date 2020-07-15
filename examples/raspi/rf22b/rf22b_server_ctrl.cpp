// rf22b_server_ctrl.cpp
// -*- mode: C++ -*-
// RH_RF22B based server on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM22B module
// Uses POSIX inter-process message queue to make available the received data packet
// Requires bcm2835 library to be already installed: http://www.airspayce.com/mikem/bcm2835/
// Requires boot/config.txt contains the line dtoverlay=gpio-no-irq
//
// Based on sample code rf22b_server.cpp
// By Istvan Z. Kovacs

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include <time.h>

#include <RH_RF22.h>

// Undef to remove output to stdout
// Output to stderr is always active
#define STDOUT_MSG

// RFM22B board
#define BOARD_RFM22B

// Now we include RasPi_Boards.h so this will expose defined
// constants with CS and IRQ pin definition
#include "../RasPiBoards.h"

// Our RFM22B Configuration
#define RF_FREQUENCY  434.0
#define RF_TXPOW      RH_RF22_TXPOW_11DBM
#define RF_GROUP_ID   22 // All devices
#define RF_GATEWAY_ID 1  // Server ID (where to send packets)
#define RF_NODE_ID    10 // Client ID (device sending the packets)


// Message queues parameters
#define TXQUEUE_NAME  "/rf22b_server_tx"
#define MAX_TXMSG_SIZE 255

#define RXQUEUE_NAME  "/rf22b_server_rx"
#define MAX_RXMSG_SIZE 10


#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(1); \
        } \
    } while (0) \


// Time string
char stime[26];
const time_t tm = time(NULL);


// Create an instance of a driver
RH_RF22 rf22(RF_CS_PIN, RF_IRQ_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;


// Signal handler
void end_sig_handler(int sig)
{
    fprintf(stdout, "\n%s Interrupt signal (%d) received. Exiting!\n", __BASEFILE__, sig);

    // End SPI & close bcm
    bcm2835_gpio_clr_len(RF_IRQ_PIN);
    bcm2835_spi_end();
    bcm2835_close();

    // Close & unlink message queues
    CHECK((mqd_t)-1 != mq_close(mqTX));
    CHECK((mqd_t)-1 != mq_unlink(mqTX));
    CHECK((mqd_t)-1 != mq_close(mqRX));
    CHECK((mqd_t)-1 != mq_unlink(mqRX));

    if(sig>0)
        exit(sig);
}


// Main Function
// TODOs:
// - Re-start if the bcm2835_init fails or hangs
// - Put RX radio packet data into a MQ msg - Done
// - Get TX radio/ cmd data from a MQ msg
// - Open/create msg queue for data write - Done
// - Open/create msg queue for data read - Done
// - Put RX data msg to the RX queue - timeout, priorities?
// - Read TX data msg from the TX queue - timout, priorities?
// - Infinite loop with error detection
// - ...
int main (int argc, const char* argv[] )
{
#ifdef STDOUT_MSG
    fprintf(stdout, "%s started\n", __BASEFILE__);
#endif

    // Signal handler
    signal(SIGABRT, end_sig_handler);
    signal(SIGTERM, end_sig_handler);
    signal(SIGINT, end_sig_handler);

    //
    // Create the message queues
    //

    // Data from RF22B module & additional info from this server app made available to other client app
    struct mq_attr tx_attr;
    tx_attr.mq_flags   = 0;                 /* Flags (ignored for mq_open()): 0 or O_NONBLOCK */
    tx_attr.mq_maxmsg  = 10;                /* Max. # of messages on queue; /proc/sys/fs/mqueue/msg_max */
    tx_attr.mq_msgsize = MAX_TXMSG_SIZE;    /* Max. message size (bytes); /proc/sys/fs/mqueue/msgsize_max */
    tx_attr.mq_curmsgs = 0;                 /* # of messages currently in queue (ignored for mq_open()) */
    mqd_t mqTX = mq_open (TXQUEUE_NAME, O_WRONLY | O_CREAT | O_NONBLOCK, 0644, &tx_attr);
    if((mqd_t)-1 == mqTX) {
        perror("TX MQ open failed");
        exit(1);
    }
    char mqTX_buffer[MAX_TXMSG_SIZE];       /* message buffer, incl. final 0x0 */
    memset(mqTX_buffer, 0, MAX_TXMSG_SIZE); /* reset message buffer */
    unsigned int TXmsg_prio = 0;            /* message priority to use */


    // Data & cmd from the other client app consuming the RF22B data (e.g. a Python script)
    struct mq_attr rx_attr;
    rx_attr.mq_flags   = 0;                 /* Flags (ignored for mq_open()): 0 or O_NONBLOCK */
    rx_attr.mq_maxmsg  = 10;                /* Max. # of messages on queue; /proc/sys/fs/mqueue/msg_max */
    rx_attr.mq_msgsize = MAX_RXMSG_SIZE;    /* Max. message size (bytes); /proc/sys/fs/mqueue/msgsize_max */
    rx_attr.mq_curmsgs = 0;                 /* # of messages currently in queue (ignored for mq_open()) */
    mqd_t mqRX = mq_open ("/rf22b_server_rx", O_RDONLY | O_CREAT | O_NONBLOCK, 0622, &rx_attr);
    if((mqd_t)-1 == mqRX) {
        perror("RX MQ open failed");
        exit(1);
    }
    char mqRX_buffer[MAX_RXMSG_SIZE];
    memset(mqRX_buffer, 0, MAX_RXMSG_SIZE);

    //
    // Init the bcm2835 library
    //

    // This can sometimes hang indefinetly (why?)
    // TODO#1: timout loop
    //unsigned long starttime = millis();
    //if ((millis() - starttime) > 100)
    if (!bcm2835_init()) {
        fprintf(stderr, "\n%s BCM2835: init() failed\n", __BASEFILE__);
        return 1;
    }

    // IRQ Pin input/pull up
    // When RX packet is available the pin is pulled down (IRQ is low on RF22B!)
    bcm2835_gpio_fsel(RF_IRQ_PIN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_UP);

    // Send msg on the queue
    time_t tm_now = time(NULL);         /* current time value */
    (void)ctime_r (&tm_now,stime);      /* formatted time string */
    stime[24] = '\0' ;                  /* drop annoying \n */
    memset(mqTX_buffer, 0, MAX_TXMSG_SIZE);
    sprintf(mqTX_buffer,"%s - BCM2835: Init OK. IRQ=GPIO%d", stime, RF_IRQ_PIN);
    if ( mq_send(mqTX, mqTX_buffer, MAX_TXMSG_SIZE, TXmsg_prio) ) {
        perror("TX MQ send failed");
    }




    // Init the RF22B module
    if (!rf22.init()) {
        fprintf(stderr, "\nRF22B: Module init() failed. Please verify wiring/module\n");
        end_sig_handler(1);
    } else {
        // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36, 8dBm Tx power
#ifdef STDOUT_MSG
        fprintf(stdout, "RF22B: Module seen OK. Using: CS=GPIO%d, IRQ=GPIO%d\n", RF_CS_PIN, RF_IRQ_PIN);
#endif
    }

    // Since we check IRQ line with bcm_2835 level detection
    // in case radio already have a packet, IRQ is low and will never
    // go to high so never fire again
    // Except if we clear IRQ flags and discard one if any by checking
    rf22b.available();

    // Enable Low Detect for the RF_IRQ_PIN pin
    // When a low level is detected, sets the appropriate pin in Event Detect Status
    bcm2835_gpio_len(RF_IRQ_PIN);
#ifdef STDOUT_MSG
    fprintf(stdout, "BCM2835: Low detect enabled on GPIO%d\n", RF_IRQ_PIN);
#endif


    // Set transmitter power to at least 11 dBm (up to 20dBm)
    rf22.setTxPower(RF_TXPOW);

    // Set Network ID (by sync words)
    // Use this for any non-default setup, else leave commented out
    uint8_t syncwords[2];
    syncwords[0] = 0x2d;
    syncwords[1] = 0xd4; //RF_GROUP_ID;
    rf22.setSyncWords(syncwords, sizeof(syncwords));

    // Adjust Frequency
    //rf22.setFrequency(RF_FREQUENCY,0.05);

    // This is our Gateway ID
    rf22.setThisAddress(RF_GATEWAY_ID);
    rf22.setHeaderFrom(RF_GATEWAY_ID);

    // Where we're sending packet
    rf22.setHeaderTo(RF_NODE_ID);

    // Be sure to grab all node packet
    // we're sniffing to display, it's a demo
    rf22.setPromiscuous(false);

    // We're ready to listen for incoming message
    rf22.setModeRx();

#ifdef STDOUT_MSG
    fprintf(stdout, "RF22B: Init OK. Group #%d, GW 0x%02X to Node 0x%02X. %3.2fMHz, 0x%02X TxPw\n", RF_GROUP_ID, RF_GATEWAY_ID, RF_NODE_ID, RF_FREQUENCY, RF_TXPOW);
#endif

    // Send msg on the queue
    tm_now = time(NULL);
    (void)ctime_r (&tm_now,stime);
    stime[24] = '\0' ;
    memset(mqTX_buffer, 0, MAX_TXMSG_SIZE);
    sprintf(mqTX_buffer,"%s - RF22B: Init OK. CS=GPIO%d, IRQ=GPIO%d. Group #%d, GW 0x%02X to Node 0x%02X. %3.2fMHz, 0x%02X TxPw", stime, RF_CS_PIN, RF_IRQ_PIN, RF_GROUP_ID, RF_GATEWAY_ID, RF_NODE_ID, RF_FREQUENCY, RF_TXPOW);
    if ( mq_send(mqTX, mqTX_buffer, MAX_TXMSG_SIZE, TXmsg_prio) ) {
        perror("TX MQ send failed");
    }


    uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from; //= rf22.headerFrom();
    uint8_t to;   //= rf22.headerTo();
    uint8_t id;   //= rf22.headerId();
    uint8_t flags;//= rf22.headerFlags();

#ifdef STDOUT_MSG
    fprintf(stdout, "\tListening ...\n");
#endif

    uint8_t last_id = 0;

    // Begin the main loop
    while (!force_exit) {

      // Low Detect ?
      if (bcm2835_gpio_eds(RF_IRQ_PIN)) {

        // Clear the eds flag
        bcm2835_gpio_set_eds(RF_IRQ_PIN);
#ifdef STDOUT_MSG
        fprintf(stdout, "BCM2835: LOW detected for pin GPIO%d\n", RF_IRQ_PIN);
#endif
        // Get the packet, header information and RSSI
        if (rf22.recvfrom(buf, &len, &from, &to, &id, &flags)) {
            int8_t rssi  = rf22.lastRssi();
#ifdef STDOUT_MSG
            fprintf(stdout, "RF22B: Packet received, %02d bytes, from 0x%02X to 0x%02X, ID: 0x%02X, F: 0x%02X, with %ddBm => '", len, from, to, id, flags, rssi);
            fprintbuffer(stdout, buf, len);
            fprintf(stdout, "'\n");
#endif
            // Send back an ACK if not done already
            if ((id != last_id) || (flags == 0x40)) {
                uint8_t ack = '!';
                rf22.setHeaderId(id);
            	rf22.setHeaderFlags(0x80);
            	rf22.sendto(&ack, sizeof(ack), from);
            	rf22.setModeRx();
                last_id = id;
            }

            // Send msg on the queue. Initial packet only.
            if (flags == 0x0) {

                tm_now = time(NULL);
                (void)ctime_r (&tm_now,stime);
                struct tm *timeinfo = localtime (&tm_now);
                stime[24] = '\0' ;

                memset(mqTX_buffer, 0, MAX_TXMSG_SIZE);
                sprintf(mqTX_buffer,"%s - RF22B: %02dB, 0x%02X - 0x%02X, ID:0x%02X, F:0x%02X, %03ddBm", stime, len, from, to, id, flags, rssi);
                if ( mq_send(mqTX, mqTX_buffer, MAX_TXMSG_SIZE, TXmsg_prio) ) {
                    perror("TX MQ send failed");
                }

                memset(mqTX_buffer, 0, MAX_TXMSG_SIZE);
                memcpy(mqTX_buffer, timeinfo, sizeof(struct tm));
                mqTX_buffer[mqTX_buffer+sizeof(struct tm)] = 0xFF;
                memcpy(mqTX_buffer+sizeof(struct tm)+1, buf, len);
                if ( mq_send(mqTX, mqTX_buffer, MAX_TXMSG_SIZE, 1) ) {
                    perror("TX MQ send failed");
                }

            }

        } else {
#ifdef STDOUT_MSG
            fprintf(stdout,"RF22B: Packet receive failed\n");
#endif
        }
      }
#ifdef STDOUT_MSG
      fflush(stdout);
#endif
      // Let OS doing other tasks
      // For timed critical application you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(100);

    }

    // End & close
#ifdef STDOUT_MSG
    fprintf(stdout, "\n%s Ending\n", __BASEFILE__ );
#endif
    end_sig_handler(0);

    return 0;
}

