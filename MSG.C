/* -------------------------------------------------------------------- */
/*  MSG.C                    Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*               This is the high level message code.                   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/*  specialMessage()    saves a special message in curent room          */
/*  clearmsgbuf()   this clears the message buffer out                  */
/*  getMessage()    reads a message off disk into RAM.                  */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/*  putMessage()    stores a message to disk                            */
/*  noteMessage()   puts message in mesgBuf into message index          */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  06/02/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code.                                      */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/* -------------------------------------------------------------------- */
void aideMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = AIDEROOM;

    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf.room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  specialMessage()    saves a special message in curent room          */
/* -------------------------------------------------------------------- */
void specialMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = (uchar)thisRoom;
    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[thisRoom].lvisit)
        talleyBuf.room[thisRoom].new--;
}

/* -------------------------------------------------------------------- */
/*  clearmsgbuf()   this clears the message buffer out                  */
/* -------------------------------------------------------------------- */
void clearmsgbuf(void)
{
    /* clear msgBuf out */
    msgBuf->mbroomno    =   0 ;
    msgBuf->mbattr      =   0 ;
    msgBuf->mbauth[ 0]  = '\0';
    msgBuf->mbtitle[0]  = '\0';
    msgBuf->mbocont[0]  = '\0';
    msgBuf->mbfpath[0]  = '\0';
    msgBuf->mbtpath[0]  = '\0';
    msgBuf->mbczip[ 0]  = '\0';
    msgBuf->mbcopy[ 0]  = '\0';
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbgroup[0]  = '\0';
    msgBuf->mbtime[ 0]  = '\0';
    msgBuf->mbId[   0]  = '\0';
    msgBuf->mbsrcId[0]  = '\0';
    msgBuf->mboname[0]  = '\0';
    msgBuf->mboreg[ 0]  = '\0';
    msgBuf->mbreply[0]  = '\0';
    msgBuf->mbroom[ 0]  = '\0';
    msgBuf->mbto[   0]  = '\0';
    msgBuf->mbsur[  0]  = '\0';
    msgBuf->mblink[ 0]  = '\0';
    msgBuf->mbx[    0]  = '\0';
    msgBuf->mbzip[  0]  = '\0';
    msgBuf->mbrzip[ 0]  = '\0';
}

/* -------------------------------------------------------------------- */
/*  getMessage()    reads a message off disk into RAM.                  */
/* -------------------------------------------------------------------- */
void getMessage(void)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do
    {
        c = (uchar)getMsgChar();
    } while (c != -1);

    /* record exact position of start of message */
    msgBuf->mbheadLoc  = (long)(ftell(msgfl) - (long)1);

    /* get message's room #         */
    msgBuf->mbroomno   = (uchar)getMsgChar();

    /* get message's attribute byte */
    msgBuf->mbattr     = (uchar)getMsgChar();

    getMsgStr(msgBuf->mbId, LABELSIZE);

    do 
    {
        c = (char)getMsgChar();
        switch (c)
        {
        case 'A':     getMsgStr(msgBuf->mbauth,  LABELSIZE);    break;
        case 'C':     getMsgStr(msgBuf->mbcopy,  LABELSIZE);    break;
        case 'D':     getMsgStr(msgBuf->mbtime,  LABELSIZE);    break;
        case 'F':     getMsgStr(msgBuf->mbfwd,   LABELSIZE);    break;
        case 'G':     getMsgStr(msgBuf->mbgroup, LABELSIZE);    break;
        case 'I':     getMsgStr(msgBuf->mbreply, LABELSIZE);    break;
        case 'L':     getMsgStr(msgBuf->mblink,  64);           break;
        case 'M':     /* will be read off disk later */         break;
        case 'N':     getMsgStr(msgBuf->mbtitle, LABELSIZE);    break;
        case 'n':     getMsgStr(msgBuf->mbsur,   LABELSIZE);    break;
        case 'O':     getMsgStr(msgBuf->mboname, LABELSIZE);    break;
        case 'o':     getMsgStr(msgBuf->mboreg,  LABELSIZE);    break;
        case 'P':     getMsgStr(msgBuf->mbfpath, 128     );     break;
        case 'p':     getMsgStr(msgBuf->mbtpath, 128     );     break;
        case 'Q':     getMsgStr(msgBuf->mbocont, LABELSIZE);    break;
        case 'q':     getMsgStr(msgBuf->mbczip,  LABELSIZE);    break;
        case 'R':     getMsgStr(msgBuf->mbroom,  LABELSIZE);    break;
        case 'S':     getMsgStr(msgBuf->mbsrcId, LABELSIZE);    break;
        case 'T':     getMsgStr(msgBuf->mbto,    LABELSIZE);    break;
        case 'X':     getMsgStr(msgBuf->mbx,     LABELSIZE);    break;
        case 'Z':     getMsgStr(msgBuf->mbzip,   LABELSIZE);    break;
        case 'z':     getMsgStr(msgBuf->mbrzip,  LABELSIZE);    break;

        default:
            getMsgStr(msgBuf->mbtext, cfg.maxtext);  /* discard unknown field  */
            msgBuf->mbtext[0]    = '\0';
            break;
        }
    }
    while (c != 'M' && isalpha(c));
}

