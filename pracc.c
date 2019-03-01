//
// original is http://katsuyaiida.tripod.com/jtag.html
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef FT2232
#include <sys/io.h>
#else
#include "ftd2xx.h"
#endif

#define SRESET  (1 << 0)
#define TMS     (1 << 1)
#define TCK     (1 << 2)
#define TDI     (1 << 3)
#define nTRST   (1 << 4)
#define VCC1    (1 << 5)
#define D6      (1 << 6)
#define VCC2    (1 << 7)

#define TDO     (1 << 7)
#define ERROR   (1 << 3)

#define LPT1BASE   0x378
#define LPT1LEN    3
#define LPT1DATA   (LPT1BASE + 0)
#define LPT1STATUS (LPT1BASE + 1)
#define LPT1CTRL   (LPT1BASE + 2)

#define SLEEPTIME  (10 * 1000) /* msec */

#define IDCODE  0x01
#define IMPCODE 0x03
#define ADDRESS 0x08
#define DATA    0x09
#define CONTROL 0x0a
#define IRLEN 5

#define ECR_ROCC     (1 << 31)
#define ECR_PRNW     (1 << 19)
#define ECR_PRACC    (1 << 18)
#define ECR_PROBEN   (1 << 15)
#define ECR_PROBTRAP (1 << 14)
#define ECR_EJTAGBRK (1 << 12)
#define ECR_DM       (1 <<  3)

//#define FTSLOW
//#define READPRID

#ifdef READPRID
const int code[] = {
  0x40027800, 	/* mfc0	v0,c0_prid */
  0x3c04ff20, 	/* lui	a0,0xff20 */
  0xac820000, 	/* sw	v0,0(a0) */
  0x0000000f, 	/* sync */
  0x4200001f, 	/* deret */
};
#else
const int code[] = {
  0x24040005, /* li	a0,5 */
  0x24020000, /* li	v0,0 */
  0x00441021, /* addu	v0,v0,a0 */
  0x2484ffff, /* addiu	a0,a0,-1 */
  0x1480fffd, /* bnez	a0,0xff200208 */
  0x00000000, /* nop */
  0x3c04ff20, /* lui	a0,0xff20 */
  0x34840000, /* ori	a0,a0,0x0000 */
  0xac820000, /* sw	v0,0(a0) */
  0x0000000f, /* sync */
  0x4200001f  /* deret */
};
#endif

#ifdef FT2232
// Globals
FT_HANDLE ftHandle = NULL;
#endif

#ifndef FT2232
int
check_wiggler(void)
{
  int t, ok;
  outb(TMS | TCK | TDI | D6 | VCC1 | VCC2, LPT1DATA);
  printf("Holding D6 high\n");
  usleep(SLEEPTIME);
  t = inb(LPT1STATUS);
  ok = t & ERROR;
  printf("ERROR is %s\n", (t & ERROR) ? "high" : "low");
  outb(TMS | TCK | TDI | VCC1 | VCC2, LPT1DATA);
  printf("Holding D6 low\n");
  usleep(SLEEPTIME);
  t = inb(LPT1STATUS);
  ok = ok && !(t & ERROR);
  printf("ERROR is %s\n", (t & ERROR) ? "high" : "low");
  if (ok)
    printf("Wiggler cable is detected\n");
  return ok ? 0 : -1;
}

int
wiggle(int tms, int tdi)
{
  int t, tdo;
#if 0
  printf("tms=%d tdi=%d ", tms, tdi);
#endif
  t = nTRST | VCC1 | VCC2;
  t |= tms ? TMS : 0;
  t |= tdi ? TDI : 0;
  outb(t, LPT1DATA);
#if 0
  usleep(SLEEPTIME);
#endif
  tdo = (inb(LPT1STATUS) & TDO) ? 0 : 1; /* Busy is active low */
  t |= TCK;
  outb(t, LPT1DATA);
#if 0
  usleep(SLEEPTIME);
  printf("tdo=%d ", tdo);
#endif
  return tdo;
}
#else
int
check_wiggler(void)
{
	int t, ok;
	
	FT_STATUS	ftStatus;
	ftStatus = FT_Open(0, &ftHandle);
	int buffCount;
	BYTE outBuffer[16];
	DWORD dwBytesInQueue = 0;
	if(ftStatus != FT_OK) {
		printf("FT_Open failed = %d\n", ftStatus);
		return -1;
	}
	ftStatus = FT_ResetDevice(ftHandle);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_ResetDevice\n");       
		return -1;
	}
