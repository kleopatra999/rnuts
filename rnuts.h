/*****************************************************************************

                   лллллл   ллллл ллллл  ллллл ллллллллллл  ллллллллл
                  ААлллллл ААллл ААллл  ААллл АлАААлллАААл лллАААААллл
         лллллллл  АлллАллл Аллл  Аллл   Аллл А   Аллл  А Аллл    ААА
        ААлллААллл АлллААлллАллл  Аллл   Аллл     Аллл    ААллллллллл
         Аллл ААА  Аллл ААлллллл  Аллл   Аллл     Аллл     ААААААААллл
         Аллл      Аллл  ААллллл  Аллл   Аллл     Аллл     ллл    Аллл
         ллллл     ллллл  ААллллл ААлллллллл      ллллл   ААллллллллл
        ААААА     ААААА    ААААА   АААААААА      ААААА     ААААААААА
    a heavily modified form of NUTS version 3.3.3 (C)Neil Robertson 1996
       Neil Robertson                Email    : neil@ogham.demon.co.uk

                  rNUTS modifications by Enginerd & Slugger
                        Macros courtesy of KnightShade
                               VERSION 3.0.2

**************************** Header file rNUTS ******************************/

#define TALKERNAME "MysticDreams"  /* the name of the talker as printed in
code */
#define BOTNAME    "blinkin"
#define BOTSITE    "milliways.kvcinc.com"
#define MAINROOM   "clearing"
#define JAILROOM   "jail"
#define BLKJKROOM  "blackjack"
#define NERFROOM   "forest"
#define PURGEDAYS  120

#define LOGFILES     "logfiles"
#define FILTERLOG    "logfiles/filterlog"
#define PROMOLOG     "logfiles/promotelog"
#define BOOTLOG      "logfiles/bootlog"
#define NEWBIE       "datafiles/newusrfile"
#define ROOMFILES    "datafiles/roomfiles"
#define WIZLISTFILE  "wizlist"
#define RANKFILES    "rankfiles"
#define FAQFILES     "faqfiles"
#define HANGDICT     "datafiles/hang_words"
#define WHOHTML      "wholist"
#define WANBE        "faqfiles/wannabe"
#define DATAFILES    "datafiles"
#define USERFILES    "userfiles"
#define HELPFILES    "helpfiles"
#define MAILSPOOL    "mailspool"
#define CONFIGFILE   "config"
#define NEWSFILE     "newsfile"
#define MAPFILE      "mapfile"
#define SITEBAN      "siteban"
#define USERBAN      "userban"
#define SYSLOG       "logfiles/syslog"
#define MOTD1        "datafiles/motd1"
#define MOTD2        "datafiles/motd2"
#define PICTUREFILES "picfiles"
#define ACCOUNTLOG   "logfiles/accountlog"
#define SUGGESTIONS  "logfiles/suggestions"
#define BOARDFILES   "datafiles/boardfiles"
#define POKERFILE    "datafiles/pokerfile"

#define VERIFYCODE   "TR-684C."   /* followed by a random number */

char *board_words[]={"news","wiznote","admnote","*"};
int board_level[]={0,4,7};

#define NR_BOT_REPLIES 15
#define FILE_NAME_LEN  80
#define MAX_COPIES     20 /* of smail */

#define MAX_MORE_LINES 23
#define FIGHT_MESG_LEN 320
#define MAX_PLAYERS    5
#define OUT_BUFF_SIZE  1000
#define MAX_WORDS      200
#define WORD_LEN       40
#define ARR_SIZE       1500
#define MAX_LINES      15
#define NUM_COLS       21

#define COLOR_LEN      10
#define USER_NAME_LEN  11
#define USER_DESC_LEN  30
#define GEND_LEN       10
#define AGE_LEN        3
#define URL_LEN        45
#define EMAIL_LEN      45
#define AFK_MESG_LEN   60
#define PHRASE_LEN     50
#define PASS_LEN       20 /* the 1st 8 chars will be used by crypt() though */
#define BUFSIZE        1000
#define ROOM_NAME_LEN  20
#define ROOM_LABEL_LEN 5
#define ROOM_DESC_LEN  1000 /* 10 lines of 80 chars each + 10 nl */
#define TOPIC_LEN      35
#define MAX_LINKS      6
#define SERV_NAME_LEN  80
#define SITE_NAME_LEN  80
#define VERIFY_LEN     20
#define REVIEW_LINES   15
#define REVTELL_LINES  5
#define REVIEW_LEN     200
/* DNL (Date Number Length) will have to become 12 on Sun Sep 9 02:46:40 2001 
   when all the unix timers will flip to 1000000000 :) */
