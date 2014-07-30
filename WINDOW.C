/* -------------------------------------------------------------------- */
/*                              window.c                                */
/*            Machine dependent windowing routines for Citadel          */
/* -------------------------------------------------------------------- */

#include <conio.h>
#include <dos.h>
#include <alloc.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*      cls()                   clears the screen                       */
/*      connectcls()            clears the screen upon carrier detect   */
/*      clearline()             blanks out specified line with attr     */
/*      cursoff()               turns cursor off                        */
/*      curson()                turns cursor on                         */
/*      gmode()                 checks for monochrome card              */
/*      help()                  toggles help menu                       */
/*      position()              positions the cursor                    */
/*      readpos()               returns current row, col position       */
/*      scroll()                scrolls window up                       */
/*      setscreen()             sets videotype flag                     */
/*      update25()              updates the 25th line                   */
/*      updatehelp()            updates the help window                 */
/*      directchar()            Direct screen write char with attr      */
/*      directstring()          Direct screen write string w/attr at row*/
/*      bioschar()              BIOS print char with attr               */
/*      biosstring()            BIOS print string w/attr at row         */
/*      setscreen()             Set SCREEN to correct address for VIDEO */
/*      ScreenFree()            Handle screen swap between screen/logo  */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */
static long f10timeout;               /* when was the f10 window opened?*/
static char far *screen;      /* memory address of RAM for direct screen I/O */
static char far *saveBuffer;  /* memory buffer for screen saves              */
static uchar row, column;     /* static vars for cursor position             */

/* -------------------------------------------------------------------- */
/*      cls()  clears the screen depending on carrier present or not    */
/* -------------------------------------------------------------------- */
void cls(void)
{
    /* scroll everything but kitchen sink */
    scroll(24, 0, cfg.attr);
    position(0,0);
}

/* -------------------------------------------------------------------- */
/*      connectcls()  clears the screen upon carrier detect             */
/* -------------------------------------------------------------------- */
void connectcls(void)
{
    if (anyEcho)
    {
        cls();
    }
    update25();
}


/* -------------------------------------------------------------------- */
/*      cursoff()  make cursor disapear                                 */
/* -------------------------------------------------------------------- */
void cursoff(void)
{
    union REGS REG;

    REG.h.ah = 01;
    REG.h.bh = 00;
    REG.h.ch = 0x26;
    REG.h.cl = 7;
    int86(0x10, &REG, &REG);
}


/* -------------------------------------------------------------------- */
/*      curson()  Put cursor back on screen checking for adapter.       */
/* -------------------------------------------------------------------- */
void curson(void)
{
    union REGS regs;
    uchar st, en;

    if (gmode() == 7)  /* Monocrone adapter */
    {  
        st = 12;
        en = 13;
    }
    else               /*  Color graphics adapter. */
    {                  
        st = 6;
        en = 7;
    }

    regs.h.ah = 0x01;
    regs.h.bh = 0x00;
    regs.h.ch = st;
    regs.h.cl = en;
    int86(0x10, &regs, &regs);
}


/* -------------------------------------------------------------------- */
/*      gmode()  Check for monochrome or graphics.                      */
/* -------------------------------------------------------------------- */
int gmode(void)
{

    union REGS REG;

    REG.h.ah = 15;
    REG.h.bh = 00;
    REG.h.ch = 0;
    REG.h.cl = 0;
    int86(0x10, &REG, &REG);

    return(REG.h.al);
}


