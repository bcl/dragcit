/************************************************************************/
/*                              confg.c                                 */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#include <dir.h>
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "keywords.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*      buildcopies()           copies appropriate msg index members    */
/*      buildhalls()            builds hall-table (all rooms in Maint.) */
/*      buildroom()             builds a new room according to msg-buf  */
/*      clearaccount()          sets all group accounting data to zero  */
/*      configcit()             the main configuration for citadel      */
/*      illegal()               abort config.exe program                */
/*      initfiles()             opens & initalizes any missing files    */
/*      logInit()               indexes log.dat                         */
/*      logSort()               Sorts 2 entries in logTab               */
/*      msgInit()               builds message table from msg.dat       */
/*      readaccount()           reads grpdata.cit values into grp struct*/
/*      readconfig()            reads config.cit values                 */
/*      RoomTabBld()            builds room.tab, index's room.dat       */
/*      showtypemsg()           displays what kind of message was read  */
/*      slidemsgTab()           frees slots at beginning of msg table   */
/*      zapGrpFile()            initializes grp.dat                     */
/*      zapHallFile()           initializes hall.dat                    */
/*      zapLogFile()            initializes log.dat                     */
/*      zapMsgFile()            initializes msg.dat                     */
/*      zapRoomFile()           initializes room.dat                    */
/************************************************************************/

/************************************************************************/
/*                External variable declarations in CONFG.C             */
/************************************************************************/
          
/************************************************************************/
/*      buildcopies()  copies appropriate msg index members             */
/************************************************************************/
void buildcopies(void)
{
    int i;

    for( i = 0; i < sizetable(); ++i)
    {
        if (msgTab[i].mtmsgflags.COPY)
        {
            if (msgTab[i].mtoffset <= i)
            {
                copyindex( i, (i - msgTab[i].mtoffset));
            }
        }
    }
}

/************************************************************************/
/*      buildhalls()  builds hall-table (all rooms in Maint.)           */
/************************************************************************/
void buildhalls(void)
{
    int i;

    doccr(); cPrintf("Building hall file "); doccr();

    for (i = 4; i < MAXROOMS; ++i)
    {
        if (roomTab[i].rtflags.INUSE)
        {
            hallBuf->hall[1].hroomflags[i].inhall = 1;  /* In Maintenance */
            hallBuf->hall[1].hroomflags[i].window = 0;  /* Not a Window   */
        }
    }
    putHall();
}

/************************************************************************/
/*      buildroom()  builds a new room according to msg-buf             */
/************************************************************************/
void buildroom(void)
{
    int roomslot;

    if (*msgBuf->mbcopy) return;
    roomslot = msgBuf->mbroomno;

    if (msgBuf->mbroomno < MAXROOMS)
    {
        getRoom(roomslot);

        if ((strcmp(roomBuf.rbname, msgBuf->mbroom) != SAMESTRING)
        || (!roomBuf.rbflags.INUSE))
        {
            if (msgBuf->mbroomno > 3)
            {
                roomBuf.rbflags.INUSE     = TRUE;
                roomBuf.rbflags.PERMROOM  = FALSE;
                roomBuf.rbflags.MSDOSDIR  = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbroomtell[0]     = '\0';
                roomBuf.rbflags.PUBLIC    = TRUE;
            }
            strcpy(roomBuf.rbname, msgBuf->mbroom);

            putRoom(thisRoom);
        }
    }
}

/************************************************************************/
/*      clearaccount()  initializes all group data                      */
/************************************************************************/
void clearaccount(void)
{
    int i;
    int groupslot;

    for (groupslot = 0; groupslot < MAXGROUPS; groupslot++)
    {
        /* init days */
        for ( i = 0; i < 7; i++ )
            accountBuf.group[groupslot].account.days[i] = 1;

        /* init hours & special hours */
        for ( i = 0; i < 24; i++ )
        {
            accountBuf.group[groupslot].account.hours[i]   = 1;
            accountBuf.group[groupslot].account.special[i] = 0;
        }

        accountBuf.group[groupslot].account.have_acc      = FALSE;
        accountBuf.group[groupslot].account.dayinc        = 0.;
        accountBuf.group[groupslot].account.sp_dayinc     = 0.;
        accountBuf.group[groupslot].account.maxbal        = 0.;
        accountBuf.group[groupslot].account.priority      = 0.;
        accountBuf.group[groupslot].account.dlmult        = -1;
        accountBuf.group[groupslot].account.ulmult        =  1;

    }
}

/************************************************************************/
/*      configcit() the <main> for configuration                        */
/************************************************************************/
void configcit(void)
{
    fcloseall();

    /* read config.cit */
    readconfig();

    /* move to home-path */
    changedir(cfg.homepath);

    /* initialize & open any files */
    initfiles();

    if (msgZap )  zapMsgFile();
    if (roomZap)  zapRoomFile();
    if (logZap )  zapLogFile();
    if (grpZap )  zapGrpFile();
    if (hallZap)  zapHallFile();

    if (roomZap && !msgZap)  roomBuild = TRUE;
    if (hallZap && !msgZap)  hallBuild = TRUE;

    logInit();
    msgInit();
    RoomTabBld();

    if (hallBuild)  buildhalls();

    fclose(grpfl);
    fclose(hallfl);
    fclose(roomfl);
    fclose(msgfl);
    fclose(logfl);

    doccr();
    cPrintf("Config Complete");
    doccr();
}

