/* -------------------------------------------------------------------- */
/*  NET.C                    Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <conio.h>
#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

#ifdef NETWORK
/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  GetStr()        gets a null-terminated string from a file           */
/*  PutStr()        puts a null-terminated string to a file             */
/*  GetMessage()    Gets a message from a file, returns sucess          */
/*  PutMessage()    Puts a message to a file                            */
/*  NewRoom()       Puts all new messages in a room to a file           */
/*  saveMessage()   saves a message to file if it is netable            */
/*  ReadMsgFile()   Reads a message file into thisRoom                  */
/*  NfindRoom()     find the room for main (unimplmented, ret: MAILROOM)*/
/*  readnode()      read the node.cit to get the nodes info for logbuf  */
/*  getnode()       read the node.cit to get the nodes info             */
/*  net_slave()     network entry point from LOGIN                      */
/*  net_master()    entry point to call a node                          */
/*  slave()         Actual networking slave                             */
/*  master()        During network master code                          */
/*  n_dial()        call the bbs in the node buffer                     */
/*  n_login()       Login to the bbs with the macro in the node file    */
/*  wait_for()      wait for a string to come from the modem            */
/*  net_callout()   Entry point from Cron.C                             */
/*  cleanup()       Done with other system, save mail and messages      */
/*  netcanseeroom() Can the node see this room?                         */
/*  alias()         return the name of the BBS from the #ALIAS          */
/*  route()         return the routing of a BBS from the #ROUTE         */
/*  alias_route()   returns the route or alias specified                */
/*  get_first_room()    get the first room in the room list             */
/*  get_next_room() gets the next room in the list                      */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  06/05/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code.                                      */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  GetStr()        gets a null-terminated string from a file           */
/* -------------------------------------------------------------------- */
void GetStr(FILE *fl, char *str, int mlen)
{
    int l;
    char ch;
  
    l=0; ch=1;
    while(!feof(fl) && ch)
    {
        ch=(uchar)fgetc(fl);
        if (ch != '\r' && ch != '\xFF' && l < mlen)
        {
            str[l]=ch;
            l++;
        }
    }
    str[l]='\0';
}

/* -------------------------------------------------------------------- */
/*  PutStr()        puts a null-terminated string to a file             */
/* -------------------------------------------------------------------- */
void PutStr(FILE *fl, char *str)
{
    fwrite(str, sizeof(char), (strlen(str) + 1), fl);
}