#define DNL            11 

#define PUBLIC         0
#define PRIVATE        1
#define FIXED          2
#define FIXED_PUBLIC   2
#define FIXED_PRIVATE  3
#define PERSONAL       5

#define ZER 0
#define ONE 1
#define TWO 2
#define THR 3
#define FOU 4
#define FIV 5
#define SIX 6
#define SEV 7
#define EIG 8
#define BOT 9

#define USER_TYPE          0
#define CLONE_TYPE         1
#define REMOTE_TYPE        2
#define BOT_TYPE           3   
#define CLONE_HEAR_NOTHING 0
#define CLONE_HEAR_SWEARS  1
#define CLONE_HEAR_ALL     2

enum melee_flag {NONE, FIGHTING, BRAWLING, GANG_RED, GANG_BLUE};

char fight_buff[FIGHT_MESG_LEN+2];

int blackjack_status;

/* The elements vis, ignall, prompt, command_mode etc could all be bits in 
   one flag variable as they're only ever 0 or 1, but I tried it and it
   made the code unreadable. Better to waste a few bytes */
struct user_struct {
	char name[USER_NAME_LEN+1];
	char desc[USER_DESC_LEN+1];
	char pass[PASS_LEN+6];
	char gend[GEND_LEN+1];
	char age[AGE_LEN+1];
	char url[URL_LEN+1];
	char email[EMAIL_LEN+1];
	char fwdaddy[EMAIL_LEN+1];
	char pref_rm[ROOM_NAME_LEN+1];
	char color[COLOR_LEN+1];
	char in_phrase[PHRASE_LEN+1],out_phrase[PHRASE_LEN+1];
	char buff[BUFSIZE],site[81],last_site[81],page_file[81];
	char mail_to[WORD_LEN+1],revbuff[REVTELL_LINES][REVIEW_LEN+2];
	char afk_mesg[AFK_MESG_LEN+1],inpstr_old[REVIEW_LEN+1];
	struct room_struct *room,*invite_room;
	int type,port,site_port,login,socket,attempts,buffpos,filepos;
	int vis,ignall,prompt,command_mode,muzzled,charmode_echo,jailed;
	int level,misc_op,remote_com,edit_line,charcnt,warned,hide_email;
	int accreq,last_login_len,ignall_store,clone_hear,afk,hide_url;
	int edit_op,colour,ignshout,igntell,revline,cursed,autoread;
	int first,home,hang_stage,mail_verified,autofwd,inedit;
	int blackjack_status,blackjack_total,wipe_from,wipe_to;
	time_t last_input,last_login,total_login,read_mail;
        char hang_word[WORD_LEN],hang_word_show[WORD_LEN];
	char hang_guess[WORD_LEN],copyto[MAX_COPIES][USER_NAME_LEN];
	char *malloc_start,*malloc_end,array[10];
	char verify_code[80];
	struct netlink_struct *netlink,*pot_netlink;
	struct user_struct *prev,*next,*owner,*opponent,*c4_opponent;
        struct macro_struct *macrolist; /* macro added May 8, 1997 KnightShade */
        struct ignore_struct *ignorelist; /* ignore added May, 1997 KnightShade */	
	struct poker_player *pop; /*** poker ***/
        enum melee_flag melee_status;
	int tic_win,tic_lose,tic_draw; /*added for tictac stats -- RobinHood */
	int hang_win,hang_lose;        /*added for hangman stats -- LittleJohn */
	int nerflife,nerfcharge,nerf_win,nerf_lose;
        char matchsite_check_store[ARR_SIZE],boardwrite[30];
        int matchsite_all_store;
	int board[8][7],turn;
	};

typedef struct user_struct* UR_OBJECT;
UR_OBJECT user_first,user_last,bot_bot;

