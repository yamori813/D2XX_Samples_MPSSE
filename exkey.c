/*
  This is key input program for EX-80.
  H.M 2024.05.28
  use RXD and EXC bit by bitbang
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "ftd2xx.h"

#define RXD	(1 << 5)
#define EXC	(1 << 6)

// Globals
FT_HANDLE ftHandle = NULL;
struct termios save_settings;

void quit()
{  
	tcsetattr( fileno( stdin ), TCSANOW, &save_settings );
	if(ftHandle != NULL) {
		FT_Close(ftHandle);
		ftHandle = NULL;
		printf("Closed device\n");
	}

	exit(1);
}

int addbyte(int ch, unsigned char *buf)
{
	int count, i;
	int bit;
	
	count = 0;
	// start bit
	buf[count++] = 0x00;
	buf[count++] = EXC;
	
	for(i = 0; i < 8; ++i) {
		bit = (ch >> i) & 1;
		buf[count++] = 0x00 | (bit ? RXD : 0);
		buf[count++] = EXC | (bit ? RXD : 0);
	}
	// stop bit
	buf[count++] = 0x00 | RXD;
	buf[count++] = EXC | RXD;
	buf[count++] = 0x00 | RXD;
	buf[count++] = EXC | RXD;
	
	return count;
}

int main(int argc, char *argv[])
{
	int i, j;
	DWORD dwBytesInQueue = 0;
	DWORD dwRxBytes;
	FT_STATUS	ftStatus;
	int iport;
	int buffCount;
	BYTE outBuffer[1024*16*32];
	int ch;
	struct termios settings;

	tcgetattr( fileno( stdin ), &save_settings );
	settings = save_settings;

	settings.c_lflag &= ~( ECHO | ICANON );
	tcsetattr( fileno( stdin ), TCSANOW, &settings );

	signal(SIGINT, quit);		// trap ctrl-c call quit fn 
	
	ftStatus = FT_Open(0, &ftHandle);
	if(ftStatus != FT_OK) {
		/* 
			This can fail if the ftdi_sio driver is loaded
		 	use lsmod to check this and rmmod ftdi_sio to remove
			also rmmod usbserial
		 */
		printf("FT_Open(%d) failed = %d\n", iport, ftStatus);
		return 1;
	}

	ftStatus = FT_SetBitMode(ftHandle, RXD | EXC, 0x01);
	if(ftStatus != FT_OK) {
		printf("Failed to set Asynchronous Bit bang Mode");
	}
	usleep(1000*200);
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Purge\n");	
	}
	ftStatus = FT_SetBaudRate(ftHandle, 4800);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetBaudRate\n");	
	}
	usleep(1000*200);
	while(1) {
		ch = getchar();

		if(ch != EOF) {
			buffCount = 0;

			buffCount += addbyte(ch, &outBuffer[buffCount]);

			ftStatus = FT_Write(ftHandle, outBuffer, buffCount,
			    &dwBytesInQueue);
			if(ftStatus != FT_OK) {
				printf("Failed to FT_Write\n");	
			}
			usleep(1000);
		}
	}

	return 0;
}