/* -------------------------------------------------------------------- */
/*  GetMessage()    Gets a message from a file, returns sucess          */
/* -------------------------------------------------------------------- */
BOOL GetMessage(FILE *fl)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do
    {
        c = (uchar)fgetc(fl);
    } while (c != -1 && !feof(fl));

    if (feof(fl))
        return FALSE;

    /* get message's attribute byte */
    msgBuf->mbattr = (uchar)fgetc(fl);

    GetStr(fl, msgBuf->mbId, LABELSIZE);

    do 
    {
        c = (uchar)fgetc(fl);
        switch (c)
        {
        case 'A':     GetStr(fl, msgBuf->mbauth,  LABELSIZE);    break;
        case 'D':     GetStr(fl, msgBuf->mbtime,  LABELSIZE);    break;
        case 'F':     GetStr(fl, msgBuf->mbfwd,   LABELSIZE);    break;
        case 'G':     GetStr(fl, msgBuf->mbgroup, LABELSIZE);    break;
        case 'I':     GetStr(fl, msgBuf->mbreply, LABELSIZE);    break;
        case 'M':     /* will be read off disk later */         break;
        case 'N':     GetStr(fl, msgBuf->mbtitle, LABELSIZE);    break;
        case 'n':     GetStr(fl, msgBuf->mbsur,   LABELSIZE);    break;
        case 'O':     GetStr(fl, msgBuf->mboname, LABELSIZE);    break;
        case 'o':     GetStr(fl, msgBuf->mboreg,  LABELSIZE);    break;
        case 'P':     GetStr(fl, msgBuf->mbfpath, 128     );    break;
        case 'p':     GetStr(fl, msgBuf->mbtpath, 128     );    break;
        case 'Q':     GetStr(fl, msgBuf->mbocont, LABELSIZE);    break;
        case 'q':     GetStr(fl, msgBuf->mbczip,  LABELSIZE);    break;
        case 'R':     GetStr(fl, msgBuf->mbroom,  LABELSIZE);    break;
        case 'S':     GetStr(fl, msgBuf->mbsrcId, LABELSIZE);    break;
        case 'T':     GetStr(fl, msgBuf->mbto,    LABELSIZE);    break;
/*      case 'X':     GetStr(fl, msgBuf->mbx,     LABELSIZE);    break; */
        case 'Z':     GetStr(fl, msgBuf->mbzip,   LABELSIZE);    break;
        case 'z':     GetStr(fl, msgBuf->mbrzip,  LABELSIZE);    break;

        default:
            GetStr(fl, msgBuf->mbtext, cfg.maxtext);  /* discard unknown field  */
            msgBuf->mbtext[0]    = '\0';
            break;
        }
    } while (c != 'M' && !feof(fl));

    if (!*msgBuf->mboname)
    {
        strcpy(msgBuf->mboname, node.ndname);
    }

    if (!*msgBuf->mboreg)
    {
        strcpy(msgBuf->mboreg, node.ndregion);
    }

    if (!*msgBuf->mbsrcId)
    {
        strcpy(msgBuf->mbsrcId, msgBuf->mbId);
    }

    /*
     * If the other node did not set up a from path, do it.
     */
    if (!*msgBuf->mbfpath)
    {
        if (strcmpi(msgBuf->mboname, node.ndname) == 0)
        {
            strcpy(msgBuf->mbfpath, msgBuf->mboname);
        }
        else
        {
            /* last node did not originate, make due with what we got... */
            strcpy(msgBuf->mbfpath, msgBuf->mboname);
            strcat(msgBuf->mbfpath, "!..!");
            strcat(msgBuf->mbfpath, node.ndname);
        }
    }

    if (feof(fl))
    {
        return FALSE;
    }

    GetStr(fl, msgBuf->mbtext, cfg.maxtext);  /* get the message field  */

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  PutMessage()    Puts a message to a file                            */
/* -------------------------------------------------------------------- */
void PutMessage(FILE *fl)
{
    /* write start of message */
    fputc(0xFF, fl);

    /* put message's attribute byte */
    msgBuf->mbattr = msgBuf->mbattr & (ATTR_RECEIVED|ATTR_REPLY);
    fputc(msgBuf->mbattr, fl);

    /* put local ID # out */
    PutStr(fl, msgBuf->mbId);

    if (!msgBuf->mboname[0])
    {   
        strcpy(msgBuf->mboname, cfg.nodeTitle);
        strcpy(msgBuf->mboreg,  cfg.nodeRegion);
        /*strcpy(msgBuf->mbocont, cfg.nodeRegion);*/
    }
    
    if (!msgBuf->mbsrcId[0])
    {
        strcpy(msgBuf->mbsrcId, msgBuf->mbId);
    }
    
    if (*msgBuf->mbfpath)
    {
        strcat(msgBuf->mbfpath, "!");
    }
    strcat(msgBuf->mbfpath, cfg.nodeTitle);

    if (!msgBuf->mbtime[0])
    {
        sprintf(msgBuf->mbtime, "%ld", time(NULL));
    }
    
    fputc('A', fl); PutStr(fl, msgBuf->mbauth);
    fputc('D', fl); PutStr(fl, msgBuf->mbtime);
    fputc('O', fl); PutStr(fl, msgBuf->mboname);
    fputc('o', fl); PutStr(fl, msgBuf->mboreg);
    fputc('S', fl); PutStr(fl, msgBuf->mbsrcId);
    fputc('P', fl); PutStr(fl, msgBuf->mbfpath);
    
    if (msgBuf->mbfwd[0])   { fputc('F', fl); PutStr(fl, msgBuf->mbfwd);   }
    if (msgBuf->mbgroup[0]) { fputc('G', fl); PutStr(fl, msgBuf->mbgroup); }
    if (msgBuf->mbreply[0]) { fputc('I', fl); PutStr(fl, msgBuf->mbreply); }
    if (msgBuf->mbtitle[0]) { fputc('N', fl); PutStr(fl, msgBuf->mbtitle); }
    if (msgBuf->mbsur[0])   { fputc('n', fl); PutStr(fl, msgBuf->mbsur);   }
    if (msgBuf->mbtpath[0]) { fputc('p', fl); PutStr(fl, msgBuf->mbtpath); }
    if (msgBuf->mbocont[0]) { fputc('Q', fl); PutStr(fl, msgBuf->mbocont); }
    if (msgBuf->mbczip[0])  { fputc('q', fl); PutStr(fl, msgBuf->mbczip);  }
    if (msgBuf->mbroom[0])  { fputc('R', fl); PutStr(fl, msgBuf->mbroom);  }
    if (msgBuf->mbto[0])    { fputc('T', fl); PutStr(fl, msgBuf->mbto);    }
    if (msgBuf->mbzip[0])   { fputc('Z', fl); PutStr(fl, msgBuf->mbzip);   }
    if (msgBuf->mbrzip[0])  { fputc('z', fl); PutStr(fl, msgBuf->mbrzip);  }

    /* put the message field  */
    fputc('M', fl); PutStr(fl, msgBuf->mbtext);
}

