/************************************************************************/
/*                              ctdl.c                                  */
/*              Command-interpreter code for Citadel                    */
/************************************************************************/

#define MAIN

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

unsigned _stklen = 1024*12;          /* set up our stackspace */

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      doAide()                handles Aide-only       commands        */
/*      doChat()                handles C(hat)          command         */
/*      doEnter()               handles E(nter)         command         */
/*      doGoto()                handles G(oto)          command         */
/*      doHelp()                handles H(elp)          command         */
/*      doIntro()               handles I(ntro)         command         */
/*      doKnown()               handles K(nown rooms)   command         */
/*      doLogin()               handles L(ogin)         command         */
/*      doLogout()              handles T(erminate)     command         */
/*      doRead()                handles R(ead)          command         */
/*      doRegular()             fanout for above commands               */
/*      doSysop()               handles sysop-only      commands        */
/*      doNext()                handles '+' next room                   */
/*      doPrevious()            handles '-' previous room               */
/*      doNextHall()            handles '>' next room                   */
/*      doPreviousHall()        handles '<' previous room               */
/*      getCommand()            prints prompt and gets command char     */
/*      greeting()              System-entry blurb etc                  */
/*      main()                  has the central menu code               */
/************************************************************************/

/************************************************************************/
/*              External function definitions for CTDL.C                */
/************************************************************************/
static  void doAide(char moreYet,char first);
static  void doDownload(char ex);
static  void doUpload(char ex);
static  void doChat(char moreYet,char first);
static  void doEnter(char moreYet,char first);
static  void exclude(void );
static  void doGoto(char expand,int skip);
static  void doHelp(char expand);
static  void doIntro(void );
static  void doKnown(char moreYet,char first);
static  void doLogin(char moreYet);
static  void doLogout(char expand,char first);
static  void doRead(char moreYet,char first);
static  void doXpert(void );
static  char doRegular(char x,char c);
static  char doSysop(void );
static  void do_SysopGroup(void );
static  void do_SysopHall(void );
static  void doNext(void );
static  void doPrevious(void );
static  void doNextHall(void );
static  void doPreviousHall(void );
static  char getCommand(char *c);
static  void greeting(void );
extern  int main(int argc,char **argv);

/************************************************************************/
/*      doAide() handles the aide-only menu                             */
/*          return FALSE to fall invisibly into default error msg       */
/************************************************************************/
void doAide(moreYet, first)
char moreYet;
char first;     /* first parameter if TRUE              */
{
    int  roomExists();
    char oldchat;

    if (moreYet)   first = '\0';

    mPrintf("\bAide special fn: ");

    if (first)     oChar(first);

    switch (toupper( first ? (char)first : (char)iChar() ))
    {
    case 'A':
        mPrintf("\bAttributes ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else attributes();
        break;

    case 'C':
        chatReq = TRUE;
        oldchat = (char)cfg.noChat;
        cfg.noChat = FALSE;
        mPrintf("\bChat\n ");
        if (whichIO == MODEM)   ringSysop();
        else                    chat() ;
        cfg.noChat = oldchat;
        break;

    case 'E':
        mPrintf("\bEdit room\n  \n");
        renameRoom();
        break;

    case 'F':
        mPrintf("\bFile set\n  \n");
        batchinfo(TRUE);
        break;
    
    case 'G':
        mPrintf("\bGroup membership\n  \n");
        groupfunc();
        break;

    case 'H':
        mPrintf("\bHallway changes\n  \n");
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            hallfunc();
        }
        break;

    case 'I':
        mPrintf("\bInsert %s\n ", cfg.msg_nym);
        insert();
        break;

    case 'K':
        mPrintf("\bKill room\n ");
        killroom();
        break;

    case 'L':
        mPrintf("\bList group ");
        listgroup();
        break;

    case 'M':
        mPrintf("\bMove file ");
        moveFile();
        break;

    case 'N':
        mPrintf("\bName Messages");
        msgNym();
        break;

    case 'R':
        mPrintf("\bRename file ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            renamefile();
        }
        break;

    case 'S':
        mPrintf("\bSet file info\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            setfileinfo();
        }
        break;

    case 'U':
        mPrintf("\bUnlink file\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            unlinkfile();
        }
        break;

    case 'V':
        mPrintf("\bView Help Text File\n ");
        tutorial("aide.hlp");
        break;

    case 'W':
        mPrintf("\bWindow into hallway\n ");
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            windowfunc();
        }
        break;

    case '?':
        tutorial("aide.mnu");
        break;

    default:
        if (!expert)   mPrintf("\n '?' for menu.\n " );
        else           mPrintf(" ?\n "               );
        break;
    }
}

/************************************************************************/
/*      doDownload()                                                    */
/************************************************************************/
void doDownload(char ex)
{
    ex = ex;

    mPrintf("\bDownload ");

    if  (!loggedIn && !cfg.unlogReadOk) 
    {
        mPrintf("\n --Must log in to download.\n ");
        return;
    }

    /* handle uponly flag! */
    if ( roomTab[thisRoom].rtflags.UPONLY && !groupseesroom(thisRoom) )
    {
        mPrintf("\n --Room is upload only.\n ");
        return;
    }

    if ( !roomBuf.rbflags.MSDOSDIR )
    {
        if (expert) mPrintf("? ");
        else        mPrintf("\n Not a directory room.");
        return;
    }
    download('\0');
}