#ifndef FTSLOW
	// 5 milliseconds
	ftStatus = FT_SetLatencyTimer(ftHandle, 5);
#else
	// 30 milliseconds
	ftStatus = FT_SetLatencyTimer(ftHandle, 30);
#endif
	if(ftStatus != FT_OK) {
		printf("Failed to FT_SetLatencyTimer\n");       
		return -1;
	}
	ftStatus = FT_SetBitMode(ftHandle, 0xFB, 0x02);
	if(ftStatus != FT_OK) {
		printf("Failed to set mpsse mode\n");	
		return -1;
	}
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Purge\n");	
		return -1;
	}
	buffCount = 0;
	outBuffer[buffCount++] = 0x86;  // Set TCK/SK Divisor
	outBuffer[buffCount++] = 0x00;  // 6 MHz
	outBuffer[buffCount++] = 0x00;
	
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
		return -1;
	}
	ok = 1;
	if (ok)
		printf("FT2232 module is detected\n");
	return ok ? 0 : -1;
}

int
wiggle(int tms, int tdi)
{
	int t, tdo;
	
//	printf("tms=%d tdi=%d ", tms, tdi);
	BYTE pbyRxBuffer[16];
	DWORD dwRxSize = 1;
	DWORD dwBytesReturned;
	int buffCount;
	BYTE outBuffer[32];
	FT_STATUS	ftStatus;
	DWORD dwBytesInQueue = 0;
	DWORD dwRxBytes;
	buffCount = 0;
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	if(tms)
		outBuffer[buffCount++] = 0x08; // Value
	else
		outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN
	
	// Clock Data Bits In and Out on LSB First
	// Out on positive edge, in on negative edge
	outBuffer[buffCount++] = 0x3B;
	outBuffer[buffCount++] = 0x00; // Length
	if(tdi)
		outBuffer[buffCount++] = 0x01; // Byte1
	else
		outBuffer[buffCount++] = 0x00; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	do {
		usleep(1000);
		ftStatus = FT_GetQueueStatus(ftHandle, &dwRxBytes);
		if(ftStatus != FT_OK) {
			printf("FT_GetQueueStatus\n");	
		}
	} while(dwRxBytes == 0);
	ftStatus = FT_Read(ftHandle, pbyRxBuffer, dwRxSize, &dwBytesReturned);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Read\n");	
	} else {
		tdo = pbyRxBuffer[0] >> 7;
	}
//	printf("tdo=%d ", tdo);
	return tdo;
}

void
ir_ft(int tdi, int len)
{
	int i, t;
	
	//	printf("tms=%d tdi=%d ", tms, tdi);
	int buffCount;
	BYTE outBuffer[128];
	FT_STATUS	ftStatus;
	DWORD dwBytesInQueue = 0;
	buffCount = 0;
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	// Clock Data Bits In and Out on LSB First
	// Out on positive edge, in on negative edge
#if 1
	for (i = 0; i < len; i++)
    {
		if(i == (len - 1)) {
			outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
			outBuffer[buffCount++] = 0x08; // Value
			outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
			outBuffer[buffCount++] = 0x1A;
			outBuffer[buffCount++] = 0; // Length
			outBuffer[buffCount++] = (tdi >> i) & 1; // Byte1
		} else {
			outBuffer[buffCount++] = 0x1A;
			outBuffer[buffCount++] = 0; // Length
			outBuffer[buffCount++] = (tdi >> i) & 1; // Byte1
		}
    }
#else
	// why not work ???
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 3; // Length
	outBuffer[buffCount++] = (tdi & 0x0f); // Byte1
	
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = tdi >> 4; // Byte1
#endif
	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1

	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
}