/* -------------------------------------------------------------------- */
/*  NewRoom()       Puts all new messages in a room to a file           */
/* -------------------------------------------------------------------- */
void NewRoom(int room, char *filename)
{
    int   i, h;
    char str[100];
    ulong lowLim, highLim, msgNo;
    FILE *file;

    lowLim  = logBuf.lbvisit[ logBuf.lbroom[room].lvisit ] + 1;
    highLim = cfg.newest;

    logBuf.lbroom[room].lvisit = 0;

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim)  lowLim = cfg.oldest;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "ab");
    if (!file)
    {
        return;
    }

    h = hash(cfg.nodeTitle);
    
    for (i = 0; i != sizetable(); i++)
    {
        msgNo = (ulong)(cfg.mtoldest + i);
        
        if ( msgNo >= lowLim && highLim >= msgNo )
        {
            /* skip messages not in this room */
            if (msgTab[i].mtroomno != (uchar)room) continue;
    
            /* no open messages from the system */
            if (msgTab[i].mtauthhash == h) continue;
    
            /* skip mail */
            if (msgTab[i].mtmsgflags.MAIL) continue;
    
            /* No problem user shit */
            if (
                (msgTab[i].mtmsgflags.PROBLEM || msgTab[i].mtmsgflags.MODERATED) 
            && !(msgTab[i].mtmsgflags.MADEVIS || msgTab[i].mtmsgflags.MADEVIS)
            && !sysop)
            { 
#ifdef DEBUGEM                
                if (debug) 
                {
                    cPrintf("\nPROBLEM USER MESSAGE NOT SAVED! (Problem = %d, Moderated = %d, Released = %d)\n",
                        msgTab[i].mtmsgflags.PROBLEM,
                        msgTab[i].mtmsgflags.MODERATED, 
                        msgTab[i].mtmsgflags.RELEASED);
                }
#endif                
                continue;
            }

            saveMessage( msgNo, file );
            mread ++;
        }
    }
    fclose(file);
}

/* -------------------------------------------------------------------- */
/*  saveMessage()   saves a message to file if it is netable            */
/* -------------------------------------------------------------------- */
#define msgstuff  msgTab[slot].mtmsgflags  
void saveMessage(ulong id, FILE *fl)
{
    ulong here;
    ulong loc;
    int   slot;

    slot = indexslot(id);
    
    if (slot == ERROR) return;

    if (msgTab[slot].mtmsgflags.COPY)
    {
        copyflag     = TRUE;
        originalId   = id;
        originalattr = 0;

        originalattr = (uchar)
                       (originalattr | (msgstuff.RECEIVED)?ATTR_RECEIVED :0 );
        originalattr = (uchar)
                       (originalattr | (msgstuff.REPLY   )?ATTR_REPLY : 0 );
        originalattr = (uchar)
                       (originalattr | (msgstuff.MADEVIS )?ATTR_MADEVIS : 0 );
                       
        if (msgTab[slot].mtoffset <= slot)
            saveMessage( (ulong)(id - (ulong)msgTab[slot].mtoffset), fl);

        return;
    }

    /* in case it returns without clearing buffer */
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbto[   0]  = '\0';

    loc = msgTab[slot].mtmsgLoc;
    if (loc == ERROR) return;

    if (copyflag)  slot = indexslot(originalId);

    if (!mayseeindexmsg(slot) && !msgTab[slot].mtmsgflags.NET) return;

    fseek(msgfl, loc, 0);

    getMessage();
    getMsgStr(msgBuf->mbtext, cfg.maxtext);

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if ((int)here == 1) return;

    if (!mayseemsg() && !msgTab[slot].mtmsgflags.NET) return;

    if (here != id )
    {
        cPrintf("Can't find message. Looking for %lu at byte %ld!\n ",
                 id, loc);
        return;
    }

    if (msgBuf->mblink[0]) return;

    PutMessage(fl);
}