/* -------------------------------------------------------------------- */
/*      help()  this toggles our help menu                              */
/* -------------------------------------------------------------------- */
void help(void)
{
    long time();
    uchar row, col;

    readpos(&row, &col);

    if (scrollpos == 23)  /* small window */
    {
        if (row > 19)
        {
            scroll( 23, row - 19, cfg.wattr);
            position(19, col);
        }
 
        if (cfg.bios)
        {
            clearline(20, cfg.wattr);
            clearline(21, cfg.wattr);
            clearline(22, cfg.wattr);
            clearline(23, cfg.wattr);
        }
 
        scrollpos = 19;    /* big window */

        time(&f10timeout);
    }
    else  /* big window */
    {
        clearline(20, cfg.attr);
        clearline(21, cfg.attr);
        clearline(22, cfg.attr);
        clearline(23, cfg.attr);

        scrollpos = 23;    /* small window */

        time(&f10timeout);
    }
    position(row, col);
}


/* -------------------------------------------------------------------- */
/*      position()  positions the cursor                                */
/* -------------------------------------------------------------------- */
void position(uchar row , uchar column)
{
    union REGS regs;

    regs.h.ah = 0x02;        /* set cursor position interrupt */
    regs.h.dh = row;         /* row                           */
    regs.h.dl = column;      /* column                        */
    regs.h.bh = 0;           /* display page #0               */
    int86( 0x10, &regs,  &regs);

}


/* -------------------------------------------------------------------- */
/*      clearline()  clears specified line to attr                      */
/* -------------------------------------------------------------------- */
void clearline(unsigned int row, uchar attr)
{
    union REGS regs;

    position((uchar)row, (uchar)0); /* set cursor on the bottom line   */
    regs.h.ah = 9;           /* service 9, write character # attribute */
    regs.h.bl = attr;        /* character attribute                    */
    regs.h.al = 32;          /* write spaces     0x0900                */
    regs.x.cx = 80;          /* clear whole line                       */
    regs.h.bh = 0;           /* display page                           */
    int86( 0x10, &regs,  &regs);          
}


/* -------------------------------------------------------------------- */
/*      readpos()  returns current cursor position                      */
/* -------------------------------------------------------------------- */
void readpos(uchar *row, uchar *column)
{
    union REGS regs;

    regs.h.ah = 0x03;        /* set cursor position interrupt */
    regs.h.bh = 0;           /* display page #0               */
    int86( 0x10, &regs,  &regs);

    *row    = regs.h.dh;     /* row                           */
    *column = regs.h.dl;     /* column                        */
}


/* -------------------------------------------------------------------- */
/*      scroll()  scrolls window up from specified line                 */
/* -------------------------------------------------------------------- */
void scroll( uchar row, uchar howmany, uchar attr)
{
    union REGS regs;
    uchar rw, col;

    readpos( &rw, &col);

    regs.h.al = howmany;     /* scroll how many lines                */

    regs.h.cl = 0;           /* row # of upper left hand corner      */
    regs.h.ch = 0x00;        /* col # of upper left hand corner      */
    regs.h.dh = row;         /* row # of lower left hand corner      */
    regs.h.dl = 79;          /* col # of upper left hand corner      */
    
    regs.h.ah = 0x06;        /* scroll window up interrupt           */
    regs.h.bh = attr;        /* char attribute for blank lines       */

    int86( 0x10, &regs,  &regs);

    /* put cursor back! */
    position( rw, col);
}


