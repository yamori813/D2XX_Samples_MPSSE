/*
  This is bitbang base ir remocon control.
  H.M 2010.09.23
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ftd2xx.h"

#define DIV 25

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

int setdata(BYTE *buff, int val)
{
	int i, offset;
	offset = 0;
	if(val == 0) {
		for(i = 0; i < 660 / DIV; ++i) {
			buff[offset++] = 0x01;
			buff[offset++] = 0x01;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
		}
		for(i = 0; i < 540 / DIV; ++i) {
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
		}
 	} else {
		for(i = 0; i < 1245 / DIV; ++i) {
			buff[offset++] = 0x01;
			buff[offset++] = 0x01;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
		}
		for(i = 0; i < 540 / DIV; ++i) {
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
			buff[offset++] = 0x00;
		}
	}
	return offset;
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
	BYTE outBuffer[1024*32];
	int loopCount;
	
	if(argc > 1) {
		sscanf(argv[1], "%d", &iport);
	}
	else {
		iport = 0;
	}

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
/*
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Purge\n");	
	}
	usleep(1000*200);
*/
	ftStatus = FT_SetLatencyTimer(ftHandle, 10);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetLatencyTimer\n");       
		return -1;
	}
	ftStatus = FT_SetBitMode(ftHandle, 0x01, 0x01);
	if(ftStatus != FT_OK) {
		printf("Failed to set Asynchronous Bit bang Mode");
	}
	usleep(1000*200);
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Purge\n");	
	}
//	ftStatus = FT_SetBaudRate(ftHandle, 9600); // 50us
//	ftStatus = FT_SetBaudRate(ftHandle, 9600/16); // 100us
//	ftStatus = FT_SetBaudRate(ftHandle, 9600*3); // 2us
	ftStatus = FT_SetBaudRate(ftHandle, 9600*5);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetBaudRate\n");	
	}
	usleep(1000*200);
	buffCount = 0;
	for(j = 0; j < 2; ++j) {
		// start
		for(i = 0; i < 2460 / DIV; ++i) {
			outBuffer[buffCount++] = 0x01;
			outBuffer[buffCount++] = 0x01;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
		}
		for(i = 0; i < 525 / DIV; ++i) {
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
		}
		// data
		unsigned char cmd[2];
		// SONY PT-D4W A ON
		cmd[0] = 0x02;
		cmd[1] = 0x30;
		// SONY TV Power
		cmd[0] = 0xa9;
		cmd[1] = 0x00;

		for(i = 0; i < 12; ++i) {
			buffCount += setdata(&outBuffer[buffCount], 
				(cmd[i / 8] >> (7 - (i % 8)) & 0x01));
		}
		// stop
		for(i = 0; i < 25100 / DIV; ++i) {
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
			outBuffer[buffCount++] = 0x00;
		}
	}
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	} else {
		printf("FT_Write dwBytesInQueue = %d\n",dwBytesInQueue);
	}
	usleep(1000*100);

	if(ftHandle != NULL) {
		FT_Close(ftHandle);
		ftHandle = NULL;
	}
	
	return 0;
}

