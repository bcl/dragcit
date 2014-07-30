/************************************************************************/
/*                               sysop1.c                               */
/*        Sysop function code for Citadel bulletin board system         */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      force()                 sysop special to force access into hall */
/*      groupfunc()             aide fn: to add/remove group members    */
/*      hallfunc()              adds/removes room from current hall     */
/*      killgroup()             sysop special to kill a group           */
/*      killhall()              sysop special to kill a hall            */
/*      listgroup()             aide fn: to list groups                 */
/*      listhalls()             sysop special to list all hallways      */
/*      newgroup()              sysop special to add a group            */
/*      newhall()               sysop special to add a new hall         */
/*      renamegroup()           sysop special to rename a group         */
/*      renamehall()            sysop special to rename a hall          */
/*      sysopunlink()           unlinks ambiguous filenames sysop only  */
/*      windowfunc()            windows/unwindows room from current hall*/
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*      defaulthall() handles enter default hallway   .ed               */
/************************************************************************/
void defaulthall(void)
{
    label hallname;
    int slot, accessible;

    getString("hallway", hallname, NAMESIZE, FALSE, ECHO, "");

    slot = hallexists(hallname);
    if (slot == ERROR)  slot = partialhall(hallname);

    if (slot != ERROR) accessible = accesshall(slot);

    if ( (slot == ERROR) || !strlen(hallname) || !accessible )
    {
        mPrintf("\n No such hall.");
        return;
    }

    strcpy(hallname, hallBuf->hall[slot].hallname);

    doCR();
    mPrintf(" Default hallway: %s", hallname);
    
    logBuf.hallhash = hash(hallname);

    /* 0 for root hallway */
    if (slot == 0) logBuf.hallhash = 0;

    if (loggedIn) storeLog();
}

/************************************************************************/
/*      force()  sysop special to force access into a hallway           */
/************************************************************************/
void force(void)
{
    label hallname;
    int slot;

    getString("hallway", hallname, NAMESIZE, FALSE, ECHO, "");

    slot = hallexists(hallname);
    if (slot == ERROR)  slot = partialhall(hallname);

    if ( (slot == ERROR) || !strlen(hallname) )
    {
        mPrintf("\n No such hall.");
        return;
    }

    thisHall = (unsigned char)slot;

    sprintf(msgBuf->mbtext,
    "Access forced to hallway %s", hallBuf->hall[thisHall].hallname );

    mPrintf("%s", msgBuf->mbtext); doCR();

    trap(msgBuf->mbtext, T_SYSOP);
}

/************************************************************************/
/*      groupfunc()  aide fn: to add/remove group members               */
/************************************************************************/
void groupfunc(void)
{
    label               who;
    label groupname;
    int groupslot,      logNo;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = groupexists(groupname);
    if (groupslot == ERROR)  groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].lockout && !sysop )
        groupslot = ERROR;

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    getNormStr("who", who, NAMESIZE, ECHO);
    logNo   = findPerson(who, &lBuf);
    if (logNo == ERROR || !strlen(who) )  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
    {
        if (getYesNo("Remove from group", 0))
        {
            lBuf.groups[groupslot] =
                (grpBuf.group[groupslot].groupgen
              + (MAXGROUPGEN - 1)) % MAXGROUPGEN;

            sprintf(msgBuf->mbtext,
                    "%s kicked out of group %s by %s",
                    lBuf.lbname,
                    grpBuf.group[groupslot].groupname,
                    logBuf.lbname );

            trap(msgBuf->mbtext, T_AIDE);

            aideMessage();
        }
    }
    else

    if (getYesNo("Add to group", 0))
    {
        lBuf.groups[groupslot] = grpBuf.group[groupslot].groupgen;

        sprintf(msgBuf->mbtext,
        "%s added to group %s by %s",
        lBuf.lbname,
        grpBuf.group[groupslot].groupname,
        logBuf.lbname );

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

    }

    putLog(&lBuf, logNo);

    /* see if it is us: */
    if (loggedIn  &&  strcmpi(logBuf.lbname, who) == SAMESTRING) 
    {
        logBuf.groups[groupslot] = lBuf.groups[groupslot];
    }

}