/* -------------------------------------------------------------------- */
/*      update25()  updates the 25th line according to global variables */
/* -------------------------------------------------------------------- */
void update25(void)
{
    char string[83];
    char str2[80];
    char name[30];
    char flags[10];
    char carr[5];
    uchar row, col;
    int i;

    readpos (&row, &col);

    if (scrollpos == 19) updatehelp();

    if(cfg.bios)  cursoff();
    
    readpos(&row, &col);

    if (loggedIn)  
    {
         strcpy( name, "๗");
         for (i = 0; i < ((20 - strlen(logBuf.lbname)) / 2); i++)
           strcat(name, "๗");
         strcat( name, "๗๗ ");
         strcat( name, logBuf.lbname);
         strcat( name, " ๗๗๗");
         for (i = 0; i < ((20 - strlen(logBuf.lbname)) / 2); i++)
           strcat(name, "๗");
    }
    else
    {
         strcpy( name, "ออออออ Not logged in ออออออ");
    }

    if      ( justLostCarrier)  strcpy( carr, "JL");
    else if ( haveCarrier)      strcpy( carr, "CD");
    else                        strcpy( carr, "NC");

    strcpy(flags, "       ");

    if ( aide )                     flags[0] = 'A';
    if ( twit )                     flags[1] = 'T';
    if ( sysop )                    flags[5] = 'S';

    if (loggedIn)
    {
        if ( logBuf.lbflags.PERMANENT ) flags[2] = 'P';
        if ( logBuf.lbflags.UNLISTED )  flags[3] = 'U';
        if ( logBuf.lbflags.NETUSER )   flags[4] = 'N';
        if ( logBuf.lbflags.NOMAIL )    flags[6] = 'M';
    }

    sprintf( string, " %-27.27s ณ%sณ%sณ%4d baudณ%sณ%c%c%c%cณ%sณ%sณ%sณ%s",
      name,
      (whichIO == CONSOLE) ? "Console" : " Modem ",
      carr,
      bauds[speed],
      (disabled)    ? "DS" : "EN",
      (cfg.noBells) ? ' ' : '',
      (backout)     ? '' : ' ',
      (debug)       ? '่' : ' ',
      (ConLock)     ? '' : ' ',
      (cfg.noChat)  ? ((chatReq) ? "rcht" : "    " ) : 
                      ((chatReq) ? "RCht" : "Chat" ),
      (printing)    ? "Prt"  : "   ",
      (sysReq)      ? "REQ"  : "   ",
      flags
    );

    sprintf(str2, "%-79s ", string);

    (*stringattr)(24, str2, cfg.wattr);

    position(row,col);

    if(cfg.bios)  curson();

}


/* -------------------------------------------------------------------- */
/*      updatehelp()  updates the help menu according to global vars    */
/* -------------------------------------------------------------------- */
void updatehelp(void)
{
    long time(), l;
    char bigline[81];
    uchar row, col;

    if ( f10timeout < (time(&l) - (long)(60 * 2)) ) 
    {
        help();
        return;
    }

    if(cfg.bios)  cursoff();

    strcpy(bigline,
    "ษอออออออออออออออัอออออออออออออออัออออออออออออออัอออออออออออออออัอออออออออออออออป");

    readpos( &row, &col);

    position(20, 0);

    (*stringattr)(20, bigline, cfg.wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
          " F1 þ Shutdown ", " F2 þ Startup  " , " F3 þ Request ",
                 (anyEcho) ? " F4 þ Echo-Off " : " F4 þ Echo-On  ",
      (whichIO == CONSOLE) ? " F5  þ Modem   " : " F5  þ Console ");

    (*stringattr)(21, bigline, cfg.wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
    " F6 þ Sysop Fn ", (cfg.noBells) ? " F7 þ Bells-On " : " F7 þ Bells-Off" ,
    " F8 þ ChatMode",  (cfg.noChat)  ? " F9 þ Chat-On  " : " F9 þ Chat-Off ",
    " F10 þ Help    ");

    (*stringattr)(22, bigline, cfg.wattr);

    strcpy(bigline,
    "ศอออออออออออออออฯอออออออออออออออฯออออออออออออออฯอออออออออออออออฯอออออออออออออออผ");


    (*stringattr)(23, bigline, cfg.wattr);

    position( row >= 19 ? scrollpos : row, col);

    if(cfg.bios)  curson();
}


/* -------------------------------------------------------------------- */
/*      directstring() print a string with attribute at row             */
/* -------------------------------------------------------------------- */
void directstring(unsigned int row, char *str, uchar attr)
{
    register int i, j, l;

    l = strlen(str);

    for(i=(row*160), j=0; j<l; i +=2, j++)
    {
      screen[i] = str[j];
      screen[i+1] = attr;
    }
}


