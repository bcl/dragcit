/* -------------------------------------------------------------------- */
/*  PORT.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  This module should contain all the code specific to the modem       */
/*  hardware. This is done in an attempt to make the code more portable */
/*                                                                      */
/*  Note: under the MS-DOS implementation there is also an ASM file     */
/*  contains some of the very low-level io rutines.                     */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

extern void         INT_INIT(int, int);
extern void         COM_EXIT(void);
extern void         COM_INIT(int, int, int, int);
extern unsigned int COM_STAT(void);
extern int          COM_READ(void);
extern void         COM_WRITE(unsigned char);
extern void         DROP_DTR(void);
extern void         RAISE_DTR(void);

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*      baud()                  sets serial baud rate                   */
/*      carrier()               checks carrier                          */
/*      drop_dtr()              turns DTR off                           */
/*      getMod()                bottom-level modem-input                */
/*      outMod()                bottom-level modem output               */
/*      Hangup()                breaks modem connection                 */
/*      Initport()              sets up modem port and modem            */
/*      portExit()              Exit cserl.obj pakage                   */
/*      portInit()              Init cserl.obj pakage                   */
/*      ringdetect()            returns 1 if ring detect port is true   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  07/23/89    (RGJ)   Fixed outMod to not overrun 300 baud on a 4.77  */
/*  04/06/89    (PAT)   Speed up outMod to keep up with 2400 on a 4.77  */
/*  04/01/89    (RGJ)   Changed outp() calls to not screw with the      */
/*                      CSERL.OBJ package.                              */
/*  04/01/89    (PAT)   Moved outMod() into port.c                      */
/*  03/22/89    (RGJ)   Change gotCarrier() and ringdetect() to         */
/*                      use COM_STAT() instead of polled I/O.           */
/*                      Also removed all outp's from baud().            */
/*                      And changed drop_dtr() and Hangup() so interupt  */
/*                      package is off when fiddling with DTR.          */
/*  02/22/89    (RGJ)   Change to Hangup, disable, initport             */
/*  02/07/89    (PAT)   Module created from MODEM.C                     */
/*                                                                      */
/* -------------------------------------------------------------------- */

static char modcheck = 1;

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
                    /*   COM     #1     #2     #3     #4  */
static int ports[] =  { 0x00, 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
static int irqs[]  =  {    0,     4,     3,     4,     3 };
static char modemcheck[] = { 1, 5, 10, 20, 40, 0 };

/* -------------------------------------------------------------------- */
/*      ringdetect() returns true if the High Speed line is up          */
/*                   if there is no High Speed line avalible to your    */
/*                   hardware it should return the ring indicator line  */
/*                   In this way you can make a custom cable and use it */
/*                   that way                                           */
/* -------------------------------------------------------------------- */
int ringdetect(void)
{
     return( (COM_STAT()&0x0040) ? TRUE : FALSE );  /* RI */
}

/* -------------------------------------------------------------------- */
/*      MOReady() is modem port ready for output                        */
/* -------------------------------------------------------------------- */
int MOReady(void)
{
    return(COM_STAT()&0x4000 ? TRUE : FALSE );
}

/* -------------------------------------------------------------------- */
/*      MIReady() Ostensibly checks to see if input from modem ready    */
/* -------------------------------------------------------------------- */
int MIReady(void)
{
    return(COM_STAT()&0x0100 ? TRUE : FALSE );
}

/* -------------------------------------------------------------------- */
/*      Initport()  sets up the modem port and modem. No return value   */
/* -------------------------------------------------------------------- */
void Initport(void) 
{
    haveCarrier = modStat = (char)gotCarrier();

    Hangup();

    disabled = FALSE;

    baud(cfg.initbaud);

    outstring(cfg.modsetup); 
    outstring("\r");

    pause(100);

    update25();
}

/* -------------------------------------------------------------------- */
/*      Hangup() breaks the modem connection                            */
/* -------------------------------------------------------------------- */
void Hangup(void)
{
    drop_dtr();
    RAISE_DTR();
    pause(50);
}

/* -------------------------------------------------------------------- */
/*      gotCarrier() returns nonzero on valid carrier, else zero        */
/* -------------------------------------------------------------------- */
int gotCarrier(void)
{
    return (COM_STAT()&0x0080) ? TRUE : FALSE; /* DCD */
}

/* -------------------------------------------------------------------- */
/*      getMod() is bottom-level modem-input routine                    */
/* -------------------------------------------------------------------- */
int getMod(void)
{
    received++;  
    return(COM_READ()); 
}

/* -------------------------------------------------------------------- */
/*      drop_dtr() turns dtr off                                        */
/* -------------------------------------------------------------------- */
void drop_dtr(void)
{
    disabled = TRUE;
    DROP_DTR();
    pause(50);
}

/* -------------------------------------------------------------------- */
/*      baud() sets up serial baud rate  0=300; 1=1200; 2=2400; 3=4800  */
/*                                       4=9600                         */
/*      and initializes port for general bbs usage   N81                */
/* -------------------------------------------------------------------- */
void baud(int baudrate)
{
    COM_INIT((int)(115200L/bauds[baudrate]), 0, 8, 1);
    speed = (char)baudrate;
    modcheck = modemcheck[speed];
}

/* -------------------------------------------------------------------- */
/*      outMod() stuffs a char out the modem port                       */
/* -------------------------------------------------------------------- */
void outMod(unsigned char ch)
{
    long t, l;

    if (!modem && !callout) return;

    /* dont go faster than the modem, check every modcheck */
    if ( !(transmitted % modcheck) )
    {
        time(&t);

        while(!MOReady())
        {
            if (time(&l) > t+3)
            {
                cPrintf("Modem write failed!\n");
                return;
            }
        }
    }

    COM_WRITE(ch);

    ++transmitted;  /* keep track of how many chars sent */
}

/* -------------------------------------------------------------------- */
/*      portInit() sets up the interupt driven I/O package              */
/* -------------------------------------------------------------------- */
void portInit(void)
{
    INT_INIT(ports[cfg.mdata], irqs[cfg.mdata]);
}

/* -------------------------------------------------------------------- */
/*      portExit() removes the interupt driven I/O package              */
/* -------------------------------------------------------------------- */
void portExit(void)
{
    COM_EXIT();
}

