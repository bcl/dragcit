/* -------------------------------------------------------------------- */
/*  MISC.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  Citadel garbage dump, if it aint elsewhere, its here.               */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#define MISC

/* MSC */
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <dir.h>
#include <io.h>
#include <alloc.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* DragCit */
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/*  exitcitadel()   Done with cit, time to leave                        */
/*  filexists()     Does the file exist?                                */
/*  hash()          Make an int out of their name                       */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/*  initCitadel()   Load up data files and open everything.             */
/*  openFile()      Special to open a .cit file                         */
/*  trap()          Record a line to the trap file                      */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/*  h2memcpy()      Terible hack to copy over 64K, beacuse MSC cant. PT2*/
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/*  amPrintf()      aide message printf                                 */
/*  amZap()         Zap aide message being made                         */
/*  changedir()     changes curent drive and directory                  */
/*  changedisk()    change to another drive                             */
/*  ltoac()         change a long into a number with ','s in it         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Many of the functions move to other modules     */
/*  02/08/89    (PAT)   History Re-Started                              */
/*                      InitAideMess and SaveAideMess added             */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data definitions                                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/* -------------------------------------------------------------------- */
void crashout(char *message)
{
    FILE *fd;           /* Record some crash data */

    Hangup();

    fcloseall();

    fd = fopen("crash.cit", "w");
    fprintf(fd, message);
    fclose(fd);

    writeTables();

    cfg.attr = 7;   /* exit with white letters */

    position(0,0);
    cPrintf("F\na\nt\na\nl\n \nS\ny\ns\nt\ne\nm\n \nC\nr\na\ns\nh\n");
    cPrintf(" %s\n", message);

    drop_dtr();

    portExit();

    farfree((void far *)msgTab);
    farfree((void far *)logTab);
    farfree((void far *)extrn);
    farfree((void far *)othCmd);
    farfree((void far *)roomTab);

    exit(1);
}

/* -------------------------------------------------------------------- */
/*  exitcitadel()   Done with cit, time to leave                        */
/* -------------------------------------------------------------------- */
void exitcitadel(void)
{
    if (loggedIn) terminate( /* hangUp == */ TRUE, FALSE);

    drop_dtr();        /* turn DTR off */

    putGroup();       /* save group table */
    putHall();        /* save hall table  */

    writeTables(); 

    trap("Citadel Terminated", T_SYSOP);

    /* close all files */
    fcloseall();

    cfg.attr = 7;   /* exit with white letters */
    cls();

    drop_dtr();

    portExit();

    farfree((void far *)msgTab);
    farfree((void far *)logTab);
    farfree((void far *)extrn);
    farfree((void far *)othCmd);
    farfree((void far *)roomTab);

    if (gmode() != 7)
    {
        outp(0x3d9,0);
    }

    exit(0);
}

/* -------------------------------------------------------------------- */
/*  filexists()     Does the file exist?                                */
/* -------------------------------------------------------------------- */
BOOL filexists(char *filename)
{
    return (BOOL)((access(filename, 4) == 0) ? TRUE : FALSE);
}

/* -------------------------------------------------------------------- */
/*  hash()          Make an int out of their name                       */
/* -------------------------------------------------------------------- */
uint hash(char *str)
{
    int  h, shift;

    for (h=shift=0;  *str;  shift=(shift+1)&7, str++)
    {
        h ^= (toupper(*str)) << shift;
    }
    return h;
}

/* -------------------------------------------------------------------- */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/* -------------------------------------------------------------------- */
void ctrl_c(void)
{
    uchar row, col;

    signal(SIGINT, ctrl_c);
    readpos( &row, &col);
    position(row-1, 19);
    ungetch('\r');
    getkey = TRUE;
}
 