/************************************************************************/
/*      doUpload()                                                      */
/************************************************************************/
void doUpload(char ex)
{
    ex = ex;

    mPrintf("\bUpload ");

    /* handle downonly flag! */
    if ( roomTab[thisRoom].rtflags.DOWNONLY && !groupseesroom(thisRoom))
    {
        mPrintf("\n\n  --Room is download only.\n ");
        return;
    }

    if ( !loggedIn && !cfg.unlogEnterOk )
    {
        mPrintf("\n\n  --Must log in to upload.\n ");
        return;
    }

    if ( !roomBuf.rbflags.MSDOSDIR )
    {
        if (expert) mPrintf("? ");
        else        mPrintf("\n Not a directory room.");
        return;
    }
    upload('\0');
    return;
}

/************************************************************************/
/*      doChat()                                                        */
/************************************************************************/
void doChat(moreYet, first)
char moreYet;   /* TRUE to accept following parameters  */
char first;     /* first parameter if TRUE              */
{
    moreYet = moreYet;  /* -W3 */
    first   = first;    /* -W3 */
    
    chatReq = TRUE;
    
    mPrintf("\bChat ");

    trap("Chat request", T_CHAT);

    if (cfg.noChat)
    {
        tutorial("nochat.blb");
        return;
    }

    if (whichIO == MODEM)  ringSysop();
    else                   chat() ;
}


/***********************************************************************/
/*     doEnter() handles E(nter) command                               */
/***********************************************************************/
void doEnter(moreYet,first)
char moreYet;           /* TRUE to accept following parameters */
char first;             /* first parameter if true             */
{
    char done;
    char letter;

    if (moreYet)  first = '\0';

    done      = TRUE ;
    mailFlag  = FALSE;
    oldFlag   = FALSE;
    limitFlag = FALSE;
    linkMess  = FALSE;

    mPrintf("\bEnter ");

    if (first)  oChar(first);

    do  
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

        letter = (char)(toupper( first ? (char)first : (char)iChar()));

        /* allow exclusive mail entry only */
        if ( !loggedIn && !cfg.unlogEnterOk && (letter != 'E') )
        {
            mPrintf("\b\n  --Must log in to enter.\n ");
            break;
        }

        /* handle readonly flag! */
        if ( roomTab[thisRoom].rtflags.READONLY && !groupseesroom(thisRoom)
        && ( (letter == '\r') || (letter == '\n') || (letter == 'M') 
        ||   (letter == 'E')  || (letter == 'O') || (letter == 'G')  ) )
        {
            mPrintf("\b\n\n  --Room is readonly.\n ");
            break;
        }

        /* handle steeve */
        if ( MessageRoom[thisRoom] == cfg.MessageRoom && !(sysop || aide)
        && ( (letter == '\r') || (letter == '\n') || (letter == 'M') 
        ||   (letter == 'E')  || (letter == 'O') || (letter == 'G') ) )
        {
            mPrintf("\b\n\n  --Only %d %s per room.\n ",cfg.MessageRoom, 
                    cfg.msgs_nym);
            break;
        }

        /* handle nomail flag! */
        if ( logBuf.lbflags.NOMAIL && (letter == 'E'))
        {
            mPrintf("\b\n\n  --You can't enter mail.\n ");
            break;
        }

        /* handle downonly flag! */
        if ( roomTab[thisRoom].rtflags.DOWNONLY && !groupseesroom(thisRoom)
        && (  (letter == 'T') || (letter == 'W') ) ) 
        {
            mPrintf("\b\n\n  --Room is download only.\n ");
            break;
        }

        if ( !sysop && (letter == '*'))
        {
            mPrintf("\b\n\n  --You can't enter a file-linked %s.\n ", 
                    cfg.msg_nym);
            break;
        }

        if ( !sysop && (letter == 'S') && !cfg.entersur )
        {
            mPrintf("\b\n\n  --Users can't enter their title and surname.\n ");
            break;
        }

        if ( !sysop && (letter == 'S') && logBuf.SURNAMLOK )
        {
            mPrintf("\b\n\n  --Your title and surname has been locked!\n ");
            break;
        }

        switch (letter)
        {
        case '\r':
        case '\n':
            moreYet   =  FALSE;
            makeMessage();
            break;
        case 'B':
            mPrintf("\bBorder Line");
            editBorder();
            break;
        case 'C':
            mPrintf("\bConfiguration ");
            configure(FALSE);
            break;
        case 'D':
            mPrintf("\bDefault-hallway ");
            doCR();
            defaulthall();
            break;
        case 'E':
            mPrintf("\bExclusive %s ", cfg.msg_nym);
            doCR();
            if (whichIO != CONSOLE) echo = CALLER;
            limitFlag = FALSE; 
            mailFlag = TRUE;
            makeMessage();
            echo = BOTH;
            break;
        case 'F':
            mPrintf("\bForwarding-address ");
            doCR();
            forwardaddr();
            break;
        case 'H':
            mPrintf("\bHallway ");
            doCR();
            enterhall();
            break;
        case 'L':
            mPrintf("\bLimited-access ");
            done      = FALSE;
            limitFlag = TRUE;
            break;
        case '*':
            mPrintf("\bFile-linked ");
            done      = FALSE;
            linkMess  = TRUE;
            break;
        case 'M':
            mPrintf("\b%s ", cfg.msg_nym);
            doCR();
            makeMessage();
            break;
        case 'G':
            mPrintf("\bGroup-only %s ", cfg.msg_nym);
            doCR();
            limitFlag = TRUE;
            makeMessage();
            break;
        case 'O':
            mPrintf("\bOld %s ", cfg.msg_nym);
            done    = FALSE;
            oldFlag = TRUE;
            break;
        case 'P':
            mPrintf("\bPassword ");
            doCR();
            newPW();
            break;
        case 'R':
            mPrintf("\bRoom ");
            if (!cfg.nonAideRoomOk && !aide)
            {
                mPrintf("\n --Must be aide to create room.\n ");
                 break;
            }
            if (!loggedIn)
            {
                 mPrintf("\n --Must log in to create new room.\n ");
                 break;
            }
            doCR();
            makeRoom();
            break;
        case 'T':
            mPrintf("\bTextfile ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
                return;
            }
            if (!loggedIn)
            {
                mPrintf("\n --Must be logged in.\n ");
                break;
            }
            entertextfile();
            break;
        case 'W':
            mPrintf("\bWC-protocol upload ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else if (!loggedIn)
            {
                mPrintf("\n --Must be logged in.\n ");
            }
            else enterwc();
            doCR();
            break;
        case 'A':
            mPrintf("\bApplication");
            if (!loggedIn)
            {
                mPrintf("\n --Must be logged in.\n ");
            }
            else ExeAplic();
            break;
        case 'X':
            mPrintf("\bExclude Room ");
            exclude();
            break;
        case 'S':
            if (cfg.surnames || cfg.titles)
            {
                label tempsur;

                mPrintf("\bSurname / Title"); doCR();
                
                if (cfg.titles)
                {
                    getString("title", tempsur, NAMESIZE, 0, ECHO, 
                              logBuf.title);
                    if (*tempsur)
                    {
                        strcpy(logBuf.title, tempsur);
                        normalizeString(logBuf.title);
                    }
                }
                
                if (cfg.surnames)
                {
                    getString("surname", tempsur, NAMESIZE, 0, ECHO, 
                              logBuf.surname);
                    if (*tempsur)
                    {
                        strcpy(logBuf.surname, tempsur);
                        normalizeString(logBuf.surname);
                    }
                }
                break;
            }
        default:
            mPrintf("? ");
            if (expert)  break;
        case '?':
            tutorial("entopt.mnu");
            break;
        }
    }
    while (!done && moreYet);

    oldFlag   = FALSE;
    mailFlag  = FALSE;
    limitFlag = FALSE;

}

