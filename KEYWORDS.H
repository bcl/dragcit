/************************************************************************/
/*                            keywords.h                                */
/*        all keywords used by Citadel configuration program            */
/************************************************************************/

/************************************************************************/
/*                         config.cit keywords                          */
/************************************************************************/
#ifdef MAIN
char *keywords[] = 
{
    "#ACCOUNTING",      
    "#AIDEHALL",
    "#ALLDONE",         
    "#APPLICATION",
    "#ATTR",            
    "#BATTR",
    "#BIOS",
    "#CATTR",
    "#COST1200",        
    "#COST2400",        
    "#DATESTAMP",
    "#DIAL_INIT",             
    "#DIAL_PREF",
    "#DUMB_MODEM",      
    "#ENTEROK",         
    "#F6PASSWORD",
    "#FORCELOGIN",      
    "#HELPPATH",        
    "#HOMEPATH",        
    "#IDLE_WAIT",       
    "#INIT_BAUD",
    "#KILL",
    "#LINEFEEDS",       
    "#LOGINSTATS",         
    "#MAXBALANCE",      
    "#MAXFILES",
    "#MAXLOGTAB",       
    "#MAXTEXT",         
    "#MAX_WARN",        
    "#MDATA",           
    "#MESSAGEK",        
    "#MESSAGE_ROOM",
    "#MODERATE",
    "#MODSETUP",        
    "#MSGPATH",         
    "#NEWBAL",
    "#NEWUSERAPP",
    "#NMESSAGES",       
    "#NODENAME",        
    "#NODEREGION",
    "#NOPWECHO",        
    "#NULLS",           
    "#OFFHOOK",
    "#OLDCOUNT",        
    "#PRINTER",         
    "#PRIVATE",
    "#READLLOG",
    "#READOK",          
    "#ROOMOK",          
    "#ROOMPATH",
    "#ROOMTELL",
    "#SUBHUBS",
    "#TABS",            
    "#TEMPPATH",        
    "#TIMEOUT",         
    "#TRAP",            
    "#TRAP_FILE",       
    "#TWIT_FEATURES",
    "#UNLOGGEDBALANCE", 
    "#UNLOGTIMEOUT",    
    "#UPPERCASE",       
    "#USER",            
    "#UTTR",
    "#VDATESTAMP",
    "#WATTR",
    "#WIDTH",           
    NULL
};
#else
extern char *keywords[];
#endif /* MAIN */
                       
#define     K_ACCOUNTING        0
#define     K_AIDEHALL          1  
#define     K_ALLDONE           2  
#define     K_APPLICATIONS      3  
#define     K_ATTR              4  
#define     K_BATTR             5  
#define     K_BIOS              6  
#define     K_CATTR             7  
#define     K_COST1200          8  
#define     K_COST2400          9  
#define     K_DATESTAMP         10 
#define     K_DIAL_INIT         11 
#define     K_DIAL_PREF         12 
#define     K_DUMB_MODEM        13 
#define     K_ENTEROK           14 
#define     K_F6PASSWORD        15 
#define     K_FORCELOGIN        16 
#define     K_HELPPATH          17 
#define     K_HOMEPATH          18 
#define     K_IDLE_WAIT         19 
#define     K_INIT_BAUD         20 
#define     K_KILL              21 
#define     K_LINEFEEDS         22 
#define     K_LOGINSTATS        23 
#define     K_MAXBALANCE        24 
#define     K_MAXFILES          25 
#define     K_MAXLOGTAB         26 
#define     K_MAXTEXT           27 
#define     K_MAX_WARN          28 
#define     K_MDATA             29 
#define     K_MESSAGEK          30 
#define     K_MESSAGE_ROOM      31 
#define     K_MODERATE          32 
#define     K_MODSETUP          33 
#define     K_MSGPATH           34 
#define     K_NEWBAL            35 
#define     K_NEWUSERAPP        36 
#define     K_NMESSAGES         37 
#define     K_NODENAME          38 
#define     K_NODEREGION        39 
#define     K_NOPWECHO          40 
#define     K_NULLS             41 
#define     K_OFFHOOK           42 
#define     K_OLDCOUNT          43 
#define     K_PRINTER           44 
#define     K_PRIVATE           45 
#define     K_READLLOG          46 
#define     K_READOK            47 
#define     K_ROOMOK            48 
#define     K_ROOMPATH          49 
#define     K_ROOMTELL          50 
#define     K_SUBHUBS           51 
#define     K_TABS              52 
#define     K_TEMPPATH          53 
#define     K_TIMEOUT           54 
#define     K_TRAP              55 
#define     K_TRAP_FILE         56 
#define     K_TWIT_FEATURES     57 
#define     K_UNLOGGEDBALANCE   58 
#define     K_UNLOGTIMEOUT      59 
#define     K_UPPERCASE         60 
#define     K_USER              61 
#define     K_UTTR              62 
#define     K_VDATESTAMP        63 
#define     K_WATTR             64 
#define     K_WIDTH             65 

