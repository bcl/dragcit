/* -------------------------------------------------------------------- */
/*                              aplic.c                                 */
/*                    Aplication code for Citadel                       */
/* -------------------------------------------------------------------- */
#include <dos.h>
#include <alloc.h>
#include <string.h>
#include <process.h>

#include "ctdl.h"
#include "proto.h"
#include "global.h"
#include "applic.h"
#include "apstruct.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/*                                                                      */
/*      aplreadmess()           read message in from application        */
/*      apsystem()              turns off interupts and makes           */
/*                              a system call                           */
/*      ExeAplic()              gets name of aplication and executes    */
/*      readuserin()            reads userdati.apl from disk            */
/*      shellescape()           handles the sysop '!' shell command     */
/*      tableIn()               allocates RAM and reads log and msg     */
/*                              and tab files into RAM                  */
/*      tableOut()              writes msg and log tab files to disk    */
/*                              and frees RAM                           */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*              External declarations in APLIC.C                        */
/* -------------------------------------------------------------------- */

static int  tableOut(void);
static void tableIn(void);
static void readAplFile(void);
static void writeAplFile(void);

#define     NUM_SAVE    15

int saveem[]    = { 0x00, 0x03, 0x22, 0x34,
                    0x35, 0x36, 0x37, 0x38, 
                    0x39, 0x3A, 0x3B, 0x3C, 
                    0x3D, 0x3E, 0x3F 
                   } ;

static void (interrupt far *int_save[NUM_SAVE])(), 
            (interrupt far *int_temp[NUM_SAVE])();

/* -------------------------------------------------------------------- */
/*              External variable definitions for APLIC.C               */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*      ExeAplic() gets the name of an aplication and executes it.      */
/* -------------------------------------------------------------------- */
void ExeAplic(void)
{
    char stuff[100];
    char comm[5];

    doCR();
    doCR();

    if (!roomBuf.rbflags.APLIC) 
    {
      mPrintf("  -- Room has no application.\n\n");
      changedir(cfg.homepath);
      return;
    }
    if (changedir(cfg.aplpath) == ERROR)
    {
      mPrintf("  -- Can't find application directory.\n\n");
      changedir(cfg.homepath);
      return;
    }

    sprintf(comm, "COM%d", cfg.mdata);
    sprintf(stuff,"%s %s %d %d %s",
           roomBuf.rbaplic,
           onConsole ? "LOCAL" : comm,
           onConsole ? 2400    : bauds[speed],
           sysop,
           logBuf.lbname); 

    apsystem(stuff);
    changedir(cfg.homepath);
}

/* -------------------------------------------------------------------- */
/*      shellescape()  handles the sysop '!' shell command              */
/* -------------------------------------------------------------------- */
void shellescape(super)
char super;
{
    char prompt[92];
    static char oldprompt[92];
    char *envprompt;
    char command[80];

    envprompt = getenv("PROMPT");

    sprintf(prompt,   "PROMPT=_%s", envprompt);
    sprintf(oldprompt,"PROMPT=%s",  envprompt);

    putenv(prompt);

    changedir(roomBuf.rbflags.MSDOSDIR ? roomBuf.rbdirname : cfg.homepath);

    sprintf(command, "%s%s", super ? "!" : "", getenv("COMSPEC"));

    apsystem(command);

    putenv(oldprompt);

    update25();

    changedir(cfg.homepath);
}

/* -------------------------------------------------------------------- */
/* tableIn   allocates RAM and reads log and msg tab file into RAM      */
/* -------------------------------------------------------------------- */
static void tableIn(void)
{
    FILE *fd;
    char scratch[64];

    if (dowhat != NETWORKING)
    {
        doCR();
        mPrintf("Restoring system variables, please wait.");
        doCR();
    }

    sprintf(scratch, "%s\\%s", cfg.temppath, "tables.tmp");

    if ((fd  = fopen(scratch, "rb")) == NULL)
    {
        mPrintf("\n Fatal System Crash!\n System tables destroyed!");
        crashout("Log table lost in application");
    }

    allocateTables();

    if (logTab == NULL)
    {
        mPrintf("\n Fatal System Crash!\n Memory FUBAR!");
        crashout("Can not allocate log table after application");
    }

    if (msgTab == NULL)
    {
        mPrintf("\n Fatal System Crash!\n Memory FUBAR!");
        crashout("Can not allocate message table after application");
    }

    if (!readMsgTab())
    {
        mPrintf("\n Fatal System Crash!\n Message table destroyed!");
        crashout("Message table lost in application");
    }

    if (!fread(logTab, (sizeof(*logTab) * cfg.MAXLOGTAB) , 1, fd))
    {
        mPrintf("\n Fatal System Crash!\n Log table destroyed!");
        crashout("Log table lost in application");
    }

    fclose(fd);

    unlink(scratch);
}

