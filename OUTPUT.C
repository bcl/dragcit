/* -------------------------------------------------------------------- */
/*  OUTPUT.C                 Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  This file contains the output functions                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  getWord()       Gets the next word from the buffer and returns it   */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/*  putWord()       Writes one word to modem and console, w/ wordwrap   */
/*  asciitable()    initializes the ascii translation table             */
/*  doBS()          does a backspace to modem & console                 */
/*  doCR()          does a return to both modem & console               */
/*  dospCR()        does CR for entry of initials & pw                  */
/*  doTAB()         prints a tab to modem & console according to flag   */
/*  oChar()         is the top-level user-output function (one byte)    */
/*  updcrtpos()     updates crtColumn according to character            */
/*  mPrintf()       sends formatted output to modem & console           */
/*  cPrintf()       send formatted output to console                    */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Created from MISC.C to break that moduel into   */
/*                      more managable and logical peices. Also draws   */
/*                      off MODEM.C and FORMAT.C                        */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data/functions                                               */
/* -------------------------------------------------------------------- */
static void ansiCode(char *str);
static char buff[512];
/* -------------------------------------------------------------------- */
/*  getWord()       Gets the next word from the buffer and returns it   */
/* -------------------------------------------------------------------- */
int getWord(char *dest, char *source, register int offset, int lim)
{
    register int i = 0;

    if (isspace(source[offset]) || source[offset] == 10)
    {
        /* step over word */
        for (;  (isspace(source[offset+i]) || source[offset+i] == 10)
                && (i < lim) && source[offset+i]; i++)
            ;
    }
    else
    {
        /* step over word */
        for (; !isspace(source[offset+i]) && (i < lim) && source[offset+i]; i++)
            ;
    }
    
    strncpy(dest, source + offset, i);

    dest[i] = NULL;
    
    return(offset+i);
}

/* -------------------------------------------------------------------- */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/* -------------------------------------------------------------------- */
#define MAXWORD 256     /* maximum length of a word */
void mFormat(char *string)
{
    static char wordBuf[MAXWORD + 8];
    int  i;

    for (i = 0;  string[i] && 
    (outFlag == OUTOK || outFlag == IMPERVIOUS || outFlag == OUTPARAGRAPH); )
    {
        i = getWord(wordBuf, string, i, MAXWORD);
        putWord(wordBuf);
        if (mAbort()) return;
    }
}