/***********************************************************************/
/*    illegal() Prints out configur error message and aborts           */
/***********************************************************************/
void illegal(char *errorstring)
{
    doccr();
    cPrintf("%s", errorstring);
    doccr();
    cPrintf("Fatal error in config. Aborting."); doccr();
    exit(7);
}

/************************************************************************/
/*      initfiles() -- initializes files, opens them                    */
/************************************************************************/
void initfiles(void)
{
    char  *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
    char scratch[64];

    chdir(cfg.homepath);

    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");

    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    msgFile     =  scratch  ;
    roomFile    = "room.dat";

    /* open group file */
    if ((grpfl = fopen(grpFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", grpFile);  doccr();
        if ((grpfl = fopen(grpFile, "w+b")) == NULL)
            illegal("Can't create the group file!");
        {
            cPrintf(" It will be initialized. "); doccr();
            grpZap = TRUE;
        }
    }

    /* open hall file */
    if ((hallfl = fopen(hallFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", hallFile); doccr();
        if ((hallfl = fopen(hallFile, "w+b")) == NULL)
            illegal("Can't create the hall file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            hallZap = TRUE;
        }
    }

    /* open log file */
    if ((logfl = fopen(logFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", logFile); doccr();
        if ((logfl = fopen(logFile, "w+b")) == NULL)
            illegal("Can't create log file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            logZap = TRUE;
        }
    }

    /* open message file */
    if ((msgfl = fopen(msgFile, "r+b")) == NULL)
    {
        cPrintf(" msg.dat not found, creating new file. ");  doccr();
        if ((msgfl = fopen(msgFile, "w+b")) == NULL)
            illegal("Can't create the message file!");
        {
            cPrintf(" It will be initialized. ");  doccr();
            msgZap = TRUE;
        }
    }

    /* open room file */
    if ((roomfl = fopen(roomFile, "r+b")) == NULL)
    {
        cPrintf(" %s not found, creating new file. ", roomFile);  doccr();
        if ((roomfl = fopen(roomFile, "w+b")) == NULL)
            illegal("Can't create room file!");
        {
            cPrintf(" It will be initialized. "); doccr();
            roomZap = TRUE;
        }
    }
}

/************************************************************************/
/*      logInit() indexes log.dat                                       */
/************************************************************************/
void logInit(void)
{
    int i;
    int count = 0;

    doccr(); doccr();
    cPrintf("Building log table "); doccr();

    cfg.callno = 0l;

    rewind(logfl);
    /* clear logTab */
    for (i = 0; i < cfg.MAXLOGTAB; i++) logTab[i].ltcallno = 0l;

    /* load logTab: */
    for (thisLog = 0;  thisLog < cfg.MAXLOGTAB;  thisLog++)
    {
  
        cPrintf("log#%3d\r",thisLog);

        getLog(&logBuf, thisLog);

        if (logBuf.callno > cfg.callno)  cfg.callno = logBuf.callno;

        /* count valid entries:             */

        if (logBuf.lbflags.L_INUSE == 1)  count++;

      
        /* copy relevant info into index:   */
        logTab[thisLog].ltcallno = logBuf.callno;
        logTab[thisLog].ltlogSlot= thisLog;
        logTab[thisLog].permanent = logBuf.lbflags.PERMANENT;

        if (logBuf.lbflags.L_INUSE == 1)
        {
            logTab[thisLog].ltnmhash = hash(logBuf.lbname);
            logTab[thisLog].ltinhash = hash(logBuf.lbin  );
            logTab[thisLog].ltpwhash = hash(logBuf.lbpw  );
        }
        else
        {
            logTab[thisLog].ltnmhash = 0;
            logTab[thisLog].ltinhash = 0;
            logTab[thisLog].ltpwhash = 0;
        }
    }
    doccr();
    cPrintf("%lu calls.", cfg.callno);
    doccr();
    cPrintf("%d valid log entries.", count);  doccr();

    qsort(logTab, (unsigned)cfg.MAXLOGTAB, (unsigned)sizeof(*logTab), logSort);
}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
int logSort(s1, s2)
struct lTable *s1, *s2;
{
    if (s1->ltnmhash == 0 && s2->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltcallno < s2->ltcallno)
        return 1;
    if (s1->ltcallno > s2->ltcallno)
        return -1;
    return 0;
}

/************************************************************************/
/*      msgInit() sets up lowId, highId, cfg.catSector and cfg.catChar, */
/*      by scanning over message.buf                                    */
/************************************************************************/
void msgInit(void)
{
    ulong first, here;
    int makeroom;
    int skipcounter = 0;   /* maybe need to skip a few . Dont put them in
                              message index */
    int slot;

    doccr(); doccr();
    cPrintf("Building message table"); doccr();

    /* start at the beginning */
    fseek(msgfl, 0l, 0);

    getMessage();

    /* get the ID# */
    sscanf(msgBuf->mbId, "%ld", &first);

    showtypemsg(first);

    /* put the index in its place */
    /* mark first entry of index as a reference point */

    cfg.mtoldest = first;
    
    indexmessage(first);

    cfg.newest = cfg.oldest = first;

    cfg.catLoc = ftell(msgfl);

    while (TRUE)
    {
        getMessage();

        sscanf(msgBuf->mbId, "%ld", &here);

        if (here == first) break;

        showtypemsg(here);

        /* find highest and lowest message IDs: */
        /* >here< is the dip pholks             */
        if (here < cfg.oldest)
        {
            slot = ( indexslot(cfg.newest) + 1 );

            makeroom = (int)(cfg.mtoldest - here);

            /* check to see if our table can hold  remaining msgs */
            if (cfg.nmessages < (makeroom + slot))
            {
                skipcounter = (makeroom + slot) - cfg.nmessages;

                slidemsgTab(makeroom - skipcounter);

                cfg.mtoldest = (here + (ulong)skipcounter);
 
            }
            /* nmessages can handle it.. Just make room */
            else
            {
                slidemsgTab(makeroom);
                cfg.mtoldest = here;
            }
            cfg.oldest = here;
        }

        if (here > cfg.newest)
        {
            cfg.newest = here;

            /* read rest of message in and remember where it ends,      */
            /* in case it turns out to be the last message              */
            /* in which case, that's where to start writing next message*/
            while (dGetWord(msgBuf->mbtext, MAXTEXT));

            cfg.catLoc = ftell(msgfl);
        }

        /* check to see if our table is big enough to handle it */
        if ( (int)(here - cfg.mtoldest) >= cfg.nmessages)
        {
            crunchmsgTab(1);
        }

        if (skipcounter) 
        {
            skipcounter--;
        }
        else
        {
            indexmessage(here);
        }
    }    
    buildcopies();
}             

/************************************************************************/
/*      readaccount()  reads grpdata.cit values into group structure    */
/************************************************************************/
void readaccount(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  i, j, k, l, count;
    int groupslot = ERROR;
    int hour;
   
    clearaccount();   /* initialize all accounting data */

    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("grpdata.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Grpdata.cit!"); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; grpkeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], grpkeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        switch(i)
        {
            case GRK_DAYS:              
                if (groupslot == ERROR)  break;

                /* init days */
                for ( j = 0; j < 7; j++ )
                    accountBuf.group[groupslot].account.days[j] = 0;

                for (j = 1; j < count; j++)
                {
                    for (k = 0; daykeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                        {
                            break;
                        }
                    }
                    if (k < 7)
                        accountBuf.group[groupslot].account.days[k] = TRUE;
                    else if (k == 7)  /* any */
                    {
                        for ( l = 0; l < MAXGROUPS; ++l)
                            accountBuf.group[groupslot].account.days[l] = TRUE;
                    }
                    else
                    {
                        doccr();
                   cPrintf("Grpdata.Cit - Warning: Unknown day %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case GRK_GROUP:             
                groupslot = groupexists(words[1]);
                if (groupslot == ERROR)
                {
                    doccr();
                    cPrintf("Grpdata.Cit - Warning: Unknown group %s", words[1]);
                    doccr();
                }
                accountBuf.group[groupslot].account.have_acc = TRUE;
                break;

            case GRK_HOURS:             
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.hours[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.hours[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 ) 
                        {
                            doccr();
                            cPrintf("Grpdata.Cit - Warning: Invalid hour %d ",
                            hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.hours[hour] = TRUE;
                    }
                }
                break;

            case GRK_DAYINC:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dayinc);   /* float */
                }
                break;

            case GRK_DLMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dlmult);   /* float */
                }
                break;

            case GRK_ULMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.ulmult);   /* float */
                }
                break;

            case GRK_PRIORITY:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.priority);  /* float */
                }

                break;

            case GRK_MAXBAL:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.maxbal);   /* float */
                }

                break;



            case GRK_SPECIAL:           
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.special[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.special[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 )
                        {
                            doccr();
                            cPrintf("Grpdata.Cit - Warning: Invalid special hour %d ", hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.special[hour] = TRUE;
                    }

                }
                break;
        }

    }
    fclose(fBuf);
}