/* -------------------------------------------------------------------- */
/*  initCitadel()   Load up data files and open everything.             */
/* -------------------------------------------------------------------- */
void initCitadel(void)
{
    /*FILE *fd, *fp;*/

    char *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
    char scratch[80];

    /* lets ignore ^C's  */
    signal(SIGINT, ctrl_c);

    asciitable();

    /* This sillyness opens two files and reads 1 byte from each so that */
    /* the 8K block will be allocated in front of our halloc'ed blocks.  */
    /* I tested opening the same file twice, it seems ok, as long as they*/
    /* are 2 different pointers. Also they are read only. Can't be sure  */
    /* that you can find any other files. They might be else where.      */

    if ((edit = farcalloc(MAXEXTERN, sizeof(struct ext_editor))) == NULL)
    {
        crashout("Can not allocate external editors");
    }
    if ((hallBuf = farcalloc(1L, sizeof(struct hallBuffer))) == NULL)
    {
        crashout("Can not allocate space for halls");
    }
    if ((msgBuf = farcalloc(1L, sizeof(struct msgB))) == NULL)
    {
        crashout("Can not allocate space for message buffer 1");
    }
    if ((msgBuf2 = farcalloc(1L, sizeof(struct msgB))) == NULL)
    {
        crashout("Can not allocate space for message buffer 2");
    }
    if ((extrn = farcalloc(MAXEXTERN, sizeof(struct ext_prot))) == NULL)
    {
        crashout("Can not allocate space for external protocol");
    }
    if ((othCmd = farcalloc(MAXEXTERN, sizeof(struct ext_other))) == NULL)
    {
        crashout("Can not allocate space for other extern commands");
    }
    if ((roomTab = farcalloc(MAXROOMS, sizeof(struct rTable))) == NULL)
    {
        crashout("Can not allocate space for other extern commands");
    }
    
    if (!readTables())
    {
        cPrintf("Etc.dat not found"); doccr();
        pause(300);
        cls();
        configcit();
    }

    portInit();

    setscreen();

    update25();

    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");

    /* open message files: */
    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    msgFile     =  scratch  ;
    roomFile    = "room.dat";

    openFile(grpFile,  &grpfl );
    openFile(hallFile, &hallfl);
    openFile(logFile,  &logfl );
    openFile(msgFile,  &msgfl );
    openFile(roomFile, &roomfl);

    /* open Trap file */
    trapfl = fopen(cfg.trapfile, "a+");

    trap("Citadel Started", T_SYSOP);

    getGroup();
    getHall();

    if (cfg.accounting)
    {
        readaccount();    /* read in accounting data */
    }
    readprotocols();
    readcron();

    getRoom(LOBBY);     /* load Lobby>  */
    Initport();
    Initport();
    whichIO = MODEM;

    /* record when we put system up */
    time(&uptimestamp);

    cls();
    setdefaultconfig();
    update25();
    setalloldrooms();
    roomtalley();
}

/* -------------------------------------------------------------------- */
/*  openFile()      Special to open a .cit file                         */
/* -------------------------------------------------------------------- */
void openFile(char *filename, FILE **fd)
{
    /* open message file */
    if ((*fd = fopen(filename, "r+b")) == NULL)
    {
        crashout(".DAT file missing!");
    }
}

/* -------------------------------------------------------------------- */
/*  trap()          Record a line to the trap file                      */
/* -------------------------------------------------------------------- */
void trap(char *string, int what)
{
    char dtstr[20];

    /* check to see if we are supposed to log this event */
    if (!cfg.trapit[what])  return;

    strftime(dtstr, 19, "%y%b%D %X", 0l);

    fprintf(trapfl, "%s:  %s\n", dtstr, string);

    fflush(trapfl);
}

/* -------------------------------------------------------------------- */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/* -------------------------------------------------------------------- */
#define K32  (32840L)
void hmemcpy(void huge *xto, void huge *xfrom, long size)
{
    char huge *from;
    char huge *to;

    to = xto; from = xfrom;

    if (to > from)
    {
        h2memcpy(to, from, size);
        return;
    }

    while (size > K32)
    {
        memcpy((char far *)to, (char far *)from, (unsigned int)K32);
        size -= K32;
        to   += K32;
        from += K32;
    }

    if (size)
        memcpy((char far *)to, (char far *)from, (uint)size);
}

/* -------------------------------------------------------------------- */
/*  h2memcpy()      Terible hack to copy over 64K, beacuse MSC cant. PT2*/
/* -------------------------------------------------------------------- */
void h2memcpy(char huge *to, char huge *from, long size)
{
    to += size;
    from += size;

    size++;

    while(size--)
        *to-- = *from--;
}