struct room_struct {
	char name[ROOM_NAME_LEN+1];
	char label[ROOM_LABEL_LEN+1];
	char desc[ROOM_DESC_LEN+1];
	char topic[TOPIC_LEN+1];
	char revbuff[REVIEW_LINES][REVIEW_LEN+2];
	int inlink; /* 1 if room accepts incoming net links */
	int access; /* public , private etc */
	int revline; /* line number for review */
	int mesg_cnt;
	char netlink_name[SERV_NAME_LEN+1]; /* temp store for config parse */
	char link_label[MAX_LINKS][ROOM_LABEL_LEN+1]; /* temp store for parse */
	struct netlink_struct *netlink; /* for net links, 1 per room */
	struct room_struct *link[MAX_LINKS];
	struct room_struct *next,*prev;
	};

typedef struct room_struct *RM_OBJECT;
RM_OBJECT room_first,room_last;
RM_OBJECT create_room();

/* Netlink stuff */
#define UNCONNECTED 0 
#define INCOMING 1 
#define OUTGOING 2
#define DOWN 0
#define VERIFYING 1
#define UP 2
#define ALL 0
#define IN 1
#define OUT 2

/* Structure for net links, ie server initiates them */
struct netlink_struct {
	char service[SERV_NAME_LEN+1];
	char site[SITE_NAME_LEN+1];
	char verification[VERIFY_LEN+1];
	char buffer[ARR_SIZE*2];
	char mail_to[WORD_LEN+1];
	char mail_from[WORD_LEN+1];
	FILE *mailfile;
	time_t last_recvd; 
	int port,socket,type,connected;
	int stage,lastcom,allow,warned,keepalive_cnt;
	int ver_major,ver_minor,ver_patch;
	struct user_struct *mesg_user;
	struct room_struct *connect_room;
	struct netlink_struct *prev,*next;
	};

typedef struct netlink_struct *NL_OBJECT;
NL_OBJECT nl_first,nl_last;
NL_OBJECT create_netlink();

enum bool {FALSE, TRUE};
enum bool fighting, brawling, gangbash;
enum win_fl {DRW1, WIN1, WIN2, DRW2};

char *syserror="Sorry, a system error has occured";
char *nosuchroom="Sorry, that realm does not exist yet.....\n";
char *nosuchuser="Sorry, that Dreamer does not exist yet.....\n";
char *notloggedon="There is no Dreamer of that name logged on.\n";
char *invisenter="A Spirit enters this realm...\n";
char *invisleave="A Spirit leaves this realm.\n";
char *invisname="Spirit";
char *noswearing="We are not allowing swearing here.\n";

char *level_name[]={
"DRIFTER","DREAMER","POWERDREAMER","SPIRITGUIDE",
"SPIRIT","POWERSPIRIT","SPIRITGUARD","DREAMGUIDE",
"DREAMOWNER","BOT","*"
};

char *who_user[]={
"NEW ","DRMR","PWDR","SPGD","SPRT","PSRT","SPGD","DRGD","DMNR","BOT ","*"
};

char *who_wiz[]={
"DFTR","DRMR","PWDR","SPGD","SPRT","PSRT","SPGD","DRGD","DMNR","BOT ","*"
};


/*** Array for picture names. --RobinHood ***/
char *picnames[]={
"action","arm","babe","batman","beer","beer2","catgal","dragon","dream","finger",
"heart","lips","love","lust","mickey","panic","penguin","rose","sick","spam","teeth",
"woodstock","*"
};