/************************************************************************/
/*      globalgroup() aide fn: to add/remove group members  (global)    */
/************************************************************************/
void globalgroup(void)
{
    label groupname;
    int groupslot, i, yn, add, logNo;

    mPrintf(" Add/Remove/Both: ");

    switch (toupper( iChar() ))
    {
    case 'A':
        mPrintf("\bAdd\n ");
        add = 1;
        break;
    case 'R':
        mPrintf("\bRemove\n ");
        add = 2;
        break;
    case 'B':
        mPrintf("\bBoth\n ");
        add = 0;
        break;
    default:
        mPrintf("\bBoth\n ");
        add = 0;
        break;
    }

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = groupexists(groupname);
    if (groupslot == ERROR)  groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    if ( grpBuf.group[groupslot].lockout && !sysop )
    {
        mPrintf("\n Group is locked.");
        return;
    }

    for (i = 0; i < cfg.MAXLOGTAB; i++)
    {
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash !=0)
        {
            logNo=logTab[i].ltlogSlot;
            getLog(&lBuf, logNo);
            if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
            {
                if(add == 2 || add == 0)
                {
                    mPrintf(" %s", lBuf.lbname);
                    yn=getYesNo("Remove from group", 0+3);
                    if (yn == 2)
                    {
                        SaveAideMess();
                        return;
                    }
      
                    if (yn)
                    {
                        lBuf.groups[groupslot] =
                            (grpBuf.group[groupslot].groupgen
                          + (MAXGROUPGEN - 1)) % MAXGROUPGEN;
          
                        sprintf(msgBuf->mbtext,
                            "%s kicked out of group %s by %s",
                            lBuf.lbname,
                            grpBuf.group[groupslot].groupname,
                            logBuf.lbname );
           
                        trap(msgBuf->mbtext, T_SYSOP);
          
                        amPrintf(" %s\n", msgBuf->mbtext);
                    }
                }
            }else{
                if (add == 0 || add == 1)
                {
                    mPrintf(" %s", lBuf.lbname);
                    yn=getYesNo("Add to group", 0+3);
                    if (yn == 2)
                    {
                        SaveAideMess();
                        return;
                    }
                    if (yn)
                    {
                        lBuf.groups[groupslot] =
                            grpBuf.group[groupslot].groupgen;
           
                        sprintf(msgBuf->mbtext,
                            "%s added to group %s by %s",
                            lBuf.lbname,
                            grpBuf.group[groupslot].groupname,
                            logBuf.lbname );
         
                        trap(msgBuf->mbtext, T_AIDE);
           
                        amPrintf(" %s\n",msgBuf->mbtext);
                    }
                }
            }
  
            putLog(&lBuf, logNo);
     
            /* see if it is us: */
            if (loggedIn  &&  strcmpi(logBuf.lbname, lBuf.lbname) == SAMESTRING)
            {
                logBuf.groups[groupslot] = lBuf.groups[groupslot];
            }
        }
    }

    SaveAideMess();
}

/************************************************************************/
/*      globaluser() aide fn: to add/remove user from groups (global)   */
/************************************************************************/
void globaluser(void)
{
    label               who;
    int groupslot, yn, logNo;

    getNormStr("who", who, NAMESIZE, ECHO);
    logNo   = findPerson(who, &lBuf);
    if (logNo == ERROR || !strlen(who) )  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    for(groupslot=0; groupslot < MAXGROUPS; groupslot++)
    {
      if (grpBuf.group[groupslot].g_inuse 
          && ( !grpBuf.group[groupslot].lockout || sysop )
          && ( !grpBuf.group[groupslot].hidden  || ingroup(groupslot) )
         )
      {
        mPrintf(" %s", grpBuf.group[groupslot].groupname);
        if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
        {
            if ((yn = getYesNo("Remove from group", 3)) != NULL)
            {
                if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }

                lBuf.groups[groupslot] =
                  (grpBuf.group[groupslot].groupgen
                  + (MAXGROUPGEN - 1)) % MAXGROUPGEN;
                sprintf(msgBuf->mbtext,
                  "%s kicked out of group %s by %s",
                  lBuf.lbname,
                grpBuf.group[groupslot].groupname,
                  logBuf.lbname );

                trap(msgBuf->mbtext, T_AIDE);

              amPrintf(" %s\n", msgBuf->mbtext);
            }
        }else{
          if ((yn = getYesNo("Add to group", 3)) != NULL)
          {
            if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }

            lBuf.groups[groupslot] = grpBuf.group[groupslot].groupgen;

            sprintf(msgBuf->mbtext,
              "%s added to group %s by %s",
              lBuf.lbname,
              grpBuf.group[groupslot].groupname,
              logBuf.lbname );
            trap(msgBuf->mbtext, T_SYSOP);

              amPrintf(" %s\n",msgBuf->mbtext);
          }
        }
        putLog(&lBuf, logNo);

        /* see if it is us: */
        if (loggedIn  &&  strcmpi(logBuf.lbname, who) == SAMESTRING) 
        {
            logBuf.groups[groupslot] = lBuf.groups[groupslot];
        }
      }
    }

    SaveAideMess();
}