/* -------------------------------------------------------------------- */
/*  SaveAideMess()  Save aide message from AIDEMSG.TMP                  */
/* -------------------------------------------------------------------- */
void SaveAideMess(void)
{
    char temp[90];
    FILE *fd;

    /*
     * Close curent aide message (if any)
     */
    if (aideFl == NULL)
    {
        return;
    }
    fclose(aideFl);
    aideFl = NULL;

    clearmsgbuf();

    /*
     * Read the aide message
     */
    sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");
    if ((fd  = fopen(temp, "rb")) == NULL)
    {
        crashout("AIDEMSG.TMP file not found during aide message save!");
    }
    GetFileMessage(fd, msgBuf->mbtext, cfg.maxtext);

    fclose(fd);
    unlink(temp);

    if (strlen(msgBuf->mbtext) < 10)
        return;

    strcpy(msgBuf->mbauth, cfg.nodeTitle);  

    msgBuf->mbroomno = AIDEROOM;

    putMessage();
    noteMessage();
}

/* -------------------------------------------------------------------- */
/*  amPrintf()      aide message printf                                 */
/* -------------------------------------------------------------------- */
void amPrintf(char *fmt, ... )
{
    va_list ap;
    char temp[90];

    /* no message in progress? */
    if (aideFl == NULL)
    {
        sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");

        unlink(temp);
 
        if ((aideFl = fopen(temp, "w")) == NULL)
        {
            crashout("Can not open AIDEMSG.TMP!");
        }
    }

    va_start(ap, fmt);
    vfprintf(aideFl, fmt, ap);
    va_end(ap);

    fflush(aideFl);
}

/* -------------------------------------------------------------------- */
/*  amZap()         Zap aide message being made                         */
/* -------------------------------------------------------------------- */
void amZap(void)
{
    char temp[90];

    if (aideFl != NULL)
    {
        fclose(aideFl);
    }

    sprintf(temp, "%s\\%s", cfg.temppath, "aidemsg.tmp");

    unlink(temp);

    aideFl = NULL;
}

/* -------------------------------------------------------------------- */
/*  changedir()     changes curent drive and directory                  */
/* -------------------------------------------------------------------- */
int changedir(char *path)
{
    /* uppercase   */ 
    path[0] = (char)toupper(path[0]);

    /* change disk */
    changedisk(path[0]);

    /* change path */
    if (chdir(path)  == -1) return -1;

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  changedisk()    change to another drive                             */
/* -------------------------------------------------------------------- */
void changedisk(char disk)
{
    union REGS REG;

    REG.h.ah = 0x0E;      /* select drive */

    REG.h.dl = (disk - 'A');

    intdos(&REG, &REG);
}

/* -------------------------------------------------------------------- */
/*  ltoac()         change a long into a number with ','s in it         */
/* -------------------------------------------------------------------- */
char *ltoac(long num)
{
    char s1[30];
    static char s2[40];
    int i, i2, i3, l;

    sprintf(s1, "%lu", num);

    l = strlen(s1);

    for (i = l, i2 = 0, i3 = 0; s1[i2]; i2++, i--)
    {
        if (!(i % 3) && i != l)
        {
            s2[i3++] = ',';
        }
        s2[i3++] = s1[i2];
    }

    s2[i3] = NULL;

    return s2;
}

/* -------------------------------------------------------------------- */
/*  editBorder()    edit a boarder line.                                */
/* -------------------------------------------------------------------- */
void editBorder(void)
{
    int i;

    doCR();
    doCR();
        
    if (!cfg.borders)
    {
        mPrintf(" Border lines not enabled!");
        doCR();
        return;
    }

    outFlag = OUTOK;
    
    for (i = 0; i < MAXBORDERS; i++)
    {
        mPrintf("Border %d:", i+1);
        if (*cfg.border[i])
        {
            doCR();
            mPrintf("%s", cfg.border[i]);
        }
        else
        {
            mPrintf(" Empty!"); 
        }
        doCR();
        doCR();
    }

    i = (int)getNumber("border line to change", 0L, (long)MAXBORDERS, 0L);

    if (i)
    {
        doCR();
        getString("border line", cfg.border[i-1], 80, FALSE, ECHO, "");
    }
}

/* -------------------------------------------------------------------- */
/*  doBorder()      print a boarder line.                               */
/* -------------------------------------------------------------------- */
void doBorder(void)
{
    static count = 0;
    static line  = 0;
    int    tries;

    if (count++ == 20)
    {
        count = 0;

        for (line == MAXBORDERS-1 ? line = 0 : line++, tries = 0; 
             tries < MAXBORDERS + 1;
             line == MAXBORDERS-1 ? line = 0 : line++, tries++)
        {
            if (*cfg.border[line])
            {
                doCR();
                mPrintf("%s", cfg.border[line]);
                break;
            }
        }
    }
}