char *command[]={
"quit",    "look",     "mode",      "say '",   "shout !",   "ctell",
"tell ,",  "emote ;",  "shemote #", "pemote /","echo -",    "force",
"go >",    "ignall",   "prompt",    "desc",    "inphr",     "motd",
"outphr",  "public",   "private",   "letmein", "invite",    "forwarding",
"topic",   "move",     "bcast",     "who @",   "people",    "faq",
"help ?",  "shutdown", "read",      "write",   "join",
"wipe",    "search",   "review <",  "home",    "status",    "bring",
"version", "rmail",    "smail",     "dmail",   "from",	    "last",
"entpro",  "examine &","rmst",      "rmsn",    "netstat",   "wizlist",
"netdata", "connect",  "disconnect","passwd",  "kill",      "actions",
"promote", "demote",   "listbans",  "ban",     "unban",     "sysaction",
"vis",     "invis",    "site",      "wake",    "wtell",     "wemote",
"muzzle",  "unmuzzle", "map",       "logging", "minlogin",  "delold",
"system",  "charecho", "clearline", "fix",     "unfix",     "show",
"viewlog", "accreq",   "cbuff",     "clone",   "destroy",   "arrest",
"myclones","allclones","switch",    "csay",    "chear",     "release",
"rstat",   "swban",    "afk",       "cls",     "colour",    "curse",
"ignshout","igntell",  "suicide",   "nuke",    "reboot",    "enchant",
"recount", "revtell",  "set",	    "ranks",   "revshout",  "matchsite",
"users",   "macro",    "sysmacro",  "time",    "rsuggest",  "cshout",
"suggest", "ignore",   "unignore",  "ulist",   "showignore","duel",
"picture", "throw",    "recent",    "reset",   "tictactoe", "cbot",
"qbot",    "bact",     "bmail",     "session", "comment",   "hangman",
"guess",   "copyto",   "nocopy",    "verify",  "pop",       "finger",
"nerf",    "charge",   "whois",     "wannabe", "nslookup",  "blackjack",
"poker",   "startpo",  "joinpo",    "leave",   "games",     "score",
"deal",    "fold",     "bet",	    "check",   "raise",     "see",
"discpo",  "hand",     "chips",     "casino",  "connect4",  "*"
};


/* Values of commands , used in switch in exec_com() */
enum comvals {
QUIT,     LOOK,     MODE,     SAY,    SHOUT,     CTELLS,
TELL,     EMOTE,    SEMOTE,   PEMOTE, ECHO,      FORCE,
GO,       IGNALL,   PROMPT,   DESC,   INPHRASE,  VIEWMOTD2,
OUTPHRASE,PUBCOM,   PRIVCOM,  LETMEIN,INVITE,    FORWARDING,
TOPIC,    MOVE,     BCAST,    WHO,    PEOPLE,    FAQ,
HELP,     SHUTDOWN, READ,     WRITE,  JOIN,
WIPE,     SEARCH,   REVIEW,   HOME,   STATUS,    BRING,
VER,      RMAIL,    SMAIL,    DMAIL,  FROM,      LAST,
ENTPRO,   EXAMINE,  RMST,     RMSN,   NETSTAT,   WIZZERLIST,
NETDATA,  CONN,     DISCONN,  PASSWD, KILL,      ACTIONCMD,
PROMOTE,  DEMOTE,   LISTBANS, BAN,    UNBAN,     SYSACTION,
VIS,      INVIS,    SITE,     WAKE,   WTELL,     WEMOTE,
MUZZLE,   UNMUZZLE, MAP,      LOGGING,MINLOGIN,  DELOLD,
SYSTEM,   CHARECHO, CLEARLINE,FIX,    UNFIX,     SHOWCOM,
VIEWLOG,  ACCREQ,   REVCLR,   CREATE, DESTROY,   ARREST,
MYCLONES, ALLCLONES,SWITCH,   CSAY,   CHEAR,     RELEASE,
RSTAT,    SWBAN,    AFK,      CLS,    COLOUR,    CURSE,
IGNSHOUT, IGNTELL,  SUICIDE,  NUKE,   REBOOT,    ENCHANT,
RECOUNT,  REVTELL,  SET,      RANKS,  REVSHOUT,  MATCH,
USERS,	  MACROCMD, SYSMACRO, TIMED,  RSUGGEST,  CSHOUT,
SUGGEST,  IGNORE,   UNIGNORE, LIST,   SHOWIGNORE,DUEL,  
PICTURE,  THROW,    RECENT,   RESET,  TICTAC,    CBOT,
QBOT,     BOTACT,   BMAIL,    SESSION,COMMENT,   HANGMAN,
GUESS,    COPYTO,   NOCOPIES, VERIFY, POP,       SYSFINGER,
NERF,	  CNERF,    WHOIS,    WANNABE,NSLOOKUP,  BLACKJACK,
POKER,    STARTPO,  JOINPO,   LEAVE,  GAMES,     SCORE,
DEAL,	  FOLD,     BET,      CHECK,  RAISE,     SEE,
DISCPO,   HAND,     CHIPS,    CASINO, CONNECT4
} com_num;