/*----------------------------------------------------------------------*/
/*      globalverify()  does global sweep to verify any un-verified     */
/*----------------------------------------------------------------------*/
void globalverify(void)
{
    int    logNo, i, yn, tabslot;

    outFlag = OUTOK;

    for (i=0;( (i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP) && !mAbort() );i++)
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash !=0)
        {
            logNo=logTab[i].ltlogSlot;
            getLog(&lBuf, logNo);
            if (lBuf.VERIFIED == TRUE)
            {
                mPrintf("\n %s", lBuf.lbname);
                doCR();
                yn=getYesNo("Verify", 0+3);
                if (yn == 2)
                {
                    SaveAideMess();
                    return;
                }
                if (yn)
                {
                    lBuf.VERIFIED = FALSE;
                    if (strcmpi(logBuf.lbname, lBuf.lbname) == SAMESTRING)
                        logBuf.VERIFIED = FALSE;
                    sprintf(msgBuf->mbtext, "%s verified by %s",
                                           lBuf.lbname, logBuf.lbname );
                    trap(msgBuf->mbtext, T_SYSOP);
                    amPrintf(" %s\n", msgBuf->mbtext);
                }
                else
                if (strcmpi(logBuf.lbname, lBuf.lbname) != SAMESTRING)
                    if (getYesNo("Kill account", 0))
                    {
                        mPrintf( "\n \'%s\' terminated.\n ", lBuf.lbname);
                            /* trap it */
                        sprintf(msgBuf->mbtext,
                            "Un-verified user %s terminated", lBuf.lbname);
                        trap(msgBuf->mbtext, T_SYSOP);
                            /* get log tab slot for person */
                        tabslot = personexists(lBuf.lbname);
                        logTab[tabslot].ltpwhash   = 0;
                        logTab[tabslot].ltinhash   = 0;
                        logTab[tabslot].ltnmhash   = 0;
                        logTab[tabslot].permanent  = 0;
                        lBuf.lbname[0] = '\0';
                        lBuf.lbin[  0] = '\0';
                        lBuf.lbpw[  0] = '\0';
                        lBuf.lbflags.L_INUSE   = FALSE;
                        lBuf.lbflags.PERMANENT = FALSE;
                    }
                putLog(&lBuf, logNo);
            }
        }
    SaveAideMess();
}

/************************************************************************/
/*      globalhall()  adds/removes rooms from current hall              */
/************************************************************************/
int changed_room;

void globalhall(void)
{
    int roomslot;

    changed_room = FALSE;

    for (roomslot = 0; roomslot < MAXROOMS; roomslot++)
      if (roomTab[roomslot].rtflags.INUSE)
      {
        mPrintf(" %s", roomTab[roomslot].rtname);
        if (xhallfunc(roomslot, 3, TRUE)==FALSE)
          break;
      }

    if (changed_room)
    {
        amPrintf("\n By %s, in hall %s\n", logBuf.lbname,
                hallBuf->hall[thisHall].hallname);

        SaveAideMess();
    }

    putHall();
}       

/************************************************************************/
/*      hallfunc()  adds/removes room from current hall                 */
/************************************************************************/
void hallfunc(void)
{
    label roomname;
    int roomslot;

    getString("room name", roomname, NAMESIZE, FALSE, ECHO, "");

    roomslot = roomExists(roomname);

    if ((roomslot) == ERROR || !strlen(roomname) )
    {
        mPrintf("\n No %s room", roomname);
        return;
    }

    xhallfunc(roomslot, 0, FALSE);

    putHall();
}       