/* -------------------------------------------------------------------- */
/*      directchar() print a char directly with attribute at row        */
/* -------------------------------------------------------------------- */
void directchar(char ch, uchar attr)
{
    int i;
    uchar row, col;

    readpos( &row, &col);

    i = (row*160)+(col*2);

    screen[i] = ch;
    screen[i+1] = attr;

    position( row, col+1);
}


/* -------------------------------------------------------------------- */
/*      biosstring() print a string with attribute                      */
/* -------------------------------------------------------------------- */
void biosstring(unsigned int row, char *str, uchar attr)
{
    union REGS regs;
    union REGS temp_regs;
    register int i=0;

    regs.h.ah = 9;           /* service 9, write character # attribute */
    regs.h.bl = attr;        /* character attribute                    */
    regs.x.cx = 1;           /* number of character to write           */
    regs.h.bh = 0;           /* display page                           */

    while(str[i])
    {
      position((uchar)row, (uchar)i);/* Move cursor to the correct position */
      regs.h.al = str[i];            /* set character to write     0x0900   */
      int86( 0x10, &regs, &temp_regs);
      i++;
    }
}


/* -------------------------------------------------------------------- */
/*      bioschar() print a char with attribute                          */
/* -------------------------------------------------------------------- */
void bioschar(char ch, uchar attr)
{
    uchar row, col;
    union REGS regs;

    regs.h.ah = 9;           /* service 9, write character # attribute */
    regs.h.bl = attr;        /* character attribute                    */
    regs.h.al = ch;          /* write 0x0900                           */
    regs.x.cx = 1;           /* write 1 character                      */
    regs.h.bh = 0;           /* display page                           */ 
    int86( 0x10, &regs, &regs);

    readpos( &row, &col);
    position( row, col+1);
}

/* -------------------------------------------------------------------- */
/*      setscreen() set video mode flag 0 mono 1 cga                    */
/* -------------------------------------------------------------------- */
void setscreen(void)
{
    if(gmode() ==7)
        screen = (char far *)0xB0000000L;    /* mono */
    else
        screen = (char far *)0xB8000000L;    /* cga */

    if (gmode() != 7)                     /* if color display */
    {
        outp(0x03d9, cfg.battr);        /* set border color */
    }

    if(cfg.bios)
    {
        charattr = bioschar;
        stringattr = biosstring;
    }
    else
    {
        charattr = directchar;
        stringattr = directstring;
    }
    ansiattr = cfg.attr;
}

