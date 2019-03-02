//
// original
// https://kiidax.wordpress.com/2011/12/11/jtag_tap/
// refered
// http://www.eonet.ne.jp/~maeda/cpp/jyunretu.htm

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ftd2xx.h"

#define     NUM     5
int         tbl[NUM];
char pin[NUM][6] = {"TCK", "TDI", "TDO", "TMS", "nTRST"};

#define TCK     (1 << tbl[0])
#define TDI     (1 << tbl[1])
#define TDO     (1 << tbl[2])
#define TMS     (1 << tbl[3])

#define nTRST   (1 << tbl[4])

int find;

// Globals
FT_HANDLE ftHandle = NULL;

#define SLEEPTIME  (100) /* msec */

int irchainlen;

void  list()
{   int     i;
    for(i=0; i<NUM; i++)  printf("%d:%s ", tbl[i], pin[i]);
    printf("\n");
}

int
check_wiggler(void)
{
  int t, ok;

  FT_STATUS	ftStatus;
  BYTE port;
  int len;

  ftStatus = FT_Open(0, &ftHandle);
  if(ftStatus != FT_OK) {
    printf("FT_Open failed = %d\n", ftStatus);
	return -1;
  }
  ftStatus = FT_ResetDevice(ftHandle);
  if(ftStatus != FT_OK) {
    printf("Failed to FT_ResetDevice\n");       
    return -1;
  }
  ftStatus =  FT_SetBaudRate(ftHandle, 9600);
  ftStatus = FT_SetTimeouts(ftHandle, 1000, 1000);
	ftStatus = FT_SetBitMode(ftHandle, 0xff & ~TDO, FT_BITMODE_SYNC_BITBANG);
  if(ftStatus != FT_OK) {
    printf("Failed to set bitbang mode\n");	
    return -1;
  }
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
  port = TMS | TCK | TDI;
  ftStatus = FT_Write(ftHandle, &port, 1, &len);
	if(ftStatus != FT_OK) {
		printf("Failed to write bit\n");	
		return -1;
  }
  ftStatus = FT_Read(ftHandle, &port, 1, &len);
  ok = 1;

  return ok ? 0 : -1;
}

int
wiggle(int tms, int tdi)
{
  int t, tdo;

//  printf("tms=%d tdi=%d ", tms, tdi);
	FT_STATUS	ftStatus;
	BYTE port[2];
	int len;
	t = nTRST;
	t |= tms ? TMS : 0;
	t |= tdi ? TDI : 0;
	port[0] = t;
	port[1] = t | TCK;
	ftStatus = FT_Write(ftHandle, &port, sizeof(port), &len);	
	if(ftStatus != FT_OK) {
		printf("Failed to write bit\n");	
		return -1;
	}
	ftStatus = FT_Read(ftHandle, &port, sizeof(port), &len);	
	if(ftStatus != FT_OK) {
		printf("Failed to get bit\n");	
		return -1;
	}
	tdo = (port[1] & TDO) ? 1 : 0; /* Busy is active low */
//	usleep(SLEEPTIME);
//  printf("tdo=%d ", tdo);
  return tdo;
}

void
ir(int nir, int len)
{
  int i, tdo;
  wiggle(1, 0);// printf("Select-DR-Scan\n");
  wiggle(1, 0);// printf("Select-IR-Scan\n");
  wiggle(0, 0);// printf("Capture-IR\n");
  wiggle(0, 0);// printf("Shift-IR\n");
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (nir >> i) & 1);
/*
      if (i == len - 1)
        printf("IR %d, Exit1-IR\n", i);
      else
        printf("IR %d\n", i);
 */
    }
  wiggle(1, 0);// printf("Update-IR\n");
  wiggle(0, 0);// printf("Run-Test/Idle\n");
}