/************************************************************************/
/*      exclude() handles X>clude room,  toggles the bit                */
/************************************************************************/
void exclude(void)
{
    if  (!logBuf.lbroom[thisRoom].xclude)
    {
         mPrintf("\n \n Room now excluded from G)oto loop.\n ");
         logBuf.lbroom[thisRoom].xclude = TRUE;  
    }else{
         mPrintf("\n \n Room now in G)oto loop.\n ");
         logBuf.lbroom[thisRoom].xclude = FALSE;
    }
}

/************************************************************************/
/*      doGoto() handles G(oto) command                                 */
/************************************************************************/
void doGoto(expand, skip)
char expand;    /* TRUE to accept following parameters  */
{
    label roomName;

    if (!skip)
    {
        mPrintf("\bGoto ");
        skiproom = FALSE;
    } 
    else 
    {
        mPrintf("\bBypass to ");
        skiproom = TRUE;
    }

    if (!expand)
    {
        gotoRoom("");
        return;
    }

    getString("", roomName, NAMESIZE, 1, ECHO, "");
    normalizeString(roomName);

    if (roomName[0] == '?')
    {
        listRooms(OLDNEW, FALSE, FALSE);
    }
    else 
    {
        gotoRoom(roomName);
    }
}

/************************************************************************/
/*      doHelp() handles H(elp) command                                 */
/************************************************************************/
void doHelp(expand)
char expand;    /* TRUE to accept following parameters  */
{
    label fileName;

    mPrintf("\bHelp ");
    if (!expand)
    {
        mPrintf("\n\n");
        tutorial("dohelp.hlp");
        return;
    }

    getString("", fileName, 9, 1, ECHO, "");
    normalizeString(fileName);

    if (strlen(fileName) == 0)  strcpy(fileName, "dohelp");

    if (fileName[0] == '?')
    {
        tutorial("helpopt.hlp");
    } else {
        /* adding the extention makes things look simpler for           */
        /* the user... and restricts the files which can be read        */
        strcat(fileName, ".hlp");

        tutorial(fileName);
    }
}