/* -------------------------------------------------------------------- */
/* Handle ansi escape sequences                                         */
/* -------------------------------------------------------------------- */
char ansi(char c)
{
    static char args[20], first = FALSE;
    static uchar c_x = 0, c_y = 0;
    uchar argc, a[5];
    int i;
    uchar x, y;
    char *p;
  
    if (c == 27 /* ESC */)
    {
        strcpy(args, "");
        first = TRUE;
        return TRUE;
    }

    if (first && c != '[')
    {
        first = FALSE;
        return FALSE;
    }

    if (first && c == '[')
    {
        first = FALSE;
        return TRUE;
    }
    
    if (isalpha(c))
    {
        i=0; p=args; argc=0;
        while(*p)
        {
            if (isdigit(*p))
            {
                char done = 0;

                a[argc]=(uchar)atoi(p);
                while(!done)
                {
                    p++;
                    if (!(*p) || !isdigit(*p))
                    done = TRUE;
                }
                argc++;
            }else p++;
        }
        switch(c)
        {
        case 'J': /* cls */
                cls();
                update25();
                break;
        case 'K': /* del to end of line */
                clreol();
                break;
        case 'm':
                for (i = 0; i < argc; i++)
                {
                    switch(a[i])
                    {
                    case 5:
                        ansiattr = ansiattr | 128;  /* blink */
                        break;
                    case 4:
                        ansiattr = ansiattr | 1;    /* underline */
                        break;
                    case 7:
                        ansiattr = cfg.wattr;       /* Reverse Vido */
                        break;
                    case 0:
                        ansiattr = cfg.attr;        /* default */
                        break;
                    case 1:
                        ansiattr = cfg.cattr;       /* Bold */
                        break;
                    default:
                        break;
                    }
                }
            break;
        case 's': /* save cursor */
                readpos(&c_x, &c_y);
                break;
        case 'u': /* restore cursor */
                position(c_x, c_y);
                break;
        case 'A':
                readpos(&x, &y);
                if (argc)
                    x -= a[0];
                else
                    x--;
                x = x % 24;
                position(x, y);
                break;
        case 'B':
                readpos(&x, &y);
                if (argc)
                    x += a[0];
                else
                    x++;
                x = x % 24;
                position(x, y);
                break;
        case 'D':
                readpos(&x, &y);
                if (argc)
                    y -= a[0];
                else
                    y --;
                y = y % 80;
                position(x, y);
                break;
        case 'C':
                readpos(&x, &y);
                if (argc)
                    y += a[0];
                else
                    y ++;
                y = y % 80;
                position(x, y);
                break;
        case 'f':
        case 'H':
                if (!argc)
                {
                    position(0,0);
                    break;
                }
                if (argc == 1)
                {
                    if (args[0] == ';')
                    {
                        a[1] = a[0];
                        a[0] = 1;
                    }else{
                        a[1] = 1;
                    }
                    argc = 2;
                }
                if (argc == 2 && a[0] < 25 && a[1] < 80)
                {
                    position(a[0]-1, a[1]-1);
                    break;
                }
        default:
                {
                    char str[80];
           
                    sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
                    (*stringattr)(0, str, cfg.wattr);
                }
                break;
        }
        if (debug)
        {
            char str[80];
           
            sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
            (*stringattr)(0, str, cfg.wattr);
        }
        return FALSE;
    }else{
        {
            i = strlen(args);
            args[i]=c;
            args[i+1]=NULL;
        }
        return TRUE;
    }
}       

/* -------------------------------------------------------------------- */
/*  save_screen() allocates a buffer and saves the screen               */
/* -------------------------------------------------------------------- */
void save_screen(void)
{
    saveBuffer = farcalloc(4000l, sizeof(char));
    memcpy(saveBuffer, screen, 4000);
    readpos( &row , &column);
}


/* -------------------------------------------------------------------- */
/*   restore_screen() restores screen and free's buffer                 */
/* -------------------------------------------------------------------- */
void restore_screen(void)
{
    memcpy(screen, saveBuffer, 4000);
    farfree((void *)saveBuffer);
    position(row , column);

    if (gmode() != 7)                     /* if color display */
    {
        outp(0x03d9, cfg.battr);        /* set border color */
    }

}

/* -------------------------------------------------------------------- */
/* ScreenFree() either saves the screen and displays the opening logo   */
/*      or restores, depending on anyEcho                               */
/* -------------------------------------------------------------------- */
void ScreenFree(void)
{
    static uchar row, col, helpVal = 0;

    if (anyEcho)
    {
        if (scrollpos == 19)        /* the help window is open */
        {
            helpVal = 1;
        }
        else
        {
            helpVal = 0;
        }

        save_screen();
        readpos( &row, &col);
        cursoff();
        logo();

        if (helpVal)
        {
            updatehelp();
        }
    }
    else
    {
        restore_screen();
        if (helpVal == 0 && scrollpos == 19)   /* window opened while locked */
        {
            if (row > 19)
            {
                scroll( 23, row - 19, cfg.wattr);
                position(19, col);
            }
            updatehelp();

        }

        if (helpVal == 1 && scrollpos == 23)   /* window closed while locked */
        {
            clearline(20, cfg.attr);
            clearline(21, cfg.attr);
            clearline(22, cfg.attr);
            clearline(23, cfg.attr);
        }
        position(row, col);
        curson();
    }
}