/************************************************************************/
/*      readprotocols() reads protocol.cit values into ext   structure  */
/************************************************************************/
void readprotocols(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  j, count;

    numOthCmds   = 0;
    extrncmd[0] = NULL;
    editcmd[0]  = NULL;
   
    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("external.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find external.cit!"); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        if (strcmpi("#PROTOCOL", words[0]) == SAMESTRING)
        {
            j = strlen(extrncmd);

            if (strlen( words[1] ) > 19 )
              illegal("Protocol name to long; must be less than 20");
            if (strlen( words[3] ) > 39 )
              illegal("Recive command to long; must be less than 40");
            if (strlen( words[4] ) > 39 )
              illegal("Send command to long; must be less than 40");
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Batch field bad; must be 0 or 1");
            if (atoi(words[2]) < 0 || atoi(words[2]) > 10 * 1024)
              illegal("Block field bad; must be 0 to 10K");
            if (j >= MAXEXTERN)
              illegal("To many external proticals");
    
            strcpy(extrn[j].ex_name, words[1]);
            extrn[j].ex_batch = atoi(words[2]);
            extrn[j].ex_block = atoi(words[3]);
            strcpy(extrn[j].ex_rcv,  words[4]);
            strcpy(extrn[j].ex_snd,  words[5]);
            extrncmd[j]   = tolower(*words[1]);
            extrncmd[j+1] = '\0';
        }
        if (strcmpi("#EDITOR", words[0]) == SAMESTRING)
        {
            j = strlen(editcmd);

            if (strlen( words[1] ) > 19 )
              illegal("Protocol name to long; must be less than 20");
            if (strlen( words[3] ) > 29 )
              illegal("Command line to long; must be less than 30");
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Local field bad; must be 0 or 1");
            if (j > 19)
              illegal("Only 20 external editors");
    
            strcpy(edit[j].ed_name,  words[1]);
            edit[j].ed_local  = atoi(words[2]);
            strcpy(edit[j].ed_cmd,   words[3]);
            editcmd[j]    = tolower(*words[1]);
            editcmd[j+1]                = '\0';
        }
        if (strcmpi("#OTHER", words[0]) == SAMESTRING)
        {
            if (count < 6)
              illegal("To few arguments for #OTHER command");
            if (strlen( words[1] ) > 39 )
              illegal("Command line to long; must be less than 40");
            if (strlen( words[3] ) > 19 )
              illegal("Protocol name (#1) to long; must be less than 20");
            if (strlen( words[5] ) > 19 )
              illegal("Protocol name (#2) to long; must be less than 20");
            if (numOthCmds >= (MAXEXTERN) )
              illegal("To many #OTHER external commands");
    
            strcpy(othCmd[numOthCmds].eo_cmd,   words[1]);
            othCmd[numOthCmds].eo_cmd1 =        tolower(*words[2]);
            strcpy(othCmd[numOthCmds].eo_name1, words[3]);
            othCmd[numOthCmds].eo_cmd1 =        tolower(*words[4]);
            strcpy(othCmd[numOthCmds].eo_name2, words[5]);
            numOthCmds++;
        }
    }
    fclose(fBuf);
}