/* -------------------------------------------------------------------- */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/* -------------------------------------------------------------------- */
BOOL mAbort(void)
{
    char c, toReturn, oldEcho;

    /* Check for abort/pause from user */
    if (outFlag == IMPERVIOUS)  return FALSE;

    if (!BBSCharReady() && outFlag != OUTPAUSE)
    {
        /* Let modIn() report the problem */
        if (haveCarrier && !gotCarrier())  iChar(); 
        toReturn        = FALSE;
    } 
    else 
    {
        oldEcho  = echo;
        echo     = NEITHER;
        echoChar = 0;

        if (outFlag == OUTPAUSE)
        {
            outFlag = OUTOK;
            c = 'P';
        }
        else
        {
            c = (char)toupper(iChar());
        }

        switch (c)
        {
        case 19: /* XOFF */
        case 'P':                            /* pause:         */
            c = (char)toupper(iChar());             /* wait to resume */
            switch(c)
            {
               case 'J':                            /* jump paragraph:*/
                   outFlag     = OUTPARAGRAPH;
                   toReturn    = FALSE;
                   break;
               case 'K':                            /* kill:          */
                   if ( aide ||
                      (cfg.kill && 
                      (strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
                      && loggedIn))
                      dotoMessage = PULL_IT;
                   toReturn               = FALSE;
                   break;
               case 'M':                            /* mark:          */
                   if (aide)  dotoMessage = MARK_IT;
                   toReturn               = FALSE;
                   break;
               case 'N':                            /* next:          */
                   outFlag     = OUTNEXT;
                   toReturn    = TRUE;
                   break;
               case 'S':                            /* skip:          */
                   outFlag     = OUTSKIP;
                   toReturn    = TRUE;
                   break;
               case 'R':
                   dotoMessage = REVERSE_READ;
                   toReturn    = FALSE;
                   break;
               case 'C':
                   dotoMessage = COPY_IT;
                   toReturn    = FALSE;
                   break;
               case '~':
                   termCap(TERM_NORMAL);
                   ansiOn = (BOOL)(!ansiOn);
                   break;
               default:
                   toReturn    = FALSE;
                   break;
            }
            break;
        case 'C':
            dotoMessage = COPY_IT;
            toReturn    = FALSE;
            break;
        case 'J':                            /* jump paragraph:*/
            outFlag     = OUTPARAGRAPH;
            toReturn    = FALSE;
            break;
        case 'K':                            /* kill:          */
            if ( aide ||
               (cfg.kill && (strcmpi(logBuf.lbname, msgBuf->mbauth) == SAMESTRING)
               && loggedIn))
               dotoMessage = PULL_IT;
            toReturn               = FALSE;
            break;
        case 'M':                            /* mark:          */
            if (aide)  dotoMessage = MARK_IT;
            toReturn               = FALSE;
            break;
        case 'N':                            /* next:          */
            outFlag     = OUTNEXT;
            toReturn    = TRUE;
            break;
        case 'S':                            /* skip:          */
            outFlag     = OUTSKIP;
            toReturn    = TRUE;
            break;
        case 'R':
            dotoMessage = REVERSE_READ;
            toReturn    = FALSE;
            break;
        case '~':
            termCap(TERM_NORMAL);
            ansiOn = (BOOL)(!ansiOn);
            break;
        default:
            toReturn    = FALSE;
            break;
        }
        echo = oldEcho;
    }
    return toReturn;
}

/* -------------------------------------------------------------------- */
/*  putMessage()    stores a message to disk                            */
/* -------------------------------------------------------------------- */
BOOL putMessage(void)
{
    long timestamp;
    char stamp[20];

    time(&timestamp);

    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

    sprintf(stamp, "%ld", timestamp);

    /* record start of message to be noted */
    msgBuf->mbheadLoc = (long)cfg.catLoc;

    /* tell putMsgChar where to write   */
    fseek(msgfl, cfg.catLoc, 0);
 
    /* start-of-message              */
    overwrite(1);
    putMsgChar(0xFF);

    /* write room #                  */
    overwrite(1);
    putMsgChar(msgBuf->mbroomno);

    /* write attribute byte  */
    overwrite(1);
    putMsgChar(msgBuf->mbattr);  

    /* write message ID */
    dPrintf("%s", msgBuf->mbId);         

    /* Don't write out the author sometimes.. */
    if (!(loggedIn || strcmpi(msgBuf->mbauth, cfg.nodeTitle) == SAMESTRING))
    {
        *msgBuf->mbauth = NULL;
    }

    /* setup time/datestamp: */
    if (!msgBuf->mbcopy[0])
    {
        if(!*msgBuf->mbtime)
        {
            strcpy(msgBuf->mbtime, stamp);
        }
    }
    else
    {
        *msgBuf->mbtime = NULL;
    }


    /* write room name out:     */
    if (!*msgBuf->mboname)
    {
        if (!msgBuf->mbcopy[0]) 
        { 
            strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);
        }
        else
        {
            *msgBuf->mbroom = NULL;
        }
    }

    if (!msgBuf->mbcopy[0])  { dPrintf("A%s", msgBuf->mbauth);      }
    if (msgBuf->mbcopy[0])   { dPrintf("C%s", msgBuf->mbcopy);      }
    if (msgBuf->mbtime[0])   { dPrintf("D%s", msgBuf->mbtime);      }
    if (msgBuf->mbfwd[0])    { dPrintf("F%s", msgBuf->mbfwd);       }
    if (msgBuf->mbgroup[0])  { dPrintf("G%s", msgBuf->mbgroup);     }
    if (msgBuf->mbreply[0])  { dPrintf("I%s", msgBuf->mbreply);     }
    if (msgBuf->mblink[0])   { dPrintf("L%s", msgBuf->mblink);      }
    if (msgBuf->mbtitle[0])  { dPrintf("N%s", msgBuf->mbtitle);     }
    if (msgBuf->mbsur[0])    { dPrintf("n%s", msgBuf->mbsur);       }
    if (msgBuf->mboname[0])  { dPrintf("O%s", msgBuf->mboname);     }
    if (msgBuf->mboreg[0])   { dPrintf("o%s", msgBuf->mboreg);      }
    if (msgBuf->mbfpath[0])  { dPrintf("P%s", msgBuf->mbfpath);     }
    if (msgBuf->mbtpath[0])  { dPrintf("p%s", msgBuf->mbtpath);     }
    if (msgBuf->mbocont[0])  { dPrintf("Q%s", msgBuf->mbocont);     }
    if (msgBuf->mbczip[0])   { dPrintf("q%s", msgBuf->mbczip);      }
    if (msgBuf->mbroom[0])   { dPrintf("R%s", msgBuf->mbroom);      }
    if (msgBuf->mbsrcId[0])  { dPrintf("S%s", msgBuf->mbsrcId);     }
    if (msgBuf->mbto[0])     { dPrintf("T%s", msgBuf->mbto);        }
    if (msgBuf->mbx[0])      { dPrintf("X%s", msgBuf->mbx);         }
    if (msgBuf->mbzip[0])    { dPrintf("Z%s", msgBuf->mbzip);       }
    if (msgBuf->mbrzip[0])   { dPrintf("z%s", msgBuf->mbrzip);      }

    /* M-for-message. */
    overwrite(1);
    putMsgChar('M'); putMsgStr(msgBuf->mbtext);

    /* now finish writing */
    fflush(msgfl);

    /* record where to begin writing next message */
    cfg.catLoc = ftell(msgfl);

    talleyBuf.room[msgBuf->mbroomno].total++;

    if (mayseemsg()) 
    {
        talleyBuf.room[msgBuf->mbroomno].messages++;
        talleyBuf.room[msgBuf->mbroomno].new++;
    }

    return  TRUE;
}