/* -------------------------------------------------------------------- */
/*  ReadMsgFile()   Reads a message file into thisRoom                  */
/* -------------------------------------------------------------------- */
int ReadMsgFl(int room, char *filename, char *here, char *there)
{
    FILE *file;
    char str[100];
    ulong oid, loc;
    long l;
    int i, bad, oname, temproom, lp, goodmsg = 0;

    expired = 0;   duplicate = 0;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "rb");

    if (!file)
        return -1;

    while(GetMessage(file) == TRUE)
    {
        msgBuf->mbroomno = (uchar)room;

        sscanf(msgBuf->mbsrcId, "%ld", &oid);
        oname = hash(msgBuf->mboname);

        memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );

        bad = FALSE;

        if (strcmpi(cfg.nodeTitle, msgBuf->mboname) == SAMESTRING)
        { 
            bad = TRUE; 
            duplicate++; 
        }

        if (*msgBuf->mbzip) /* is mail */
        {
            /* not for this system */
            if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING)
            {
                if (!save_mail())
                {
                    clearmsgbuf();
                    strcpy(msgBuf->mbauth, cfg.nodeTitle);
                    strcpy(msgBuf->mbto,   msgBuf2->mbauth);
                    strcpy(msgBuf->mbzip,  msgBuf2->mboname);
                    strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
                    strcpy(msgBuf->mbroom, msgBuf2->mbroom);
                    sprintf(msgBuf->mbtext, 
                           " \n Can not find route to '%s'.", msgBuf2->mbzip);
                    amPrintf( 
                        " Can not find route to '%s' in message from '%s'.\n",
                        msgBuf2->mbzip, msgBuf2->mboname);
                    netError = TRUE;
                
                    save_mail();
                }
                bad = TRUE;
            } else {
            /* for this system */
                if (*msgBuf->mbto && personexists(msgBuf->mbto) == ERROR
                    && strcmpi(msgBuf->mbto, "Sysop") != SAMESTRING)
                {
                    clearmsgbuf();
                    strcpy(msgBuf->mbauth, cfg.nodeTitle);
                    strcpy(msgBuf->mbto,   msgBuf2->mbauth);
                    strcpy(msgBuf->mbzip,  msgBuf2->mboname);
                    strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
                    strcpy(msgBuf->mbroom, msgBuf2->mbroom);
                    sprintf(msgBuf->mbtext, 
                        " \n No '%s' user found on %s.", msgBuf2->mbto,
                        cfg.nodeTitle);
                    save_mail();
                    bad = TRUE;
                }
            }
        } else {
            /* is public */
            if (!bad)
            {
                for (i = sizetable(); i != -1 && !bad; i--)
                {
                    if (msgTab[i].mtorigin == oname
                       && oid == msgTab[i].mtomesg)
                    {
                        loc = msgTab[i].mtmsgLoc;
                        fseek(msgfl, loc, 0);
                        getMessage();
                        if (strcmpi(msgBuf->mbauth, msgBuf2->mbauth)   
                                                                == SAMESTRING
                         && strcmpi(msgBuf->mboname, msgBuf2->mboname)
                                                                == SAMESTRING
                         && strcmpi(msgBuf->mbtime, msgBuf2->mbtime)  
                                                                == SAMESTRING
                       /*&& strcmpi(msgBuf->mboreg, msgBuf2->mboreg)  
                                                                == SAMESTRING*/
                       /* Changed beacuse of region name changes */
                           )
                        {
                            bad = TRUE; 
                            duplicate++; 
                        }
                    }
                }
            }

            memcpy( msgBuf, msgBuf2, sizeof(struct msgB) );
    
            /* fix group only messages, or discard them! */
            if (*msgBuf->mbgroup && !bad)
            {
                bad = TRUE;
                for (i=0; node.ndgroups[i].here[0]; i++)
                {
                    if (strcmpi(node.ndgroups[i].there, msgBuf->mbgroup) 
                        == SAMESTRING)
                    {
                        strcpy(msgBuf->mbgroup, node.ndgroups[i].here);
                        bad = FALSE;
                    }
                }
                /* put it in RESERVED_2 */
                if (bad)
                {
                    bad = FALSE;
                    strcpy(msgBuf->mbgroup, grpBuf.group[1].groupname);
                }
            }
    
            /* Expired? */
            if ( atol(msgBuf2->mbtime) 
                < (time(&l) - ((long)node.ndexpire *60*60*24)) ) 
            {
                bad = TRUE;
                expired++;
            }
        }

        if (!bad)
        { /* its good, save it */
            temproom = room;

            if (strcmpi(msgBuf->mbroom, there) == SAMESTRING)
                strcpy(msgBuf->mbroom, here);

            if (*msgBuf->mbto)
                temproom = NfindRoom(msgBuf->mbroom);

            msgBuf->mbroomno = (uchar)temproom;

            putMessage();
            noteMessage();
            goodmsg++;

            if (*msgBuf->mbto)
            {
                lp = thisRoom;
                thisRoom = temproom;
                notelogmessage(msgBuf->mbto);
                thisRoom = lp;
            }
        }
    }
    fclose(file);

    return goodmsg;
}

/* -------------------------------------------------------------------- */
/*  NfindRoom()     find the room for main (unimplmented, ret: MAILROOM)*/
/* -------------------------------------------------------------------- */
int NfindRoom(char *str)
{
    int i = MAILROOM; 

    str[0] = str[0]; /* -W3 */

/*  i = roomExists(str);

    if (i == ERROR)
        i = MAILROOM;  */

    return(i);
}

/* -------------------------------------------------------------------- */
/*  readnode()      read the node.cit to get the nodes info for logbuf  */
/* -------------------------------------------------------------------- */
BOOL readnode(void)
{
    return getnode(logBuf.lbname);
}

