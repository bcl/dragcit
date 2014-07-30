/************************************************************************/
/*                            library.c                                 */
/*                                                                      */
/*                  Routines used by Ctdl & Confg                       */
/************************************************************************/

/* TURBO C */
#include <dir.h>
#include <alloc.h>

/* Citadel */
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getGroup()              loads groupBuffer                       */
/*      putGroup()              stores groupBuffer to disk              */
/*                                                                      */
/*      getHall()               loads hallBuffer                        */
/*      putHall()               stores hallBuffer to disk               */
/*                                                                      */
/*      getLog()                loads requested CTDLLOG record          */
/*      putLog()                stores a logBuffer into citadel.log     */
/*                                                                      */
/*      getRoom()               load given room into RAM                */
/*      putRoom()               store room to given disk slot           */
/*                                                                      */
/*      writeTables()           writes all system tables to disk        */
/*      readTables()            loads all tables into ram               */
/*                                                                      */
/*      allocateTables()        allocate table space with halloc        */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*      getGrooup() loads group data into RAM buffer                    */
/************************************************************************/
void getGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fread(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("getGroup-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putGroup() stores group data into grp.cit                       */
/************************************************************************/
void putGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fwrite(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("putGroup-write fail");
    }
    fflush(grpfl);
}

/************************************************************************/
/*      getHall() loads hall data into RAM buffer                       */
/************************************************************************/
void getHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fread(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("getHall-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putHall() stores group data into hall.cit                       */
/************************************************************************/
void putHall(void)
{
    fseek(hallfl, 0L, 0);

    if (fwrite(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("putHall-write fail");
    }
    fflush(hallfl);
}

/************************************************************************/
/*      getLog() loads requested log record into RAM buffer             */
/************************************************************************/
void getLog(struct logBuffer *lBuf, int n)
{
    long int s;

    if (lBuf == &logBuf)  thisLog = n;

    s = (long)n * (long)sizeof logBuf;

    fseek(logfl, s, 0);

    if (fread(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("getLog-read fail EOF detected!");
    }
}

/************************************************************************/
/*      putLog() stores given log record into log.cit                   */
/************************************************************************/
void putLog(struct logBuffer *lBuf, int n)
{
    long int s;

    s = (long)n * (long)(sizeof(struct logBuffer));

    fseek(logfl, s, 0);  

    if (fwrite(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("putLog-write fail");
    }
    fflush(logfl);
}

/************************************************************************/
/*      getRoom()                                                       */
/************************************************************************/
void getRoom(int rm)
{
    long int s;

    /* load room #rm into memory starting at buf */
    thisRoom    = rm;
    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fread(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)  
    {
        crashout("getRoom(): read failed error or EOF!");
    }
}

/************************************************************************/
/*      putRoom() stores room in buf into slot rm in room.buf           */
/************************************************************************/
void putRoom(int rm)
{
    long int s;

    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fwrite(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)
    {
        crashout("putRoom() crash! 0 returned.");
    }
    fflush(roomfl);
}

/************************************************************************/
/*      readTables()  loads all tables into ram                         */
/************************************************************************/
readTables()
{
    FILE  *fd;

    getcwd(etcpath, 64);

    /*
     * ETC.DAT
     */
    if ((fd  = fopen("etc.dat" , "rb")) == NULL)
        return(FALSE);
    if (!fread(&cfg, sizeof cfg, 1, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("etc.dat");

    changedir(cfg.homepath);

    allocateTables();
    if (logTab == NULL)
        crashout("Error allocating logTab \n");
    if (msgTab == NULL)
        crashout("Error allocating msgTab \n");

    /*
     * LOG.TAB
     */
    if ((fd  = fopen("log.tab" , "rb")) == NULL)
        return(FALSE);
    if (!fread(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("log.tab" );

    /*
     * MSG.TAB
     */
    if (readMsgTab() == FALSE)  return FALSE;

    /*
     * ROOM.TAB
     */
    if ((fd = fopen("room.tab", "rb")) == NULL)
        return(FALSE);
    if (!fread(roomTab, sizeof(struct rTable), MAXROOMS, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("room.tab");

    return(TRUE);
}

/************************************************************************/
/*      writeTables()  stores all tables to disk                        */
/************************************************************************/
void writeTables(void)
{
    FILE  *fd;

    changedir(etcpath);

    if ((fd     = fopen("etc.dat" , "wb")) == NULL)
    {
        crashout("Can't make Etc.dat");
    }
    /* write out Etc.dat */
    fwrite(&cfg, sizeof cfg, 1, fd);
    fclose(fd);

    changedir(cfg.homepath);

    if ((fd  = fopen("log.tab" , "wb")) == NULL)
    {
        crashout("Can't make Log.tab");
    }
    /* write out Log.tab */
    fwrite(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd);
    fclose(fd);
 
    writeMsgTab();

    if ((fd = fopen("room.tab", "wb")) == NULL)
    {
        crashout("Can't make Room.tab");
    }
    /* write out Room.tab */
    fwrite(roomTab, sizeof(struct rTable), MAXROOMS, fd);
    fclose(fd);

    changedir(etcpath);
}

/************************************************************************/
/*    allocateTables()   allocate msgTab and logTab                     */
/************************************************************************/
void allocateTables(void)
{
    logTab = farcalloc((long)cfg.MAXLOGTAB+1, (long)sizeof(struct lTable));
    msgTab = farcalloc((long)cfg.nmessages+1, (long)sizeof(struct messagetable));
}

#define MAXRW   3000

/* -------------------------------------------------------------------- */
/*  readMsgTable()     Avoid the 64K limit. (stupid segments)           */
/* -------------------------------------------------------------------- */
int readMsgTab(void)
{
    FILE *fd;
    int  i;
    char huge *tmp;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    tmp = (char huge *)msgTab;

    if ((fd  = fopen(temp , "rb")) == NULL)
        return(FALSE);

    for (i = 0; i < cfg.nmessages / MAXRW; i++)
    {
        if (
            fread((void far *)(tmp), sizeof(struct messagetable), MAXRW, fd)
            != MAXRW
           )
        {
            fclose(fd);
            return FALSE;
        }
        tmp += (sizeof(struct messagetable) * MAXRW);
    }

    i = cfg.nmessages % MAXRW;
    if (i)
    {
        if (
            fread((void far *)(tmp), sizeof(struct messagetable), i, fd) 
            != i
           )
        {
            fclose(fd);
            return FALSE;
        }
    }
    
    fclose(fd);
    unlink(temp);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  writeMsgTable()     Avoid the 64K limit. (stupid segments)          */
/* -------------------------------------------------------------------- */
void writeMsgTab(void)
{
    FILE *fd;
    int i;
    char huge *tmp;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    tmp = (char huge *)msgTab;

    if ((fd  = fopen(temp , "wb")) == NULL)
        return;

    for (i = 0; i < cfg.nmessages / MAXRW; i++)
    {
        if (
            fwrite((void far *)(tmp), sizeof(struct messagetable), MAXRW, fd)
            != MAXRW
           )
        {
            fclose(fd);
        }
        tmp += (sizeof(struct messagetable) * MAXRW);
    }

    i = cfg.nmessages % MAXRW;
    if (i)
    {
        if (
            fwrite((void far *)(tmp), sizeof(struct messagetable), i, fd) 
            != i
           )
        {
            fclose(fd);
        }
    }
    
    fclose(fd);
}

