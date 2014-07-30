/* -------------------------------------------------------------------- */
/*  INPUT.C                  Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  This file contains the input functions                              */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <conio.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  getNormStr()    gets a string and normalizes it. No default.        */
/*  getNumber()     Get a number in range (top, bottom)                 */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/*  setio()         set io flags according to whicio, echo and outflag  */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Created from MISC.C to break that moduel into   */
/*                      more managable and logical peices. Also draws   */
/*                      off MODEM.C                                     */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  getNormStr()    gets a string and normalizes it. No default.        */
/* -------------------------------------------------------------------- */
void getNormStr(char *prompt, char *s, int size, char doEcho)
{
    getString(prompt, s, size, FALSE, doEcho, "");
    normalizeString(s);
}

/* -------------------------------------------------------------------- */
/*  getNumber()     Get a number in range (top, bottom)                 */
/* -------------------------------------------------------------------- */
long getNumber(char *prompt, long bottom, long top, long dfaultnum)
{
    long try;
    label numstring;
    label buffer;
    char *dfault;

    dfault = ltoa(dfaultnum, buffer, 10);

    if (dfaultnum == -1l) dfault[0] = '\0';

    do
    {
        getString(prompt, numstring, NAMESIZE, FALSE, ECHO, dfault);
        try     = atol(numstring);
        if (try < bottom)  mPrintf("Sorry, must be at least %ld\n", bottom);
        if (try > top   )  mPrintf("Sorry, must be no more than %ld\n", top);
    }
    while ((try < bottom ||  try > top) && (haveCarrier || onConsole));
    return  (long) try;
}

/* -------------------------------------------------------------------- */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/* -------------------------------------------------------------------- */
void getString(prompt, buf, lim, QuestIsSpecial, doEcho, dfault)
char *prompt;           /* Enter PROMPT */
char *buf;              /* Where to put it */
char doEcho;            /* To echo, or not to echo, that is the question */
int  lim;               /* max # chars to read */
char QuestIsSpecial;    /* Return immediately on '?' input? */
char *dfault;           /* Default for the lazy. */
{
    char c, oldEcho, errors = 0;
    int  i;

    outFlag = IMPERVIOUS;

    if ( strlen(prompt) )
    {
        doCR();

        if (strlen(dfault))
        {
            sprintf(gprompt, "Enter %s [%s]: ", prompt, dfault);
        }
        else
        {
            sprintf(gprompt, "Enter %s: ", prompt);
        }

        mPrintf(gprompt);

        dowhat = PROMPT;    
    }

    oldEcho = echo;

    echo     = NEITHER;

    if (!doEcho)
    {
        if (!cfg.nopwecho)
        {
            echoChar = 1;
        }
        else if (cfg.nopwecho == 1)
        {
            echoChar = '\0';
        }
        else 
        {
            echoChar = cfg.nopwecho;
        }
    }

    i   = 0;

    for (i =  0, c = (char)iChar(); 
         c != 10 /* NEWLINE */ && (haveCarrier || onConsole);
         c = (char)iChar()
        )
    {
        outFlag = OUTOK;

        /*
         * handle delete chars: 
         */
        if (c == '\b')
        {
            if (i != 0)
            {
                doBS();
                i--;

                if ( (echoChar >= '0') && (echoChar <= '9'))
                {
                    echoChar--;
                    if (echoChar < '0') echoChar = '9';
                }
            }
            else 
            {
                oChar(7 /* bell */);
            }
        }
        else
        if (c == 0)
        {
            i = 0;
            break;
        }
        else
        {
            if (i < lim && c != '\t')
            {
                if ( (echoChar >= '0') && (echoChar <= '9'))
                {
                    echoChar++;
                    if (echoChar > '9') echoChar = '0';
                }

                buf[i] = c;

                if (doEcho && cfg.nopwecho)
                {
                    oChar(c);
                }
                else
                {
                    oChar(echoChar);   
                }

                i++;
            }
            else
            {
                oChar(7 /* bell */);

                errors++;
                if (errors > 15 && !onConsole)
                {
                    drop_dtr();
                }
            }
        }

        /* kludge to return immediately on single '?': */
        if (QuestIsSpecial && *buf == '?')  
        {
            doCR();
            break;
        }
    }

    echo     = oldEcho;
    buf[i]   = '\0';
    echoChar = '\0';

    if ( strlen(dfault) && !strlen(buf) ) strcpy(buf,dfault);

    dowhat = DUNO;

    doCR();
}


/* -------------------------------------------------------------------- */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/* -------------------------------------------------------------------- */
int getYesNo(char *prompt, char dfault)
{
    int  toReturn;
    char  c;
    char oldEcho;

/*    while (MIReady()) getMod(); */

    doCR();
    toReturn = ERROR;

    outFlag = IMPERVIOUS;
    sprintf(gprompt, "%s? ", prompt);

    switch(dfault)
    {
    case 0: strcat(gprompt, "(Y/N)[N]: ");      break;
    case 1: strcat(gprompt, "(Y/N)[Y]: ");      break;
    case 2: strcat(gprompt, "(Y/N/A)[A]: ");    break;
    case 3: strcat(gprompt, "(Y/N/A)[N]: ");    break;
    case 4: strcat(gprompt, "(Y/N/A)[Y]: ");    break;
    default:                   
            strcat(gprompt, "(Y/N)[N]: ");
            dfault = 0;
            break;
    }

    mPrintf(gprompt);

    dowhat = PROMPT;    
    
    do {
        oldEcho = echo;
        echo    = NEITHER;
        c       = (char)iChar();
        echo    = oldEcho;

        if ( (c == '\n') || (c == '\r') )
        {
            if (dfault == 1 || dfault == 4)  c = 'Y';
            if (dfault == 0 || dfault == 3)  c = 'N';
            if (dfault == 2)                 c = 'A';
        }

        switch (toupper(c))
        {
            case 'Y': mPrintf("Yes"  ); doCR(); toReturn   = 1;  break;
            case 'N': mPrintf("No"   ); doCR(); toReturn   = 0;  break;
            case 'A': 
                if (dfault > 1) 
                {
                    mPrintf("Abort");  doCR();

                    toReturn   = 2; 
                }
                break;
        }
    } while( toReturn == ERROR && (haveCarrier || onConsole) );

    outFlag = OUTOK;
    dowhat = DUNO;
    return   toReturn;
}