/************************************************************************/
/*      doIntro() handles Intro to ....  command.                       */
/************************************************************************/
void doIntro()
{
    mPrintf("\bIntro to %s\n ", cfg.nodeTitle);
    tutorial("intro.hlp");
}


/***********************************************************************/
/*      doKnown() handles K(nown rooms) command.                       */
/***********************************************************************/
void doKnown(moreYet,first)
char moreYet;           /* TRUE to accept following parameters */
char first;             /* first parameter if true             */
{
    char letter;
    char verbose = FALSE;
    char numMess = FALSE;
    char done;

    if (moreYet)  first = '\0';

    mPrintf("\bKnown ");

    if (first)  oChar(first);

    do  
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

        letter = (char)(toupper( first ? (char)first : (char)iChar()  ));
        switch (letter)
        {
            case 'A':
                mPrintf("\bApplication Rooms ");
                mPrintf("\n ");
                listRooms(APLRMS, verbose, numMess);
                break;
            case 'D':
                mPrintf("\bDirectory Rooms ");
                mPrintf("\n ");
                listRooms(DIRRMS, verbose, numMess);
                break;
            case 'H':
                mPrintf("\bHallways ");
                knownhalls();
                break;
            case 'L':
                mPrintf("\bLimited Access Rooms ");
                mPrintf("\n ");
                listRooms(LIMRMS, verbose, numMess);
                break;
            case 'N':
                mPrintf("\bNew Rooms ");
                mPrintf("\n ");
                listRooms(NEWRMS, verbose, numMess);
                break;
            case 'O':
                mPrintf("\bOld Rooms ");
                mPrintf("\n ");
                listRooms(OLDRMS, verbose, numMess);
                break;
            case 'M':
                mPrintf("\bMail Rooms ");
                mPrintf("\n ");
                listRooms(MAILRM, verbose, numMess);
                break;
            case 'S':
                mPrintf("\bShared Rooms ");
                mPrintf("\n ");
                listRooms(SHRDRM, verbose, numMess);
                break;
            case 'I':
                mPrintf("\bRoom Info");
                mPrintf("\n ");
                RoomStatus();
                break;
            case '\r':
            case '\n':
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'R':
                mPrintf("\bRooms ");
                mPrintf("\n ");
                listRooms(OLDNEW, verbose, numMess);
                break;
            case 'V':
                mPrintf("\bVerbose ");
                done    = FALSE;
                verbose = TRUE;
                break;
            case 'W':
                mPrintf("\bWindows ");
                mPrintf("\n ");
                listRooms(WINDWS, verbose, numMess);
                break;
            case 'X':
                mPrintf("\bXcluded Rooms ");
                mPrintf("\n ");
                listRooms(XCLRMS, verbose, numMess);
                break;
            case '#':
                mPrintf(" of %s ", cfg.msgs_nym);
                done    = FALSE;
                numMess = TRUE;
                break;
            default:
                mPrintf("? ");
                if (expert)  break;
            case '?':
                tutorial("known.mnu");
                break;
        }
    }
    while (!done && moreYet);
}

/************************************************************************/
/*      doLogin() handles L(ogin) command                               */
/************************************************************************/
void doLogin(moreYet)
char moreYet;   /* TRUE to accept following parameters  */
{
    char InitPw[80];
    char passWord[80];
    char Initials[80];
    char *semicolon;

    if (justLostCarrier)  return;

    if (moreYet == 2)
        moreYet = FALSE;
    else
        mPrintf("\bLogin ");

    /* we want to be in console mode when we log in from local */
    if (!gotCarrier() && !loggedIn) 
    {
        whichIO = CONSOLE;
        onConsole = (char)(whichIO == CONSOLE);
        update25();
        if (cfg.offhook)  offhook();
    }

    if (loggedIn)  
    {
        mPrintf("\n Already logged in!\n ");
        return;
    }

    getNormStr( (moreYet) ? "" : "your initials", InitPw, 40, NO_ECHO);
    dospCR();

    semicolon = strchr(InitPw, ';');

    if (!semicolon)
    {
        strcpy(Initials, InitPw);
        getNormStr( "password",  passWord, NAMESIZE, NO_ECHO);
        dospCR();
    }     
    else  normalizepw(InitPw, Initials, passWord);

    /* dont allow anything over 19 characters */
    Initials[19] = '\0';

    login(Initials, passWord);
}