/* -------------------------------------------------------------------- */
/* tableOut   writes msg and log tab files to disk and frees RAM        */
/* -------------------------------------------------------------------- */
static int tableOut(void)
{
    FILE *fd;
    char scratch[64];

    if (dowhat != NETWORKING)
    {
        mPrintf("Saving system variables, please wait."); doCR();
    }

    if (cfg.homepath[ (strlen(cfg.homepath) - 1) ]  == '\\')
        cfg.homepath[ (strlen(cfg.homepath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.temppath, "tables.tmp");

    if ((fd  = fopen(scratch , "wb")) == NULL)
    {
        mPrintf("Can not save system tables!\n "); 
        return ERROR;
    }

    /* write out Msg.tab */
    writeMsgTab();

    farfree((void *)msgTab);

    /* write out Log.tab */
    fwrite(logTab, (sizeof(*logTab) * cfg.MAXLOGTAB), 1, fd);

    farfree((void *)logTab);

    fclose(fd);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*      apsystem() turns off interupts and makes a system call          */
/* -------------------------------------------------------------------- */
void apsystem(stuff)
char *stuff;
{
    int clearit = TRUE,
        superit = FALSE,
        batch   = FALSE;
    char scratch[80];
    static char *words[256];
    int  count, i, i2;

    writeAplFile();

    while (*stuff == '!' || *stuff == '@' || *stuff == '$')
    {
        if (*stuff == '!')
            superit = TRUE;
        if (*stuff == '@')
            clearit = FALSE;
        if (*stuff == '$')
            batch   = TRUE;
        stuff++;
    }

    if (superit)
        if (tableOut() == ERROR)
            return;
    
    if (disabled)
      drop_dtr();

    portExit();

    fcloseall();

    if(clearit)
    {
        save_screen();
        cls();
    }

    if (debug) cPrintf("(%s)\n", stuff);

    if (stricmp(stuff, getenv("COMSPEC")) == SAMESTRING)
        cPrintf("Use the EXIT command to return to DragCit \n");

    if (!batch)
    {
        count = parse_it(words, stuff);
        words[count] = NULL;
    }

    /*
     *  Save the Floting point emulator interupts & the overlay interupt.
     */
    for (i = 0; i < NUM_SAVE; i++)
    {
        int_save[i] = getvect(saveem[i]);
    }

    if (batch)
    {
        system( stuff );
    }
    else
    {
        spawnv(P_WAIT, words[0], words);
    }

    /*
     * Load interupts for checking. 
     */
    for (i = 0; i < NUM_SAVE; i++)
    {
        int_temp[i] = getvect(saveem[i]);
    }
        
    /*
     * Restore interupts. 
     */
    for (i = 0; i < NUM_SAVE; i++)
    {
        setvect(saveem[i], int_save[i]);
    }

    if(clearit) restore_screen();

    for (i = 0, i2 = 0; i < NUM_SAVE; i++)
    {
        if((int_save[i] != int_temp[i]) && saveem[i] != 0x22)
        {
            if (!i2)
            {
                cPrintf(" '%s' changed interupt(s): ", stuff); doccr();
            }
            
            cPrintf("    0x%2X to %p from %p", 
                    saveem[i], int_temp[i], int_save[i]);
            doccr();

            i2++;
        }
    }

    portInit();
    baud((int)speed);

    if (superit) tableIn();

    if (aideFl != NULL)
    {
        sprintf(scratch, "%s\\%s", cfg.temppath, "aidemsg.tmp");
        if ((aideFl = fopen(scratch, "a")) == NULL)
        {
            crashout("Can not open AIDEMSG.TMP!");
        }
    }
    trapfl = fopen(cfg.trapfile, "a+");
    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");
    openFile(scratch,    &msgfl );
    sprintf(scratch, "%s\\%s", cfg.homepath, "grp.dat");
    openFile(scratch,  &grpfl );
    sprintf(scratch, "%s\\%s", cfg.homepath, "hall.dat");
    openFile(scratch,  &hallfl );
    sprintf(scratch, "%s\\%s", cfg.homepath, "log.dat");
    openFile(scratch,  &logfl );
    sprintf(scratch, "%s\\%s", cfg.homepath, "room.dat");
    openFile(scratch,  &roomfl );

    if (disabled)
    {
        drop_dtr();
    }

    readAplFile();

    unlink("output.apl");
    readMessage = TRUE;
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void writeAplFile(void)
{
    FILE *fd;
    char buff[80];
    int i;

    unlink("output.apl");
    unlink("input.apl");
    if (readMessage)
    {
        unlink("message.apl");
    }
    unlink("readme.apl");

    if ((fd = fopen("output.apl" , "wb")) == NULL)
    {
        mPrintf("Can't make userdato.apl");
        return;
    }

    for (i = 0; AplTab[i].item != APL_END; i++)
    {
        switch(AplTab[i].type)
        {
        case TYP_STR:
            sprintf(buff, "%c%s\n", AplTab[i].item, AplTab[i].variable);
            break;

        case TYP_BOOL:
        case TYP_CHAR:
            sprintf(buff, "%c%d\n", AplTab[i].item,
                *((char *)AplTab[i].variable));
            break;

        case TYP_INT:
            sprintf(buff, "%c%d\n", AplTab[i].item,
                *((int *)AplTab[i].variable));
            break;

        case TYP_FLOAT:
            sprintf(buff, "%c%f\n", AplTab[i].item,
                    *((float *)AplTab[i].variable));
            break;

        case TYP_LONG:
            sprintf(buff, "%c%ld\n", AplTab[i].item,
                    *((long *)AplTab[i].variable));
            break;           

        case TYP_OTHER:
            switch (AplTab[i].item)
            {
            case APL_MDATA:
                if (onConsole)  
                {
                    sprintf(buff, "%c0 (LOCAL)\n", AplTab[i].item);
                }
                else
                {
                    sprintf(buff, "%c%d\n", AplTab[i].item, cfg.mdata);
                }
                break;

            case APL_HALL:
                sprintf(buff, "%c%s\n", AplTab[i].item, 
                        hallBuf->hall[thisHall].hallname);
                break;

            case APL_ROOM:
                sprintf(buff, "%c%s\n", AplTab[i].item, roomBuf.rbname);
                break;

            case APL_ACCOUNTING:
                if(!logBuf.lbflags.NOACCOUNT && cfg.accounting)
                {
                    sprintf(buff, "%c1\n", AplTab[i].item);
                }
                else
                {
                    sprintf(buff, "%c0\n", AplTab[i].item);
                }
                break;
            
            case APL_PERMANENT:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.PERMANENT);
                break;
            
            case APL_VERIFIED:
                sprintf(buff, "%c%d\n", AplTab[i].item,
                        lBuf.VERIFIED ? 0 : 1);
                break;

            case APL_NETUSER:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NETUSER);
                break;

            case APL_NOMAIL:
                sprintf(buff, "%c%d\n", AplTab[i].item, lBuf.lbflags.NOMAIL);
                break;

            case APL_CHAT:
                sprintf(buff, "%c%d\n", AplTab[i].item, cfg.noChat);
                break;

            case APL_BELLS:
                sprintf(buff, "%c%d\n", AplTab[i].item, cfg.noBells);
                break;

            default:
                buff[0] = 0;
                break;
            }
            break;

        default:
            buff[0] = 0;
            break;
        }

        if (strlen(buff) > 1)
        {
            fputs(buff, fd);
        }
    }

    fprintf(fd, "%c\n", APL_END);
    
    fclose(fd);
}



/************************************************************************/
/*  Extended Download                                                   */ 
/************************************************************************/ 
void wxsnd(char *path, char *file, char trans)
{
    char  stuff[100];
    label tmp1, tmp2;

    if (changedir(path) == -1 )  return;

    sprintf(tmp1, "%d", cfg.mdata);
    sprintf(tmp2, "%d", bauds[speed]);
    sformat(stuff, extrn[trans-1].ex_snd, "fpsa", file, tmp1, tmp2, cfg.aplpath);
    apsystem(stuff);

    if (debug)  cPrintf("(%s)", stuff);
}

void wxrcv(char *path, char *file, char trans)
{
    char stuff[100];
    label tmp1, tmp2;

    if (changedir(path) == -1 )  return;

    sprintf(tmp1, "%d", cfg.mdata);
    sprintf(tmp2, "%d", bauds[speed]);
    sformat(stuff, extrn[trans-1].ex_rcv, "fpsa", file, tmp1, tmp2, cfg.aplpath);
    apsystem(stuff);

    if (debug)  cPrintf("(%s)", stuff);
}

/* -------------------------------------------------------------------- */
/*      readuserin()  reads userdati.apl from disk                      */
/* -------------------------------------------------------------------- */
static void readAplFile(void)
{
    FILE *fd;
    int i;
    char buff[200];
    int item;
    int roomno;
    int found;
    int slot;

    if (readMessage)
    {
        clearmsgbuf();
        strcpy(msgBuf->mbauth, cfg.nodeTitle);
        msgBuf->mbroomno = thisRoom;
    }

    if ((fd = fopen("input.apl", "rt")) != NULL)
    {
        do
        {
            item = fgetc(fd);
            if (feof(fd)) 
            {
                break;
            }

            fgets(buff, 198, fd);
            buff[strlen(buff)-1] = 0;
    
            found = FALSE;

            for(i = 0; AplTab[i].item != APL_END; i++)
            {
                if (AplTab[i].item == item && AplTab[i].keep)
                {
                    found = TRUE;

                    switch(AplTab[i].type)
                    {
                    case TYP_STR:
                        strncpy((char *)AplTab[i].variable, buff, AplTab[i].length);
                        ((char *)AplTab[i].variable)[ AplTab[i].length - 1 ] = 0;
                        break;
    
                    case TYP_BOOL:
                    case TYP_CHAR:
                        *((char *)AplTab[i].variable) = (char)atoi(buff);
                        break;
    
                    case TYP_INT:
                        *((int *)AplTab[i].variable) = atoi(buff);
                        break;
    
                    case TYP_FLOAT:
                        *((float *)AplTab[i].variable) = atof(buff);
                        break;
    
                    case TYP_LONG:
                        *((long *)AplTab[i].variable) = atol(buff);
                        break;
    
                    case TYP_OTHER:
                        switch (AplTab[i].item)
                        {
                        case APL_HALL:
                            if (stricmp(buff, hallBuf->hall[thisHall].hallname)
                                != SAMESTRING)
                            {
                                slot = hallexists(buff);
                                if (slot != ERROR)
                                {
                                    mPrintf("Hall change to: %s", buff);
                                    doCR();
                                    thisHall = (unsigned char)slot;
                                }
                                else
                                {
                                    cPrintf("No such hall %s!\n", buff);
                                }
                            }
                            break;
            
                        case APL_ROOM:
                            if ( (roomno = roomExists(buff)) != ERROR)
                            {
                                if (roomno != thisRoom)
                                {
                                    mPrintf("Room change to: %s", buff);
                                    doCR();
                                    logBuf.lbroom[thisRoom].lbgen   
                                            = roomBuf.rbgen; 
                                    ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
                                    ug_new    = talleyBuf.room[thisRoom].new;
                                    logBuf.lbroom[thisRoom].lvisit   = 0; 
                                    logBuf.lbroom[thisRoom].mail     = 0;
                                    /* zero new count in talleybuffer */
                                    talleyBuf.room[thisRoom].new     = 0;

                                    getRoom(roomno);
 
                                    if ((logBuf.lbroom[thisRoom].lbgen ) 
                                        != roomBuf.rbgen)
                                    {
                                        logBuf.lbroom[thisRoom].lbgen
                                                = roomBuf.rbgen;
                                        logBuf.lbroom[thisRoom].lvisit
                                                = (MAXVISIT - 1);
                                    }
                                }
                            }
                            else
                            {
                                cPrintf("No such room: %s!\n", buff);
                            }
                            break;
                        
                        case APL_PERMANENT:
                            lBuf.lbflags.PERMANENT = atoi(buff);
                            break;
                        
                        case APL_VERIFIED:
                            lBuf.VERIFIED = !atoi(buff);
                            break;
            
                        case APL_NETUSER:
                            lBuf.lbflags.NETUSER = atoi(buff);
                            break;
            
                        case APL_NOMAIL:
                            lBuf.lbflags.NOMAIL = atoi(buff);
                            break;
            
                        case APL_CHAT:
                            cfg.noChat = atoi(buff);
                            break;
            
                        case APL_BELLS:
                            cfg.noBells = atoi(buff);
                            break;
            
                        default:
                            mPrintf("Bad value %d \"%s\"", item, buff); doCR();
                            break;
                        }
                        break;
    
                    default:
                        break;
                    }
                }
            }

            if (!found && readMessage)
            {
                found = TRUE;

                switch (item)
                {
                case MSG_NAME:
                    strcpy(msgBuf->mbauth, buff);
                    break;
    
                case MSG_TO:
                    strcpy(msgBuf->mbto, buff);
                    break;
    
                case MSG_GROUP:
                    strcpy(msgBuf->mbgroup, buff);
                    break;
    
                case MSG_ROOM:
                    if ( (roomno = roomExists(buff)) == ERROR)
                    {
                        cPrintf(" AP: No room \"%s\"!\n", buff);
                    }
                    else
                    {
                        msgBuf->mbroomno = roomno;
                    }
                    break;

                default:
                    doCR();
                    found = FALSE;
                    break;
                }
            }

            if (!found && AplTab[i].item != APL_END)
            {
                mPrintf("Bad value %d \"%s\"", item, buff); doCR();
            }
        }
        while (item != APL_END && !feof(fd));

#ifdef BAD
        unlink("input.apl");
#endif        

        fclose(fd);
    }

    update25();

    if (readMessage)
    {
        if ((fd = fopen("message.apl", "rb")) != NULL)
        {
            GetFileMessage(fd, msgBuf->mbtext, cfg.maxtext);
            fclose(fd);
          
            putMessage();
            noteMessage();
        }
        unlink("message.apl");
    }

    if (filexists("readme.apl"))
    {
        dumpf("readme.apl");
        unlink("readme.apl");
        doCR();
    }
}