/* -------------------------------------------------------------------- */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/* -------------------------------------------------------------------- */
int BBSCharReady(void)
{
    return ( (haveCarrier && (whichIO == MODEM) && MIReady()) ||
              KBReady()  );
}

/* -------------------------------------------------------------------- */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/* -------------------------------------------------------------------- */
int iChar(void)
{
    char c = 0;
    long timer, curent;
    char str[40]; /* for baud detect */

    if (justLostCarrier)   return 0;    /* ugly patch   */

    sysopkey = FALSE; /* go into sysop mode from main()? */
    eventkey = FALSE; /* fo an event? */

    time(&timer);

    while(TRUE)
    {
        if (ExitToMsdos) return 0;

        /* just in case person hangs up in console mode */
        if (detectflag && !gotCarrier())
        {
            Initport();
            detectflag = FALSE;
        }

        if (!carrier())
        {
            return 0;
        }

        /* got carrier in console mode, switched to modem */
        if (detectflag && !onConsole && gotCarrier())
        {
            carrdetect();
            detectflag = FALSE;
            return(0);
        }    

        if (KBReady())
        {
            c = (char)getch();
            getkey = 0;
            ++received;  /* increment received char count */
            break;
        }

        if (MIReady() && !detectflag)
        {
            if (!modStat && (cfg.dumbmodem == 0))
            {
                if (getModStr(str))
                {
                    c = TRUE;

                    switch(atoi(str))   
                    {
                        case 13:  /* 9600   13 or 17   */
                        case 17:
                            baud(4);
                            break;

                        case 10:  /* 2400   10 or 16   */
                        case 16:
                            baud(2);
                            break;

                        case 5:   /* 1200   15 or  5   */
                        case 15:
                            baud(1);
                            break;

                        case 1:   /* 300    1  */
                            baud(0);
                            break;

                        case 2:   /* ring, hold cron event */
                            time(&timer);
                            c = FALSE;
                            break;

                        default:  /* something else */
                            c = FALSE;
                            break;
                    }
    
                    if (c)  /* detected */
                    {
                        if (!onConsole) 
                        {
                            detectflag = FALSE;
                            carrdetect();
                            return(0);
                        } else {
                            detectflag = TRUE;
                            c = 0;
                            update25();
                        }
                    }
                } else {
                    /* failed to adjust */
                    Initport();
                }
            } else {
                c = (char)getMod();
            }

            if (haveCarrier)
            {
                if (whichIO == MODEM) break;
            } else {
                c = 0;
            }
        }

        /* CRON events */
        time(&curent);
        if ( ((int)(curent - timer)/(int)60) >= cfg.idle
             && !loggedIn && !gotCarrier() && dowhat == MAINMENU)
        {
            time(&timer);
            if (do_cron(CRON_TIMEOUT))
                return(0);
        }

        if ((chatkey || sysopkey || eventkey) && dowhat == MAINMENU)  return(0);

        if (chatkey && dowhat == PROMPT)
        {
            char oldEcho;

            oldEcho = echo;
            echo    = BOTH;

            doCR();
            chat();
            doCR();
            mPrintf(gprompt);

            echo   = oldEcho;

            time(&timer);

            chatkey = FALSE;
        }

        if (systimeout(timer)) 
        { 
            mPrintf("Sleeping? Call again :-) \n "); 
            Initport(); 
        } 
    }

    c = ( c & 0x7F );

    c = filter[c];

    if (c != 1    /* don't print ^A's          */
        && ((c != 'p' && c != 'P') || dowhat != MAINMENU)
        /* dont print out the P at the main menu... */
       ) echocharacter(c);  

    return(c);
}

/* -------------------------------------------------------------------- */
/*  setio()         set io flags according to whicio, echo and outflag  */
/* -------------------------------------------------------------------- */
void setio(char whichio, char echo, char outflag)
{
    if ( (outflag != OUTOK) && (outFlag != IMPERVIOUS))
    {
        modem   = FALSE;
        console = FALSE;
    }
    else if (echo == BOTH)
    {
        modem   = TRUE;
        console = TRUE;
    }  
    else if (echo == CALLER)
    {
        if (whichio == MODEM)
        {
           modem   = TRUE;
           console = FALSE;
        } 
        else if (whichio == CONSOLE)
        {
           modem   = FALSE;
           console = TRUE;
        }
    }
    else if (echo == NEITHER)
    {
        modem   = TRUE;  /* FALSE; */
        console = TRUE;  /* FALSE; */
    }

    if (!haveCarrier)  modem = FALSE;
}