xhallfunc(int roomslot, int xyn, int fl)
{
    int yn;

    if (hallBuf->hall[thisHall].hroomflags[roomslot].inhall)
    {

        yn=getYesNo("Exclude from hall", (char)xyn);
        if (yn == 2) return FALSE;
        if (yn)
        {
            hallBuf->hall[thisHall].hroomflags[roomslot].inhall = FALSE;

            sprintf(msgBuf->mbtext,
            "Room %s excluded from hall %s by %s",
             roomTab[roomslot].rtname,
             hallBuf->hall[thisHall].hallname,
             logBuf.lbname );

            trap(msgBuf->mbtext, T_AIDE);

            if (!fl)
            {
                aideMessage();
            }else{              
                amPrintf(" Excluded %s\n",
                       roomTab[roomslot].rtname,
                       hallBuf->hall[thisHall].hallname);
                changed_room = TRUE;
            }
        }
    }else{
      yn=getYesNo("Add to hall", (char)xyn);
      if (yn == 2) return FALSE;
      if (yn)
      {
          hallBuf->hall[thisHall].hroomflags[roomslot].inhall = TRUE;

          sprintf(msgBuf->mbtext,
             "Room %s added to hall %s by %s",
             roomTab[roomslot].rtname,
             hallBuf->hall[thisHall].hallname,
             logBuf.lbname );

          trap(msgBuf->mbtext, T_AIDE);

            if (!fl)
            {
                aideMessage();
            }else{
                amPrintf(" Added    %s\n",
                       roomTab[roomslot].rtname);
                changed_room = TRUE;
            }
      }
    }
    return TRUE;
}

/************************************************************************/
/*      killgroup()  sysop special to kill a group                      */
/************************************************************************/
void killgroup(void)
{
    label groupname;
    int groupslot, i;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = groupexists(groupname);

    if ( groupslot == 0 || groupslot == 1)
    {
        mPrintf("\n Cannot delete Null or Reserved_2 groups.");
        return;
    }

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    for (i = 0; i < MAXROOMS; i++)
    {
         if  (   roomTab[i].rtflags.INUSE
             &&  roomTab[i].rtflags.GROUPONLY
             && (roomTab[i].grpno  == (unsigned char)groupslot)
             && (roomTab[i].grpgen == grpBuf.group[groupslot].groupgen) )
         {
             mPrintf("\n Group still has rooms.");
             return;
         }
    }

    for (i = 0; i < MAXHALLS; i++)
    {
        if ( hallBuf->hall[i].h_inuse
        &&   hallBuf->hall[i].owned 
        &&   hallBuf->hall[i].grpno == (unsigned char)groupslot)
        {
            mPrintf("\n Group still has hallways.");
            return;
        }
    }

    if (!getYesNo(confirm, 0)) return;

    grpBuf.group[groupslot].g_inuse = 0;

    putGroup();

    sprintf(msgBuf->mbtext,
    "Group %s deleted", groupname );

    trap(msgBuf->mbtext, T_SYSOP);
}

/************************************************************************/
/*      killhall()  sysop special to kill a hall                        */
/************************************************************************/
void killhall(void)
{
    int empty = TRUE, i;

    if (thisHall == 0 || thisHall == 1)
    {
        mPrintf("\n Can not kill Root or Maintenance hallways, Dumb Shit!");
        return;
    }
    
    /* Check hall for any rooms */
    for (i = 0; i < MAXROOMS; i++)
    {
        if (hallBuf->hall[thisHall].hroomflags[i].inhall) empty = FALSE;
    }

    if (!empty) 
    {
        mPrintf("\n Hall still has rooms.");
        return;
    }

    if (getYesNo(confirm, 0))
    {
        hallBuf->hall[thisHall].h_inuse = FALSE;
        hallBuf->hall[thisHall].owned   = FALSE;
        putHall();

       sprintf(msgBuf->mbtext,
       "Hallway %s deleted", hallBuf->hall[thisHall].hallname );

       trap(msgBuf->mbtext, T_SYSOP);
    }
}    