/* These are the minimum levels at which the commands can be executed. 
   Alter to suit. */
int com_level[]={
ZER,	ONE,	ZER,	ZER,	TWO,	ZER,
ZER,	ONE,	TWO,	ONE,	THR,	SEV,
ONE,	TWO,	ZER,	ZER,	ONE,	ZER,
ONE,	TWO,	TWO,	ONE,	TWO,	EIG,
ONE,	FOU,	FIV,	ZER,	FIV,	ONE,
ZER,	FIV,	ZER,	ONE,	TWO,
FOU,	ONE,	ONE,	ONE,	ONE,	TWO,
ZER,	ZER,	ONE,	ONE,	ONE,	ONE,
ONE,	ONE,	ONE,	FIV,	FIV,	ZER,
SIX,	EIG,	EIG,	ONE,	FIV,	ONE,
SIX,	SIX,	FIV,	SIX,	SIX,	SIX,
SIX,	SIX,	FOU,	TWO,	FOU,	FOU,
FIV,	FIV,	ONE,	EIG,	SEV,	EIG,
SIX,	ZER,	SIX,	SEV,	SEV,	ONE,
SEV,	ZER,	TWO,	SIX,	SIX,	SIX,
SIX,	TWO,	SIX,	SIX,	SIX,	SIX,
SIX,	SIX,	ONE,	ZER,	ONE,	THR,
TWO,	TWO,	ONE,	SEV,	EIG,	THR,
SEV,	ONE,	ZER,	ONE,	TWO,	SEV,
FOU,	ONE,	SIX,	ONE,	SEV,	THR,
ONE,	THR,	THR,	SIX,	THR,	ONE,
THR,	THR,	TWO,	ONE,	ONE,	SEV,
SEV,	SEV,	EIG,	ONE,	ONE,	ONE,
ONE,	SEV,	SEV,	ONE,	SEV,	EIG,
ONE,	ONE,	EIG,	ONE,	EIG,	ONE,
ONE,	ONE,	ONE,	ONE,	ONE,	ONE,
ONE,	ONE,	ONE,	ONE,	ONE,	ONE,
ONE,	ONE,	FIV,	ONE,	ONE
};


#define CAR 0
#define FIG 1
#define BOA 2

char *is_game_name[]={
"poker",   "startpo", "joinpo",   "games",     "score", "deal",
"fold",    "bet",     "check",    "raise",     "see",   "discpo",
"hand",    "casino",  "connect4", "blackjack", "nerf",  "charge",
"hangman", "guess",   "duel",     "tictactoe", "reset", "*"
};

int game_type_ref[]={
CAR,	CAR,	CAR,	CAR,	CAR,	CAR,
CAR,	CAR,	CAR,	CAR,	CAR,	CAR,
CAR,	CAR,	BOA,	CAR,	FIG,	FIG,
BOA,	BOA,	FIG,	BOA,	BOA
};

char *game_type[]={"CARD  ","COMBAT","BOARD "};

/* 
Colcode values equal the following:
RESET,BOLD,BLINK,REVERSE

Foreground & background colours in order..
BLACK,RED,GREEN,YELLOW/ORANGE,
BLUE,MAGENTA,TURQUIOSE,WHITE
*/

char *colcode[NUM_COLS]={
/* Standard stuff */
"\033[0m", "\033[1m", "\033[4m", "\033[5m", "\033[7m",
/* Foreground colour */
"\033[30m","\033[31m","\033[32m","\033[33m",
"\033[34m","\033[35m","\033[36m","\033[37m",
/* Background colour */
"\033[40m","\033[41m","\033[42m","\033[43m",
"\033[44m","\033[45m","\033[46m","\033[47m"
};

/* Codes used in a string to produce the colours when prepended with a '~' */
char *colcom[NUM_COLS]={
"RS","OL","UL","LI","RV",
"FK","FR","FG","FY",
"FB","FM","FT","FW",
"BK","BR","BG","BY",
"BB","BM","BT","BW"
};