#define     K_NWORDS            65   /* must be changed to add new keywords */

#ifdef MAIN
char *trapkeywords[] =
{
    "ALL",
    "CARRIER",
    "LOGIN",
    "NEWROOM",
    "PROBLEM_USER",
    "CHAT",
    "PASSWORD",
    "AIDE",
    "SYSOP",
    "UPLOAD",
    "DOWNLOAD",
    "ERROR",
    "NETWORK",
    "ACCOUNT",
    "APPLIC",
    NULL
};

char *userkeywords[] = 
{   
    "PROBLEM_USER",
    "PERMANENT",
    "NO_ACCOUNTING",
    "NETWORK",
    "NO_MAIL",
    "AIDE",
    "SYSOP",
    NULL
};

char *twitfeatures[] = 
{   
    "MSG_NYMS",
    "BORDER_LINES",
    "TITLES",
    "NET_TITLES",
    "SURNAMES",
    "NET_SURNAMES",
    "ENTER_TITLES",
    NULL
};
#else
extern char *trapkeywords[];
extern char *userkeywords[];
extern char *twitfeatures[];
#endif /* MAIN */

/************************************************************************/
/*                        grpdata.cit keywords                          */
/************************************************************************/

#ifdef MAIN
char *grpkeywords[] =
{
    "#DAYS",
    "#GROUP",
    "#HOURS",
    "#DAY_INC",
    "#SPECIAL",
    "#PRIORITY",
    "#MAX_BAL",
    "#DL_MULT",
    "#UL_MULT",
    NULL
};
#else
extern char *grpkeywords[];
#endif /* MAIN */

#define GRK_DAYS              0
#define GRK_GROUP             1
#define GRK_HOURS             2
#define GRK_DAYINC            3
#define GRK_SPECIAL           4
#define GRK_PRIORITY          5
#define GRK_MAXBAL            6
#define GRK_DLMULT            7
#define GRK_ULMULT            8

#ifdef MAIN
char *daykeywords[] = 
{
   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Any", NULL 
};
#else
extern char *daykeywords[];
#endif


/************************************************************************/
/*                         nodes.cit keywords                           */
/************************************************************************/

#ifdef MAIN
char *nodekeywords[] = 
{
    "#BAUD",
    "#DISCARD",
    "#DISCARD_USER",
    "#EXPIRE",
    "#GROUP",
    "#DIAL_TIMEOUT",
    "#LOGIN",
    "#WAIT_TIMEOUT",
    "#PROTOCOL",
    "#NODE",
    "#PHONE",
    "#REDIAL",
    "#MAIL_TMP",
    "#ROOM",
    "#ROUTE",
    "#ALIAS",
    "#REGION",
    NULL
};
#else
extern char *nodekeywords[];
#endif /* MAIN */

#define NOK_BAUD          0
#define NOK_DISCARD       1
#define NOK_DISCARD_USER  2
#define NOK_EXPIRE        3
#define NOK_GROUP         4
#define NOK_DIAL_TIMEOUT  5
#define NOK_LOGIN         6
#define NOK_WAIT_TIMEOUT  7
#define NOK_PROTOCOL      8
#define NOK_NODE          9
#define NOK_PHONE         10
#define NOK_REDIAL        11 
#define NOK_MAIL_TMP      12
#define NOK_ROOM          13
#define NOK_ROUTE         14
#define NOK_ALIAS         15
#define NOK_REGION        16

#ifdef MAIN
char *loginkeywords[] = 
{
    "S", "W", NULL     /* S = Send, W = Wait */
};
#else
extern char *loginkeywords[];
#endif /* MAIN */

/************************************************************************/
/*                         cron.cit keywords                            */
/************************************************************************/
#ifdef MAIN
char *cronkeywords[] = 
{
  "#DO",
  "#HOURS",
  "#DAYS",
  "#REDO_TIME",
  "#RETRY_TIME",
  "#PRIORITY",
  NULL  
};
#else
extern char *cronkeywords[];
#endif /* MAIN */

#define CR_DO            0
#define CR_HOURS         1
#define CR_DAYS          2
#define CR_REDO          3
#define CR_RETRY         4
#define CR_PRIORITY      5

#ifdef MAIN
char *crontypes[] = 
{
  "NETWORK",
  "SHELL_1",
  "SHELL_2",
  NULL
};
#else
extern char *crontypes[];
#endif

#define CR_NET      0
#define CR_SHELL_1  1
#define CR_SHELL_2  2

/* -------------------------------------------------------------------- */
/*                         Words for adventure                          */
/* -------------------------------------------------------------------- */
#ifdef MAIN
char *dirnams[] = 
{
    "North",
    "Sourth",
    "East",
    "West",
    "Up",
    "Down",
    NULL
};

int diropp[] = {0, 1, 3, 2, 5, 4};
#else
extern char *dirnams[];
extern int  diropp[];
#endif

#define DR_NORTH    0
#define DR_SOUTH    1
#define DR_EAST     2
#define DR_WEST     3
#define DR_UP       4
#define DR_DOWN     5