/*
 * count the lines that start with keyword...
 *
int keyword_count(key, filename)
char *key;
char *filename;
{
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  count = 0;
   
    changedir(cfg.homepath);

    if ((fBuf = fopen(filename, "r")) == NULL) 
    {  
        cPrintf("Can't find %s!", filename); doccr();
        exit(1);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        parse_it( words, line);

        if (strcmpi(key, words[0]) == SAMESTRING)
          count++;
   }

   fclose(fBuf);

   return (count == 0 ? 1 : count);
} */

/************************************************************************/
/*      readconfig() reads config.cit values                            */
/************************************************************************/
void readconfig(void)
{
    FILE *fBuf;
    char line[256];
    char *words[256];
    int  i, j, k, l, count, att;
    char notkeyword[20];
    char valid = FALSE;
    char found[K_NWORDS+2];
    int  lineNo = 0;

/**** not needed, if not found it is complained about */
/*  cfg.MAXLOGTAB = 0;             * Initialize, just in case */
/*  cfg.maxfiles = 255;            * Initialize, just in case */
/*  strcpy(cfg.tdformat, "%x %X"); * Initialize, just in case */
    strcpy(cfg.msg_nym,  "message");
    strcpy(cfg.msgs_nym, "messages");
    strcpy(cfg.msg_done, "saved");
    cfg.version = 3110000L;
    
    for (i=0; i <= K_NWORDS; i++)
        found[i] = FALSE;

    if ((fBuf = fopen("config.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Config.cit!"); doccr();
        exit(3);
    }

    while (fgets(line, 255, fBuf) != NULL)
    {
        lineNo++;

        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; keywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], keywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        if (keywords[i] == NULL)
        {
            cPrintf("CONFIG.CIT (%d) Warning: Unknown variable %s ", lineNo, 
                words[0]);
            doccr();
            continue;
        }else{
            if (found[i] == TRUE)
            {
                cPrintf("CONFIG.CIT (%d) Warning: %s mutiply defined!", lineNo, 
                    words[0]);
                doccr();
            }else{
                found[i] = TRUE;
            }
        }

        switch(i)
        {
            case K_ACCOUNTING:
                cfg.accounting = atoi(words[1]);
                break;

            case K_IDLE_WAIT:
                cfg.idle = atoi(words[1]);
                break;

            case K_ALLDONE: 
                break;

            case K_ATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.attr = (uchar)att;
                break;

            case K_WATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.wattr = (uchar)att;
                break;

            case K_CATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.cattr = (uchar)att;
                break;

            case K_BATTR:
                sscanf(words[1], "%x ", &att);    /* hex! */
                cfg.battr = att;
                break;

            case K_UTTR:
                sscanf(words[1], "%x ", &att);     /* hex! */
                cfg.uttr = att;
                break;


            case K_INIT_BAUD:
                cfg.initbaud = atoi(words[1]);
                break;

            case K_BIOS:
                cfg.bios = atoi(words[1]);
                break;

            case K_COST1200:
                sscanf(words[1], "%f ", &cfg.cost1200); /* float */
                break;

            case K_COST2400:
                sscanf(words[1], "%f ", &cfg.cost2400); /* float */
                break;

            case K_DUMB_MODEM:
                cfg.dumbmodem    = atoi(words[1]);
                break;

            case K_READLLOG:
                cfg.readluser    = atoi(words[1]);
                break;

/*            
            case K_ENTERSUR:
                cfg.entersur     = atoi(words[1]);
                break;
 */                

            case K_DATESTAMP:
                if (strlen( words[1] ) > 63 )
                illegal("#DATESTAMP too long; must be less than 64");

                strcpy( cfg.datestamp, words[1] );
                break;

            case K_VDATESTAMP:
                if (strlen( words[1] ) > 63 )
                illegal("#VDATESTAMP too long; must be less than 64");

                strcpy( cfg.vdatestamp, words[1] );
                break;

            case K_ENTEROK:
                cfg.unlogEnterOk = atoi(words[1]);
                break;

            case K_FORCELOGIN: 
                cfg.forcelogin   = atoi(words[1]);
                break;

            case K_MODERATE: 
                cfg.moderate     = atoi(words[1]);
                break;

            case K_HELPPATH:  
                if (strlen( words[1] ) > 63 )
                illegal("helppath too long; must be less than 64");

                strcpy( cfg.helppath, words[1] );  
                break;

            case K_TEMPPATH:
                if (strlen( words[1] ) > 63 )
                illegal("temppath too long; must be less than 64");

                strcpy( cfg.temppath, words[1] );
                break;


            case K_HOMEPATH:
                if (strlen( words[1] ) > 63 )
                    illegal("homepath too long; must be less than 64");

                strcpy(cfg.homepath, words[1] );  
                break;

            case K_KILL:
                cfg.kill = atoi(words[1]);
                break;

            case K_LINEFEEDS:
                cfg.linefeeds = atoi(words[1]);
                break;
            
            case K_LOGINSTATS:
                cfg.loginstats = atoi(words[1]);
                break;

            case K_MAXBALANCE:
                sscanf(words[1], "%f ", &cfg.maxbalance); /* float */
                break;

            case K_MAXLOGTAB:
                cfg.MAXLOGTAB    = atoi(words[1]);

                break;

            case K_MESSAGE_ROOM:
                cfg.MessageRoom = atoi(words[1]);
                break;

            case K_NEWUSERAPP:
                if (strlen( words[1] ) > 12 )
                illegal("NEWUSERAPP too long; must be less than 13");

                strcpy( cfg.newuserapp, words[1] );
                break;

            case K_PRIVATE:
                cfg.private = atoi(words[1]);
                break;

            case K_MAXTEXT:
                cfg.maxtext = atoi(words[1]);
                break;

            case K_MAX_WARN:
                cfg.maxwarn = atoi(words[1]);
                break;

            case K_MDATA:
                cfg.mdata   = atoi(words[1]);

                if ( (cfg.mdata < 1) || (cfg.mdata > 4) )
                {
                    illegal("MDATA port can only currently be 1, 2, 3 or 4");
                }
                break;

            case K_MAXFILES:
                cfg.maxfiles = atoi(words[1]);
                break;

            case K_MSGPATH:
                if (strlen(words[1]) > 63)
                    illegal("msgpath too long; must be less than 64");

                strcpy(cfg.msgpath, words[1]);  
                break;

            case K_F6PASSWORD:
                if (strlen(words[1]) > 19)
                    illegal("f6password too long; must be less than 20");

                strcpy(cfg.f6pass, words[1]);  
                break;

            case K_APPLICATIONS:
                if (strlen(words[1]) > 63)
                    illegal("applicationpath too long; must be less than 64");

                strcpy(cfg.aplpath, words[1]);  
                break;

            case K_MESSAGEK:
                cfg.messagek = atoi(words[1]);
                break;

            case K_MODSETUP:
                if (strlen(words[1]) > 63)
                    illegal("Modsetup too long; must be less than 64");

                strcpy(cfg.modsetup, words[1]);  
                break;
                
            case K_DIAL_INIT:
                if (strlen(words[1]) > 63)
                    illegal("Dial_Init too long; must be less than 64");

                strcpy(cfg.dialsetup, words[1]);  
                break;
                
            case K_DIAL_PREF:
                if (strlen(words[1]) > 20)
                    illegal("Dial_Prefix too long; must be less than 20");

                strcpy(cfg.dialpref, words[1]);  
                break;

            case K_NEWBAL:
                sscanf(words[1], "%f ", &cfg.newbal);  /* float */
                break;

/*            
            case K_SURNAMES:
                cfg.surnames = (atoi(words[1]) != 0);
                cfg.netsurname = (atoi(words[1]) == 2);
                break;
 */                

            case K_AIDEHALL:
                cfg.aidehall = atoi(words[1]);
                break;

            case K_NMESSAGES:
                cfg.nmessages  = atoi(words[1]);

                break;

            case K_NODENAME:
                if (strlen(words[1]) > 19)
                    illegal("nodeName too long; must be less than 20");

                strcpy(cfg.nodeTitle, words[1]);  
                break;

            case K_NODEREGION:
                if (strlen(words[1]) > 19)
                    illegal("nodeRegion too long; must be less than 20");

                strcpy(cfg.nodeRegion, words[1]);
                break;


            case K_NOPWECHO:
                cfg.nopwecho = (unsigned char)atoi(words[1]);
                break;

            case K_NULLS:
                cfg.nulls = atoi(words[1]);
                break;

            case K_OFFHOOK:
                cfg.offhook = atoi(words[1]);
                break;

            case K_OLDCOUNT:
                cfg.oldcount = atoi(words[1]);
                break;

            case K_PRINTER:
                if (strlen(words[1]) > 63)
                    illegal("printer too long; must be less than 64");

                strcpy(cfg.printer, words[1]);  
                break;

            case K_READOK:
                cfg.unlogReadOk = atoi(words[1]);
                break;

            case K_ROOMOK:
                cfg.nonAideRoomOk = atoi(words[1]);
                break;

            case K_ROOMTELL:
                cfg.roomtell = atoi(words[1]);
                break;

            case K_ROOMPATH:
                if (strlen(words[1]) > 63)
                    illegal("roompath too long; must be less than 64");

                strcpy(cfg.roompath, words[1]);  
                break;

            case K_SUBHUBS:
                cfg.subhubs = atoi(words[1]);
                break;

            case K_TABS:
                cfg.tabs = atoi(words[1]);
                break;
            
            case K_TIMEOUT:
                cfg.timeout = atoi(words[1]);
                break;

            case K_TRAP:
                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; trapkeywords[k] != NULL; k++)
                    {
                        sprintf(notkeyword, "!%s", trapkeywords[k]);

                        if (strcmpi(words[j], trapkeywords[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = TRUE;
                            }
                            else cfg.trapit[k] = TRUE;
                        }
                        else if (strcmpi(words[j], notkeyword) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = FALSE;
                            }
                            else cfg.trapit[k] = FALSE; 
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TRAP parameter %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case K_TRAP_FILE:
                if (strlen(words[1]) > 63)
                    illegal("Trap filename too long; must be less than 64");
  
                strcpy(cfg.trapfile, words[1]);  

                break;

            case K_UNLOGGEDBALANCE:
                sscanf(words[1], "%f ", &cfg.unlogbal);  /* float */
                break;

            case K_UNLOGTIMEOUT:
                cfg.unlogtimeout = atoi(words[1]);
                break;

            case K_UPPERCASE:
                cfg.uppercase = atoi(words[1]);
                break;

            case K_USER:
                for ( j = 0; j < 5; ++j)  cfg.user[j] = 0;

                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; userkeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], userkeywords[k]) == SAMESTRING)
                        {
                           valid = TRUE;

                           cfg.user[k] = TRUE;
                        }
                    }

                    if (!valid)
                    {
                        doccr();
                   cPrintf("Config.Cit - Warning: Unknown #USER parameter %s ",
                        words[j]);
                        doccr();
                    }
                }
                break;

            case K_WIDTH:
                cfg.width = atoi(words[1]);
                break;

            case K_TWIT_FEATURES:
                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; twitfeatures[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], twitfeatures[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            switch (k)
                            {
                            case 0:     /* MESSAGE NYMS */
                                cfg.msgNym = TRUE;
                                break;

                            case 1:     /* BOARDERS */
                                cfg.borders = TRUE;
                                break;
                            
                            case 2:     /* TITLES */
                                cfg.titles = TRUE;
                                break;
                            
                            case 3:     /* NET_TITLES */
                                cfg.nettitles = TRUE;
                                break;
                            
                            case 4:     /* SURNAMES */
                                cfg.surnames = TRUE;
                                break;
                            
                            case 5:     /* NET_SURNAMES */
                                cfg.netsurname = TRUE;
                                break;
                            
                            case 6:     /* ENTER_TITLES */
                                cfg.entersur = TRUE;
                                break;

                            default:
                                break;
                            }
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TWIT_FEATURES parameter %s ",
                                words[j]);
                        doccr();
                    }
                }
                break;

            default:
                cPrintf("Config.Cit - Warning: Unknown variable %s", words[0]);
                doccr();
                break;
        }
    }
    fclose(fBuf);

    for (i = 0, valid = TRUE; i <= K_NWORDS; i++)
    {
        if (!found[i])
        {
            cPrintf("CONFIG.CIT : ERROR: can not find %s keyword!\n",
                keywords[i]);
            valid = FALSE;
        }
    }

    if (!valid)
        illegal("");

    allocateTables();

    if (logTab == NULL)
        illegal("Can not allocate log table");

    if (msgTab == NULL)
        illegal("Can not allocate message table");
}