/************************************************************************/
/*      listgroup()  aide fn: to list groups                            */
/************************************************************************/
void listgroup(void)
{
    label groupname;
    int i, groupslot;

    getString("", groupname, NAMESIZE, FALSE, ECHO, "");

    outFlag = OUTOK;

    if ( !strlen(groupname))
    {
        for (i = 0; i < MAXGROUPS; i++)
        {
            /* can they see the group */
            if (  grpBuf.group[i].g_inuse
              && (!grpBuf.group[i].lockout || sysop)
              && (!grpBuf.group[i].hidden  || onConsole || ingroup(i))
              )
            {
                mPrintf(" %-20s ", grpBuf.group[i].groupname);
                if (grpBuf.group[i].lockout) mPrintf("(Locked) ");
                if (grpBuf.group[i].hidden)  mPrintf("(Hidden) ");
                doCR();
            }
        }  
        return;
    }

    groupslot = groupexists(groupname);
    if (groupslot == ERROR)  groupslot = partialgroup(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) )
        groupslot = ERROR;

    if (groupslot == ERROR)
    {
        mPrintf("\n No such group.");
        return;
    }

    if ( grpBuf.group[groupslot].lockout
    && (!(logBuf.lbflags.SYSOP || onConsole)) )
    {
        mPrintf("\n Group is locked.");
        return;
    }

    doCR();

    outFlag = OUTOK;

    mPrintf(" Users:");

    for (i = 0; ( ( i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
        if (logTab[i].ltpwhash != 0 && logTab[i].ltnmhash != 0)
        {
            getLog(&lBuf, logTab[i].ltlogSlot);

            if (lBuf.groups[groupslot] == grpBuf.group[groupslot].groupgen)
            {
                doCR();
                mPrintf(" %s", lBuf.lbname);
            }
        }
    }
    
    if (outFlag == OUTSKIP)
    {
        doCR();
        return;
    }  

    outFlag = OUTOK;

    doCR(); 
    doCR(); 

    mPrintf(" Rooms:"); 

    for (i = 0; ( ( i < MAXROOMS) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
         if  (   roomTab[i].rtflags.INUSE 
             &&  roomTab[i].rtflags.GROUPONLY
             && (roomTab[i].grpno  == (unsigned char)groupslot)
             && (roomTab[i].grpgen == grpBuf.group[groupslot].groupgen) )
         {
             doCR();
             mPrintf(" %s" , roomTab[i].rtname);
         }
    }

    if (outFlag == OUTSKIP)
    {
        doCR();
        return;
    }  

    outFlag = OUTOK;

    doCR(); 
    doCR(); 

    mPrintf(" Hallways:"); 

    for (i = 0; ( ( i < MAXHALLS) && (outFlag != OUTSKIP)
        && (outFlag != OUTNEXT) ); i++)
    {
        if ( hallBuf->hall[i].h_inuse &&
             hallBuf->hall[i].owned   &&
            (hallBuf->hall[i].grpno == (unsigned char)groupslot) )
        {
            doCR();
            mPrintf(" %s", hallBuf->hall[i].hallname);
        }
    }
}

/************************************************************************/
/*      listhalls()  sysop special to list all hallways                 */
/************************************************************************/
void listhalls(void)
{
    int i;

    doCR();
    doCR();
    prtList(LIST_START);
    for (i = 0; i < MAXHALLS; i++)
    {
        if (hallBuf->hall[i].h_inuse)
        {
            prtList(hallBuf->hall[i].hallname);
        }
    }
    prtList(LIST_END);
}


/************************************************************************/
/*      newgroup()  sysop special to add a new group                    */
/************************************************************************/
void newgroup(void)
{
    label groupname;
    int slot, i;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    if ( (groupexists(groupname) != ERROR) || !strlen(groupname) )
    {
        mPrintf("\n We already have a \'%s\' group.", groupname);
        return;
    }

    /* search for a free group slot */

    for (i = 0, slot = 0; i < MAXGROUPS && !slot ; i++)
    {
        if (!grpBuf.group[i].g_inuse) slot = i;
    }

    if (!slot)
    {
        mPrintf("\n Group table full.");
        return;
    }
    
    /*  locked group? */
    grpBuf.group[slot].lockout = (getYesNo("Lock group from aides", 0 ));

    grpBuf.group[slot].hidden  = (getYesNo("Hide group", 0 ));

    strcpy(grpBuf.group[slot].groupname, groupname);
    grpBuf.group[slot].g_inuse = 1;

    /* increment group generation # */
    grpBuf.group[slot].groupgen =
   (grpBuf.group[slot].groupgen + 1) % MAXGROUPGEN;


    if (getYesNo(confirm, 0))
    {
        putGroup();

        sprintf(msgBuf->mbtext,
        "Group %s created", grpBuf.group[slot].groupname );

        trap(msgBuf->mbtext, T_SYSOP);
    }
    else getGroup();
}

/************************************************************************/
/*      newhall()  sysop special to add a new hall                      */
/************************************************************************/
void newhall(void)
{
    label hallname, groupname;
    int i, slot = 0, groupslot;


    getString("hall", hallname, NAMESIZE, FALSE, ECHO, "");

    if ( (hallexists(hallname) != ERROR) || !strlen(hallname) )
    {
        mPrintf("\n We already have a %s hall.", hallname);
        return;
    }

    /* search for a free hall slot */

    for (i = 0; i < MAXHALLS && !slot ; i++)
    {
        if (!hallBuf->hall[i].h_inuse)  slot = i;
    }

    if (!slot)
    {
        mPrintf("\n Hall table full.");
        return;
    }

    strcpy(hallBuf->hall[slot].hallname, hallname);
    hallBuf->hall[slot].h_inuse = TRUE;


    getString("group for hall", groupname, NAMESIZE, FALSE, ECHO, "");

    if (!strlen(groupname)) hallBuf->hall[slot].owned = FALSE;

    else
    {
        groupslot = groupexists(groupname);

        if ( groupslot == ERROR  )
        {
            mPrintf("\n No such group.");
            getHall();
            return;
        }

        else
        {
            hallBuf->hall[slot].owned  = TRUE;
            hallBuf->hall[slot].grpno  = (unsigned char)groupslot;
        }
    }

    /* make current room a window into current hallway */
    /* so we can get back                              */
    hallBuf->hall[thisHall].hroomflags[thisRoom].window = TRUE;

    /* put current room in hall */
    hallBuf->hall[slot].hroomflags[thisRoom].inhall = TRUE;

    /* make current room a window into new hallway */
    hallBuf->hall[slot].hroomflags[thisRoom].window = TRUE;


    if (getYesNo(confirm, 0))
    {
        putHall();

        sprintf(msgBuf->mbtext,
        "Hall %s created", hallBuf->hall[slot].hallname );

        trap(msgBuf->mbtext, T_SYSOP);
    }
    else  getHall();

}

/************************************************************************/
/*      renamegroup()  sysop special to rename a group                  */
/************************************************************************/
void renamegroup(void)
{
    label groupname, newname;
    int groupslot, locked, hidden;

    getString("group", groupname, NAMESIZE, FALSE, ECHO, "");

    groupslot = groupexists(groupname);

    if ( grpBuf.group[groupslot].hidden && !ingroup(groupslot) && !onConsole)
        groupslot = ERROR;

    if ( groupslot == ERROR || !strlen(groupname) )
    {
        mPrintf("\n No such group.");
        return;
    }

    getString("new group name", newname, NAMESIZE, FALSE, ECHO, "");

    if (groupexists(newname) != ERROR)
    {
        mPrintf("\n A \'%s\' group already exists.", newname);
        return;
    }

    /*  locked group? */
    locked =
        (getYesNo("Lock group from aides", (char)grpBuf.group[groupslot].lockout ));

    hidden =
        (getYesNo("Make group hidden", (char)grpBuf.group[groupslot].hidden ));

    if (!getYesNo(confirm, 0)) return;

    grpBuf.group[groupslot].lockout = locked;
    grpBuf.group[groupslot].hidden  = hidden;

    if (strlen(newname))
        strcpy(grpBuf.group[groupslot].groupname, newname);

    sprintf(msgBuf->mbtext,
    "Group %s renamed %s", groupname, newname);

    trap(msgBuf->mbtext, T_SYSOP);

    putGroup();
}

/************************************************************************/
/*      renamehall()  sysop special to rename a hallway                 */
/************************************************************************/
void renamehall(void)
{
    label hallname, newname, groupname;
    int groupslot, hallslot;

    getString("hall", hallname, NAMESIZE, FALSE, ECHO, "");
    
    hallslot = hallexists(hallname);

    if ( hallslot == ERROR || !strlen(hallname) )
    {
        mPrintf("\n No hall %s", hallname);
        return;
    }

    getString("new name for hall", newname, NAMESIZE, FALSE, ECHO, "");

    if (strlen(newname) && (hallexists(newname) != ERROR))
    {
        mPrintf("\n A %s hall already exists", newname);
        return;
    }

    if(hallBuf->hall[hallslot].owned)
        mPrintf("\n Owned by group %s\n ",
                    grpBuf.group[ hallBuf->hall[hallslot].grpno ].groupname);

    if (getYesNo("Description", (char)hallBuf->hall[hallslot].described))
    {
        getString("Description Filename", hallBuf->hall[hallslot].htell, 13,
                    FALSE, ECHO, hallBuf->hall[hallslot].htell);

        if (hallBuf->hall[hallslot].htell[0])
        {
            hallBuf->hall[hallslot].described = TRUE;
        } else {
            hallBuf->hall[hallslot].described = FALSE;
        }
    } else {
        hallBuf->hall[hallslot].described = FALSE;
    }

    if (getYesNo("Change group",0))
    {
        getString("new group for hall", groupname, NAMESIZE, FALSE, ECHO, "");

        if (!strlen(groupname))
        {
            hallBuf->hall[hallslot].owned = FALSE;
        } else {
            groupslot = groupexists(groupname);

            if ( groupslot == ERROR )
            {
                mPrintf("\n No such group.");
                getHall();
                return;
            }

            hallBuf->hall[hallslot].owned = TRUE;
            hallBuf->hall[hallslot].grpno = (unsigned char)groupslot;
        }
    }

    if (strlen(newname))
    {
        strcpy( hallBuf->hall[hallslot].hallname, newname );
    }

    if (getYesNo(confirm, 0))
    {
        putHall();
        sprintf(msgBuf->mbtext,
        "Hall %s renamed %s", hallname, newname);

        trap(msgBuf->mbtext, T_SYSOP);
    } else {
        getHall();
    }
    
}

/************************************************************************/
/*     sysopunlink()   unlinks ambiguous filenames sysop only           */
/************************************************************************/
void sysopunlink(void)
{
    label files;
    int i;

    getString("file(s) to unlink", files, NAMESIZE, FALSE, ECHO, "");

    if(files[0])
    {
        i = ambigUnlink(files, TRUE);
        if(i)
            updateinfo();
        doCR();
        mPrintf("(%d) file(s) unlinked", i);
        doCR();

        sprintf(msgBuf->mbtext, "File(s) %s unlinked in room %s]",
                    files, roomBuf.rbname);

        trap(msgBuf->mbtext, T_SYSOP);
    }
}

/************************************************************************/
/*      windowfunc()  windows/unwindows room from current hall          */
/************************************************************************/
void windowfunc(void)
{
    if (hallBuf->hall[thisHall].hroomflags[thisRoom].window)
    {
        if (getYesNo("Unwindow",0))
        {
             hallBuf->hall[thisHall].hroomflags[thisRoom].window = FALSE;

             sprintf(msgBuf->mbtext,
             "Hall %s made invisible from room %s> by %s",
             hallBuf->hall[thisHall].hallname,
             roomBuf.rbname,
             logBuf.lbname );

             trap(msgBuf->mbtext, T_AIDE);

            aideMessage();

        }
    }
    else

    if (getYesNo("Window",0))
    {
        hallBuf->hall[thisHall].hroomflags[thisRoom].window = TRUE;

        sprintf(msgBuf->mbtext,
        "Hall %s made visible from room %s> by %s",
         hallBuf->hall[thisHall].hallname,
         roomBuf.rbname,
         logBuf.lbname );

         trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

    }
    putHall();
}

/* -------------------------------------------------------------------- */
/*  msgNym()        Aide message nym setting function                   */
/* -------------------------------------------------------------------- */
void msgNym(void)
{
    doCR();
    if (!cfg.msgNym)
    {
        doCR();
        printf(" Message nyms not enabled!");
        doCR();
        return;
    }

    getString("name (SINGLE)", cfg.msg_nym,  LABELSIZE, FALSE, ECHO, "");
    getString("name (PLURAL)", cfg.msgs_nym, LABELSIZE, FALSE, ECHO, "");
    getString("what to do to message", 
               cfg.msg_done, LABELSIZE, FALSE, ECHO, "");

    sprintf(msgBuf->mbtext, "\n Message nym changed by %s to\n "
                            "Single:   %s\n "
                            "Plural:   %s\n "
                            "Verb  :   %s\n ",
                            logBuf.lbname, 
                            cfg.msg_nym, cfg.msgs_nym, cfg.msg_done );
    aideMessage();
    doCR();
}