/* -------------------------------------------------------------------- */
/*  getnode()       read the node.cit to get the nodes info             */
/* -------------------------------------------------------------------- */
BOOL getnode(char *nodename)
{                          
    FILE *fBuf;
    char line[90], ltmp[90];
    char *words[256];
    int  i, j, k, found = FALSE;
    long pos;
    char path[80];

    memset(&node, 0, sizeof(struct nodest));
    
    sprintf(path, "%s\\nodes.cit", cfg.homepath);

    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find nodes.cit!"); doccr();
        return FALSE;
    }

    pos = ftell(fBuf);
    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')
        {
            pos = ftell(fBuf);
            continue;
        }

        if (!found && strnicmp(line, "#NODE", 5) != SAMESTRING)
        {
            pos = ftell(fBuf);
            continue;
        }
        
        strcpy(ltmp, line);
        parse_it( words, line);

        for (i = 0; nodekeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], nodekeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        if (i == NOK_NODE)
        {
            if (found)
            {
                fclose(fBuf);
                return TRUE;
            }

            if (strcmpi(nodename,  words[1]) == SAMESTRING)
            {
                found = TRUE;  
            }
            else
            {
                continue;
            }
        }     

        switch(i)
        {
        case NOK_BAUD:
            j = atoi(words[1]);
            switch(j) /* ycky hack */
            {
            case 300:
                node.ndbaud = 0;
                break;
            case 1200:
                node.ndbaud = 1;
                break;
            case 2400:
                node.ndbaud = 2;
                break;
            case 4800:
                node.ndbaud = 3;
                break;
            case 9600:
                node.ndbaud = 4;
                break;
            default:
                node.ndbaud = 1;
                break;
            }
            break;

        case NOK_PHONE:
            if (strlen(words[1]) < 20)
                strcpy(node.ndphone, words[1]);
            break;

        case NOK_PROTOCOL:
            if (strlen(words[1]) < 20)
                strcpy(node.ndprotocol, words[1]);
            break;

        case NOK_MAIL_TMP:
            if (strlen(words[1]) < 20)
                strcpy(node.ndmailtmp, words[1]);
            break;

        case NOK_LOGIN:
            strcpy(node.ndlogin, ltmp);
            break;

        case NOK_NODE:
            if (strlen(words[1]) < 20)
                strcpy(node.ndname, words[1]);
            if (strlen(words[2]) < 20)
                strcpy(node.ndregion, words[2]);
            for (j=0; j<MAXGROUPS; j++)
                node.ndgroups[j].here[0] = '\0';
            node.roomoff = 0L;
            break;

        case NOK_REDIAL:
            node.ndredial = atoi(words[1]);
            break;

        case NOK_DIAL_TIMEOUT:
            node.nddialto = atoi(words[1]);
            break;

        case NOK_WAIT_TIMEOUT:
            node.ndwaitto = atoi(words[1]);
            break;

        case NOK_EXPIRE:
            node.ndexpire = atoi(words[1]);
            break;

        case NOK_ROOM:
            node.roomoff = pos;
            fclose(fBuf);
            return TRUE;

        case NOK_GROUP:
            for (j = 0, k = ERROR; j < MAXGROUPS; j++)
            {
                if (node.ndgroups[j].here[0] == '\0') 
                {
                    k = j;
                    j = MAXGROUPS;
                }
            } 

            if (k == ERROR) 
            {
                cPrintf("To many groups!!\n ");
               break;
            }
            
            if (strlen(words[1]) < 20)
                strcpy(node.ndgroups[k].here,  words[1]);
            if (strlen(words[2]) < 20)
                strcpy(node.ndgroups[k].there, words[2]);
            if (!strlen(words[2]))
                strcpy(node.ndgroups[k].there, words[1]);
            break;

        default:
            cPrintf("Nodes.cit - Warning: Unknown variable %s", words[0]);
            doccr();
            break;
        }
        pos = ftell(fBuf);
    }
    fclose(fBuf);
    return (BOOL)(found);
}