/************************************************************************/
/*      RoomTabBld() -- build RAM index to ROOM.DAT, displays stats.    */
/************************************************************************/
void RoomTabBld(void)
{
    int  slot;
    int  roomCount = 0;

    doccr(); doccr();
    cPrintf("Building room table"); doccr();

    for (slot = 0;  slot < MAXROOMS;  slot++)
    {
        getRoom(slot);

        cPrintf("Room No: %3d\r", slot);

        if (roomBuf.rbflags.INUSE)  ++roomCount;
   
        noteRoom();
        putRoom(slot);
    }
    doccr();
    cPrintf(" %d of %d rooms in use", roomCount, MAXROOMS); doccr();

}

/************************************************************************/
/*      showtypemsg() prints out what kind of message is being read     */
/************************************************************************/
void showtypemsg(ulong here)
{
#   ifdef DEBUG
    cPrintf("(%7lu)", msgBuf->mbheadLoc);
#   endif

    if  (*msgBuf->mbcopy)           cPrintf("Dup Mess# %6lu\r", here);
    else
    {
        if       (*msgBuf->mbto)    cPrintf("Pri Mess# %6lu\r", here);
        else if  (*msgBuf->mbx == 'Y')
                                   cPrintf("Prb Mess# %6lu\r", here);
        else if  (*msgBuf->mbx == 'M')
                                   cPrintf("Mod Mess# %6lu\r", here);
        else if  (*msgBuf->mbgroup) cPrintf("Grp Mess# %6lu\r", here);
        else                       cPrintf("Pub Mess# %6lu\r", here);
    }
}