int
dr_ft(int ndr)
{
	int i, t, cdr;
	
	//	printf("tms=%d tdi=%d ", tms, tdi);
	BYTE pbyRxBuffer[32];
	DWORD dwBytesReturned;
	int buffCount;
	BYTE outBuffer[512];
	FT_STATUS	ftStatus;
	DWORD dwBytesInQueue = 0;
	DWORD dwRxBytes;
	buffCount = 0;
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1

	for (i = 0; i < 32; i++)
    {
		if(i == 31) {
			outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
			outBuffer[buffCount++] = 0x08; // Value
			outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
			outBuffer[buffCount++] = 0x3B;
			outBuffer[buffCount++] = 0; // Length
			outBuffer[buffCount++] = (ndr >> i) & 1; // Byte1
		} else {
			outBuffer[buffCount++] = 0x3B;
			outBuffer[buffCount++] = 0; // Length
			outBuffer[buffCount++] = (ndr >> i) & 1; // Byte1
		}
    }
	
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1

	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	do {
	 usleep(1000);
	 ftStatus = FT_GetQueueStatus(ftHandle, &dwRxBytes);
	 if(ftStatus != FT_OK) {
		 printf("FT_GetQueueStatus\n");	
	 }
	} while(dwRxBytes != 32);
	ftStatus = FT_Read(ftHandle, pbyRxBuffer, dwRxBytes, &dwBytesReturned);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Read\n");	
	} else {
		cdr = 0;
		for (i = 0; i < 32; i++)
		{
			cdr |= (pbyRxBuffer[i] >> 7) << i;
		}
	}
	return cdr;
}	

void
reset_ft(void)
{
	int i;
	DWORD dwBytesReturned;
	int buffCount;
	BYTE outBuffer[128];
	FT_STATUS	ftStatus;
	DWORD dwBytesInQueue = 0;
	buffCount = 0;
	printf("Resetting TAP...\n");
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x08; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	for (i = 0; i < 6; i++) {
		outBuffer[buffCount++] = 0x1A;
		outBuffer[buffCount++] = 0; // Length
		outBuffer[buffCount++] = 0; // Byte1
	}
	outBuffer[buffCount++] = 0x80; // Set Data Bits Low Bytes
	outBuffer[buffCount++] = 0x00; // Value
	outBuffer[buffCount++] = 0x0B; // Direction 1:OUT, 0:IN	
	outBuffer[buffCount++] = 0x1A;
	outBuffer[buffCount++] = 0; // Length
	outBuffer[buffCount++] = 0; // Byte1
	ftStatus = FT_Write(ftHandle, outBuffer, buffCount, &dwBytesInQueue);
	if(ftStatus != FT_OK) {
		printf("Failed to FT_Write\n");	
	}
	printf("Run-Test/Idle\n");

}
#endif

void
ir(int nir, int len)
{
  int i, tdo;
  wiggle(1, 0); /* printf("Select-DR-Scan\n"); */
  wiggle(1, 0); /* printf("Select-IR-Scan\n"); */
  wiggle(0, 0); /* printf("Capture-IR\n"); */
  wiggle(0, 0); /* printf("Shift-IR\n"); */
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (nir >> i) & 1);
#if 0
      if (i == len - 1)
        printf("IR %d, Exit1-IR\n", i);
      else
        printf("IR %d\n", i);
#endif
    }
  wiggle(1, 0); /* printf("Update-IR\n"); */
  wiggle(0, 0); /* printf("Run-Test/Idle\n"); */
}

int
dr(int ndr, int len)
{
  int i, tdo, cdr;
  wiggle(1, 0); /* printf("Select-DR-Scan\n"); */
  wiggle(0, 0); /* printf("Capture-DR\n"); */
  wiggle(0, 0); /* printf("Shift-DR\n"); */
  cdr = 0;
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (ndr >> i) & 1);
#if 0
      if (i == len - 1)
        printf("DR %d, Exit1-DR\n", i);
      else
        printf("DR %d\n", i);