/* -------------------------------------------------------------------- */
/*  net_slave()     network entry point from LOGIN                      */
/* -------------------------------------------------------------------- */
BOOL net_slave(void)
{
    if (readnode() == FALSE) return FALSE;

    if (onConsole) 
    {
        if (debug)
        {
          cPrintf("Node:  \"%s\" \"%s\"", node.ndname, node.ndregion);  doccr();
          cPrintf("Phone: \"%s\" %d", node.ndphone, node.nddialto);     doccr();
          cPrintf("Login: \"%s\" %d", node.ndlogin, node.ndwaitto);     doccr();
          cPrintf("Baud:  %d    Protocol: \"%s\"\n ", node.ndbaud, node.ndprotocol);
          cPrintf("Expire:%d    Waitout:  %d", node.ndexpire, node.ndwaitto); doccr();
        }
    }
    else
    {
        netError = FALSE;
        
        /* cleanup */
        changedir(cfg.temppath);
        ambigUnlink("room.*",   FALSE);
        ambigUnlink("roomin.*", FALSE);
        
        slave();
        if (master())
        {
            cleanup();
            did_net(node.ndname);
        }
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  net_master()    entry point to call a node                          */
/* -------------------------------------------------------------------- */
BOOL net_master(void)
{
    if (readnode() == FALSE)
    {
        cPrintf("\n No node.cit entry!\n ");
        return FALSE;
    }
  
    if (n_dial()  == FALSE) return FALSE;
    if (n_login() == FALSE) return FALSE;
    netError = FALSE;

    /* cleanup */
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);

    if (master()  == FALSE) return FALSE;
    if (slave()   == FALSE) return FALSE;
    cleanup();

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  slave()         Actual networking slave                             */
/* -------------------------------------------------------------------- */
BOOL slave(void)
{
    char line[75];
    label troo, fn;
    FILE *file, *fopen();
    int i = 0, rm;
    
    cPrintf(" Sending mail file.");
    doccr();

    sprintf(line, "%s\\%s", cfg.homepath, node.ndmailtmp);
    wxsnd(cfg.temppath, line, 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
  
    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving room request file.");
    doccr();
    
    wxrcv(cfg.temppath, "roomreq.tmp", 
            (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    if (!gotCarrier()) return FALSE;
    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    file = fopen(line, "rb");
    if (!file)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    doccr();
    cPrintf(" Fetching:");
    doccr();

    GetStr(file, troo, 30);
    while(strlen(troo) && !feof(file))
    {
        if ((rm = roomExists(troo)) != ERROR)
        {
            if (netcanseeroom(rm))
            {
                sprintf(fn, "room.%d", i);
                cPrintf(" %-20s  ", troo);
                if( !((i+1) % 3) ) doccr();
                NewRoom(rm, fn);
            }
            else
            {
                doccr();
                cPrintf(" No access to %s room.", troo);
                doccr();
                amPrintf(" '%s' room not avalible to remote.\n", troo);
                netError = TRUE;
            }
        }else{
            doccr();
            cPrintf(" No %s room on system.", troo);
            doccr();
            amPrintf(" '%s' room not found for remote.\n", troo);
            netError = TRUE;
        }

        i++;
        GetStr(file, troo, 30);
    }
    doccr();
    fclose(file);
    unlink(line);

    cPrintf(" Sending message files.");
    doccr();
  
    if (!gotCarrier()) return FALSE;
    wxsnd(cfg.temppath, "room.*",
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    ambigUnlink("room.*",   FALSE);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  master()        During network master code                          */
/* -------------------------------------------------------------------- */
BOOL master(void)
{
    char line[75], line2[75];
    label here, there;
    FILE *file, *fopen();
    int i, rms;
    time_t t;

    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\mesg.tmp", cfg.temppath);
    unlink(line);

    cPrintf(" Receiving mail file.");
    doccr();

    wxrcv(cfg.temppath, "mesg.tmp", 
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    unlink(line);

    if((file = fopen(line, "ab")) == NULL)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    for (i=get_first_room(here, there), rms=0;
         i;
         i=get_next_room(here, there), rms++)
    {
        PutStr(file, there);
    }

    PutStr(file, "");
    fclose(file);

    cPrintf(" Sending room request file.");
    doccr();

    wxsnd(cfg.temppath, "roomreq.tmp", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    unlink(line);

    if (!gotCarrier()) return FALSE;

    /* clear the buffer */
    while (gotCarrier() && MIReady())
    {
        getMod();
    }

    cPrintf(" Waiting for transfer.");
    /* wait for them to get thier shit together */
    t = time(NULL);
    while (gotCarrier() && !MIReady())
    {
        if (time(NULL) > (t + (15 * 60))) /* only wait 15 minutes */
        {
            drop_dtr();
        }
    }
    doccr();

    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving message files.");
    doccr();

    wxrcv(cfg.temppath, "", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    for (i=0; i<rms; i++)
    {
        sprintf(line,  "room.%d",   i);
        sprintf(line2, "roomin.%d", i);
        rename(line, line2);
    }

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  n_dial()        call the bbs in the node buffer                     */
/* -------------------------------------------------------------------- */
BOOL n_dial(void)
{
    long ts;
    char str[40];
    char ch;

    cPrintf("\n \n Dialing...");

    if (debug) cPrintf("(%s%s)", cfg.dialpref, node.ndphone);
    
    baud(node.ndbaud);
    update25();

    outstring(cfg.dialsetup);
    outstring("\r");

    pause(100);
  
    strcpy(str, cfg.dialpref);
    strcat(str, node.ndphone);
    outstring(str);
    outstring("\r");

    time(&ts);
  
    while(TRUE)
    {
        if ((int)(time(NULL) - ts) > node.nddialto)  /* Timeout */
            break;

        if (KBReady())                             /* Aborted by user */
        {
            getch();
            getkey = 0;
            break;
        }

        if (gotCarrier())                          /* got carrier!  */ 
        {
            cPrintf("success");
            return TRUE;
        }
    } 

    while (MIReady())
    {
        ch = (char)getMod();
        if (debug) outCon(ch);
    }

    cPrintf("failed");

    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  n_login()       Login to the bbs with the macro in the node file    */
/* -------------------------------------------------------------------- */
BOOL n_login(void)
{
    union REGS in, out;
    register int ptime, j=0, k;
    char ch;
    int i, count;
    char *words[256];

    cPrintf("\n Logging in...");

    count = parse_it( words, node.ndlogin);

    i = 1;

    while(i <= count)
    {
        switch(tolower(*words[i++]))
        {     
            case 'p':
                if (debug) cPrintf("Pause For (%s)", words[i]);
                ptime=atoi(words[i++]);
                in.h.ah=0x2C;
                intdos(&in, &out);
                k = out.h.dl/10;
                while(j < ptime)
                {
                    in.h.ah=0x2C;
                    intdos(&in, &out);
                    if(out.h.dl/10 < (uchar)k)
                        j += (10+(out.h.dl/10))-k;
                    else
                        j += (out.h.dl/10)-k;
                    k = out.h.dl/10;
                    if (MIReady())
                    {
                        ch = (char)getMod();
                        if (debug) outCon(ch);
                    }
                }
                break;
            case 's':
                outstring(words[i++]);
                break;
            case 'w':
                if (debug) cPrintf("Wait For (%s)", words[i]);
                if (!wait_for(words[i++]))
                {
                    cPrintf("failed");
                    return FALSE;
                }
                break;
            case '!':
                apsystem(words[i++]);
                break;
            default:
                break;
        }
    }
    cPrintf("success");
    doccr();
    doccr();
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  wait_for()      wait for a string to come from the modem            */
/* -------------------------------------------------------------------- */
BOOL wait_for(char *str)
{
    char line[80];
    long st;
    int i, stl;
   
    stl = strlen(str);

    for (i=0; i<stl; i++) 
        line[i] = '\0';

    time(&st);
   
    while(TRUE)
    {
        if (MIReady())
        {
            memcpy(line, line+1, stl);
            line[stl-1]  = (char) getMod();
            line[stl] = '\0';
            if (debug) outCon(line[stl-1]);
            if (strcmpi(line, str) == SAMESTRING) 
                return TRUE;
        }else{
            if ((time(NULL) - st) > (long)node.ndwaitto)
                return FALSE;
            if (KBReady())                             /* Aborted by user */
            {
                getch();
                getkey = 0;
                return FALSE;
            }
        }
    }   
    return FALSE;
}   

/* -------------------------------------------------------------------- */
/*  net_callout()   Entry point from Cron.C                             */
/* -------------------------------------------------------------------- */
BOOL net_callout(char *node)
{
    int slot;
    int tmp;

    /* login user */

    mread = 0; entered = 0;

    slot = personexists(node);

    if (slot == ERROR)
    {
        cPrintf("\n No such node in userlog!");
        return FALSE;
    }

    getLog(&logBuf, logTab[slot].ltlogSlot);

    thisSlot = slot;
    thisLog = logTab[slot].ltlogSlot;
 
    loggedIn    = TRUE;
    setsysconfig();
    setgroupgen();
    setroomgen();
    setlbvisit();

    update25();

    sprintf( msgBuf->mbtext, "NetCallout %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_NETWORK);

    /* node logged in */
     
    tmp = net_master();

    /* terminate user */

    if (tmp == TRUE)
    { 
        logBuf.callno      = cfg.callno;
        time(&logtimestamp);
        logBuf.calltime    = logtimestamp;
        logBuf.lbvisit[0]  = cfg.newest;
        logTab[0].ltcallno = cfg.callno;

        slideLTab(thisSlot);
        cfg.callno++;

        storeLog();
        loggedIn = FALSE;

        /* trap it */
        sprintf(msgBuf->mbtext, "NetLogout %s (succeded)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);

        outFlag = IMPERVIOUS;
        cPrintf("Networked with \"%s\"\n ", logBuf.lbname);

        if (cfg.accounting)  unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        logBuf.lbname[0] = 0;

        setalloldrooms();
    }
    else
    {
        loggedIn = FALSE;

        sprintf(msgBuf->mbtext, "NetLogout %s (failed)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);
    }

    setdefaultconfig();

    /* user terminated */
    onConsole       = FALSE;
    callout         = FALSE;

    pause(100);
   
    Initport();

    return (BOOL)(tmp);
}

/* -------------------------------------------------------------------- */
/*  cleanup()       Done with other system, save mail and messages      */
/* -------------------------------------------------------------------- */
void cleanup(void)
{
    int t, i, rm, err;
    int new=0, exp=0, dup=0, rms=0;
    label fn, here, there;
    char line[100];
  
    sprintf(line, "%s\\%s", cfg.homepath, node.ndmailtmp);
    unlink(line);

    drop_dtr();

    outFlag = IMPERVIOUS;

    doccr();
    cPrintf(" Incorporating:");
    doccr();
    cPrintf("                 Room:  New: Expired: Duplicate:");
    doccr();/* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();
    for (t=get_first_room(here, there), i=0;
         t;
         t=get_next_room(here, there), i++)
    {
        sprintf(fn, "roomin.%d", i);

        rm = roomExists(here);
        if (rm != ERROR)
        {
            cPrintf(" %20.20s  ", here);
            err = ReadMsgFl(rm, fn, here, there);
            if (err != ERROR)
            {
                cPrintf("  %2d     %2d     %2d", err, expired, duplicate);
                new += err;
                exp += expired;
                dup += duplicate;
                rms ++;
            }else{
                amPrintf(" No '%s' room on remote\n", there);
                netError = TRUE;
                cPrintf(" Room not found on other system.");
            }
            doccr();
        }else{
            cPrintf(" %20.20s   Room not found on local system.", here);
            amPrintf(" No '%s' room local.\n", here);
            netError = TRUE;
            doccr();
        }

        unlink(fn);
    }
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");
    doccr();
    cPrintf("Totals:            %2d    %2d     %2d     %2d", rms, new, exp, dup);
           /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/
    doccr();
    doccr();
    cPrintf("Incorporating MAIL");
    i =  ReadMsgFl(MAILROOM, "mesg.tmp", "", "");
    cPrintf(" %d new message(s)", i == ERROR ? 0 : i);
    doccr();
  
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);
    changedir(cfg.homepath);
  
    xpd = exp;
    duplic = dup;
    entered = new;
  
    if (netError)
    {
        amPrintf(" \n Netting with '%s'\n", logBuf.lbname );
        SaveAideMess();
    }
}

/* -------------------------------------------------------------------- */
/*  netcanseeroom() Can the node see this room?                         */
/* -------------------------------------------------------------------- */
BOOL netcanseeroom(int roomslot)
{ 
        /* is room in use              */
    if ( roomTab[roomslot].rtflags.INUSE

        /* and it is shared            */
        && roomTab[roomslot].rtflags.SHARED 

        /* and group can see this room */
        && (groupseesroom(roomslot)
        || roomTab[roomslot].rtflags.READONLY
        || roomTab[roomslot].rtflags.DOWNONLY )       

        /* only aides go to aide room  */ 
        &&   ( roomslot != AIDEROOM || aide) )
    {
        return TRUE;
    }

    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  alias()         return the name of the BBS from the #ALIAS          */
/* -------------------------------------------------------------------- */
BOOL alias(char *str)
{
    return alias_route(str, "#ALIAS");
}

/* -------------------------------------------------------------------- */
/*  route()         return the routing of a BBS from the #ROUTE         */
/* -------------------------------------------------------------------- */
BOOL route(char *str)
{
    return alias_route(str, "#ROUTE");
}

/* -------------------------------------------------------------------- */
/*  alias_route()   returns the route or alias specified                */
/* -------------------------------------------------------------------- */
BOOL alias_route(char *str, char *srch)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    char path[80];

    sprintf(path, "%s\\route.cit", cfg.homepath);
    
    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find route.cit!");
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#') continue;
   
        if (strnicmp(line, srch, 5) != SAMESTRING) continue;
     
        parse_it( words, line);

        if (strcmpi(srch, words[0]) == SAMESTRING)
        {
            if (strcmpi(str, words[1]) == SAMESTRING)
            {
                fclose(fBuf);
                strcpy(str, words[2]);
                return TRUE;
            }
        }
    }
    fclose(fBuf);
    return FALSE;
}

/* ------------------------------------------------------------------------ */
/*  the folowing two rutines are used for scaning through the rooms         */
/*  requested from a node                                                   */
/* ------------------------------------------------------------------------ */
FILE *nodefile;

/* -------------------------------------------------------------------- */
/*  get_first_room()    get the first room in the room list             */
/* -------------------------------------------------------------------- */
BOOL get_first_room(char *here, char *there)
{
    if (!node.roomoff) return FALSE;

    /* move to home-path */
    changedir(cfg.homepath);

    if ((nodefile = fopen("nodes.cit", "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find nodes.cit!");
    }

    changedir(cfg.temppath);

    fseek(nodefile, node.roomoff, SEEK_SET);

    return get_next_room(here, there);
}

/* -------------------------------------------------------------------- */
/*  get_next_room() gets the next room in the list                      */
/* -------------------------------------------------------------------- */
BOOL get_next_room(char *here, char *there)
{
    char  line[90];
    char *words[256];
   
    while (fgets(line, 90, nodefile) != NULL)
    {
        if (line[0] != '#')  continue;

        parse_it( words, line);

        if (strcmpi(words[0], "#NODE") == SAMESTRING)
        {
            fclose(nodefile);
            return FALSE;
        }

        if (strcmpi(words[0], "#ROOM") == SAMESTRING)
        {
            strcpy(here,  words[1]);
            strcpy(there, words[2]);
            return TRUE;
        }
    }
    fclose(nodefile);
    return FALSE;
}  

/* -------------------------------------------------------------------- */
/*  save_mail()     save a message bound for another system             */
/* -------------------------------------------------------------------- */
BOOL save_mail()
{
    label tosystem;
    char  filename[100];
    FILE *fl;

    /* where are we sending it? */
    strcpy(tosystem, msgBuf->mbzip);

    /* send it vila... */
    route(tosystem);

    /* get the node entery */
    if (!getnode(tosystem))
        return FALSE;
  
    sprintf(filename, "%s\\%s", cfg.homepath, node.ndmailtmp);

    fl = fopen(filename, "ab");
    if (!fl) return FALSE;

    PutMessage(fl);

    fclose(fl);

    return TRUE;
}
#endif