/************************************************************************/
/*      slidemsgTab() frees >howmany< slots at the beginning of the     */
/*      message table.                                                  */
/************************************************************************/
void slidemsgTab(int howmany)
{
    hmemcpy(&msgTab[howmany], &msgTab[0],(long)
      ((long)( (long)cfg.nmessages - (long)howmany) * (long)(sizeof(*msgTab)) )
    );
}

/************************************************************************/
/*      zapGrpFile(), erase & reinitialize group file                   */
/************************************************************************/
void zapGrpFile(void)
{
    doccr();
    cPrintf("Writing group table."); doccr();

    setmem(&grpBuf, sizeof grpBuf, 0);

    strcpy( grpBuf.group[0].groupname, "Null");
    grpBuf.group[0].g_inuse  = 1;
    grpBuf.group[0].groupgen = 1;      /* Group Null's gen# is one      */
                                       /* everyone's a member at log-in */

    strcpy( grpBuf.group[1].groupname, "Reserved_2");
    grpBuf.group[1].g_inuse   = 1;
    grpBuf.group[1].groupgen  = 1;

    putGroup();
}

/************************************************************************/
/*      zapHallFile(), erase & reinitialize hall file                   */
/************************************************************************/
void zapHallFile(void)
{
    doccr();
    cPrintf("Writing hall table.");  doccr();

    strcpy( hallBuf->hall[0].hallname, "Root");
    hallBuf->hall[0].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[0].h_inuse = 1;
    hallBuf->hall[0].hroomflags[0].inhall = 1;  /* Lobby> in Root        */
    hallBuf->hall[0].hroomflags[1].inhall = 1;  /* Mail>  in Root        */
    hallBuf->hall[0].hroomflags[2].inhall = 1;  /* Aide)  in Root        */

    strcpy( hallBuf->hall[1].hallname, "Maintenance");
    hallBuf->hall[1].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[1].h_inuse = 1;
    hallBuf->hall[1].hroomflags[0].inhall = 1;  /* Lobby> in Maintenance */
    hallBuf->hall[1].hroomflags[1].inhall = 1;  /* Mail>  in Maintenance */
    hallBuf->hall[1].hroomflags[2].inhall = 1;  /* Aide)  in Maintenance */


    hallBuf->hall[0].hroomflags[2].window = 1;  /* Aide) is the window   */
    hallBuf->hall[1].hroomflags[2].window = 1;  /* Aide) is the window   */

    putHall();
}