/************************************************************************/
/*      doLogout() handles T(erminate) command                          */
/************************************************************************/
void doLogout(expand, first)
char expand;    /* TRUE to accept following parameters  */
char first;     /* first parameter if TRUE              */
{
    char done = FALSE, verbose = FALSE;

    if (expand)   first = '\0';

    mPrintf("\bTerminate ");

    if (first)   oChar(first);

    if (first == 'q')
        verbose = 1;
    
    while(!done)
    {
        done = TRUE;

        switch (toupper( first ? (int)first : (int)iChar() ))
        {
        case '?':
            mPrintf("\n Logout options:\n \n ");
    
            mPrintf("Q>uit-also\n " );
            mPrintf("S>tay\n "      );
            mPrintf("V>erbose\n "   );
            mPrintf("? -- this\n "  );
            break;
        case 'Y':
        case 'Q':
            mPrintf("\bQuit-also\n ");
            if (!expand)  
            {
                if (!getYesNo(confirm, 0))   break;
            }
            if (!(haveCarrier || onConsole)) break;
            terminate( /* hangUp == */ TRUE, verbose);
            break;
        case 'S':
            mPrintf("\bStay\n ");
            terminate( /* hangUp == */ FALSE, verbose);
            break;
        case 'V':
            mPrintf("\bVerbose ");
            verbose = 2;
            done = FALSE;
            break;
        default:
            if (expert)
                mPrintf("? ");
            else
                mPrintf("? for help");
            break;
        }
        first = '\0';
    }
}

