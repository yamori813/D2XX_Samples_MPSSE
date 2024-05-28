/*
  This is data transfer program for EX-80.
  H.M 2019.06.18
  use RXD and EXC bit by bitbang
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "ftd2xx.h"

#define RXD	(1 << 5)
#define EXC	(1 << 6)

#define	MAXMEM	(16 * 1024)	// byte

// Globals
FT_HANDLE ftHandle = NULL;

void quit()
{  
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
	unsigned char ucMode = 0x00;
	int iport;
	int buffCount;
	BYTE outBuffer[MAXMEM*32];
	int loopCount;
	FILE *binFile;
	BYTE binbuff[MAXMEM];
	int binsize;
	int startaddr, endaddr;
	int sum;

	if(argc != 2) {
		printf("Usage: exlda <bin file>\n");
		exit(-1);
	}

	binFile = fopen(argv[1], "rb");
	if (binFile == NULL) {
		printf("Can't open file\n");
		exit(-1);
	}
	binsize = fread(&binbuff, 1, sizeof(binbuff), binFile);
	fclose(binFile);

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
	for(j = 0; j < 4; ++j) {
		buffCount = 0;
		// start
		for(i = 0; i < 1024; ++i) {
			outBuffer[buffCount++] = 0x00 | RXD;
			outBuffer[buffCount++] = EXC | RXD;
		}
		ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	}
	buffCount = 0;
	startaddr = 0x8200;
	sum = 0;

	buffCount += addbyte(startaddr >> 8, &outBuffer[buffCount]);
	buffCount += addbyte(startaddr & 0xff, &outBuffer[buffCount]);
	sum = (startaddr >> 8) + (startaddr & 0xff);

	endaddr = startaddr + binsize - 1;
	buffCount += addbyte(endaddr >> 8, &outBuffer[buffCount]);
	buffCount += addbyte(endaddr & 0xff, &outBuffer[buffCount]);
	sum += (endaddr >> 8) + (endaddr & 0xff);

	for (i = 0;i < binsize; ++i) {
		buffCount += addbyte(binbuff[i], &outBuffer[buffCount]);
		sum += binbuff[i];
	}

	sum = 0x100 - (sum & 0xff);
	buffCount += addbyte(sum, &outBuffer[buffCount]);

	for(i = 0; i < 128; ++i) {
		outBuffer[buffCount++] = 0x00 | RXD;
		outBuffer[buffCount++] = EXC | RXD;
	}

	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	} else {
//		printf("FT_Write dwBytesInQueue = %d\n",dwBytesInQueue);
		printf("send %d bytes\n", binsize);
	}
	usleep(1000*100);

	if(ftHandle != NULL) {
		FT_Close(ftHandle);
		ftHandle = NULL;
	}
	
	return 0;
}
