/*
  H.M 2010.05.04
  referred http://www.hdl.co.jp/USB/mpsse_spi/
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ftd2xx.h"

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

int main(int argc, char *argv[])
{
	DWORD dwBytesInQueue = 0;
	DWORD dwRxBytes;
	FT_STATUS	ftStatus;
	unsigned char ucMode = 0x00;
	int iport;
	int buffCount;
	BYTE outBuffer[16];
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

	ftStatus = FT_SetLatencyTimer(ftHandle, 100);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetLatencyTimer\n");	
	}
	usleep(1000*200);

	ftStatus = FT_SetBitMode(ftHandle, 0, 0);
	if(ftStatus != FT_OK) {
		printf("Failed to set mpsse mode\n");	
	}
	usleep(1000*200);
*/

	ftStatus = FT_SetLatencyTimer(ftHandle, 10);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetLatencyTimer\n");       
		return -1;
	}
	ftStatus = FT_SetBitMode(ftHandle, 0xFB, 0x02);
	if(ftStatus != FT_OK) {
		printf("Failed to set mpsse mode\n");	
	}
	usleep(1000*200);
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Purge\n");	
	}
	usleep(1000*200);
	buffCount = 0;
	outBuffer[buffCount++] = 0x86;  // Set TCK/SK Divisor
	outBuffer[buffCount++] = 0x00;
	outBuffer[buffCount++] = 0x00;

	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN

//	outBuffer[buffCount++] = 0x85; // Disconnect TDI/DO to TTDO/DI for Loopback
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	usleep(1000*100);
/*
	buffCount = 0;
	// Clock Data Bytes Out on -ve Clock Edge MSB First (no Read)
	outBuffer[buffCount++] = 0x11;
	outBuffer[buffCount++] = 0x00; // LengthL
	outBuffer[buffCount++] = 0x00; // LengthH
	outBuffer[buffCount++] = 0xAA; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	} else {
		printf("FT_Write dwBytesInQueue %d\n", dwBytesInQueue);	
	}
	buffCount = 0;
	// Clock Data Bytes Out on -ve Clock Edge LSB First (no Read)
	outBuffer[buffCount++] = 0x19;
	outBuffer[buffCount++] = 0x00; // LengthL
	outBuffer[buffCount++] = 0x00; // LengthH
	outBuffer[buffCount++] = 0xAA; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
*/
	for(loopCount = 0; loopCount < 1024; ++loopCount) {
	buffCount = 0;
	// Clock Data Bits In and Out on LSB First
	outBuffer[buffCount++] = 0x3A;
	outBuffer[buffCount++] = 0x00; // Length
	outBuffer[buffCount++] = 0x00; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	do {
	ftStatus = FT_GetQueueStatus(ftHandle, &dwRxBytes);
	if(ftStatus != FT_OK) {
		printf("FT_GetQueueStatus\n");	
	}
	} while(dwRxBytes == 0);
	BYTE pbyRxBuffer[16];
	DWORD dwRxSize = 1;
	DWORD dwBytesReturned;
	ftStatus = FT_Read(ftHandle, pbyRxBuffer, dwRxSize, &dwBytesReturned);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Read\n");
	} else {
//		printf("FT_Read %02x\n", pbyRxBuffer[0] >> 7);	
	}
 }
	/*
	buffCount = 0;
	// Clock Data Bits Out on -ve Clock Edge MSB First (no Read)
	outBuffer[buffCount++] = 0x13;
	outBuffer[buffCount++] = 0x03; // Length
	outBuffer[buffCount++] = 0xA0; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	*/	

	usleep(1000*100);

	if(ftHandle != NULL) {
		FT_Close(ftHandle);
		ftHandle = NULL;
	}
	
	return 0;
}