/* -------------------------------------------------------------------- */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */
void noteMessage(void)
{
    ulong id,copy;
    int crunch = 0;
    int slot, copyslot;

    logBuf.lbvisit[0]   = ++cfg.newest;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* mush up any obliterated messages */
    if (cfg.mtoldest < cfg.oldest)
    {
        crunch = ((ushort)(cfg.oldest - cfg.mtoldest));
    }

    /* scroll index at #nmessages mark */
    if ( (ushort)(id - cfg.mtoldest) >= cfg.nmessages)
    {
        crunch++;
    }

    if (crunch)
    {
        crunchmsgTab(crunch);
    }

    /* now record message info in index */
    indexmessage(id);

    /* special for duplicated messages */
    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        copyslot = indexslot(copy);
        slot     = indexslot(id);

        if (copyslot != ERROR)
        {
            copyindex(slot, copyslot);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */
void indexmessage(ulong here)
{
    ushort slot;
    ulong copy;
    ulong oid;
    struct messagetable huge *msgSlt;
    
    slot = indexslot(here);

    msgSlt = &msgTab[slot];

    msgSlt->mtmsgLoc            = (long)0;

    msgSlt->mtmsgflags.MAIL     = 0;
    msgSlt->mtmsgflags.RECEIVED = 0;
    msgSlt->mtmsgflags.REPLY    = 0;
    msgSlt->mtmsgflags.PROBLEM  = 0;
    msgSlt->mtmsgflags.MADEVIS  = 0;
    msgSlt->mtmsgflags.LIMITED  = 0;
    msgSlt->mtmsgflags.MODERATED= 0;
    msgSlt->mtmsgflags.RELEASED = 0;
    msgSlt->mtmsgflags.COPY     = 0;
    msgSlt->mtmsgflags.NET      = 0;

    msgSlt->mtauthhash  = 0;
    msgSlt->mttohash    = 0;
    msgSlt->mtfwdhash   = 0;
    msgSlt->mtoffset    = 0;
    msgSlt->mtorigin    = 0;
    msgSlt->mtomesg     = (long)0;

    msgSlt->mtroomno    = DUMP;

    /* --- */
    
    msgSlt->mtmsgLoc    = msgBuf->mbheadLoc;

    if (*msgBuf->mbsrcId)
    {
        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        msgSlt->mtomesg = oid;
    }

    if (*msgBuf->mbauth)  msgSlt->mtauthhash =  hash(msgBuf->mbauth);

    if (*msgBuf->mbto)
    {
        msgSlt->mttohash   =  hash(msgBuf->mbto);    

        msgSlt->mtmsgflags.MAIL = 1;

        if (*msgBuf->mbfwd)  msgSlt->mtfwdhash = hash(msgBuf->mbfwd);
    }
    else if (*msgBuf->mbgroup)
    {
        msgSlt->mttohash   =  hash(msgBuf->mbgroup);
        msgSlt->mtmsgflags.LIMITED = 1;
    }

    if (*msgBuf->mboname)
      msgSlt->mtorigin = hash(msgBuf->mboname);

    if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING && *msgBuf->mbzip)
    {
        msgSlt->mtmsgflags.NET = 1;
        msgSlt->mttohash       = hash(msgBuf->mbzip);
    }

    if (*msgBuf->mbx)  msgSlt->mtmsgflags.PROBLEM = 1;

    msgSlt->mtmsgflags.RECEIVED = 
        ((msgBuf->mbattr & ATTR_RECEIVED) == ATTR_RECEIVED);

    msgSlt->mtmsgflags.REPLY    = 
        ((msgBuf->mbattr & ATTR_REPLY   ) == ATTR_REPLY   );

    msgSlt->mtmsgflags.MADEVIS  = 
        ((msgBuf->mbattr & ATTR_MADEVIS ) == ATTR_MADEVIS );

    msgSlt->mtroomno = msgBuf->mbroomno;

    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        msgSlt->mtmsgflags.COPY = 1;

        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        msgSlt->mtoffset = (ushort)(here - copy);
    }

    if (roomBuild) buildroom();
}