/************************************************************************/
/*      doRead() handles R(ead) command                                 */
/************************************************************************/
void doRead(moreYet, first)
char moreYet;           /* TRUE to accept following parameters */
char first;             /* first parameter if TRUE             */
{
    char abort, done, letter;
    char whichMess, revOrder, verbose;

    if (moreYet)   first = '\0';

    mPrintf("\bRead ");

    abort      = FALSE;
    revOrder   = FALSE;
    verbose    = FALSE;
    whichMess  = NEWoNLY;
    mf.mfPub   = FALSE;
    mf.mfMai   = FALSE;
    mf.mfLim   = FALSE;
    mf.mfUser[0] = FALSE;
    mf.mfGroup[0] = FALSE;

    if  (!loggedIn && !cfg.unlogReadOk) 
    {
        mPrintf("\n --Must log in to read.\n ");
        return;
    }

    if (first)  oChar(first);

    do
    {
        outFlag = IMPERVIOUS;
        done    = TRUE;

        letter = (char)(toupper(first ? (int)first : (int)iChar()));

        /* handle uponly flag! */
        if ( roomTab[thisRoom].rtflags.UPONLY && !groupseesroom(thisRoom)
        && (  (letter == 'T') || (letter == 'W') ) ) 
        {
            mPrintf("\b\n\n  --Room is upload only.\n ");
            break;
        }

        switch (letter)
        {
        case '\n':
        case '\r':
            moreYet = FALSE;
            break;
        case 'B':
            mPrintf("\bBy-User ");
            mf.mfUser[0] = TRUE;
            done         = FALSE;
            break;
        case 'C':
            mPrintf("\bConfiguration ");
            showconfig(&logBuf);
            abort     = TRUE;
            break;
        case 'D':
            mPrintf("\bDirectory ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readdirectory(verbose);
            abort      = TRUE;
            break;
        case 'E':
            mPrintf("\bExclusive ");
            mf.mfMai     = TRUE;
            done         = FALSE;
            break;
        case 'F':
            mPrintf("\bForward ");
            revOrder     = FALSE;
            whichMess    = OLDaNDnEW;
            done         = FALSE;
            break;
        case 'H':
            mPrintf("\bHallways ");
            readhalls();
            abort     = TRUE;
            break;
        case 'I':
            mPrintf("\bInfo file ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readinfofile(verbose);
            abort      = TRUE;
            break;
        case 'L':
            mPrintf("\bLimited-access ");
            mf.mfLim     = TRUE;
            done         = FALSE;
            break;
        case 'N':
            mPrintf("\bNew ");
            whichMess  = NEWoNLY;
            done       = FALSE;
            break;
        case 'O':
            mPrintf("\bOld ");
            revOrder   = TRUE;
            whichMess  = OLDoNLY;
            done       = FALSE;
            break;
        case 'P':
            mPrintf("\bPublic ");
            mf.mfPub     = TRUE;
            done         = FALSE;
            break;
        case 'R':
            mPrintf("\bReverse ");
            revOrder   = TRUE;
            whichMess  = OLDaNDnEW;
            done       = FALSE;
            break;
        case 'S':
            mPrintf("\bStatus\n ");
            systat();
            abort         = TRUE;
            break;
        case 'T':
            mPrintf("\bTextfile ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readtextfile();
            abort         = TRUE;
            break;
        case 'U':
            mPrintf("\bUserlog ");
            Readlog(verbose);
            abort         = TRUE;
            break;
        case 'V':
            mPrintf("\bVerbose ");
            verbose       = TRUE;
            done          = FALSE;
            break;
        case 'W':
            mPrintf("\bWC-protocol download ");
            if (!roomBuf.rbflags.MSDOSDIR)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else readwc();
            abort    = TRUE;
            break;
        case 'Z':
            mPrintf("\bZIP-file ");
            if ( !roomBuf.rbflags.MSDOSDIR )
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }else readzip(verbose);
            abort     = TRUE;
            break;
        default:
            mPrintf("? ");
            abort    = TRUE;
            if(expert) break;
        case '?':
            tutorial("readopt.mnu");
            abort    = TRUE;
            break;
        }
        first = '\0';

    }
    while (!done && moreYet && !abort);

    if (abort) return;

    showMessages(whichMess, revOrder, verbose);
}


/************************************************************************/
/*      doXpert                                                         */
/************************************************************************/
void doXpert()
{
    mPrintf("\bXpert %s", (expert) ? "Off" : "On");
    doCR();
    expert = (char)(!expert);
}

/************************************************************************/
/*      doRegular()                                                     */
/************************************************************************/
char doRegular(x, c)
char x, c;
{
    char toReturn;
    int i;

    toReturn = FALSE;

    switch (c)
    {

    case 'S':
        if (sysop && x)
        {
            mPrintf("\b\bSysop Menu");
            doCR();
            doSysop();
        } 
        else 
        {
            toReturn=TRUE;
        }
        break;

    case 'A':
        if (aide)
        {
            doAide(x, 'E');
        } else {
            toReturn=TRUE;
        }
        break;

    case 'C': doChat(  x, '\0');                    break;
    case 'D': doDownload( x )  ;                    break;
    case 'E': doEnter( x, 'm' );                    break;
    case 'F': doRead(  x, 'f' );                    break;
    case 'G': doGoto(  x, FALSE);                   break;
    case 'H': doHelp(  x      );                    break;
    case 'I': doIntro()        ;                    break;
    case 'J': mPrintf("\bJump back to "); unGotoRoom(); break;
    case 'K': doKnown( x, 'r');                     break;
    case 'L': doLogin( x      );                    break;    
    case 'N': doRead(  x, 'n' );                    break;
    case 'O': doRead(  x, 'o' );                    break;
    case 'R': doRead(  x, 'r' );                    break;
    case 'B': doGoto(  x, TRUE);                    break;
    case 'T': doLogout(x, 'q' );                    break;
    case 'U': doUpload( x )    ;                    break;
    case 'X':
        if (!x)
        {
            doEnter( x, 'x' );
        }else{
            doXpert();
        }
        break;

    case '+': doNext()         ;                    break;
    case '-': doPrevious()     ;                    break;

    case ']':
    case '>': doNextHall()     ;                    break;
    case '[':
    case '<': doPreviousHall() ;                    break;
    case '~': 
        mPrintf("\bAnsi %s\n ", ansiOn ? "Off" : "On");
        ansiOn = (char)(!ansiOn);
        break;

    case '?': tutorial("mainopt.mnu");              break;

    case 0:
        if (newCarrier)  
        {
            greeting();

            if (cfg.forcelogin)
            {
                doCR();
                doCR();
                i = 0;
                while (!loggedIn && gotCarrier())
                {
                  doLogin( 2      );
                  if (++i > 3) Initport();
                }
            } 
            newCarrier  = FALSE;
        }

#ifdef NETWORK
        if (logBuf.lbflags.NODE && loggedIn)
        {
            dowhat = NETWORKING;
            net_slave();
            dowhat = DUNO;

            warned = FALSE;
            haveCarrier = FALSE;
            modStat = FALSE;
            newCarrier = FALSE;
            justLostCarrier = FALSE;
            onConsole = FALSE;
            disabled  = FALSE;
            callout    = FALSE;

            pause(200);
          
            Initport();

            cfg.callno++;
            terminate(FALSE, FALSE);
        }
#endif

        if (justLostCarrier)
        {
            justLostCarrier = FALSE;
            if (loggedIn) terminate(FALSE, FALSE);
        }
        break;  /* irrelevant value */

    default:
        toReturn=TRUE;
        break;
    }
    /* if they get unverified online */
    if (logBuf.VERIFIED)  terminate(FALSE, FALSE);
    
    update25();
    return toReturn;
}

/************************************************************************/
/*      doSysop() handles the sysop-only menu                           */
/*          return FALSE to fall invisibly into default error msg       */
/************************************************************************/
char doSysop(void)
{
    char  oldIO;
    int   c;

    oldIO = whichIO;
    
    /* we want to be in console mode when we go into sysop menu */
    if (!gotCarrier() || !sysop)
    {
        whichIO = CONSOLE;
        onConsole = (char)(whichIO == CONSOLE);
    }

    sysop = TRUE;

    update25();
 
    while (!ExitToMsdos)
    {
        amZap();
        
        outFlag = IMPERVIOUS;
        doCR();
        mPrintf("2Privileged function:0 ");
       
        dowhat = SYSOPMENU;
        c = iChar();
        dowhat = DUNO;
        
        switch (toupper( c ))
        {
        case 'A':
            mPrintf("\b");
        case 0:
            mPrintf("Abort");
            doCR();
            /* restore old mode */
            whichIO = oldIO;
            sysop = (char)(loggedIn ? logBuf.lbflags.SYSOP : 0);
            onConsole = (char)(whichIO == CONSOLE);
            update25();
            return FALSE;

        case 'C':
            mPrintf("\bCron special: ");
            cron_commands();
            break;

        case 'D':
            mPrintf("\bDate change\n ");
            changeDate();
            break;

        case 'E':
            mPrintf("\bEnter EXTERNAL.CIT and GRPDATA.CIT files.\n ");
            readaccount();
            readprotocols();
            break;

        case 'F':
            doAide( 1, 'E');
            break;

        case 'G':
            mPrintf("\bGroup special: ");
            do_SysopGroup();
            break;

        case 'H':
            mPrintf("\bHallway special: ");
            do_SysopHall();
            break;

        case 'K':
            mPrintf("\bKill account\n ");
            killuser();
            break;

        case 'L':
            mPrintf("\bLogin enabled\n ");
            sysopNew = TRUE;
            break;

        case 'M':
            mPrintf("\bMass delete\n ");
            massdelete();
            break;

        case 'N':
            mPrintf("\bNewUser Verification\n ");
            globalverify();
            break;

        case 'O':
            mPrintf("\bOff hook\n ");
            if (!onConsole) break;
            offhook();
            break;    
        
        case 'R':
            mPrintf("\bReset file info\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else updateinfo();
            break;

        case 'S':
            mPrintf("\bShow user\n ");
            showuser();
            break;

        case 'U':
            mPrintf("\bUserlog edit\n ");
            userEdit();
            break;

        case 'V':
            mPrintf("\bView Help Text File\n ");
            tutorial("sysop.hlp");
            break;

        case 'X':
            mPrintf("\bExit to MS-DOS\n ");
            doCR();
            if (!onConsole) break;
            if (!getYesNo(confirm, 0))   break;
            ExitToMsdos = TRUE;
            return FALSE;

        case 'Z': 
            mPrintf("\bZap empty rooms\n ");
            killempties(); 
            break; 

        case '!':        
            mPrintf("\b ");
            doCR();
            if (!onConsole) break;
            shellescape(0);
            break;

        case '@':        
            mPrintf("\b ");
            doCR();
            if (!onConsole) break;
            shellescape(1);
            break;

        case '#':
            mPrintf("\bRead by %s number\n ", cfg.msg_nym);
            readbymsgno();
            break;

        case '*':
            mPrintf("\bUnlink file(s)\n ");
            if (roomBuf.rbflags.MSDOSDIR != 1)
            {
                if (expert) mPrintf("? ");
                else        mPrintf("\n Not a directory room.");
            }
            else sysopunlink();
            break;

        case '?':
            tutorial("sysop.mnu");
            break;

        default:
            if (!expert)  mPrintf("\n '?' for menu.\n "  );
            else          mPrintf(" ?\n "                );
            break;
        }
    }
    return FALSE;
}

/************************************************************************/
/*     do_SysopGroup() handles doSysop() Group functions                */
/************************************************************************/
void do_SysopGroup()
{
    switch(toupper(iChar()))
    {
    case 'G':
        mPrintf("\bGlobal Group membership\n  \n");
        globalgroup();
        break;
    case 'K':
        mPrintf("\bKill group");
        killgroup();
        break;
    case 'N':
        mPrintf("\bNew group");
        newgroup();
        break;
    case 'U':
        mPrintf("\bGlobal user membership\n  \n");
        globaluser();
        break;
    case 'R':
        mPrintf("\bRename group");
        renamegroup();
        break;
    case '?':
        doCR();
        mPrintf(" K>ill group");   doCR();
        mPrintf(" N>ew group");    doCR();
        mPrintf(" G>lobal membership");    doCR();
        mPrintf(" U>ser global membership");    doCR();
        mPrintf(" R>ename group ");
        break;
    default:
        if (!expert)  mPrintf("\n '?' for menu.\n "  );
        else          mPrintf(" ?\n "                );
        break;
    }
}

/************************************************************************/
/*     do_SysopHall() handles the doSysop hall functions                */
/************************************************************************/
void do_SysopHall()
{
    switch(toupper(iChar()))
    {
    case 'F':
        mPrintf("\bForce access");
        force();
        break;
    case 'K':
        mPrintf("\bKill hallway");
        killhall();
        break;
    case 'L':
        mPrintf("\bList halls");
        listhalls();
        break;
    case 'N':
        mPrintf("\bNew hall");
        newhall();
        break;
    case 'R':
        mPrintf("\bRename hall");
        renamehall();
        break;
    case 'G':
        mPrintf("\bGlobal Hall func"); doCR();
        globalhall();
        break;
    case '?':
        doCR();
        mPrintf(" F>orce");   doCR();
        mPrintf(" G>lobal hall func"); doCR();
        mPrintf(" K>ill");    doCR();
        mPrintf(" L>ist");    doCR();
        mPrintf(" N>ew");     doCR();
        mPrintf(" R>ename ");
        break;
    default:
        if (!expert)  mPrintf("\n '?' for menu.\n "  );
        else          mPrintf(" ?\n "                );
        break;
    }
}

/************************************************************************/
/*     doNext() handles the '+' for next room                           */
/************************************************************************/
void doNext()
{
    mPrintf("\bNext Room: ");
    stepRoom(1);
}

/************************************************************************/
/*     doPrevious() handles the '-' for previous room                   */
/************************************************************************/
void doPrevious()
{
    mPrintf("\bPrevious Room: ");
    stepRoom(0);
}

/************************************************************************/
/*     doNextHall() handles the '>' for next hall                       */
/************************************************************************/
void doNextHall()
{
    mPrintf("\bNext Hall: ");
    stephall(1);
}

/************************************************************************/
/*     doPreviousHall() handles the '<' for previous hall               */
/************************************************************************/
void doPreviousHall()
{
    mPrintf("\bPrevious Hall: ");
    stephall(0);
}

/************************************************************************/
/*      getCommand() prints menu prompt and gets command char           */
/*      Returns: char via parameter and expand flag as value  --        */
/*               i.e., TRUE if parameters follow else FALSE.            */
/************************************************************************/
char getCommand(c)
char *c;
{
    char expand;

    outFlag = IMPERVIOUS;

    /* update user's balance */
    if( cfg.accounting && !logBuf.lbflags.NOACCOUNT )
        updatebalance();

#ifdef DRAGON
    dragonAct();    /* user abuse rutine :-) */
#endif

    if (cfg.borders)
    {
        doBorder();
    }

    givePrompt();

    do{
         dowhat = MAINMENU;
         *c = (char)toupper(iChar());
         dowhat = DUNO;
    }while(*c == 'P'); 

    expand  = (char)
              ( (*c == ' ') || (*c == '.') || (*c == ',') || (*c == '/') );

    if (expand)
    {
        *c = (char)toupper(iChar());
    }

    if (justLostCarrier)
    {
        justLostCarrier = FALSE;
        if (loggedIn) terminate(FALSE, FALSE);
    }
    return expand;
}

/************************************************************************/
/*      greeting() gives system-entry blurb etc                         */
/************************************************************************/
void greeting()
{
    int messages;
    char dtstr[80];

    if (loggedIn) terminate(FALSE, FALSE);
    echo  =  BOTH;

    setdefaultconfig();
    initroomgen();
    cleargroupgen();
    if (cfg.accounting) unlogthisAccount();

    pause(10);

    if (newCarrier)  hello();

    mPrintf("\n Welcome to %s, %s", cfg.nodeTitle, cfg.nodeRegion);
    mPrintf("\n Running DragCit v%s (Turbo C)", version);
    if (strlen(testsite))
    {
        mPrintf("\n %s", testsite);
    }
    doCR();
    doCR();

    strftime(dtstr, 79, cfg.vdatestamp, 0l);
    mPrintf(" %s", dtstr);

    if (!cfg.forcelogin)
    {
        mPrintf("\n H for Help");
        mPrintf("\n ? for Menu");
        mPrintf("\n L to Login");
    }

    getRoom(LOBBY);

    messages = talleyBuf.room[thisRoom].messages;

    doCR();

    mPrintf("  %d %s ", messages, 
        (messages == 1) ? cfg.msg_nym: cfg.msgs_nym);

    doCR();

    while (MIReady()) getMod();

}

/************************************************************************/
/*      main() contains the central menu code                           */
/************************************************************************/
int main(int argc, char *argv[])
{
    int i;
    char c, x = FALSE, floppy = FALSE;

    cfg.bios = 1;

    for(i = 1; i < argc; i++)
    {
        if (   argv[i][0] == '/'
            || argv[i][0] == '-')
        {
            switch(toupper((int)argv[i][1]))
            {
            case 'D':
                cfg.bios = 0;
                break;

            case 'C':
                x = TRUE;
                break;

            case 'F':
                floppy = TRUE;
                break;

            case 'N':
                if (toupper(argv[i][2]) == 'B')
                {
                    cfg.noBells = TRUE;
                }
                if (toupper(argv[i][2]) == 'C')
                {
                    cfg.noChat = TRUE;
                }
                break;

            default:
                printf("\nUnknown commandline switch '%s'.\n", argv[i]);
                exit(1);
            }
        }
    }

    cfg.attr = 7;   /* logo gets white letters */

    setscreen();

    if (floppy)
    {
        cPrintf("\nInsert data disk(s) now and press any key.\n\n");
        getch();
    }

    if (x == TRUE) unlink("etc.dat");

    logo();

    if (time(NULL) < 607415813L)
    {
        cPrintf("\n\nPlease set your time and date!\n");
        exit(1);
    }

    initCitadel();

    greeting();

    sysReq = FALSE;

    if (cfg.f6pass[0])
        ConLock = TRUE;
    update25();

    while (!ExitToMsdos) 
    {
        if (sysReq == TRUE && !loggedIn && !haveCarrier)
        {
            sysReq=FALSE;
            if (cfg.offhook)
            {
                offhook();
            }else{
                drop_dtr();
            }
            ringSystemREQ();
        }

        x       = getCommand(&c);

        outFlag = IMPERVIOUS;

        if (chatkey) chat();
        if (eventkey && !haveCarrier)
        { 
            do_cron(CRON_TIMEOUT);
            eventkey = FALSE;
        }

        if ((sysopkey)  ?  doSysop()  :  doRegular(x, c)) 
        {
            if (!expert)  mPrintf("\n '?' for menu, 'H' for help.\n \n" );
            else          mPrintf(" ?\n \n" );
        }
    }
    exitcitadel();

    return 0; /* never realy gets here */
}

