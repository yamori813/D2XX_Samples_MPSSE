//
// original is https://kiidax.wordpress.com/2011/12/11/jtag_tap/
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

#ifdef FT2232
// TCK(output)	AD0	0001
// TDI(output)	AD1	0010
// TDO(input)	AD2	0100
// TMS(output)	AD3	1000

// Globals
FT_HANDLE ftHandle = NULL;
#endif

int irchainlen;

int
check_wiggler(void)
{
  int t, ok;

#ifdef FT2232
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
  // 15 milliseconds
  ftStatus = FT_SetLatencyTimer(ftHandle, 15);
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
#else
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
#endif
  return ok ? 0 : -1;
}

int
wiggle(int tms, int tdi)
{
  int t, tdo;

  printf("tms=%d tdi=%d ", tms, tdi);
#ifdef FT2232
  BYTE pbyRxBuffer[16];
  DWORD dwRxSize = 1;
  DWORD dwBytesReturned;
  int buffCount;
  BYTE outBuffer[16];
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
  // out on -ve edge, in on +ve edge
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
#else
  t = nTRST | VCC1 | VCC2;
  t |= tms ? TMS : 0;
  t |= tdi ? TDI : 0;
  outb(t, LPT1DATA);
  usleep(SLEEPTIME);
  tdo = (inb(LPT1STATUS) & TDO) ? 0 : 1; /* Busy is active low */
  t |= TCK;
  outb(t, LPT1DATA);
  usleep(SLEEPTIME);
#endif
  printf("tdo=%d ", tdo);
  return tdo;
}

void
ir(int nir, int len)
{
  int i, tdo;
  wiggle(1, 0); printf("Select-DR-Scan\n");
  wiggle(1, 0); printf("Select-IR-Scan\n");
  wiggle(0, 0); printf("Capture-IR\n");
  wiggle(0, 0); printf("Shift-IR\n");
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (nir >> i) & 1);
      if (i == len - 1)
        printf("IR %d, Exit1-IR\n", i);
      else
        printf("IR %d\n", i);
    }
  wiggle(1, 0); printf("Update-IR\n");
  wiggle(0, 0); printf("Run-Test/Idle\n");
}

int
dr(int ndr, int len)
{
  int i, tdo, cdr;
  wiggle(1, 0); printf("Select-DR-Scan\n");
  wiggle(0, 0); printf("Capture-DR\n");
  wiggle(0, 0); printf("Shift-DR\n");
  cdr = 0;
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (ndr >> i) & 1);
      if (i == len - 1)
        printf("DR %d, Exit1-DR\n", i);
      else
        printf("DR %d\n", i);
      cdr += tdo << i;
    }
  wiggle(1, 0); printf("Update-DR\n");
  wiggle(0, 0); printf("Run-Test/Idle\n");
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

void
detect(void)
{
  int i, j, tdo, cdr, chain;

  wiggle(1, 0); printf("Select-DR-Scan\n");
  wiggle(0, 0); printf("Capture-DR\n");
  wiggle(0, 0); printf("Shift-DR\n");

  j = 0;
  cdr = 0;
  chain = 0;
  for (i = 0; i < 100; i++)
    {
      tdo = wiggle(0, i == 0 ? 1 : 0); printf("IDCODE %d\n", j);
      if (j == 0 && tdo == 0)
        {
          /* BYPASS */
          printf("IDCODE not supported\n");
        }
      else
        {
          cdr += (tdo ? 1 : 0) << j;
          j++;
          if (j == 32)
            {
              printf("IDCODE: chain -%d, %08x\n", chain, cdr);
              if ((cdr & 0xff) == 1)
                {
                  printf("Invalid IDCODE. No more chains.\n");
                  break;
                }
              j = 0;
              cdr = 0;
              chain++;
            }
        }
    }
  wiggle(1, 1); printf("Exit1-DR\n");
  wiggle(1, 0); printf("Update-DR\n");
  wiggle(0, 0); printf("Run-Test/Idle\n");
}

void
irlen(void)
{
  int i, tdo, ptdo;

  wiggle(1, 0); printf("Select-DR-Scan\n");
  wiggle(1, 0); printf("Select-IR-Scan\n");
  wiggle(0, 0); printf("Capture-IR\n");
  wiggle(0, 0); printf("Shift-IR\n");

  tdo = wiggle(0, 1); printf("IR 0, tdo must be 1.\n");
  tdo = wiggle(0, 1); printf("IR 1, tdo must be 0.\n");
  ptdo = tdo;
  for (i = 2; i < 30; i++)
    {
      tdo = wiggle(0, 0); printf("IR %d\n", i);
      if (tdo == 1 && ptdo == 1)
        {
          printf("tdo sequence 11 found at IR %d\n", i);
          printf("The IR chain length is %d\n", i - 1);
          irchainlen = i - 1;
          break;
        }
      ptdo = tdo;
    }
  tdo = wiggle(1, 0); printf("IR %d, Exit1-IR\n", i);
  wiggle(1, 0); printf("Update-IR\n");
  wiggle(0, 0); printf("Run-Test/Idle\n");
}

#define IDCODE 0x1
#define IMPCODE 0x3

void
idcode(void)
{
  int cdr;
  ir(IDCODE, 4);
  cdr = dr(0, 32);
  printf("IDCODE=%08x\n", cdr);
}

void
impcode(void)
{
  int cdr;
  ir(IMPCODE, irchainlen);
  cdr = dr(0, 32);
  printf("IMPCDOE=%08x\n", cdr);
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
  reset();
  detect();
  irlen();
  idcode();
  impcode();

#ifdef FT2232
  if(ftHandle != NULL) {
    FT_SetBitMode(ftHandle, 0x00, 0x00);
    FT_Close(ftHandle);
    ftHandle = NULL;
  }
#endif
  exit(0);
}
