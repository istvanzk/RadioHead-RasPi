# Makefile
# Sample for RH_RF22B (client and server) on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC            = g++
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS          = -lbcm2835 -lrt
RADIOHEADBASE = ../../..
INCLUDE       = -I$(RADIOHEADBASE)

all: rf22b_client rf22b_server rf22b_reliable_datagram_client rf22b_reliable_datagram_server

RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
				$(CC) $(CFLAGS) -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(INCLUDE)

rf22b_client.o: rf22b_client.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf22b_server.o: rf22b_server.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf22b_reliable_datagram_client.o: rf22b_reliable_datagram_client.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf22b_reliable_datagram_server.o: rf22b_reliable_datagram_server.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RH_RF22.o: $(RADIOHEADBASE)/RH_RF22.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHReliableDatagram.o: $(RADIOHEADBASE)/RHReliableDatagram.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHSPIDriver.o: $(RADIOHEADBASE)/RHSPIDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf22b_client: rf22b_client.o RH_RF22.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o rf22b_client

rf22b_server: rf22b_server.o RH_RF22.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o rf22b_server

rf22b_reliable_datagram_client: rf22b_reliable_datagram_client.o RH_RF22.o RHDatagram.o RHReliableDatagram.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o rf22b_reliable_datagram_client

rf22b_reliable_datagram_server: rf22b_reliable_datagram_server.o RH_RF22.o RHDatagram.o RHReliableDatagram.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o rf22b_reliable_datagram_server


clean:
				rm -rf *.o rf22b_client rf22b_server rf22b_reliable_datagram_client rf22b_reliable_datagram_server