char *month[12]={
"January","February","March","April","May","June",
"July","August","September","October","November","December"
};

char *day[7]={
"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

char *noyes1[]={ " NO","YES" };
char *noyes2[]={ "NO ","YES" };
char *offon[]={ "OFF","ON " };

/* These MUST be in lower case - the contains_swearing() function converts
   the string to be checked to lower case before it compares it against
   these. Also even if you dont want to ban any words you must keep the 
   star as the first element in the array. */
char *swear_words[]={
"fuck","shit","cunt","*"
};

char verification[SERV_NAME_LEN+1];
char text[ARR_SIZE*2];

char *word[MAX_WORDS];
char word_buffer[ARR_SIZE];
char punct_buffer[]="Z";   /* set an obvious default */   
char NULL_STRING[]="";     /* define a null string to point to */

char wrd[8][81];
char progname[40],confile[40];
time_t boot_time;
jmp_buf jmpvar;

struct ftr_struct {
    UR_OBJECT user;            /* pts to the fighter */
    struct ftr_struct *next;
};

struct card_struct {
      char card_type;
      int i;
      struct card_struct *next;
};

struct player_struct {
      UR_OBJECT user;
      int totalpoints,total,nocards,total2,ace;
      struct card_struct *cards;
      struct player_struct *next;
};

typedef struct card_struct *CRD_OBJECT;
typedef struct player_struct *PLR_OBJECT;

int port[3],listen_sock[3],wizport_level,minlogin_level;
int colour_def,password_echo,ignore_sigterm;
int max_users,max_clones,num_of_users,num_of_logins,heartbeat;
int login_idle_time,user_idle_time,config_line,word_count;
int tyear,tmonth,tday,tmday,twday,thour,tmin,tsec;
int mesg_life,system_logging,prompt_def,no_prompt;
int force_listen,gatecrash_level,min_private_users;
int ignore_mp_level,rem_user_maxlevel,rem_user_deflevel;
int destructed,mesg_check_hour,mesg_check_min,net_idle_time;
int keepalive_interval,auto_connect,ban_swearing,crash_action;
int time_out_afks,allow_caps_in_name,rs_countdown;
int charecho_def,time_out_maxlevel,nuts_talk_style;
int num_recent_users,forwarding;
int d;
int f;
char logged_users[11][80];
time_t rs_announce,rs_which;
UR_OBJECT rs_user;

/* extern char *sys_errlist[]; */
char *long_date();

char shoutbuff[REVIEW_LINES][REVIEW_LEN+2];
int sbuffline, num_of_home=20;

/* Vars for tictactoe with bot */
int game_with_bot;
int bot_move;
int bot_check;
char pos_array[9];
int game_on;

/* Vars for session */
char whosession[USER_NAME_LEN+1];
time_t sessiontime;
char thesession[101];

char *hanged[8]={
"~FY~OL+~RS~FY---~OL+  \n~FY|      \n~FY|~RS           ~OLWord:~RS %s\n~FY|~RS           ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS           ~OLWord:~RS %s\n~FY|~RS           ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",  
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS           ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS   |       ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS  /|       ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS  /|\\      ~OLLetters guessed:~RS %s\n~FY|~RS      \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS  /|\\      ~OLLetters guessed:~RS %s\n~FY|~RS  /   \n~FY|______\n",
"~FY~OL+~RS~FY---~OL+  \n~FY|   |  \n~FY|~RS   O       ~OLWord:~RS %s\n~FY|~RS  /|\\      ~OLLetters guessed:~RS %s\n~FY|~RS  / \\ \n~FY|______\n"
};

char *talker_signature=
"+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-+
| MysticDreamsII-- A fun, enjoyable, and friendly atmosphere for adults to attend |
| Please email fklopfe@sherwood.cybermac.net with ANY comments.  Thankx           |
+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n";

/**** Poker ****/
#define GAME_NAME_LEN 12
struct poker_player {
  int hand[5];
  int putin;
  int touched;
  int rank;
  struct casino_player_hist *hist;
  struct user_struct *user;
  struct poker_game *game;
  struct poker_player *prev, *next;
};

struct poker_game {
  char name[GAME_NAME_LEN+1];
  struct room_struct *room;
  struct poker_player *players;
  struct poker_player *dealer;
  struct poker_player *opened;
  int num_players;
  int num_raises;
  int in_players;
  int newdealer;
  int deck[52];
  int top_card;
  int state;
  int bet;
  int pot;
  struct poker_player *curr_player, *first_player, *last_player;
  struct poker_game *prev, *next;
};

struct poker_game *poker_game_first, *poker_game_last;
struct casino_player_hist {
  char name[USER_NAME_LEN+1];
  int total;
  int given;
};

struct casino_player_hist *casino_hist[50];
int max_casino_hist;

/******* Cards **********/
#define CARD_LENGTH 5
#define CARD_WIDTH  5
static char *cards[52][CARD_LENGTH]={
  {".---.",
   "|~OL2~RS  |",
   "| ~OLC~RS |",
   "|  ~OL2~RS|",
   "`---'"},
  {".---.",
   "|~OL3~RS  |",
   "| ~OLC~RS |",
   "|  ~OL3~RS|",
   "`---'"},
  {".---.",
   "|~OL4~RS  |",
   "| ~OLC~RS |",
   "|  ~OL4~RS|",
   "`---'"},
  {".---.",
   "|~OL5~RS  |",
   "| ~OLC~RS |",
   "|  ~OL5~RS|",
   "`---'"},
  {".---.",
   "|~OL6~RS  |",
   "| ~OLC~RS |",
   "|  ~OL6~RS|",
   "`---'"},
  {".---.",
   "|~OL7~RS  |",
   "| ~OLC~RS |",
   "|  ~OL7~RS|",
   "`---'"},
  {".---.",
   "|~OL8~RS  |",
   "| ~OLC~RS |",
   "|  ~OL8~RS|",
   "`---'"},
  {".---.",
   "|~OL9~RS  |",
   "| ~OLC~RS |",
   "|  ~OL9~RS|",
   "`---'"},
  {".---.",
   "|~OL10~RS |",
   "| ~OLC~RS |",
   "| ~OL10~RS|",
   "`---'"},
  {".---.",
   "|~OLJ~RS  |",
   "| ~OLC~RS |",
   "|  ~OLJ~RS|",
   "`---'"},
  {".---.",
   "|~OLQ~RS  |",
   "| ~OLC~RS |",
   "|  ~OLQ~RS|",
   "`---'"},
  {".---.",
   "|~OLK~RS  |",
   "| ~OLC~RS |",
   "|  ~OLK~RS|",
   "`---'"},
  {".---.",
   "|~OLA~RS  |",
   "| ~OLC~RS |",
   "|  ~OLA~RS|",
   "`---'"},

  {".---.",
   "|~FR2~RS  |",
   "| ~FRd~RS |",
   "|  ~FR2~RS|",
   "`---'"},
  {".---.",
   "|~FR3~RS  |",
   "| ~FRd~RS |",
   "|  ~FR3~RS|",
   "`---'"},
  {".---.",
   "|~FR4~RS  |",
   "| ~FRd~RS |",
   "|  ~FR4~RS|",
   "`---'"},
  {".---.",
   "|~FR5~RS  |",
   "| ~FRd~RS |",
   "|  ~FR5~RS|",
   "`---'"},
  {".---.",
   "|~FR6~RS  |",
   "| ~FRd~RS |",
   "|  ~FR6~RS|",
   "`---'"},
  {".---.",
   "|~FR7~RS  |",
   "| ~FRd~RS |",
   "|  ~FR7~RS|",
   "`---'"},
  {".---.",
   "|~FR8~RS  |",
   "| ~FRd~RS |",
   "|  ~FR8~RS|",
   "`---'"},
  {".---.",
   "|~FR9~RS  |",
   "| ~FRd~RS |",
   "|  ~FR9~RS|",
   "`---'"},
  {".---.",
   "|~FR10~RS |",
   "| ~FRd~RS |",
   "| ~FR10~RS|",
   "`---'"},
  {".---.",
   "|~FRJ~RS  |",
   "| ~FRd~RS |",
   "|  ~FRJ~RS|",
   "`---'"},
  {".---.",
   "|~FRQ~RS  |",
   "| ~FRd~RS |",
   "|  ~FRQ~RS|",
   "`---'"},
  {".---.",
   "|~FRK~RS  |",
   "| ~FRd~RS |",
   "|  ~FRK~RS|",
   "`---'"},
  {".---.",
   "|~FRA~RS  |",
   "| ~FRd~RS |",
   "|  ~FRA~RS|",
   "`---'"},
  {".---.",
   "|~FR2~RS  |",
   "| ~FRh~RS |",
   "|  ~FR2~RS|",
   "`---'"},
  {".---.",
   "|~FR3~RS  |",
   "| ~FRh~RS |",
   "|  ~FR3~RS|",
   "`---'"},
  {".---.",
   "|~FR4~RS  |",
   "| ~FRh~RS |",
   "|  ~FR4~RS|",
   "`---'"},
  {".---.",
   "|~FR5~RS  |",
   "| ~FRh~RS |",
   "|  ~FR5~RS|",
   "`---'"},
  {".---.",
   "|~FR6~RS  |",
   "| ~FRh~RS |",
   "|  ~FR6~RS|",
   "`---'"},
  {".---.",
   "|~FR7~RS  |",
   "| ~FRh~RS |",
   "|  ~FR7~RS|",
   "`---'"},
  {".---.",
   "|~FR8~RS  |",
   "| ~FRh~RS |",
   "|  ~FR8~RS|",
   "`---'"},
  {".---.",
   "|~FR9~RS  |",
   "| ~FRh~RS |",
   "|  ~FR9~RS|",
   "`---'"},
  {".---.",
   "|~FR10~RS |",
   "| ~FRh~RS |",
   "| ~FR10~RS|",
   "`---'"},
  {".---.",
   "|~FRJ~RS  |",
   "| ~FRh~RS |",
   "|  ~FRJ~RS|",
   "`---'"},
  {".---.",
   "|~FRQ~RS  |",
   "| ~FRh~RS |",
   "|  ~FRQ~RS|",
   "`---'"},
  {".---.",
   "|~FRK~RS  |",
   "| ~FRh~RS |",
   "|  ~FRK~RS|",
   "`---'"},
  {".---.",
   "|~FRA~RS  |",
   "| ~FRh~RS |",
   "|  ~FRA~RS|",
   "`---'"},

  {".---.",
   "|~OL2~RS  |",
   "| ~OLS~RS |",
   "|  ~OL2~RS|",
   "`---'"},
  {".---.",
   "|~OL3~RS  |",
   "| ~OLS~RS |",
   "|  ~OL3~RS|",
   "`---'"},
  {".---.",
   "|~OL4~RS  |",
   "| ~OLS~RS |",
   "|  ~OL4~RS|",
   "`---'"},
  {".---.",
   "|~OL5~RS  |",
   "| ~OLS~RS |",
   "|  ~OL5~RS|",
   "`---'"},
  {".---.",
   "|~OL6~RS  |",
   "| ~OLS~RS |",
   "|  ~OL6~RS|",
   "`---'"},
  {".---.",
   "|~OL7~RS  |",
   "| ~OLS~RS |",
   "|  ~OL7~RS|",
   "`---'"},
  {".---.",
   "|~OL8~RS  |",
   "| ~OLS~RS |",
   "|  ~OL8~RS|",
   "`---'"},
  {".---.",
   "|~OL9~RS  |",
   "| ~OLS~RS |",
   "|  ~OL9~RS|",
   "`---'"},
  {".---.",
   "|~OL10~RS |",
   "| ~OLS~RS |",
   "| ~OL10~RS|",
   "`---'"},
  {".---.",
   "|~OLJ~RS  |",
   "| ~OLS~RS |",
   "|  ~OLJ~RS|",
   "`---'"},
  {".---.",
   "|~OLQ~RS  |",
   "| ~OLS~RS |",
   "|  ~OLQ~RS|",
   "`---'"},
  {".---.",
   "|~OLK~RS  |",
   "| ~OLS~RS |",
   "|  ~OLK~RS|",
   "`---'"},
  {".---.",
   "|~OLA~RS  |",
   "| ~OLS~RS |",
   "|  ~OLA~RS|",
   "`---'"}
};