int
dr(int ndr, int len)
{
  int i, tdo, cdr;
  wiggle(1, 0);// printf("Select-DR-Scan\n");
  wiggle(0, 0);// printf("Capture-DR\n");
  wiggle(0, 0);// printf("Shift-DR\n");
  cdr = 0;
  for (i = 0; i < len; i++)
    {
      tdo = wiggle(i == len - 1, (ndr >> i) & 1);
/*
		if (i == len - 1)
        printf("DR %d, Exit1-DR\n", i);
      else
        printf("DR %d\n", i);
 */
      cdr += tdo << i;
    }
  wiggle(1, 0);// printf("Update-DR\n");
  wiggle(0, 0);// printf("Run-Test/Idle\n");
  return cdr;
}

void
reset(void)
{
  int i, tdo;
  for (i = 0; i < 6; i++)
    {
      tdo = wiggle(1, 0);// printf("Resetting TAP...\n");
    }
  wiggle(0, 0);// printf("Run-Test/Idle\n");
}

void
detect(void)
{
  int i, j, tdo, cdr, chain;

  wiggle(1, 0);// printf("Select-DR-Scan\n");
  wiggle(0, 0);// printf("Capture-DR\n");
  wiggle(0, 0);// printf("Shift-DR\n");

  j = 0;
  cdr = 0;
  chain = 0;
  for (i = 0; i < 100; i++)
    {
      tdo = wiggle(0, i == 0 ? 1 : 0);// printf("IDCODE %d\n", j);
      if (j == 0 && tdo == 0)
        {
          /* BYPASS */
//          printf("IDCODE not supported\n");
        }
      else
        {
          cdr += (tdo ? 1 : 0) << j;
          j++;
          if (j == 32)
            {
              if(cdr != 0xffffffff) {
                list();
                printf("IDCODE: chain -%d, %08x\n", chain, cdr);
                find = 1;
              }
              if ((cdr & 0xff) == 1)
                {
//                  printf("Invalid IDCODE. No more chains.\n");
                  break;
                }
              j = 0;
              cdr = 0;
              chain++;
            }
        }
    }
  wiggle(1, 1);// printf("Exit1-DR\n");
  wiggle(1, 0);// printf("Update-DR\n");
  wiggle(0, 0);// printf("Run-Test/Idle\n");
}

void
irlen(void)
{
  int i, tdo, ptdo;

  wiggle(1, 0);// printf("Select-DR-Scan\n");
  wiggle(1, 0);// printf("Select-IR-Scan\n");
  wiggle(0, 0);// printf("Capture-IR\n");
  wiggle(0, 0);// printf("Shift-IR\n");

  tdo = wiggle(0, 1);// printf("IR 0, tdo must be 1.\n");
  tdo = wiggle(0, 1);// printf("IR 1, tdo must be 0.\n");
  ptdo = tdo;
  for (i = 2; i < 30; i++)
    {
      tdo = wiggle(0, 0);// printf("IR %d\n", i);
      if (tdo == 1 && ptdo == 1)
        {
          printf("tdo sequence 11 found at IR %d\n", i);
          printf("The IR chain length is %d\n", i - 1);
          irchainlen = i - 1;
          break;
        }
      ptdo = tdo;
    }
  tdo = wiggle(1, 0);// printf("IR %d, Exit1-IR\n", i);
  wiggle(1, 0);// printf("Update-IR\n");
  wiggle(0, 0);// printf("Run-Test/Idle\n");
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
scan()
{
  if (check_wiggler() == -1)
    {
      fprintf(stderr, "Wiggler cable is not detected\n");
      exit(1);
    }
  find = 0;
  reset();
  detect();
  if (find) {
    irlen();
//    idcode();
//    impcode();
  }
  if(ftHandle != NULL) {
    FT_SetBitMode(ftHandle, 0x00, 0x00);
    FT_Close(ftHandle);
    ftHandle = NULL;
  }
}

void  jyun(int t[], int n)
{   int     i,k,w;

    if (n<2)
    {   scan();
        return;
    }
    k= n-1;
    for(i=n-1; i>=0; i--)
    {   w= t[k];
        t[k]= t[i];
        t[i]= w;
        jyun(t,k);
        t[i]= t[k];
        t[k]= w;
    }
    return;
}

//¡ú MAIN PROGRAM
int  main()
{   int     i;

    for(i=0; i<NUM; i++)    tbl[i]= i;
    jyun(tbl,NUM);

    return(0);
}