/************************************************************************/
/*      zapLogFile() erases & re-initializes userlog.buf                */
/************************************************************************/
zapLogFile()
{
    int  i;

    /* clear RAM buffer out:                    */
    logBuf.lbflags.L_INUSE   = FALSE;
    logBuf.lbflags.NOACCOUNT = FALSE;
    logBuf.lbflags.AIDE      = FALSE;
    logBuf.lbflags.NETUSER   = FALSE;
    logBuf.lbflags.NODE      = FALSE;
    logBuf.lbflags.PERMANENT = FALSE;
    logBuf.lbflags.PROBLEM   = FALSE;
    logBuf.lbflags.SYSOP     = FALSE;
    logBuf.lbflags.ROOMTELL  = FALSE;
    logBuf.lbflags.NOMAIL    = FALSE;
    logBuf.lbflags.UNLISTED  = FALSE;

    logBuf.callno = 0l;
 
    for (i = 0;  i < NAMESIZE;  i++)
    {
        logBuf.lbname[i] = 0;
        logBuf.lbin[i]   = 0;
        logBuf.lbpw[i]   = 0;
    }
    doccr();  doccr();
    cPrintf("MAXLOGTAB=%d",cfg.MAXLOGTAB);  doccr();

    /* write empty buffer all over file;        */
    for (i = 0; i < cfg.MAXLOGTAB;  i++)
    {
        cPrintf("Clearing log entry %3d\r", i);
        putLog(&logBuf, i);
        logTab[i].ltcallno = logBuf.callno;
        logTab[i].ltlogSlot= i;
        logTab[i].ltnmhash = hash(logBuf.lbname);
        logTab[i].ltinhash = hash(logBuf.lbin  );
        logTab[i].ltpwhash = hash(logBuf.lbpw  );
    }
    doccr();
    return TRUE;
}