#endif
      cdr += tdo << i;
    }
  wiggle(1, 0); /* printf("Update-DR\n"); */
  wiggle(0, 0); /* printf("Run-Test/Idle\n"); */
  return cdr;
}

void
reset(void)
{
  int i, tdo;
  for (i = 0; i < 6; i++)
    {
      tdo = wiggle(1, 0); printf("Resetting TAP...\n");
    }
  wiggle(0, 0); printf("Run-Test/Idle\n");
}

int
update(int inst, int instlen, int data, int datalen)
{
  char *msg;
#ifndef FTSLOW
  ir_ft(inst, instlen);
  data = dr_ft(data);
#else
  ir(inst, instlen);
  data = dr(data, 32);
#endif
  msg = (inst == CONTROL) ? "CONTROL" :
    (inst == ADDRESS) ? "ADDRESS" :
    (inst == DATA) ? "DATA" :
    "UNKNOWN";
  printf("%s=%08x\n", msg, data);
  return data;
}

void
pracc(void)
{
  int data, w;
  data = ECR_ROCC | ECR_PRACC | ECR_PROBEN | ECR_PROBTRAP | ECR_EJTAGBRK;
  data = update(CONTROL, IRLEN, data, 32);
  if (data & ECR_ROCC)
    {
      printf("Rocc bit set\n");
      data = ECR_PRACC | ECR_PROBEN | ECR_PROBTRAP | ECR_EJTAGBRK;
      data = update(CONTROL, IRLEN, data, 32);
      data = ECR_ROCC | ECR_PRACC | ECR_PROBEN | ECR_PROBTRAP | ECR_EJTAGBRK;
      data = update(CONTROL, IRLEN, data, 32);
    }
  data = ECR_ROCC | ECR_PRACC | ECR_PROBEN | ECR_PROBTRAP;
  data = update(CONTROL, IRLEN, data, 32);
  if (data & ECR_DM)
    {
      printf("DM bit set.\n");
    }
  else
    {
      printf("DM bit unset. Abort.\n");
      return;
    }

  for (;;)
    {
      for (;;)
        {
          data = ECR_ROCC | ECR_PRACC | ECR_PROBEN | ECR_PROBTRAP;
          data = update(CONTROL, IRLEN, data, 32);
          if (!(data & ECR_DM))
            {
              printf("DM bit unset\n");
              return;
            }
          if (data & ECR_PRACC)
            {
              printf("PrAcc bit set.\n");
              w = (data & ECR_PRNW) ? 1 : 0;
              printf("PRnW bit %s.\n", w ? "set" : "unset");
              break;
            }
          else
            printf("PrAcc bit unset.\n");
        }
      data = update(ADDRESS, IRLEN, 0, 32);
      if (w)
        {
          data = update(DATA, IRLEN, 0, 32);
          printf("Target wrote: %08x\n", data);
        }
      else
        {
          if ((unsigned int)(data - 0xff200200) < sizeof code)
            data = update(DATA, IRLEN, code[(data - 0xff200200) >> 2], 32);
          else
            {
              printf("out of range\n");
              break;
            }
        }
      data = ECR_ROCC | ECR_PROBEN | ECR_PROBTRAP;
      data = update(CONTROL, IRLEN, data, 32);
    }
}

int
main(int argc, char *argv[])
{
#ifndef FT2232
	if (ioperm(LPT1BASE, LPT1LEN, 1) == -1)
    {
      perror("ioperm failed");
      exit(1);
    }
#endif
  if (check_wiggler() == -1)
    {
      fprintf(stderr, "Wiggler cable is not detected\n");
      exit(1);
    }
#ifndef FTSLOW
  reset_ft();
#else
  reset();
#endif

  pracc();

#ifdef FT2232
  FT_SetBitMode(ftHandle, 0x00, 0x00);
  FT_Close(ftHandle);
  ftHandle = NULL;
#endif

  exit(0);
}