/* -------------------------------------------------------------------- */
/*  putWord()       Writes one word to modem and console, w/ wordwrap   */
/* -------------------------------------------------------------------- */
void putWord(char *st)
{
    register char *s;
    register int  newColumn;

    setio(whichIO, echo, outFlag);
    
    if (!isspace(*st))
    {
        for (newColumn = crtColumn, s = st;  *s; s++)  
        {
            if (*s == '\t')      while ((++newColumn % 8) != 1);
            else if (*s == 1)    --newColumn;     /* ANSI codes*/
            else                 ++newColumn;
        }
        if (newColumn > termWidth)  doCR();
    }

    for ( ; *st; st++)
    {
        if (*st == 1)                   /* CTRL-A>nsi           */
        {
            st++;
            termCap(*st);
            continue;
        }

        /* worry about words longer than a line:   */
        if (crtColumn >= termWidth)  doCR();

        if (prevChar != 10 /* NEWLINE */  ||  (*st > ' '))
        {
            /* If there is a CR and a space before it, don't output it */
            if (!(*st == 10 /* NEWLINE */ && prevChar == ' '))
            {
                oChar(*st);
            }
            else
            {
                prevChar = 10;
            }
        }
        else
        {
            /* end of paragraph: */
            if (outFlag == OUTPARAGRAPH)  
            {
                outFlag = OUTOK;
            }
            doCR();
            oChar(*st);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  termCap()       Does a terminal command                             */
/* -------------------------------------------------------------------- */
void termCap(char c)
{
    if (!ansiOn) return;

    setio(whichIO, echo, outFlag);
    
    switch (c)
    {
    case TERM_BLINK:
        ansiCode("5m");
        ansiattr = ansiattr | 128;
        break;
    case TERM_REVERSE:
        ansiCode("7m");
        ansiattr = cfg.wattr;
        break;
    case TERM_BOLD:
        ansiCode("1m");
        ansiattr = cfg.cattr;
        break;
    case TERM_UNDERLINE:
        ansiCode("4m");
        ansiattr = cfg.uttr;
        break;
    case TERM_NORMAL:
    default:
        ansiCode("0m");
        ansiattr = cfg.attr;
        break;
    }
}

/* -------------------------------------------------------------------- */
/*   ansiCode() adds the escape if needed                               */
/* -------------------------------------------------------------------- */
static void ansiCode(char *str)
{
    char tmp[30], *p;

    if (!ansiOn) return;
  
    sprintf(tmp, "%c[%s", 27, str);

    p = tmp;

    while(*p)
    {
        outMod(*p);
        p++;
    }
}

/* -------------------------------------------------------------------- */
/*  asciitable()    initializes the ascii translation table             */
/* -------------------------------------------------------------------- */
void asciitable(void)
{
    unsigned char c;

    /* initialize input character-translation table:  */

                        /* control chars -> nulls     */
    for (c = 0;  c < '\40'; c++)  filter[c] = '\0';

                        /* pass printing chars        */
    for (c = '\40'; c < 128;   c++)  filter[c] = c;

    filter[1   ]  = 1   ;  /* ctrl-a    = ctrl-a    */
    filter[27  ]  = 27  ;  /* special   = special   */
    filter[0x7f]  = 8   ;  /* del       = backspace */
    filter[8   ]  = 8   ;  /* backspace = backspace */
    filter[19  ]  = 'P' ;  /* xoff      = 'P'       */
    filter['\r']  = 10  ;  /* '\r'      = NEWLINE   */
    filter['\t']  = '\t';  /* '\t'      = '\t'      */
    filter[10  ]  = NULL;  /* newline   = null      */
    filter[15  ]  = 'N' ;  /* ctrlo     = 'N'       */
    filter[26  ]  = 26  ;  /* ctrlz     = ctrlz     */
}

/* -------------------------------------------------------------------- */
/*  doBS()          does a backspace to modem & console                 */
/* -------------------------------------------------------------------- */
void doBS(void)
{
    oChar('\b');
    oChar(' ');
    oChar('\b');
}

/* -------------------------------------------------------------------- */
/*  doCR()          does a return to both modem & console               */
/* -------------------------------------------------------------------- */
void doCR(void)
{
    static numLines = 0;

    crtColumn = 1;

    setio(whichIO, echo, outFlag);

    domcr();
    doccr();

    if (printing)
        fprintf(printfile, "\n");

    prevChar    = ' ';

    /* pause on full screen */
    if (logBuf.linesScreen)
    {
        if (outFlag == OUTOK)
        {
            numLines++;
            if (numLines == logBuf.linesScreen)
            {
                outFlag = OUTPAUSE;
                mAbort();
                numLines = 0;
            }
        } else {
            numLines = 0;
        }
    } else {
        numLines = 0;
    }
}

/* -------------------------------------------------------------------- */
/*  dospCR()        does CR for entry of initials & pw                  */
/* -------------------------------------------------------------------- */
void dospCR(void)
{
    char oldecho;
    oldecho = echo;

    echo = BOTH;
    setio(whichIO, echo, outFlag);

    if (cfg.nopwecho == 1)  doCR(); 
    else
    {
        if (onConsole)
        {
            if (gotCarrier()) domcr();
        }
        else  doccr();
    }
    echo = oldecho;
}

/* -------------------------------------------------------------------- */
/*  doTAB()         prints a tab to modem & console according to flag   */
/* -------------------------------------------------------------------- */
void doTAB(void)
{
    int column, column2;

    column  = crtColumn;
    column2 = crtColumn;

    do { outCon(' '); } while ( (++column % 8) != 1);

    if (haveCarrier)
    {
        if (termTab)           outMod('\t');
        else
        do { outMod(' '); } while ((++column2 % 8) != 1);
    }
    updcrtpos('\t');
}    

/* -------------------------------------------------------------------- */
/*  echocharacter() echos bbs input according to global flags           */
/* -------------------------------------------------------------------- */
void echocharacter(char c)
{
    setio(whichIO, echo, outFlag);

    if (echo == NEITHER)
    {
        return;
    }
    else if (c == '\b') doBS();
    else if (c == '\n') doCR();
    else                oChar(c);
}

/* -------------------------------------------------------------------- */
/*  oChar()         is the top-level user-output function (one byte)    */
/*        sends to modem port and console both                          */
/*        does conversion to upper-case etc as necessary                */
/*        in "debug" mode, converts control chars to uppercase letters  */
/*      Globals modified:       prevChar                                */
/* -------------------------------------------------------------------- */
void oChar(register char c)
{
    static int UpDoWn=TRUE;   /* You dont want to know */

    prevChar = c;                       /* for end-of-paragraph code    */

    if (c == 1) c = 0;

    if (c == '\t')
    {
        doTAB();
        return;
    }

    if (backout)                /* You don't want to know */
    {
        if (UpDoWn)
            c = (char)toupper(c);
        else
            c = (char)tolower(c);
        UpDoWn=!UpDoWn;
    }

    if (termUpper)      c = (char)toupper(c);

    if (c == 10 /* newline */)  c = ' ';   /* doCR() handles real newlines */

    /* show on console */
    if (console)  outCon(c);

    /* show on printer */
    if (printing)  fputc(c, printfile);

    /* send out the modem  */
    if (haveCarrier && modem) outMod(c);

    updcrtpos(c);
}

/* -------------------------------------------------------------------- */
/*  updcrtpos()     updates crtColumn according to character            */
/* -------------------------------------------------------------------- */
void updcrtpos(char c)
{
    if (c == '\b') 
        crtColumn--;
    else if (c == '\t')
        while((++crtColumn  % 8) != 1);
    else if ((c == '\n') || (c == '\r')) crtColumn = 1;
    else crtColumn++;
}

/* -------------------------------------------------------------------- */
/*  mPrintf()       sends formatted output to modem & console           */
/* -------------------------------------------------------------------- */
void mPrintf(char *fmt, ... )
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    mFormat(buff);
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void cPrintf(char *fmt, ... )
{
    char register *buf = buff;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    while(*buf) {
        outCon(*buf++);
    }
}

/* -------------------------------------------------------------------- */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */
void cCPrintf(char *fmt, ... )
{
    va_list ap;
    int i;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    i = (80 - strlen(buff)) / 2;

    strrev(buff);

    while(i--)
        strcat(buff, " ");

    strrev(buff);

    (*stringattr)(wherey()-1, buff, cfg.attr);
}


/* -------------------------------------------------------------------- */
/*  prtList()   Print a list of rooms, ext.                             */
/* -------------------------------------------------------------------- */
void prtList(char *item)
{
    static int  listCol;
    static int  first;
    static int  num;
    
    if (item == LIST_START || item == LIST_END)
    {
        if (item == LIST_END)
        {
            if (num)
            {
                mPrintf(".");
                doCR();
            }
        }
        listCol = 0;
        num     = 0;
        first   = TRUE;
    }
    else
    {
        num++;
        
        if (first)
        {
            first = FALSE;
        }
        else
        {
            mPrintf(", ");
        }

        if (strlen(item) + 2 + crtColumn > termWidth)
        {
            doCR();
        }

        putWord(item);
    }
}