/************************************************************************/
/*      zapMsgFl() initializes message.buf                              */
/************************************************************************/
zapMsgFile()
{
    int i;
    unsigned sect;
    static unsigned char  sectBuf[128];

    /* put null message in first sector... */
    sectBuf[0]  = 0xFF; /*                                   */
    sectBuf[1]  = DUMP; /*  \  To the dump                   */
    sectBuf[2]  = '\0'; /*  /  Attribute                     */
    sectBuf[3]  =  '1'; /*  >                                */
    sectBuf[4]  = '\0'; /*  \  Message ID "1" MS-DOS style   */
    sectBuf[5]  =  'M'; /*  /         Null messsage          */
    sectBuf[6]  = '\0'; /*                                   */
                                                  
    cfg.newest = cfg.oldest = 1l;

    cfg.catLoc = 7l;

    if (fwrite(sectBuf, 128, 1, msgfl) != 1)
    {
        cPrintf("zapMsgFil: write failed"); doccr();
    }

    for (i = 0;  i < 128;  i++) sectBuf[i] = 0;

    doccr();  doccr();
    cPrintf("MESSAGEK=%d", cfg.messagek);  doccr();
    for (sect = 1;  sect < (cfg.messagek * 8 );  sect++)
    {
        cPrintf("Clearing block %4u\r", sect);
        if (fwrite(sectBuf, 128, 1, msgfl) != 1)
        {
            cPrintf("zapMsgFil: write failed");  doccr();
        }
    }
    return TRUE;
}


/************************************************************************/
/*      zapRoomFile() erases and re-initailizes ROOM.DAT                */
/************************************************************************/
zapRoomFile()
{
    int i;

    roomBuf.rbflags.INUSE     = FALSE;
    roomBuf.rbflags.PUBLIC    = FALSE;
    roomBuf.rbflags.MSDOSDIR  = FALSE;
    roomBuf.rbflags.PERMROOM  = FALSE;
    roomBuf.rbflags.GROUPONLY = FALSE;
    roomBuf.rbflags.READONLY  = FALSE;
    roomBuf.rbflags.SHARED    = FALSE;
    roomBuf.rbflags.MODERATED = FALSE;
    roomBuf.rbflags.DOWNONLY  = FALSE;

    roomBuf.rbgen            = 0;
    roomBuf.rbdirname[0]     = 0;
    roomBuf.rbname[0]        = 0;   
    roomBuf.rbroomtell[0]    = 0;   
    roomBuf.rbgrpgen         = 0;
    roomBuf.rbgrpno          = 0;

    doccr();  doccr();
    cPrintf("MAXROOMS=%d", MAXROOMS); doccr();

    for (i = 0;  i < MAXROOMS;  i++)
    {
        cPrintf("Clearing room %3d\r", i);
        putRoom(i);
        noteRoom();
    }

    /* Lobby> always exists -- guarantees us a place to stand! */
    thisRoom            = 0          ;
    strcpy(roomBuf.rbname, "Lobby")  ;
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(LOBBY);
    noteRoom();

    /* Mail> is also permanent...       */
    thisRoom            = MAILROOM      ;
    strcpy(roomBuf.rbname, "Mail");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(MAILROOM);
    noteRoom();

    /* Aide) also...                    */
    thisRoom            = AIDEROOM;
    strcpy(roomBuf.rbname, "Aide");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = FALSE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(AIDEROOM);
    noteRoom();

    /* Dump> also...                    */
    thisRoom            = DUMP;
    strcpy(roomBuf.rbname, "Dump");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(DUMP);
    noteRoom();

    return TRUE;
}


