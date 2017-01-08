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

                Email     : rnuts@milliways.kvcinc.com
             Homepage     : http://rnuts.cybermac.net/

*****************************************************************************/

#include <stdio.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>

#include "rnuts.h"
#include "objects/poker.c"
#include "objects/filter.c"
#include "objects/macro.h"
#include "objects/macro.c"
#include "objects/system.c"
#include "objects/who.c"
#include "objects/duel.h"
#include "objects/duel.c"
#include "objects/bot.h"
#include "objects/bot.c"
#include "objects/tic_bot.c"		
#include "objects/tictac.c"
#include "objects/hangman.c"
#include "objects/nerf.c"
#include "objects/curse.h"
#include "objects/session.c"
#include "objects/wholisthtml.c"
#include "objects/blackjack.c"

#define VERSION "3.3.3"
#define RVERSION "3.0.2"

/*** This function calls all the setup routines and also contains the
	main program loop ***/
main(argc,argv)
int argc;
char *argv[];
{
fd_set readmask; 
int i,len; 
char inpstr[ARR_SIZE];
char filename[80];
char *remove_first();
UR_OBJECT user,next;
NL_OBJECT nl;

char *tempname;
tempname=argv[0]+2;
sprintf(text,"utils/autorun.ex %s",tempname);
system(text);

strcpy(progname,argv[0]);
if (argc<2) strcpy(confile,CONFIGFILE);
else strcpy(confile,argv[1]);

/* Startup */
printf("\n-- BOOT : rNUTS Version %s server\n\n",RVERSION);
init_globals();
write_log("\n-- SYSTEM : SERVER BOOTING\n",0,3);
set_date_time();
init_signals();
load_and_parse_config();
init_sockets();
if (auto_connect) init_connections();
else printf("-- BOOT : Skipping connect stage.\n");
check_messages(NULL,1);

/* Run in background automatically. */
switch(fork()) {
	case -1: boot_exit(11);  /* fork failure */
	case  0: break; /* child continues */
	default: sleep(1); exit(0);  /* parent dies */
	}
reset_alarm();
printf("\n-- BOOT : Booted with PID %d \n\n",getpid());
sprintf(text,"-- SYSTEM : Booted successfully with PID %d %s\n\n",getpid(),long_date(1));
write_log(text,0,3);

/* added call to load system macros. May, 1997 -- KnightShade */
system_macrolist = NULL;
sprintf(filename,"%s/%s",DATAFILES,SYSMACROFILE);   
load_macros(&system_macrolist,filename);

system_actionlist = NULL;
sprintf(filename,"%s/%s",DATAFILES,SYSACTIONFILE);
load_macros(&system_actionlist,filename);

/**** Main program loop. *****/
setjmp(jmpvar); /* jump to here if we crash and crash_action = IGNORE */
while(1) {
	/* set up mask then wait */
	setup_readmask(&readmask);
	if (select(FD_SETSIZE,&readmask,0,0,0)==-1) continue;

	/* check for connection to listen sockets */
	for(i=0;i<3;++i) {
		if (FD_ISSET(listen_sock[i],&readmask)) 
			accept_connection(listen_sock[i],i);
		}

	/* Cycle through client-server connections to other talkers */
	for(nl=nl_first;nl!=NULL;nl=nl->next) {
		no_prompt=0;
		if (nl->type==UNCONNECTED || !FD_ISSET(nl->socket,&readmask)) 
			continue;
		/* See if remote site has disconnected */
		if (!(len=read(nl->socket,inpstr,sizeof(inpstr)-3))) {
			if (nl->stage==UP)
				sprintf(text,"NETLINK: Remote disconnect by %s.\n",nl->service);
			else sprintf(text,"NETLINK: Remote disconnect by site %s.\n",nl->site);
			write_syslog(text,1);
			sprintf(text,"~OLSYSTEM:~RS Lost link to %s in the %s.\n",nl->service,nl->connect_room->name);
			write_room(NULL,text);
			shutdown_netlink(nl);
			continue;
			}
		inpstr[len]='\0'; 
		exec_netcom(nl,inpstr);
		}

	/* Cycle through users. Use a while loop instead of a for because
	    user structure may be destructed during loop in which case we
	    may lose the user->next link. */
	user=user_first;
	while(user!=NULL) {
		next=user->next; /* store in case user object is destructed */
		/* If remote user or clone ignore */
		if (user->type!=USER_TYPE) {  user=next;  continue; }

		/* see if any data on socket else continue */
		if (!FD_ISSET(user->socket,&readmask)) { user=next;  continue; }
	
		/* see if client (eg telnet) has closed socket */
		inpstr[0]='\0';
		if (!(len=read(user->socket,inpstr,sizeof(inpstr)))) {
			disconnect_user(user);  user=next;
			continue;
			}
		/* ignore control code replies */
		if ((unsigned char)inpstr[0]==255) { user=next;  continue; }

		/* Deal with input chars. If the following if test succeeds we
		   are dealing with a character mode client so call function. */
		if (inpstr[len-1]>=32 || user->buffpos) {
			if (get_charclient_line(user,inpstr,len)) goto GOT_LINE;
			user=next;  continue;
			}
		else terminate(inpstr);

		GOT_LINE:
		no_prompt=0;  
		com_num=-1;
		force_listen=0; 
		destructed=0;
		user->buff[0]='\0';  
		user->buffpos=0;
		user->last_input=time(0);
		if (user->login) {
			login(user,inpstr);  user=next;  continue;  
			}

		/* If a dot on its own then execute last inpstr unless its a misc
		   op or the user is on a remote site */
		if (!user->misc_op) {
			if (!strcmp(inpstr,".") && user->inpstr_old[0]) {
				strcpy(inpstr,user->inpstr_old);
				sprintf(text,"%s\n",inpstr);
				write_user(user,text);
				}
			/* else save current one for next time */
			else {
				if (inpstr[0]) strncpy(user->inpstr_old,inpstr,REVIEW_LEN);
				}
			}

		/* Main input check */
		clear_words();
		word_count=wordfind(inpstr);
		if (user->afk) {
			if (user->afk==2) {
				if (!word_count) {  
					if (user->command_mode) prompt(user);
					user=next;  continue;  
					}
				if (strcmp((char *)crypt(word[0],"NU"),user->pass)) {
					write_user(user,"Incorrect password.\n"); 
					prompt(user);  user=next;  continue;
					}
				cls(user);
				write_user(user,"Session unlocked, you are no longer AFK.\n");
				}	
			else write_user(user,"You are no longer AFK.\n");  
			user->afk_mesg[0]='\0';
			if (user->vis) {
				sprintf(text,"%s comes back from being AFK.\n",user->name);
				write_room_except(user->room,text,user);
				}
			if (user->afk==2) {
				user->afk=0;  prompt(user);  user=next;  continue;
				}
			user->afk=0;
			}
		if (!word_count) {
			if (misc_ops(user,inpstr))  {  user=next;  continue;  }
			if (user->room==NULL) {
				sprintf(text,"ACT %s NL\n",user->name);
				write_sock(user->netlink->socket,text);
				}
			if (user->command_mode) prompt(user);
			user=next;  continue;
			}
		if (misc_ops(user,inpstr))  {  user=next;  continue;  }
		com_num=-1;


	        if (*inpstr) /* if we have any input, parse it */
                        parse(user,inpstr);

		if (!destructed) {
			if (user->room!=NULL)  prompt(user); 
			else {
				switch(com_num) {
					case -1  : /* Unknown command */
					case HOME:
					case QUIT:
					case MODE:
					case PROMPT: 
					case SUICIDE:
					case REBOOT:
					case SHUTDOWN: prompt(user);
					}
				}
			}
		user=next;
		}
	} /* end while */
}


/************ MAIN LOOP FUNCTIONS ************/

/*** Set up readmask for select ***/
setup_readmask(mask)
fd_set *mask;
{
UR_OBJECT user;
NL_OBJECT nl;
int i;

FD_ZERO(mask);
for(i=0;i<3;++i) FD_SET(listen_sock[i],mask);
/* Do users */
for (user=user_first;user!=NULL;user=user->next) 
	if (user->type==USER_TYPE) FD_SET(user->socket,mask);

/* Do client-server stuff */
for(nl=nl_first;nl!=NULL;nl=nl->next) 
	if (nl->type!=UNCONNECTED) FD_SET(nl->socket,mask);
}


/*** Accept incoming connections on listen sockets ***/
accept_connection(lsock,num)
int lsock,num;
{
UR_OBJECT user,create_user();
NL_OBJECT create_netlink();
char *get_ip_address(),site[80];
struct sockaddr_in acc_addr;
int accept_sock,size;

size=sizeof(struct sockaddr_in);
accept_sock=accept(lsock,(struct sockaddr *)&acc_addr,&size);
if (num==2) {
	accept_server_connection(accept_sock,acc_addr);  return;
	}
strcpy(site,get_ip_address(acc_addr));
if (site_banned(site)) {
	write_sock(accept_sock,"\n\rLogins from your site/domain are banned.\n\n\r");
	close(accept_sock);
	sprintf(text,"Attempted login from banned site %s.\n",site);
	write_syslog(text,1);
	return;
	}
more(NULL,accept_sock,MOTD1); /* send pre-login message */
if (num_of_users+num_of_logins>=max_users && !num) {
	write_sock(accept_sock,"\n\rSorry, the talker is full at the moment.\n\n\r");
	close(accept_sock);  
	return;
	}
if ((user=create_user())==NULL) {
	sprintf(text,"\n\r%s: unable to create session.\n\n\r",syserror);
	write_sock(accept_sock,text);
	close(accept_sock);  
	return;
	}
user->socket=accept_sock;
user->login=3;
user->last_input=time(0);
if (!num) user->port=port[0]; 
else {
	user->port=port[1];
	write_user(user,"** Wizport login **\n\n");
	}
strcpy(user->site,site);
user->site_port=(int)ntohs(acc_addr.sin_port);
echo_on(user);
if (system_logging) {
        sprintf(text,"~OL~FR-- login ~RS~OL: %s ~FR--~RS\n",user->site);
        write_level(SIX,1,text,NULL);  
        }
write_user(user,"User Nickname: ");
num_of_logins++;
}


/*** Get net address of accepted connection ***/
char *get_ip_address(acc_addr)
struct sockaddr_in acc_addr;
{
static char site[80];
struct hostent *host;

strcpy(site,(char *)inet_ntoa(acc_addr.sin_addr)); /* get number addr */
if ((host=gethostbyaddr((char *)&acc_addr.sin_addr,4,AF_INET))!=NULL)
	strcpy(site,host->h_name); /* copy name addr. */
strtolower(site);
return site;
}


/*** See if users site is banned ***/
site_banned(site)
char *site;
{
FILE *fp;
char line[82],filename[80];

sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%s",line);
while(!feof(fp)) {
	if (strstr(site,line)) {  fclose(fp);  return 1;  }
	fscanf(fp,"%s",line);
	}
fclose(fp);
return 0;
}


/*** See if user is banned ***/
user_banned(name)
char *name;
{
FILE *fp;
char line[82],filename[80];

sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%s",line);
while(!feof(fp)) {
	if (!strcmp(line,name)) {  fclose(fp);  return 1;  }
	fscanf(fp,"%s",line);
	}
fclose(fp);
return 0;
}


/*** Attempt to get '\n' terminated line of input from a character
     mode client else store data read so far in user buffer. ***/
get_charclient_line(user,inpstr,len)
UR_OBJECT user;
char *inpstr;
int len;
{
int l;

for(l=0;l<len;++l) {
	/* see if delete entered */
	if (inpstr[l]==8 || inpstr[l]==127) {
		if (user->buffpos) {
			user->buffpos--;
			if (user->charmode_echo) write_user(user,"\b \b");
			}
		continue;
		}
	user->buff[user->buffpos]=inpstr[l];
	/* See if end of line */
	if (inpstr[l]<32 || user->buffpos+2==ARR_SIZE) {
		terminate(user->buff);
		strcpy(inpstr,user->buff);
		if (user->charmode_echo) write_user(user,"\n");
		return 1;
		}
	user->buffpos++;
	}
if (user->charmode_echo
    && ((user->login!=2 && user->login!=1) || password_echo)) 
	write(user->socket,inpstr,len);
return 0;
}


/*** Put string terminate char. at first char < 32 ***/
terminate(str)
char *str;
{
int i;
for (i=0;i<ARR_SIZE;++i)  {
	if (*(str+i)<32) {  *(str+i)=0;  return;  } 
	}
str[i-1]=0;
}


/* wordfind: rewritten KnightShade May, 1997 */
/* This function allows us to have a large number of words in a
   sentence, without wasting huge amounts of memory, or worrying
   about dynamic allocation. It requires a buffer of space equal
   to the size of the original input line, and works by converting
   spaces to '\0's, and assigning the word elements apropriately.

   Since punctuation except for a '.' indicates a separate word,
   (to allow for macro expansions and stuff), we toss such a
   leading character into a punctuation string buffer, assign
   the first word to such a buffer, and proceed as usual */
        
/* global variables: word, word_buffer, punct_buffer, word_count */

int wordfind(char *input){
        char *cur_char;
        
        word_count = 0;
        strcpy(word_buffer,input);
   
        if (ispunct (word_buffer[0]) && word_buffer[0] != '.' ) {
                punct_buffer[0] = word_buffer[0];
                word[0] = punct_buffer;
                word_count++;
                cur_char = word_buffer+1;
        } else
                cur_char = word_buffer; /* point to start of sentence */
   
        while(word_count < MAX_WORDS) {
                while( *cur_char != '\0' && isspace(*cur_char) )   
                                *cur_char++; /* skip spaces */
               
		word[word_count++] = cur_char; /* point current word to start */
                
                while(! isspace(*cur_char) ) {
                        if (*cur_char++ == '\0')
                                return word_count; /* if hit end of string, quit */
                }
                
                *cur_char++ = '\0'; /* terminate string */
        }
        return word_count;
}
                
/* Re-written May 1997, KnightShade. */
/* This needed to be re-written to match the declarations of the word
   array */
/* global variables: word, word_buffer, punct_buffer, NULL_STRING, word_count */
clear_words()
{
int w;
for(w=0;w<MAX_WORDS;++w) word[w]=NULL_STRING; 
word_count=0;
word_buffer[0]='\0';
punct_buffer[0]='\0';
}              

/************ PARSE CONFIG FILE **************/

load_and_parse_config()
{
FILE *fp;
char line[81]; /* Should be long enough */
char c,filename[80];
int i,section_in,got_init,got_rooms;
RM_OBJECT rm1,rm2;
NL_OBJECT nl;

section_in=0;
got_init=0;
got_rooms=0;

sprintf(filename,"%s/%s",DATAFILES,confile);
printf("-- BOOT : Parsing config file \"%s\"...\n",filename);
if (!(fp=fopen(filename,"r"))) {
	perror("          RNUTS: Can't open config file");  boot_exit(1);
	}
/* Main reading loop */
config_line=0;
fgets(line,81,fp);
while(!feof(fp)) {
	config_line++;
	for(i=0;i<8;++i) wrd[i][0]='\0';
	sscanf(line,"%s %s %s %s %s %s %s %s",wrd[0],wrd[1],wrd[2],wrd[3],wrd[4],wrd[5],wrd[6],wrd[7]);
	if (wrd[0][0]=='#' || wrd[0][0]=='\0') {
		fgets(line,81,fp);  continue;
		}
	/* See if new section */
	if (wrd[0][strlen(wrd[0])-1]==':') {
		if (!strcmp(wrd[0],"INIT:")) section_in=1;
		else if (!strcmp(wrd[0],"ROOMS:")) section_in=2;
			else if (!strcmp(wrd[0],"SITES:")) section_in=3; 
				else {
					fprintf(stderr,"          RNUTS: Unknown section header on line %d.\n",config_line);
					fclose(fp);  boot_exit(1);
					}
		}
	switch(section_in) {
		case 1: parse_init_section();  got_init=1;  break;
		case 2: parse_rooms_section(); got_rooms=1; break;
		case 3: parse_sites_section(); break;
		default:
			fprintf(stderr,"          RNUTS: Section header expected on line %d.\n",config_line);
			boot_exit(1);
		}
	fgets(line,81,fp);
	}
fclose(fp);

/* See if required sections were present (SITES is optional) and if
   required parameters were set. */
if (!got_init) {
	fprintf(stderr,"          RNUTS: INIT section missing from config
file.\n");
	boot_exit(1);
	}
if (!got_rooms) {
	fprintf(stderr,"          RNUTS: ROOMS section missing from config file.\n");
	boot_exit(1);
	}
if (!verification[0]) {
	fprintf(stderr,"          RNUTS: Verification not set in config file.\n");
	boot_exit(1);
	}
if (!port[0]) {
	fprintf(stderr,"          RNUTS: Main port number not set in config file.\n");
	boot_exit(1);
	}
if (!port[1]) {
	fprintf(stderr,"          RNUTS: Wiz port number not set in config file.\n");
	boot_exit(1);
	}
if (!port[2]) {
	fprintf(stderr,"          RNUTS: Link port number not set in config file.\n");
	boot_exit(1);
	}
if (port[0]==port[1] || port[1]==port[2] || port[0]==port[2]) {
	fprintf(stderr,"          RNUTS: Port numbers must be unique.\n");
	boot_exit(1);
	}
if (room_first==NULL) {
	fprintf(stderr,"          RNUTS: No rooms configured in config file.\n");
	boot_exit(1);
	}

/* Parsing done, now check data is valid. Check room stuff first. */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
	for(i=0;i<MAX_LINKS;++i) {
		if (!rm1->link_label[i][0]) break;
		for(rm2=room_first;rm2!=NULL;rm2=rm2->next) {
			if (rm1==rm2) continue;
			if (!strcmp(rm1->link_label[i],rm2->label)) {
				rm1->link[i]=rm2;  break;
				}
			}
		if (rm1->link[i]==NULL) {
			fprintf(stderr,"          RNUTS: Room %s has undefined link label '%s'.\n",rm1->name,rm1->link_label[i]);
			boot_exit(1);
			}
		}
	}

/* Check external links */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
	for(nl=nl_first;nl!=NULL;nl=nl->next) {
		if (!strcmp(nl->service,rm1->name)) {
			fprintf(stderr,"          RNUTS: Service name %s is also the name of a room.\n",nl->service);
			boot_exit(1);
			}
		if (rm1->netlink_name[0] 
		    && !strcmp(rm1->netlink_name,nl->service)) {
			rm1->netlink=nl;  break;
			}
		}
	if (rm1->netlink_name[0] && rm1->netlink==NULL) {
		fprintf(stderr,"          RNUTS: Service name %s not defined for room %s.\n",rm1->netlink_name,rm1->name);
		boot_exit(1);
		}
	}

/* Load room descriptions */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
	sprintf(filename,"%s/%s.R",ROOMFILES,rm1->name);
	if (!(fp=fopen(filename,"r"))) {
		fprintf(stderr,"          RNUTS: Can't open description file for room %s.\n",rm1->name);
		sprintf(text,"ERROR: Couldn't open description file for room %s.\n",rm1->name);
		write_log(text,0,3);
		continue;
		}
	i=0;
	c=getc(fp);
	while(!feof(fp)) {
		if (i==ROOM_DESC_LEN) {
			fprintf(stderr,"          RNUTS: Description too long for room %s.\n",rm1->name);
			sprintf(text,"ERROR: Description too long for room %s.\n",rm1->name);
			write_log(text,0,3);
			break;
			}
		rm1->desc[i]=c;  
		c=getc(fp);  ++i;
		}
	rm1->desc[i]='\0';
	fclose(fp);
	}
}



/*** Parse init section ***/
parse_init_section()
{
static int in_section=0;
int op,val;
char *options[]={ 
"mainport","wizport","linkport","system_logging","minlogin_level","mesg_life",
"wizport_level","prompt_def","gatecrash_level","min_private","ignore_mp_level",
"rem_user_maxlevel","rem_user_deflevel","verification","mesg_check_time",
"max_users","heartbeat","login_idle_time","user_idle_time","password_echo",
"ignore_sigterm","auto_connect","max_clones","ban_swearing","crash_action",
"colour_def","time_out_afks","allow_caps_in_name","charecho_def",
"time_out_maxlevel","nuts_talk_style","*"
};

if (!strcmp(wrd[0],"INIT:")) { 
	if (++in_section>1) {
		fprintf(stderr,"          RNUTS: Unexpected INIT section header on line %d.\n",config_line);
		boot_exit(1);
		}
	return;
	}
op=0;
while(strcmp(options[op],wrd[0])) {
	if (options[op][0]=='*') {
		fprintf(stderr,"          RNUTS: Unknown INIT option on line %d.\n",config_line);
		boot_exit(1);
		}
	++op;
	}
if (!wrd[1][0]) {
	fprintf(stderr,"          RNUTS: Required parameter missing on line %d.\n",config_line);
	boot_exit(1);
	}
if (wrd[2][0] && wrd[2][0]!='#') {
	fprintf(stderr,"          RNUTS: Unexpected word following init parameter on line %d.\n",config_line);
	boot_exit(1);
	}
val=atoi(wrd[1]);
switch(op) {
	case 0: /* main port */
	case 1:
	case 2:
	if ((port[op]=val)<1 || val>65535) {
		fprintf(stderr,"          RNUTS: Illegal port number on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 3:  
	if ((system_logging=onoff_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: System_logging must be ON or OFF on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 4:
	if ((minlogin_level=get_level(wrd[1]))==-1) {
		if (strcmp(wrd[1],"NONE")) {
			fprintf(stderr,"          RNUTS: Unknown level specifier for minlogin_level on line %d.\n",config_line);
			boot_exit(1);	
			}
		minlogin_level=-1;
		}
	return;

	case 5:  /* message lifetime */
	if ((mesg_life=val)<1) {
		fprintf(stderr,"          RNUTS: Illegal message lifetime on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 6: /* wizport_level */
	if ((wizport_level=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for wizport_level on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 7: /* prompt defaults */
	if ((prompt_def=onoff_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Prompt_def must be ON or OFF on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 8: /* gatecrash level */
	if ((gatecrash_level=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for gatecrash_level on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 9:
	if (val<1) {
		fprintf(stderr,"          RNUTS: Number too low for min_private_users on line %d.\n",config_line);
 		boot_exit(1);
		}
	min_private_users=val;
	return;

	case 10:
	if ((ignore_mp_level=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for ignore_mp_level on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 11: 
	/* Max level a remote user can remotely log in if he doesn't have a local
	   account. ie if level set to WIZ a GOD can only be a WIZ if logging in 
	   from another server unless he has a local account of level GOD */
	if ((rem_user_maxlevel=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for rem_user_maxlevel on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 12:
	/* Default level of remote user who does not have an account on site and
	   connection is from a server of version 3.3.0 or lower. */
	if ((rem_user_deflevel=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for rem_user_deflevel on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 13:
	if (strlen(wrd[1])>VERIFY_LEN) {
		fprintf(stderr,"          RNUTS: Verification too long on line %d.\n",config_line);
		boot_exit(1);	
		}
	strcpy(verification,wrd[1]);
	return;

	case 14: /* mesg_check_time */
	if (wrd[1][2]!=':'
	    || strlen(wrd[1])>5
	    || !isdigit(wrd[1][0]) 
	    || !isdigit(wrd[1][1])
	    || !isdigit(wrd[1][3]) 
	    || !isdigit(wrd[1][4])) {
		fprintf(stderr,"          RNUTS: Invalid message check time on line %d.\n",config_line);
		boot_exit(1);
		}
	sscanf(wrd[1],"%d:%d",&mesg_check_hour,&mesg_check_min);
	if (mesg_check_hour>23 || mesg_check_min>59) {
		fprintf(stderr,"          RNUTS: Invalid message check time on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 15:
	if ((max_users=val)<1) {
		fprintf(stderr,"          RNUTS: Invalid value for max_users on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 16:
	if ((heartbeat=val)<1) {
		fprintf(stderr,"          RNUTS: Invalid value for heartbeat on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 17:
	if ((login_idle_time=val)<10) {
		fprintf(stderr,"          RNUTS: Invalid value for login_idle_time on line %d.\n",config_line);
 		boot_exit(1);
		}
	return;

	case 18:
	if ((user_idle_time=val)<10) {
		fprintf(stderr,"          RNUTS: Invalid value for user_idle_time on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 19: 
	if ((password_echo=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Password_echo must be YES or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 20: 
	if ((ignore_sigterm=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Ignore_sigterm must be YES or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 21:
	if ((auto_connect=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Auto_connect must be YES  or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 22:
	if ((max_clones=val)<0) {
		fprintf(stderr,"          RNUTS: Invalid value for max_clones on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 23:
	if ((ban_swearing=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Ban_swearing must be YES or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 24:
	if (!strcmp(wrd[1],"NONE")) crash_action=0;
	else if (!strcmp(wrd[1],"IGNORE")) crash_action=1;
		else if (!strcmp(wrd[1],"REBOOT")) crash_action=2;
			else {
				fprintf(stderr,"          RNUTS: Crash_action must be NONE, IGNORE or REBOOT on line %d.\n",config_line);
				boot_exit(1);
				}
	return;

	case 25:
	if ((colour_def=onoff_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Colour_def must be ON or OFF on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 26:
	if ((time_out_afks=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Time_out_afks must be YES or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 27:
	if ((allow_caps_in_name=yn_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Allow_caps_in_name must be YES or NO on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 28:
	if ((charecho_def=onoff_check(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Charecho_def must be ON or OFF on line %d.\n",config_line);
		boot_exit(1);
		}
	return;

	case 29:
	if ((time_out_maxlevel=get_level(wrd[1]))==-1) {
		fprintf(stderr,"          RNUTS: Unknown level specifier for time_out_maxlevel on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;

	case 30:
	if (!strcmp(wrd[1],"NEW")) nuts_talk_style=1;
	else if (!strcmp(wrd[1],"OLD")) nuts_talk_style=0;
	else {
		fprintf(stderr,"          RNUTS: Unknown say type specifier for nuts_talk_style on line %d.\n",config_line);
		boot_exit(1);	
		}
	return;
	}
}



/*** Parse rooms section ***/
parse_rooms_section()
{
static int in_section=0;
int i;
char *ptr1,*ptr2,c;
RM_OBJECT room;

if (!strcmp(wrd[0],"ROOMS:")) {
	if (++in_section>1) {
		fprintf(stderr,"          RNUTS: Unexpected ROOMS section header on line %d.\n",config_line);
		boot_exit(1);
		}
	return;
	}
if (!wrd[2][0]) {
	fprintf(stderr,"          RNUTS: Required parameter(s) missing on line %d.\n",config_line);
	boot_exit(1);
	}
if (strlen(wrd[0])>ROOM_LABEL_LEN) {
	fprintf(stderr,"          RNUTS: Room label too long on line %d.\n",config_line);
	boot_exit(1);
	}
if (strlen(wrd[1])>ROOM_NAME_LEN) {
	fprintf(stderr,"          RNUTS: Room name too long on line %d.\n",config_line);
	boot_exit(1);
	}
/* Check for duplicate label or name */
for(room=room_first;room!=NULL;room=room->next) {
	if (!strcmp(room->label,wrd[0])) {
		fprintf(stderr,"          RNUTS: Duplicate room label on line %d.\n",config_line);
		boot_exit(1);
		}
	if (!strcmp(room->name,wrd[1])) {
		fprintf(stderr,"          RNUTS: Duplicate room name on line %d.\n",config_line);
		boot_exit(1);
		}
	}
room=create_room();
strcpy(room->label,wrd[0]);
strcpy(room->name,wrd[1]);

/* Parse internal links bit ie hl,gd,of etc. MUST NOT be any spaces between
   the commas */
i=0;
ptr1=wrd[2];
ptr2=wrd[2];
while(1) {
	while(*ptr2!=',' && *ptr2!='\0') ++ptr2;
	if (*ptr2==',' && *(ptr2+1)=='\0') {
		fprintf(stderr,"          RNUTS: Missing link label on line %d.\n",config_line);
		boot_exit(1);
		}
	c=*ptr2;  *ptr2='\0';
	if (!strcmp(ptr1,room->label)) {
		fprintf(stderr,"          RNUTS: Room has a link to itself on line %d.\n",config_line);
		boot_exit(1);
		}
	strcpy(room->link_label[i],ptr1);
	if (c=='\0') break;
	if (++i>=MAX_LINKS) {
		fprintf(stderr,"          RNUTS: Too many links on line %d.\n",config_line);
		boot_exit(1);
		}
	*ptr2=c;
	ptr1=++ptr2;  
	}

/* Parse access privs */
if (wrd[3][0]=='#') {  room->access=PUBLIC;  return;  }
if (!wrd[3][0] || !strcmp(wrd[3],"BOTH")) room->access=PUBLIC; 
else if (!strcmp(wrd[3],"PUB")) room->access=FIXED_PUBLIC; 
	else if (!strcmp(wrd[3],"PRIV")) room->access=FIXED_PRIVATE;
		else {
			fprintf(stderr,"          RNUTS: Unknown room access type on line %d.\n",config_line);
			boot_exit(1);
			}
/* Parse external link stuff */
if (!wrd[4][0] || wrd[4][0]=='#') return;
if (!strcmp(wrd[4],"ACCEPT")) {  
	if (wrd[5][0] && wrd[5][0]!='#') {
		fprintf(stderr,"          RNUTS: Unexpected word following ACCEPT keyword on line %d.\n",config_line);
		boot_exit(1);
		}
	room->inlink=1;  
	return;
	}
if (!strcmp(wrd[4],"CONNECT")) {
	if (!wrd[5][0]) {
		fprintf(stderr,"          RNUTS: External link name missing on line %d.\n",config_line);
		boot_exit(1);
		}
	if (wrd[6][0] && wrd[6][0]!='#') {
		fprintf(stderr,"          RNUTS: Unexpected word following external link name on line %d.\n",config_line);
		boot_exit(1);
		}
	strcpy(room->netlink_name,wrd[5]);
	return;
	}
fprintf(stderr,"          RNUTS: Unknown connection option on line %d.\n",config_line);
boot_exit(1);
}



/*** Parse sites section ***/
parse_sites_section()
{
NL_OBJECT nl;
static int in_section=0;

if (!strcmp(wrd[0],"SITES:")) { 
	if (++in_section>1) {
		fprintf(stderr,"          RNUTS: Unexpected SITES section header on line %d.\n",config_line);
		boot_exit(1);
		}
	return;
	}
if (!wrd[3][0]) {
	fprintf(stderr,"          RNUTS: Required parameter(s) missing on line %d.\n",config_line);
	boot_exit(1);
	}
if (strlen(wrd[0])>SERV_NAME_LEN) {
	fprintf(stderr,"          RNUTS: Link name length too long on line %d.\n",config_line);
	boot_exit(1);
	}
if (strlen(wrd[3])>VERIFY_LEN) {
	fprintf(stderr,"          RNUTS: Verification too long on line %d.\n",config_line);
	boot_exit(1);
	}
if ((nl=create_netlink())==NULL) {
	fprintf(stderr,"          RNUTS: Memory allocation failure creating netlink on line %d.\n",config_line);
 	boot_exit(1);
	}
if (!wrd[4][0] || wrd[4][0]=='#' || !strcmp(wrd[4],"ALL")) nl->allow=ALL;
else if (!strcmp(wrd[4],"IN")) nl->allow=IN;
	else if (!strcmp(wrd[4],"OUT")) nl->allow=OUT;
		else {
			fprintf(stderr,"          RNUTS: Unknown netlink access type on line %d.\n",config_line);
			boot_exit(1);
			}
if ((nl->port=atoi(wrd[2]))<1 || nl->port>65535) {
	fprintf(stderr,"          RNUTS: Illegal port number on line %d.\n",config_line);
	boot_exit(1);
	}
strcpy(nl->service,wrd[0]);
strtolower(wrd[1]);
strcpy(nl->site,wrd[1]);
strcpy(nl->verification,wrd[3]);
}


yn_check(wd)
char *wd;
{
if (!strcmp(wd,"YES")) return 1;
if (!strcmp(wd,"NO")) return 0;
return -1;
}


onoff_check(wd)
char *wd;
{
if (!strcmp(wd,"ON")) return 1;
if (!strcmp(wd,"OFF")) return 0;
return -1;
}


/************ INITIALISATION FUNCTIONS *************/

/*** Initialise globals ***/
init_globals()
{
fighting = FALSE;  /* No fight */

player_turn = NULL;      /* init lists -Slugger */
shuffle_deck = NULL;
num_players = 0; 
players = NULL;
blackjack_status = 0;   /* No current game. -Slugger */

verification[0]='\0';
port[0]=0;
port[1]=0;
port[2]=0;
auto_connect=1;
max_users=50;
max_clones=1;
ban_swearing=0;
heartbeat=2;
keepalive_interval=60; /* DO NOT TOUCH!!! */
net_idle_time=300; /* Must be > than the above */
login_idle_time=180;
user_idle_time=300;
time_out_afks=0;
nuts_talk_style=1;
wizport_level=FIV;
minlogin_level=-1;
mesg_life=1;
no_prompt=0;
num_of_users=0;
num_of_logins=0;
num_recent_users=0;
system_logging=1;
password_echo=0;
ignore_sigterm=0;
crash_action=2;
prompt_def=1;
colour_def=1;
charecho_def=0;
time_out_maxlevel=THR;
mesg_check_hour=0;
mesg_check_min=0;
allow_caps_in_name=1;
rs_countdown=0;
rs_announce=0;
rs_which=-1;
rs_user=NULL;
gatecrash_level=SEV+1; /* minimum user level which can enter private
rooms */
min_private_users=2; /* minimum num. of users in room before can set to priv */
ignore_mp_level=SEV; /* User level which can ignore the above var. */
rem_user_maxlevel=ONE;
rem_user_deflevel=THR;
user_first=NULL;
user_last=NULL;
room_first=NULL;
room_last=NULL; /* This variable isn't used yet */
nl_first=NULL;
nl_last=NULL;
poker_game_first=NULL; /*** POKER ***/
poker_game_last=NULL;  /*** POKER ***/
clear_words();
time(&boot_time);
forwarding=1;

for (f=0;f==11;f++) {
	strcpy(logged_users[f],'\0');
	}
}


/*** Initialise the signal traps etc ***/
init_signals()
{
void sig_handler();

signal(SIGTERM,sig_handler);
signal(SIGSEGV,sig_handler);
signal(SIGBUS,sig_handler);
signal(SIGILL,SIG_IGN);
signal(SIGTRAP,SIG_IGN);
signal(SIGIOT,SIG_IGN);
signal(SIGTSTP,SIG_IGN);
signal(SIGCONT,SIG_IGN);
signal(SIGHUP,SIG_IGN);
signal(SIGINT,SIG_IGN);
signal(SIGQUIT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);
signal(SIGPIPE,SIG_IGN);
signal(SIGTTIN,SIG_IGN);
signal(SIGTTOU,SIG_IGN);
}


/*** Talker signal handler function. Can either shutdown , ignore or reboot
	if a unix error occurs though if we ignore it we're living on borrowed
	time as usually it will crash completely after a while anyway. ***/
void sig_handler(sig)
int sig;
{
force_listen=1;
switch(sig) {
	case SIGTERM:
	if (ignore_sigterm) {
		write_syslog("SIGTERM signal received - ignoring.\n",1);
		return;
		}
	write_room(NULL,"\n\n~OLSYSTEM:~FR~LI SIGTERM received, initiating shutdown!\n\n");
	talker_shutdown(NULL,"a termination signal (SIGTERM)",0); 

	case SIGSEGV:
	switch(crash_action) {
		case 0:	
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault, initiating shutdown!\n\n");
		talker_shutdown(NULL,"a segmentation fault (SIGSEGV)",0); 

		case 1:	
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI WARNING - A segmentation fault has just occured!\n\n");
		write_syslog("WARNING: A segmentation fault occured!\n",1);
		longjmp(jmpvar,0);

		case 2:
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault, initiating reboot!\n\n");
		talker_shutdown(NULL,"a segmentation fault (SIGSEGV)",1); 
		}

	case SIGBUS:
	switch(crash_action) {
		case 0:
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error, initiating shutdown!\n\n");
		talker_shutdown(NULL,"a bus error (SIGBUS)",0);

		case 1:
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI WARNING - A bus error has just occured!\n\n");
		write_syslog("WARNING: A bus error occured!\n",1);
		longjmp(jmpvar,0);

		case 2:
		write_room(NULL,"\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error, initiating reboot!\n\n");
		talker_shutdown(NULL,"a bus error (SIGBUS)",1);
		}
	}
}

	
/*** Initialise sockets on ports ***/
init_sockets()
{
struct sockaddr_in bind_addr;
int i,on,size;

printf("-- BOOT : Initialising sockets on ports: %d, %d, %d\n",port[0],port[1],port[2]);
on=1;
size=sizeof(struct sockaddr_in);
bind_addr.sin_family=AF_INET;
bind_addr.sin_addr.s_addr=INADDR_ANY;
for(i=0;i<3;++i) {
	/* create sockets */
	if ((listen_sock[i]=socket(AF_INET,SOCK_STREAM,0))==-1) boot_exit(i+2);

	/* allow reboots on port even with TIME_WAITS */
	setsockopt(listen_sock[i],SOL_SOCKET,SO_REUSEADDR,(char *)&on,sizeof(on));

	/* bind sockets and set up listen queues */
	bind_addr.sin_port=htons(port[i]);
	if (bind(listen_sock[i],(struct sockaddr *)&bind_addr,size)==-1) 
		boot_exit(i+5);
	if (listen(listen_sock[i],10)==-1) boot_exit(i+8);

	/* Set to non-blocking , do we need this? Not really. */
	fcntl(listen_sock[i],F_SETFL,O_NDELAY);
	}
}


/*** Initialise connections to remote servers. Basically this tries to connect
     to the services listed in the config file and it puts the open sockets in 
	the NL_OBJECT linked list which the talker then uses ***/
init_connections()
{
NL_OBJECT nl;
RM_OBJECT rm;
int ret,cnt=0;

printf("-- BOOT : Connecting to remote servers...\n");
errno=0;
for(rm=room_first;rm!=NULL;rm=rm->next) {
	if ((nl=rm->netlink)==NULL) continue;
	++cnt;
	printf("          Trying service %s at %s %d: ",nl->service,nl->site,nl->port);
	fflush(stdout);
	if ((ret=connect_to_site(nl))) {
		if (ret==1) {
			printf("%s.\n",sys_errlist[errno]);
			sprintf(text,"NETLINK: Failed to connect to %s: %s.\n",nl->service,sys_errlist[errno]);
			}
		else {
			printf("          Unknown hostname.\n");
			sprintf(text,"NETLINK: Failed to connect to %s: Unknown hostname.\n",nl->service);
			}
		write_syslog(text,1);
		}
	else {
		printf("          CONNECTED.\n");
		sprintf(text,"NETLINK: Connected to %s (%s %d).\n",nl->service,nl->site,nl->port);
		write_syslog(text,1);
		nl->connect_room=rm;
		}
	}
if (cnt) printf("          See system log for any further information.\n");
else printf("          No remote connections configured.\n");
}


/*** Do the actual connection ***/
connect_to_site(nl)
NL_OBJECT nl;
{
struct sockaddr_in con_addr;
struct hostent *he;
int inetnum;
char *sn;

sn=nl->site;
/* See if number address */
while(*sn && (*sn=='.' || isdigit(*sn))) sn++;

/* Name address given */
if(*sn) {
	if(!(he=gethostbyname(nl->site))) return 2;
	memcpy((char *)&con_addr.sin_addr,he->h_addr,(size_t)he->h_length);
	}
/* Number address given */
else {
	if((inetnum=inet_addr(nl->site))==-1) return 1;
	memcpy((char *)&con_addr.sin_addr,(char *)&inetnum,(size_t)sizeof(inetnum));
	}
/* Set up stuff and disable interrupts */
if ((nl->socket=socket(AF_INET,SOCK_STREAM,0))==-1) return 1;
con_addr.sin_family=AF_INET;
con_addr.sin_port=htons(nl->port);
signal(SIGALRM,SIG_IGN);

/* Attempt the connect. This is where the talker may hang. */
if (connect(nl->socket,(struct sockaddr *)&con_addr,sizeof(con_addr))==-1) {
	reset_alarm();  return 1;
	}
reset_alarm();
nl->type=OUTGOING;
nl->stage=VERIFYING;
nl->last_recvd=time(0);
return 0;
}

	

/************* WRITE FUNCTIONS ************/

/*** Write a NULL terminated string to a socket ***/
write_sock(sock,str)
int sock;
char *str;
{
write(sock,str,strlen(str));
}


/*** Write a string to system log ***/ 
write_log(str,write_time,log)
char *str;
int write_time;
int log;
{
char filename[80];
FILE *fp;

if (log==4) strcpy(filename,PROMOLOG);
if (log==3) strcpy(filename,BOOTLOG);        
if (log==2) strcpy(filename,FILTERLOG);
if (log==1) strcpy(filename,ACCOUNTLOG);
if (log==0) strcpy(filename,SYSLOG);

if (!system_logging || !(fp=fopen(filename,"a"))) return;
if (!write_time) fputs(str,fp);
else fprintf(fp,"%02d/%02d %02d:%02d:%02d: %s",tmday,tmonth+1,thour,tmin,tsec,str);
fclose(fp);
}

/*** Subsid function to below but this one is used the most ***/
write_syslog(str,write_time)
char *str;              
int write_time;
{                               
write_log(str,write_time,0);         
}


/*** Send message to user ***/
write_user(user,str)
UR_OBJECT user;
char *str;
{
int buffpos,sock,i;
char *start,buff[OUT_BUFF_SIZE],mesg[ARR_SIZE],*colour_com_strip();

if (user==NULL) return;
if (user->type==REMOTE_TYPE) {
	if (user->netlink->ver_major<=3 
	    && user->netlink->ver_minor<2) str=colour_com_strip(str);
	if (str[strlen(str)-1]!='\n') 
		sprintf(mesg,"MSG %s\n%s\nEMSG\n",user->name,str);
	else sprintf(mesg,"MSG %s\n%sEMSG\n",user->name,str);
	write_sock(user->netlink->socket,mesg);
	return;
	}
start=str;
buffpos=0;
sock=user->socket;
/* Process string and write to buffer. We use pointers here instead of arrays 
   since these are supposedly much faster (though in reality I guess it depends
   on the compiler) which is necessary since this routine is used all the 
   time. */
while(*str) {
	if (*str=='\n') {
		if (buffpos>OUT_BUFF_SIZE-6) {
			write(sock,buff,buffpos);  buffpos=0;
			}
		/* Reset terminal before every newline */
		if (user->colour) {
			memcpy(buff+buffpos,"\033[0m",4);  buffpos+=4;
			}
		*(buff+buffpos)='\n';  *(buff+buffpos+1)='\r';  
		buffpos+=2;  ++str;
		}
	else {  
		/* See if its a / before a ~ , if so then we print colour command
		   as text */
		if (*str=='/' && *(str+1)=='~') {  ++str;  continue;  }
		if (str!=start && *str=='~' && *(str-1)=='/') {
			*(buff+buffpos)=*str;  goto CONT;
			}
		/* Process colour commands eg ~FR. We have to strip out the commands 
		   from the string even if user doesnt have colour switched on hence 
		   the user->colour check isnt done just yet */
		if (*str=='~') {
			if (buffpos>OUT_BUFF_SIZE-6) {
				write(sock,buff,buffpos);  buffpos=0;
				}
			++str;
			for(i=0;i<NUM_COLS;++i) {
				if (!strncmp(str,colcom[i],2)) {
					if (user->colour) {
						memcpy(buff+buffpos,colcode[i],strlen(colcode[i]));
						buffpos+=strlen(colcode[i])-1;  
						}
					else buffpos--;
					++str;
					goto CONT;
					}
				}
			*(buff+buffpos)=*(--str);
			}
		else *(buff+buffpos)=*str;
		CONT:
		++buffpos;   ++str; 
		}
	if (buffpos==OUT_BUFF_SIZE) {
		write(sock,buff,OUT_BUFF_SIZE);  buffpos=0;
		}
	}
if (buffpos) write(sock,buff,buffpos);
/* Reset terminal at end of string */
if (user->colour) write_sock(sock,"\033[0m"); 
}



/*** Write to users of level 'level' and above or below depending on above
     variable; if 1 then above else below ***/
write_level(level,above,str,user)
int level,above;
char *str;
UR_OBJECT user;
{
UR_OBJECT u;

for(u=user_first;u!=NULL;u=u->next) {
	if (u!=user && !u->login && u->type!=CLONE_TYPE) {
		if ((above && u->level>=level) || (!above && u->level<=level)) 
			write_user(u,str);
		}
	}
}



/*** Subsid function to below but this one is used the most ***/
write_room(rm,str)
RM_OBJECT rm;
char *str;
{
write_room_except(rm,str,NULL);
}



/*** Write to everyone in room rm except for "user". If rm is NULL write 
     to all rooms. ***/
write_room_except(rm,str,user)
RM_OBJECT rm;
char *str;
UR_OBJECT user;
{
UR_OBJECT u;
char text2[ARR_SIZE];

for(u=user_first;u!=NULL;u=u->next) {
	if (u->login 
	    || u->room==NULL 
	    || (u->room!=rm && rm!=NULL) 
	    || (u->ignall && !force_listen)
	    || (u->ignshout && (com_num==SHOUT || com_num==SEMOTE))
	    || u==user) continue;
	if (u->type==CLONE_TYPE) {
		if (u->clone_hear==CLONE_HEAR_NOTHING || u->owner->ignall) continue;
		/* Ignore anything not in clones room, eg shouts, system messages
		   and semotes since the clones owner will hear them anyway. */
		if (rm!=u->room) continue;
		if (u->clone_hear==CLONE_HEAR_SWEARS) {
			if (!contains_swearing(str)) continue;
			}
		sprintf(text2,"~FT[ %s ]:~RS %s",u->room->name,str);
		write_user(u->owner,text2);
		}
	else write_user(u,str); 
	}
}




/******** LOGIN/LOGOUT FUNCTIONS ********/

/*** Login function. Lots of nice inline code :) ***/
login(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
int i, ret;
char name[ARR_SIZE],passwd[ARR_SIZE],agrees[ARR_SIZE];
char filename[80];

name[0]='\0';  passwd[0]='\0';
switch(user->login) {
	case 3:
	sscanf(inpstr,"%s",name);
	if(name[0]<33) {
		write_user(user,"\nUser Nickname: ");  return;
		}
	if (!strcmp(name,"quit")) {
		write_user(user,"\n\n*** Abandoning login attempt ***\n\n");
		if (system_logging) {
        		sprintf(text,"~OL~FR-- quit  ~RS~OL: %s ~FR--~RS\n",user->site);
        		write_level(SIX,1,text,NULL);  
        		}
		disconnect_user(user);  return;
		}
	if (!strcmp(name,"who")) {
		who(user,0);
		write_user(user,"\nUser Nickname: ");
		if (system_logging) {
        		sprintf(text,"~OL~FR-- who   ~RS~OL: %s ~FR--~RS\n",user->site);
        		write_level(SIX,1,text,NULL);  
        		}
		return;
		}
	if (!strcmp(name,"version")) {
		sprintf(filename,"%s/versionfile",DATAFILES);
		if (!(ret=more(user,user->socket,filename)))
			write_user(user,"Unable to find the version file.\n");
		if (ret==1) user->misc_op=2;
		write_user(user,"\nUser Nickname: ");
		if (system_logging) {
        		sprintf(text,"~OL~FR-- version   ~RS~OL: %s ~FR--~RS\n",user->site);
        		write_level(SIX,1,text,NULL);  
        		}
		return;
		}
	if (strlen(name)<3) {
		write_user(user,"\nName too short.\n\n");  
		attempts(user);  return;
		}
	if (strlen(name)>USER_NAME_LEN) {
		write_user(user,"\nName too long.\n\n");
		attempts(user);  return;
		}
	/* see if only letters in login */
	for (i=0;i<strlen(name);++i) {
		if (!isalpha(name[i])) {
			write_user(user,"\nOnly letters are allowed in a name.\n\n");
			attempts(user);  return;
			}
		}
	if (!allow_caps_in_name) strtolower(name);
	name[0]=toupper(name[0]);
	if (user_banned(name)) {
		write_user(user,"\nYou are banned from this talker.\n\n");
		disconnect_user(user);
		sprintf(text,"Attempted login by banned user %s.\n",name);
		write_syslog(text,1);
		return;
		}
	strcpy(user->name,name);
	/* If user has hung on another login clear that session */
	for(u=user_first;u!=NULL;u=u->next) {
		if (u->login && u!=user && !strcmp(u->name,user->name)) {
			disconnect_user(u);  break;
			}
		}	
	if (!load_user_details(user)) {
		if (user->port==port[1]) {
			write_user(user,"\nSorry, new logins cannot be created on this port.\n\n");
			disconnect_user(user);  
			return;
			}
		if (minlogin_level>-1) {
			write_user(user,"\nSorry, new logins cannot be created at this time.\n\n");
			disconnect_user(user);  
			return;
			}
		}
	else {
		if (user->port==port[1] && user->level<wizport_level) {
			sprintf(text,"\nSorry, only users of level %s and above can log in on this port.\n\n",level_name[wizport_level]);
			write_user(user,text);
			disconnect_user(user);  
			return;
			}
		if (user->level<minlogin_level) {
			write_user(user,"\nSorry, the talker is locked out to users of your level.\n\n");
			disconnect_user(user);  
			return;
			}
		}
	if (system_logging) {
		sprintf(text,"~OL~FR-- passwd  ~RS~OL: ~FY%s~RS~OL : %s ~FR--~RS\n",user->name,user->site);
		write_level(SIX,1,text,NULL);
		}
	write_user(user,"User Password: ");
	echo_off(user);
	user->login=2;
	return;

	case 2:
	sscanf(inpstr,"%s",passwd);
	if (strlen(passwd)<3) {
		write_user(user,"\n\nPassword too short.\n\n");  
		attempts(user);  return;
		}
	if (strlen(passwd)>PASS_LEN) {
		write_user(user,"\n\nPassword too long.\n\n");
		attempts(user);  return;
		}
	/* if new user... */
	if (!user->pass[0]) {
		strcpy(user->pass,(char *)crypt(passwd,"NU"));
		write_user(user,"\n");
		more(user,user->socket,NEWBIE);
		write_user(user,"New user...");
		write_user(user,"\nIf you agree...");
		write_user(user,"Type your password again");
		write_user(user,"\nIf you don't...Please type 'exit' now: ");
		user->login=1;
		}
	else {
		if (!strcmp(user->pass,(char *)crypt(passwd,"NU"))) {
			echo_on(user);  connect_user(user);  return;
			}
		write_user(user,"\n\nIncorrect login.\n\n");
		attempts(user);
		filter(user,1,passwd); /* check failed passwd */
		}
	return;

	case 1:
	sscanf(inpstr,"%s",passwd);
	if (!strcmp(passwd,"exit")) {
		write_user(user,"\nGoodbye...\n");
		disconnect_user(user);
		}
	if (strcmp(user->pass,(char*)crypt(passwd,"NU"))) {
		write_user(user,"\n\nPasswords do not match.\n\n");
		attempts(user);
		return;
		}
	echo_on(user);
	strcpy(user->desc,"hasn't used .desc yet");
	strcpy(user->in_phrase,"enters");	
	strcpy(user->out_phrase,"goes");	
	strcpy(user->pref_rm,MAINROOM);
	strcpy(user->color,"~FW");
        user->email[0] = '\0';
        user->url[0] = '\0';
        user->gend[0] = '\0';
        user->age[0] = '\0';
	user->last_site[0]='\0';
	user->level=0;
	user->jailed=0;
	user->muzzled=0;
	user->cursed=0;
	user->tic_win=0;
	user->tic_lose=0;
	user->tic_draw=0;
	user->hang_win=0;
	user->hang_lose=0;
	user->blackjack_status=0;
	user->blackjack_total=0;
	user->nerflife=10;
	user->nerfcharge=0;
	user->nerf_win=0;
	user->nerf_lose=0;
	user->total_login=0;
	user->command_mode=0;
	user->prompt=prompt_def;
	user->colour=colour_def;
	user->charmode_echo=charecho_def;
        user->macrolist=NULL;
	user->ignorelist=NULL;
	save_user_details(user,1);
	sprintf(text,"New user \"%s\" created.\n",user->name);
	write_syslog(text,1);
	connect_user(user);
	}
}
	


/*** Count up attempts made by user to login ***/
attempts(user)
UR_OBJECT user;
{
user->attempts++;
if (user->attempts==3) {
	write_user(user,"\nMaximum attempts reached.\n\n");
	disconnect_user(user);  return;
	}
user->login=3;
user->pass[0]='\0';
write_user(user,"User Nickname: ");
echo_on(user);
}



/*** Load the users details ***/
load_user_details(user)
UR_OBJECT user;
{
FILE *fp;
char line[81],filename[80];
int temp1,temp2,temp3;

sprintf(filename,"%s/%s.D",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) return 0;

fscanf(fp,"%s",user->pass); /* password */
fscanf(fp,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",&temp1,&temp2,&user->last_login_len,&temp3,&user->level,&user->prompt,&user->muzzled,&user->charmode_echo,&user->command_mode,&user->colour,&user->jailed,&user->cursed,&user->hide_email,
&user->hide_url,&user->autoread,&user->autofwd,&user->mail_verified);
user->last_login=(time_t)temp1;
user->total_login=(time_t)temp2;
user->read_mail=(time_t)temp3;
fscanf(fp,"%s\n",user->last_site);
strcpy(user->array, "000000000");
user->opponent = NULL;

/* Need to do the rest like this 'cos they may be more than 1 word each */
fgets(line,USER_DESC_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->desc,line);
fgets(line,GEND_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->gend,line);
fgets(line,AGE_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->age,line);
fgets(line,URL_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->url,line);
fgets(line,EMAIL_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->email,line); 
fgets(line,PHRASE_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->in_phrase,line); 
fgets(line,PHRASE_LEN+2,fp);
line[strlen(line)-1]=0;
strcpy(user->out_phrase,line); 

if (fgets(line,ROOM_NAME_LEN+2,fp)!=NULL) {
	if (!((line[0] == '\n')||(line[0] == '\0'))) {
		line[strlen(line)-1]=0;
		strcpy(user->pref_rm,line);
	}
}
else {
	strcpy(user->pref_rm,MAINROOM);
	}

if (fgets(line,COLOR_LEN+2,fp)!=NULL) {
	if (!((line[0] == '\n')||(line[0] == '\0'))) {
		line[strlen(line)-1]=0;
		strcpy(user->color,line);
	}
}
else {
	strcpy(user->color,"~FW");
	}
fgets(line,82,fp);  line[strlen(line)-1]=0;  strcpy(user->fwdaddy,line);
fscanf(fp,"%s\n",user->verify_code);
if ( fscanf(fp,"%d",&user->tic_win) == EOF ) {
	user->tic_win = 0;
	user->tic_lose = 0;
	user->tic_draw = 0;
	}
else {
	fscanf(fp,"%d %d\n",&user->tic_lose,&user->tic_draw);
	}
if ( fscanf(fp,"%d",&user->hang_win) == EOF ) {
	user->hang_win = 0;
	user->hang_lose = 0;
	}
else {
	fscanf(fp,"%d\n",&user->hang_lose);
	}
if ( fscanf(fp,"%d",&user->nerf_win) == EOF ) {
	user->nerf_win = 0;
	user->nerf_lose = 0;
	user->nerflife = 10;
	user->nerfcharge = 0;
	}
else {
	fscanf(fp,"%d %d %d\n",&user->nerf_lose,&user->nerflife,&user->nerfcharge);
	}
if (fscanf(fp,"%d",&user->blackjack_total)==EOF) {
	user->blackjack_total = 0;
	}
else {
	fscanf(fp,"%d\n",&user->blackjack_total);
	}
fclose(fp);

sprintf(filename,"%s/%s.A",USERFILES,user->name);
user->macrolist = NULL;
load_macros(&(user->macrolist),filename);
user->inedit=0; /** put here because of a problem with the who (users login
                    in EDIT mode) **/
return 1;
}



/*** Save a users stats ***/
save_user_details(user,save_current)
UR_OBJECT user;
int save_current;
{
FILE *fp;
char filename[80];

if (user->type==REMOTE_TYPE || user->type==CLONE_TYPE) return 0;
sprintf(filename,"%s/%s.D",USERFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
	sprintf(text,"%s: failed to save your details.\n",syserror);	
	write_user(user,text);
	sprintf(text,"SAVE_USER_STATS: Failed to save %s's details.\n",user->name);
	write_syslog(text,1);
	return 0;
	}
fprintf(fp,"%s\n",user->pass);
if (save_current)
	fprintf(fp,"%d %d %d ",(int)time(0),(int)user->total_login,(int)(time(0)-user->last_login));
else fprintf(fp,"%d %d %d ",(int)user->last_login,(int)user->total_login,user->last_login_len);
fprintf(fp,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",(int)user->read_mail,
	user->level,user->prompt,user->muzzled,user->charmode_echo,
	user->command_mode,user->colour,user->jailed,user->cursed,
	user->hide_email,user->hide_url,user->autoread,user->autofwd,
	user->mail_verified);
if (save_current) fprintf(fp,"%s\n",user->site);
else fprintf(fp,"%s\n",user->last_site);
fprintf(fp,"%s\n",user->desc);
fprintf(fp,"%s\n",user->gend);
fprintf(fp,"%s\n",user->age);
fprintf(fp,"%s\n",user->url);
fprintf(fp,"%s\n",user->email);
fprintf(fp,"%s\n",user->in_phrase);
fprintf(fp,"%s\n",user->out_phrase);
fprintf(fp,"%s\n",user->pref_rm);
fprintf(fp,"%s\n",user->color);
fprintf(fp,"%s\n",user->fwdaddy);
if (save_current) fprintf(fp,"%s\n",user->verify_code);
else fprintf(fp,"%s\n",user->verify_code);
fprintf(fp,"%d %d %d\n",user->tic_win,user->tic_lose,user->tic_draw);
fprintf(fp,"%d %d\n",user->hang_win,user->hang_lose);
fprintf(fp,"%d %d %d %d\n",user->nerf_win,user->nerf_lose,user->nerflife,user->nerfcharge);
fprintf(fp,"%d\n",user->blackjack_total);
fclose(fp);

/* save macro bit added May, 1997 -- KnightShade */
sprintf(filename,"%s/%s.A",USERFILES,user->name);
save_macros(&(user->macrolist),filename);
return 1;
}


/*** Connect the user to the talker proper ***/
connect_user(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
RM_OBJECT rm;
char temp[30],filename[80];

filter(user,0,"\0"); /*added as a site/level filter */
/* See if user already connected */
for(u=user_first;u!=NULL;u=u->next) {
	if (user!=u && user->type!=CLONE_TYPE && !strcmp(user->name,u->name)) {
		rm=u->room;
		if (u->type==REMOTE_TYPE) {
			write_user(u,"\n~FB~OLYou are pulled back through cyberspace...\n");
			sprintf(text,"REMVD %s\n",u->name);
			write_sock(u->netlink->socket,text);
			sprintf(text,"%s vanishes.\n",u->name);
			destruct_user(u);
			write_room(rm,text);
			reset_access(rm);
			num_of_users--;
			break;
			}
		write_user(user,"\n\nYou are already connected - switching to old session...\n");
		sprintf(text,"%s swapped sessions.\n",user->name);
		write_syslog(text,1);
		close(u->socket);
		u->socket=user->socket;
		strcpy(u->site,user->site);
		u->site_port=user->site_port;
                if (user->pop !=NULL) { u->pop=user->pop; }  /*poker*/
		destruct_user(user);
		num_of_logins--;
		sprintf(text,"~OLSESSION SWAP:~RS %s %s\n",u->name,u->desc);
		write_room_except(rm,text,u);
		if (rm==NULL) {
			sprintf(text,"ACT %s look\n",u->name);
			write_sock(u->netlink->socket,text);
			}
		else {
			look(u);  prompt(u);
			}
		/* Reset the sockets on any clones */
		for(u2=user_first;u2!=NULL;u2=u2->next) {
			if (u2->type==CLONE_TYPE && u2->owner==user) {
				u2->socket=u->socket;  u->owner=u;
				}
			}
		return;
		}
	}

/* send post-login message and other logon stuff to user */
write_user(user,"\n");
more(user,user->socket,MOTD2); 
if (user->last_site[0]) {
	sprintf(temp,"%s",ctime(&user->last_login));
	temp[strlen(temp)-1]=0;
	sprintf(text,"Welcome %s...\n\n~OL~FBYou were last logged in on %s from %s.\n\n",user->name,temp,user->last_site);
	}
else sprintf(text,"Welcome %s...\n\n",user->name);
write_user(user,text);

/* login check this does work :) -lj */
if (user->jailed>=1) {
        for(rm=room_first;rm!=NULL;rm=rm->next)
                if (!strncmp(rm->name,JAILROOM,strlen(JAILROOM))) user->room=rm;
        }
else {
	for(rm=room_first;rm!=NULL;rm=rm->next)
		if (!strncmp(rm->name,user->pref_rm,strlen(user->pref_rm))) {
			if (!has_room_access(user,rm))  user->room=room_first;
			else user->room=rm;
		}
     }
/* Announce users logon. You're probably wondering why Ive done it this strange
   way , well its to avoid a minor bug that arose when as in 3.3.1 it created 
   the string in 2 parts and sent the parts seperately over a netlink. If you 
   want more details email me. */


/* i had to split there ito two seperate strings to accomidate the wiz write
   correctly --lj */
sprintf(text,"~OL[ ENTERING ]~RS %s %s ",user->name,user->desc);
write_level(ONE,1,text,NULL);
if (user->jailed>=1) { 
	sprintf(text,"~OL~FR<%s>~RS ",JAILROOM);
	write_level(ONE,1,text,NULL);
	}
else {  /* ok i assume that this works this time :) -lj */
	sprintf(text,"~OL~FY<%s>~RS ",user->room);
	write_level(ONE,1,text,NULL);
	}
sprintf(text,"~FT(%s:%d)",user->site,user->site_port);
write_level(FOU,1,text,NULL);
write_level(ONE,1,"\n",NULL);

user->last_login=time(0); /* set to now */
sprintf(text,"~FTYour level is:~RS~OL %s\n",level_name[user->level]);
write_user(user,text);
look(user);
wholist(user,0);

if (user->level==ZER) {
    sprintf(text,"\n~OLWelcome to ~FG%s~FW!!  To be auto-promoted, please set your ~FGdescription~FW,~FGgender~FW,\n",TALKERNAME);
    write_user(user,text);
    write_user(user,"~OLand make a request for ~FB~OLan account~FW.\n");
    write_user(user,"~OL~FYtype~FR .desc <something kewl>\n");
    write_user(user,"~OL~FR     .set gend <male/female/other>\n");
    write_user(user,"~OL~FR     .accreq <email address>\n");
}

if ((user->autoread==1)&&(has_unread_mail(user)))
	rmail(user,0);
else if (has_unread_mail(user)) 
	write_user(user,"\07~OL~FR-- MAILDAEMON : ~LIYou have new mail!\n");
else if (mail_count(user)) {
	sprintf(text,"~OL~FT-- MAILDAEMON : You have %d old mail",mail_count(user));
	write_user(user,text);
	if (mail_count(user)==1) write_user(user,"~OL~FT message!\n");
	else write_user(user,"~OL~FT messages!\n");
	}
prompt(user);

/* write to syslog and set up some vars */
sprintf(text,"%s logged in on port %d from %s:%d.\n",user->name,user->port,user->site,user->site_port);
write_syslog(text,1);
if (num_recent_users>9) {
	for (d=0;d<num_recent_users;d++) {
		strcpy(logged_users[d],logged_users[d+1]);
		}
	sprintf(text,"~FT%-11s~RS logged in at: ~FG%02d/%02d %02d:%02d:%02d\n",user->name,tmonth+1,tmday,thour,tmin,tsec);
	strcpy(logged_users[9],text);
	}
else {
	sprintf(text,"~FT%-11s~RS logged in at: ~FG%02d/%02d %02d:%02d:%02d\n",user->name,tmonth+1,tmday,thour,tmin,tsec);
	strcpy(logged_users[num_recent_users],text);
	num_recent_users++;
	}
num_of_users++;
num_of_logins--;
user->login=0;
user->pop=NULL; /* Poker Reset */
if (user->vis) {
	if( (bot_bot != NULL) && (bot_bot->room==room_first) ) {
		sprintf(text,welcome_text[rand()%num_welcome_text],bot_bot->name,user->name);
		write_room(user->room,text);
		}
	}
}


/*** Disconnect user from talker ***/
disconnect_user(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;

rm=user->room;
if (user->login) {
	close(user->socket);  
	destruct_user(user);
	num_of_logins--;  
	return;
	}
if (user->type!=REMOTE_TYPE) {
	save_user_details(user,1);
        if (user->pop !=NULL) { fold_poker(user); leave_poker(user); user->pop=NULL; }
        free_macrolist( &(user->macrolist) ); /* added May, 1997 -- KnightShade */

	sprintf(text,"%s logged out.\n",user->name);
	write_syslog(text,1);
	write_user(user,"\n~OL~FBYou are removed from this reality...\n\n");
	close(user->socket);
        check_fight_status(user); /* See if user is in a fight */	
	sprintf(text,"~OL[ LEAVING ]~RS %s %s\n",user->name,user->desc);
	write_room(NULL,text);
	if (user->room==NULL) {
		sprintf(text,"REL %s\n",user->name);
		write_sock(user->netlink->socket,text);
		for(nl=nl_first;nl!=NULL;nl=nl->next) 
			if (nl->mesg_user==user) {  
				nl->mesg_user=(UR_OBJECT)-1;  break;  
				}
		}
	}
else {
	write_user(user,"\n~FR~OLYou are pulled back in disgrace to your own domain...\n");
	sprintf(text,"REMVD %s\n",user->name);
	write_sock(user->netlink->socket,text);
	sprintf(text,"~FR~OL%s is banished from here!\n",user->name);
	write_room_except(rm,text,user);
	sprintf(text,"NETLINK: Remote user %s removed.\n",user->name);
	write_syslog(text,1);
	}
if (user->malloc_start!=NULL) free(user->malloc_start);
num_of_users--;

/* Destroy any clones */
destroy_user_clones(user);
destruct_user(user);
reset_access(rm);
destructed=0;
wholist(user,0);
}


/*** Tell telnet not to echo characters - for password entry ***/
echo_off(user)
UR_OBJECT user;
{
char seq[4];

if (password_echo) return;
sprintf(seq,"%c%c%c",255,251,1);
write_user(user,seq);
}


/*** Tell telnet to echo characters ***/
echo_on(user)
UR_OBJECT user;
{
char seq[4];

if (password_echo) return;
sprintf(seq,"%c%c%c",255,252,1);
write_user(user,seq);
}



/************ MISCELLANIOUS FUNCTIONS *************/

/*** Stuff that is neither speech nor a command is dealt with here ***/
misc_ops(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char filename[80];

switch(user->misc_op) {
	case 1: 
	if (toupper(inpstr[0])=='Y') {
		if (rs_countdown && !rs_which) {
			if (rs_countdown>60) 
				sprintf(text,"\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d minutes, %d seconds!\n\n",rs_countdown/60,rs_countdown%60);
			else sprintf(text,"\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d seconds!\n\n",rs_countdown);
			write_room(NULL,text);
			sprintf(text,"%s initiated a %d seconds SHUTDOWN countdown.\n",user->name,rs_countdown);
			write_syslog(text,1);
			rs_user=user;
			rs_announce=time(0);
			user->misc_op=0;  
			prompt(user);
			return 1;
			}
		talker_shutdown(user,NULL,0); 
		}
	/* This will reset any reboot countdown that was started, oh well */
	rs_countdown=0;
	rs_announce=0;
	rs_which=-1;
	rs_user=NULL;
	user->misc_op=0;  
	prompt(user);
	return 1;

	case 2: 
	if (toupper(inpstr[0])=='E'
	    || more(user,user->socket,user->page_file)!=1) {
		user->misc_op=0;  user->filepos=0;  user->page_file[0]='\0';
		prompt(user); 
		}
	return 1;

	case 3: /* writing on board */
	case 4: /* Writing mail */
	case 5: /* doing profile */
	editor(user,inpstr);  
	return 1;

	case 6:
	if (toupper(inpstr[0])=='Y') delete_user(user,1); 
	else {  user->misc_op=0;  prompt(user);  }
	return 1;

	case 7:
	if (toupper(inpstr[0])=='Y') {
		if (rs_countdown && rs_which==1) {
			if (rs_countdown>60) 
				sprintf(text,"\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d minutes, %d seconds!\n\n",rs_countdown/60,rs_countdown%60);
			else sprintf(text,"\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d seconds!\n\n",rs_countdown);
			write_room(NULL,text);
			sprintf(text,"%s initiated a %d seconds REBOOT countdown.\n",user->name,rs_countdown);
			write_syslog(text,1);
			rs_user=user;
			rs_announce=time(0);
			user->misc_op=0;  
			prompt(user);
			return 1;
			}
		talker_shutdown(user,NULL,1); 
		}
	if (rs_which==1 && rs_countdown && rs_user==NULL) {
		rs_countdown=0;
		rs_announce=0;
		rs_which=-1;
		}
	user->misc_op=0;  
	prompt(user);
	return 1;

	case 8:
        if (!inpstr[0]) {
                write_user(user,"Abandoning your matchsite look-up.\n");
                user->misc_op=0;  user->matchsite_all_store=0;
                user->matchsite_check_store[0]='\0';
                prompt(user);
                }
        else {
          	user->misc_op=0;
          	word[0][0]=toupper(word[0][0]);
          	strcpy(user->matchsite_check_store,word[0]);
          	matchsite(user,1);
          	}
        return 1;

	case 9:
        if (!inpstr[0]) {
                write_user(user,"Abandoning your matchsite look-up.\n");
                user->misc_op=0;  user->matchsite_all_store=0;
                user->matchsite_check_store[0]='\0';
                prompt(user);
                }
        else {
          	user->misc_op=0;
          	strcpy(user->matchsite_check_store,word[0]);
          	matchsite(user,2);
          	}   
        return 1;

	case 10:
	if (toupper(inpstr[0])=='Y') {
		write_user(user,"All mail deleted.\n");
		sprintf(filename,"%s/%s.M",USERFILES,user->name);
 		unlink(filename);
		}
	else {  user->misc_op=0;  prompt(user);  }
	return 1;


	}
return 0;
}


/*** The editor used for writing profiles, mail and messages on the boards ***/
editor(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int cnt,line;
char *edprompt="\n~FGSave~RS, ~FYredo~RS or ~FRabort~RS (s/r/a): ";
char *ptr;

user->inedit=1;
if (user->edit_op) {
	switch(toupper(*inpstr)) {
		case 'S':
		sprintf(text,"%s finishes composing some text.\n",user->name);
		write_room_except(user->room,text,user);
		switch(user->misc_op) {
			case 3: write_board(user,NULL,1);  break;
			case 4: smail(user,NULL,1);  break;
			case 5: enter_profile(user,1);  break;
			}
		editor_done(user);
		user->inedit=0;
		return;

		case 'R':
		user->edit_op=0;
		user->edit_line=1;
		user->charcnt=0;
		user->malloc_end=user->malloc_start;
		*user->malloc_start='\0';
		sprintf(text,"\nRedo message...\n\n%d>",user->edit_line);
		write_user(user,text);
		return;

		case 'A':
		write_user(user,"\nMessage aborted.\n");
		sprintf(text,"%s gives up composing some text.\n",user->name);
		write_room_except(user->room,text,user);
		editor_done(user);  
		user->inedit=0;
		return;

		default: 
		write_user(user,edprompt);
		return;
		}
	}
/* Allocate memory if user has just started editing */
if (user->malloc_start==NULL) {
	if ((user->malloc_start=(char *)malloc(MAX_LINES*81))==NULL) {
		sprintf(text,"%s: failed to allocate buffer memory.\n",syserror);
		write_user(user,text);
		write_syslog("ERROR: Failed to allocate memory in editor().\n",0);
		user->misc_op=0;
		prompt(user);
		return;
		}
	user->ignall_store=user->ignall;
	user->ignall=1; /* Dont want chat mucking up the edit screen */
	user->edit_line=1;
	user->charcnt=0;
	user->malloc_end=user->malloc_start;
	*user->malloc_start='\0';
	sprintf(text,"~FTMaximum of %d lines, end with a '.' on a line by itself.\n\n1>",MAX_LINES);
	write_user(user,text);
	sprintf(text,"%s starts composing some text...\n",user->name);
	write_room_except(user->room,text,user);
	return;
	}

/* Check for empty line */
if (!word_count) {
	if (!user->charcnt) {
		sprintf(text,"%d>",user->edit_line);
		write_user(user,text);
		return;
		}
	*user->malloc_end++='\n';
	if (user->edit_line==MAX_LINES) goto END;
	sprintf(text,"%d>",++user->edit_line);
     	write_user(user,text);
	user->charcnt=0;
     	return;
	}
/* If nothing carried over and a dot is entered then end */
if (!user->charcnt && !strcmp(inpstr,".")) goto END;

line=user->edit_line;
cnt=user->charcnt;

/* loop through input and store in allocated memory */
while(*inpstr) {
	*user->malloc_end++=*inpstr++;
	if (++cnt==80) {  user->edit_line++;  cnt=0;  }
	if (user->edit_line>MAX_LINES 
	    || user->malloc_end - user->malloc_start>=MAX_LINES*81) goto END;
	}
if (line!=user->edit_line) {
	ptr=(char *)(user->malloc_end-cnt);
	*user->malloc_end='\0';
	sprintf(text,"%d>%s",user->edit_line,ptr);
	write_user(user,text);
	user->charcnt=cnt;
	return;
	}
else {
	*user->malloc_end++='\n';
	user->charcnt=0;
	}
if (user->edit_line!=MAX_LINES) {
	sprintf(text,"%d>",++user->edit_line);
	write_user(user,text);
	return;
	}

/* User has finished his message , prompt for what to do now */
END:
*user->malloc_end='\0';
if (*user->malloc_start) {
	write_user(user,edprompt);
	user->edit_op=1;  return;
	}
write_user(user,"\nNo text.\n");
sprintf(text,"%s gives up composing some text.\n",user->name);
write_room_except(user->room,text,user);
editor_done(user);
user->inedit=0;
}


/*** Reset some values at the end of editing ***/
editor_done(user)
UR_OBJECT user;
{
user->misc_op=0;
user->edit_op=0;
user->edit_line=0;
free(user->malloc_start);
user->malloc_start=NULL;
user->malloc_end=NULL;
user->ignall=user->ignall_store;
prompt(user);
}


/*** Record speech and emotes in the room. ***/
record(rm,str)
RM_OBJECT rm;
char *str;
{
strncpy(rm->revbuff[rm->revline],str,REVIEW_LEN);
rm->revbuff[rm->revline][REVIEW_LEN]='\n';
rm->revbuff[rm->revline][REVIEW_LEN+1]='\0';
rm->revline=(rm->revline+1)%REVIEW_LINES;
}


/*** Records tells and pemotes sent to the user. ***/
record_tell(user,str)
UR_OBJECT user;
char *str;
{
strncpy(user->revbuff[user->revline],str,REVIEW_LEN);
user->revbuff[user->revline][REVIEW_LEN]='\n';
user->revbuff[user->revline][REVIEW_LEN+1]='\0';
user->revline=(user->revline+1)%REVTELL_LINES;
}


/*** Records shouts and shemotes sent over the talker. ***/
record_shout(str)
char *str;
{
strncpy(shoutbuff[sbuffline],str,REVIEW_LEN);
shoutbuff[sbuffline][REVIEW_LEN]='\n';
shoutbuff[sbuffline][REVIEW_LEN+1]='\0';
sbuffline=(sbuffline+1)%REVIEW_LINES;
}


/*** Clear the tell buffer of the user ***/
clear_tells(user)
UR_OBJECT user;
{
int c;
for(c=0;c<REVTELL_LINES;++c) user->revbuff[c][0]='\0';
user->revline=0;
}


/*** Clear the shout buffer of the talker ***/
clear_shouts()
{
int c;
for(c=0;c<REVIEW_LINES;++c) shoutbuff[c][0]='\0';
sbuffline=0;
}


/*** See review of shouts ***/
revshout(user)
UR_OBJECT user;
{
int i,line,cnt;

cnt=0;
for(i=0;i<REVIEW_LINES;++i) {
        line=(sbuffline+i)%REVIEW_LINES;
        if (shoutbuff[line][0]) {
                cnt++;
                if (cnt==1) write_user(user,"~BB~FG*** Shout review buffer ***\n\n");
                write_user(user,shoutbuff[line]);
                }
        }
if (!cnt) write_user(user,"Shout review buffer is empty.\n");
else write_user(user,"\n~BB~FG*** End ***\n\n");
}


/*** Set room access back to public if not enough users in room ***/
reset_access(rm)
RM_OBJECT rm;
{
UR_OBJECT u;
int cnt;

if (rm==NULL || rm->access!=PRIVATE) return; 
cnt=0;
for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
if (cnt<min_private_users) {
	write_room(rm,"Room access returned to ~FGPUBLIC.\n");
	rm->access=PUBLIC;

	/* Reset any invites into the room & clear review buffer */
	for(u=user_first;u!=NULL;u=u->next) {
		if (u->invite_room==rm) u->invite_room=NULL;
		}
	clear_revbuff(rm);
	}
}



/*** Exit because of error during bootup ***/
boot_exit(code)
int code;
{
switch(code) {
	case 1:
	write_log("BOOT FAILURE: Error while parsing configuration file.\n",0,3);
	exit(1);

	case 2:
	perror("          RNUTS: Can't open main port listen socket");
	write_log("BOOT FAILURE: Can't open main port listen socket.\n",0,3);
	exit(2);

	case 3:
	perror("          RNUTS: Can't open wiz port listen socket");
	write_log("BOOT FAILURE: Can't open wiz port listen socket.\n",0,3);
	exit(3);

	case 4:
	perror("          RNUTS: Can't open link port listen socket");
	write_log("BOOT FAILURE: Can't open link port listen socket.\n",0,3);
	exit(4);

	case 5:
	perror("          RNUTS: Can't bind to main port");
	write_log("BOOT FAILURE: Can't bind to main port.\n",0,3);
	exit(5);

	case 6:
	perror("          RNUTS: Can't bind to wiz port");
	write_log("BOOT FAILURE: Can't bind to wiz port.\n",0,3);
	exit(6);

	case 7:
	perror("          RNUTS: Can't bind to link port");
	write_log("BOOT FAILURE: Can't bind to link port.\n",0,3);
	exit(7);
	
	case 8:
	perror("          RNUTS: Listen error on main port");
	write_log("BOOT FAILURE: Listen error on main port.\n",0,3);
	exit(8);

	case 9:
	perror("          RNUTS: Listen error on wiz port");
	write_log("BOOT FAILURE: Listen error on wiz port.\n",0,3);
	exit(9);

	case 10:
	perror("          RNUTS: Listen error on link port");
	write_log("BOOT FAILURE: Listen error on link port.\n",0,3);
	exit(10);

	case 11:
	perror("          RNUTS: Failed to fork");
	write_log("BOOT FAILURE: Failed to fork.\n",0,3);
	exit(11);
	}
}



/*** User prompt ***/
prompt(user)
UR_OBJECT user;
{
int hr,min;

if (no_prompt) return;
if (user->type==REMOTE_TYPE) {
	sprintf(text,"PRM %s\n",user->name);
	write_sock(user->netlink->socket,text);  
	return;
	}
if (user->command_mode && !user->misc_op) {  
	if (!user->vis) write_user(user,"~FTCOM+> ");
	else write_user(user,"~FTCOM> ");  
	return;  
	}
if (!user->prompt || user->misc_op) return;
hr=(int)(time(0)-user->last_login)/3600;
min=((int)(time(0)-user->last_login)%3600)/60;
if (!user->vis)
	sprintf(text,"~FT<%02d:%02d, %02d:%02d, %s+>\n",thour,tmin,hr,min,user->name);
else sprintf(text,"~FT<%02d:%02d, %02d:%02d, %s>\n",thour,tmin,hr,min,user->name);
write_user(user,text);
}



/*** Page a file out to user. Colour commands in files will only work if 
     user!=NULL since if NULL we dont know if his terminal can support colour 
     or not. Return values: 
	        0 = cannot find file, 1 = found file, 2 = found and finished ***/
more(user,sock,filename)
UR_OBJECT user;
int sock;
char *filename;
{
int i,buffpos,num_chars,lines,retval,len;
char buff[OUT_BUFF_SIZE],text2[83],*str,*colour_com_strip();
FILE *fp;

if (!(fp=fopen(filename,"r"))) {
	if (user!=NULL) user->filepos=0;  
	return 0;
	}
/* jump to reading posn in file */
if (user!=NULL) fseek(fp,user->filepos,0);

text[0]='\0';  
buffpos=0;  
num_chars=0;
retval=1; 
len=0;

/* If user is remote then only do 1 line at a time */
if (sock==-1) {
	lines=1;  fgets(text2,82,fp);
	}
else {
	lines=0;  fgets(text,sizeof(text)-1,fp);
	}

/* Go through file */
while(!feof(fp) && (lines<MAX_MORE_LINES || user==NULL)) {
	if (sock==-1) {
		lines++;  
		if (user->netlink->ver_major<=3 && user->netlink->ver_minor<2) 
			str=colour_com_strip(text2);
		else str=text2;
		if (str[strlen(str)-1]!='\n') 
			sprintf(text,"MSG %s\n%s\nEMSG\n",user->name,str);
		else sprintf(text,"MSG %s\n%sEMSG\n",user->name,str);
		write_sock(user->netlink->socket,text);
		num_chars+=strlen(str);
		fgets(text2,82,fp);
		continue;
		}
	str=text;

	/* Process line from file */
	while(*str) {
		if (*str=='\n') {
			if (buffpos>OUT_BUFF_SIZE-6) {
				write(sock,buff,buffpos);  buffpos=0;
				}
			/* Reset terminal before every newline */
			if (user!=NULL && user->colour) {
				memcpy(buff+buffpos,"\033[0m",4);  buffpos+=4;
				}
			*(buff+buffpos)='\n';  *(buff+buffpos+1)='\r';  
			buffpos+=2;  ++str;
			}
		else {  
			/* Process colour commands in the file. See write_user()
			   function for full comments on this code. */
			if (*str=='/' && *(str+1)=='~') {  ++str;  continue;  }
			if (str!=text && *str=='~' && *(str-1)=='/') {
				*(buff+buffpos)=*str;  goto CONT;
				}
			if (*str=='~') {
				if (buffpos>OUT_BUFF_SIZE-6) {
					write(sock,buff,buffpos);  buffpos=0;
					}
				++str;
				for(i=0;i<NUM_COLS;++i) {
					if (!strncmp(str,colcom[i],2)) {
						if (user!=NULL && user->colour) {
							memcpy(buffpos+buff,colcode[i],strlen(colcode[i]));
							buffpos+=strlen(colcode[i])-1;
							}
						else buffpos--;
						++str;
						goto CONT;
						}
					}
				*(buff+buffpos)=*(--str);
				}
			else *(buff+buffpos)=*str;
			CONT:
			++buffpos;   ++str;
			}
		if (buffpos==OUT_BUFF_SIZE) {
			write(sock,buff,OUT_BUFF_SIZE);  buffpos=0;
			}
		}
	len=strlen(text);
	num_chars+=len;
	lines+=len/80+(len<80);
	fgets(text,sizeof(text)-1,fp);
	}
if (buffpos && sock!=-1) write(sock,buff,buffpos);

/* if user is logging on dont page file */
if (user==NULL) {  fclose(fp);  return 2;  };
if (feof(fp)) {
	user->filepos=0;  no_prompt=0;  retval=2;
	}
else  {
	/* store file position and file name */
	user->filepos+=num_chars;
	strcpy(user->page_file,filename);
	/* We use E here instead of Q because when on a remote system and
	   in COMMAND mode the Q will be intercepted by the home system and 
	   quit the user */
	write_user(user,"           ~OL~FB*** Press <return> to continue, 'e'<return> to exit ***");
	no_prompt=1;
	}
fclose(fp);
return retval;
}



/*** Set global vars. hours,minutes,seconds,date,day,month,year ***/
set_date_time()
{
struct tm *tm_struct; /* structure is defined in time.h */
time_t tm_num;

/* Set up the structure */
time(&tm_num);
tm_struct=localtime(&tm_num);

/* Get the values */
tday=tm_struct->tm_yday;
tyear=1900+tm_struct->tm_year; /* Will this work past the year 2000? Hmm... */
tmonth=tm_struct->tm_mon;
tmday=tm_struct->tm_mday;
twday=tm_struct->tm_wday;
thour=tm_struct->tm_hour;
tmin=tm_struct->tm_min;
tsec=tm_struct->tm_sec; 
}



/*** Return pos. of second word in inpstr ***/
char *remove_first(inpstr)
char *inpstr;
{
char *pos=inpstr;
while(*pos<33 && *pos) ++pos;
while(*pos>32) ++pos;
while(*pos<33 && *pos) ++pos;
return pos;
}


/*** Get user struct pointer from name ***/
UR_OBJECT get_user(name)
char *name;
{
UR_OBJECT u;

name[0]=toupper(name[0]);
/* Search for exact name */
for(u=user_first;u!=NULL;u=u->next) {
	if (u->login || u->type==CLONE_TYPE) continue;
	if (!strcmp(u->name,name))  return u;
	}
/* Search for close match name */
for(u=user_first;u!=NULL;u=u->next) {
	if (u->login || u->type==CLONE_TYPE) continue;
	if (strstr(u->name,name))  return u;
	}
return NULL;
}


/*** Get room struct pointer from abbreviated name ***/
RM_OBJECT get_room(name)
char *name;
{
RM_OBJECT rm;

for(rm=room_first;rm!=NULL;rm=rm->next)
     if (!strncmp(rm->name,name,strlen(name))) return rm;
return NULL;
}


/*** Return level value based on level name ***/
get_level(name)
char *name;
{
int i;

i=0;
while(level_name[i][0]!='*') {
	if (!strcmp(level_name[i],name)) return i;
	++i;
	}
return -1;
}


/*** See if a user has access to a room. If room is fixed to private then
	it is considered a wizroom so grant permission to any user of WIZ and
	above for those. ***/
has_room_access(user,rm)
UR_OBJECT user;
RM_OBJECT rm;
{
if ((rm->access & PRIVATE) 
    && user->level<gatecrash_level 
    && user->invite_room!=rm
    && !((rm->access & FIXED) && user->level>=FOU)) return 0;
return 1;
}


/*** See if user has unread mail, mail file has last read time on its 
     first line ***/
has_unread_mail(user)
UR_OBJECT user;
{
FILE *fp;
int tm;
char filename[80];

sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%d",&tm);
fclose(fp);
if (tm>(int)user->read_mail) return 1;
return 0;
}


/*** This is function that sends mail to other users ***/
send_mail(user,to,ptr,iscopy)
UR_OBJECT user;
char *to,*ptr;
int iscopy;
{
NL_OBJECT nl;
FILE *infp,*outfp;
char *c,d,*service,filename[80],line[DNL+1],cc[4],header[ARR_SIZE];

/* See if remote mail */
c=to;  service=NULL;
while(*c) {
	if (*c=='@') {  
		service=c+1;  *c='\0'; 
		for(nl=nl_first;nl!=NULL;nl=nl->next) {
			if (!strcmp(nl->service,service) && nl->stage==UP) {
				send_external_mail(nl,user,to,ptr);
				return;
				}
			}
		sprintf(text,"Service %s unavailable.\n",service);
		write_user(user,text); 
		return;
		}
	++c;
	}

/* Local mail */
if (!(outfp=fopen("tempfile","w"))) {
	write_user(user,"Error in mail delivery.\n");
	write_syslog("ERROR: Couldn't open tempfile in send_mail().\n",0);
	return;
	}
/* Write current time on first line of tempfile */
fprintf(outfp,"%d\r",(int)time(0));

/* Copy current mail file into tempfile if it exists */
sprintf(filename,"%s/%s.M",USERFILES,to);
if (infp=fopen(filename,"r")) {
	/* Discard first line of mail file. */
	fgets(line,DNL,infp);

	/* Copy rest of file */
	d=getc(infp);  
	while(!feof(infp)) {  putc(d,outfp);  d=getc(infp);  }
	fclose(infp);
	}
if (iscopy) strcpy(cc,"(CC)");
else cc[0]='\0';
header[0]='\0';
/* Put new mail in tempfile */
if (user!=NULL) {
	if (user->type==REMOTE_TYPE)
		sprintf(header,"~OLFrom: %s@%s  %s %s\n",user->name,user->netlink->service,long_date(0),cc);
	else sprintf(header,"~OLFrom: %s  %s %s\n",user->name,long_date(0),cc);
	}
else sprintf(header,"~OLFrom: MAILDAEMON   %s %s\n",long_date(0),cc);
fputs(header,outfp);
fputs(ptr,outfp);
fputs("\n",outfp);
fclose(outfp);
rename("tempfile",filename);
switch(iscopy) {
    case 0: sprintf(text,"Mail is delivered to %s\n",to); break;
    case 1: sprintf(text,"Mail is copied to %s\n",to); break;
    }
write_user(user,text);
if (!iscopy) {
    sprintf(text,"%s sent mail to %s\n",user->name,to);
    write_syslog(text,1);
    }
write_user(get_user(to),"\07~OL~FR-- MAILDAEMON : ~LIYou have new mail!\n");
forward_email(to,header,ptr,user);
return 1;
}


/*** Spool mail file and ask for confirmation of users existence on remote
	site ***/
send_external_mail(nl,user,to,ptr)
NL_OBJECT nl;
UR_OBJECT user;
char *to,*ptr;
{
FILE *fp;
char filename[80];

/* Write out to spool file first */
sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,user->name,to,nl->service);
if (!(fp=fopen(filename,"a"))) {
	sprintf(text,"%s: unable to spool mail.\n",syserror);
	write_user(user,text);
	sprintf(text,"ERROR: Couldn't open file %s to append in send_external_mail().\n",filename);
	write_syslog(text,0);
	return;
	}
putc('\n',fp);
fputs(ptr,fp);
fclose(fp);

/* Ask for verification of users existence */
sprintf(text,"EXISTS? %s %s\n",to,user->name);
write_sock(nl->socket,text);

/* Rest of delivery process now up to netlink functions */
write_user(user,"Mail sent.\n");
}


/*** See if string contains any swearing ***/
contains_swearing(str)
char *str;
{
char *s;
int i;

if ((s=(char *)malloc(strlen(str)+1))==NULL) {
	write_syslog("ERROR: Failed to allocate memory in contains_swearing().\n",0);
	return 0;
	}
strcpy(s,str);
strtolower(s); 
i=0;
while(swear_words[i][0]!='*') {
	if (strstr(s,swear_words[i])) {  free(s);  return 1;  }
	++i;
	}
free(s);
return 0;
}


/*** Count the number of colour commands in a string ***/
colour_com_count(str)
char *str;
{
char *s;
int i,cnt;

s=str;  cnt=0;
while(*s) {
     if (*s=='~') {
          ++s;
          for(i=0;i<NUM_COLS;++i) {
               if (!strncmp(s,colcom[i],2)) {
                    cnt++;  s++;  continue;
                    }
               }
          continue;
          }
     ++s;
     }
return cnt;
}


/*** Strip out colour commands from string for when we are sending strings
     over a netlink to a talker that doesn't support them ***/
char *colour_com_strip(str)
char *str;
{
char *s,*t;
static char text2[ARR_SIZE];
int i;

s=str;  t=text2;
while(*s) {
	if (*s=='~') {
		++s;
		for(i=0;i<NUM_COLS;++i) {
			if (!strncmp(s,colcom[i],2)) {  s++;  goto CONT;  }
			}
		--s;  *t++=*s;
		}
	else *t++=*s;
	CONT:
	s++;
	}	
*t='\0';
return text2;
}


/*** Date string for board messages, mail, .who and .allclones ***/
char *long_date(which)
int which;
{
static char dstr[80];

if (which==1) sprintf(dstr,"on %s %d %s %d at %02d:%02d",day[twday],tmday,month[tmonth],tyear,thour,tmin);
if (which==2) sprintf(dstr,"%s %d %s %d at %02d:%02d",day[twday],tmday,month[tmonth],tyear,thour,tmin);
else sprintf(dstr,"[ %s %d %s %d at %02d:%02d ]",day[twday],tmday,month[tmonth],tyear,thour,tmin);
return dstr;
}


/*** Clear the review buffer in the room ***/
clear_revbuff(rm)
RM_OBJECT rm;
{
int c;
for(c=0;c<REVIEW_LINES;++c) rm->revbuff[c][0]='\0';
rm->revline=0;
}


/*** Clear the screen ***/
cls(user)
UR_OBJECT user;
{
int i;

for(i=0;i<5;++i) write_user(user,"\n\n\n\n\n\n\n\n\n\n");		
}


/*** Convert string to upper case ***/
strtoupper(str)
char *str;
{
while(*str) {  *str=toupper(*str);  str++; }
}


/*** Convert string to lower case ***/
strtolower(str)
char *str;
{
while(*str) {  *str=tolower(*str);  str++; }
}


/*** Returns 1 if string is a positive number ***/
isnumber(str)
char *str;
{
while(*str) if (!isdigit(*str++)) return 0;
return 1;
}


/************ OBJECT FUNCTIONS ************/

/*** Construct user/clone object ***/
UR_OBJECT create_user()
{
UR_OBJECT user;
int i;

if ((user=(UR_OBJECT)malloc(sizeof(struct user_struct)))==NULL) {
	write_syslog("ERROR: Memory allocation failure in create_user().\n",0);
	return NULL;
	}

/* Append object into linked list. */
if (user_first==NULL) {  
	user_first=user;  user->prev=NULL;  
	}
else {  
	user_last->next=user;  user->prev=user_last;  
	}
user->next=NULL;
user_last=user;

/* initialise user structure */
user->type=USER_TYPE;
user->name[0]='\0';
user->desc[0]='\0';
user->gend[0]='\0';
user->age[0]='\0';
user->email[0]='\0';
user->url[0]='\0';
user->pref_rm[0]='\0';
user->in_phrase[0]='\0'; 
user->out_phrase[0]='\0';   
user->afk_mesg[0]='\0';
strcpy(user->fwdaddy,"#UNSET");
strcpy(user->verify_code,"#NONE");
user->mail_verified=0;
user->autofwd=0;
user->pass[0]='\0';
user->site[0]='\0';
user->site_port=0;
user->last_site[0]='\0';
user->page_file[0]='\0';
user->mail_to[0]='\0';
user->inpstr_old[0]='\0';
user->buff[0]='\0';  
user->buffpos=0;
user->filepos=0;
user->read_mail=time(0);
user->room=NULL;
user->invite_room=NULL;
user->port=0;
user->login=0;
user->socket=-1;
user->attempts=0;
user->command_mode=0;
user->level=0;
user->vis=1;
user->ignall=0;
user->ignall_store=0;
user->ignshout=0;
user->igntell=0;
user->muzzled=0;
user->jailed=0;
user->cursed=0;
user->hide_email=0;
user->hide_url=0;
user->autoread=0;
user->remote_com=-1;
user->netlink=NULL;
user->pot_netlink=NULL; 
user->last_input=time(0);
user->last_login=time(0);
user->last_login_len=0;
user->total_login=0;
user->prompt=prompt_def;
user->colour=colour_def;
user->charmode_echo=charecho_def;
user->misc_op=0;
user->edit_op=0;
user->edit_line=0;
user->charcnt=0;
user->warned=0;
user->accreq=0;
user->afk=0;
user->revline=0;
user->clone_hear=CLONE_HEAR_ALL;
user->malloc_start=NULL;
user->malloc_end=NULL;
user->owner=NULL;
user->home=0;
user->hang_stage=-1;
user->hang_word[0]='\0';
user->hang_word_show[0]='\0';
user->hang_guess[0]='\0';
user->blackjack_status=0;
user->blackjack_total=0;
for (i=0;i<MAX_COPIES;++i) user->copyto[i][0]='\0';
for(i=0;i<REVTELL_LINES;++i) user->revbuff[i][0]='\0';
user->matchsite_all_store=0;
user->matchsite_check_store[0]='\0';
user->boardwrite[0]='\0';
user->pop=NULL;
return user;
}



/*** Destruct an object. ***/
destruct_user(user)
UR_OBJECT user;
{
/* Remove from linked list */
if (user==user_first) {
	user_first=user->next;
	if (user==user_last) user_last=NULL;
	else user_first->prev=NULL;
	}
else {
	user->prev->next=user->next;
	if (user==user_last) { 
		user_last=user->prev;  user_last->next=NULL; 
		}
	else user->next->prev=user->prev;
	}
free(user);
destructed=1;
}


/*** Construct room object ***/
RM_OBJECT create_room()
{
RM_OBJECT room;
int i;

if ((room=(RM_OBJECT)malloc(sizeof(struct room_struct)))==NULL) {
	fprintf(stderr,"RNUTS: Memory allocation failure in create_room().\n");
	boot_exit(1);
	}
room->name[0]='\0';
room->label[0]='\0';
room->desc[0]='\0';
room->topic[0]='\0';
room->access=-1;
room->revline=0;
room->mesg_cnt=0;
room->inlink=0;
room->netlink=NULL;
room->netlink_name[0]='\0';
room->next=NULL;
for(i=0;i<MAX_LINKS;++i) {
	room->link_label[i][0]='\0';  room->link[i]=NULL;
	}
for(i=0;i<REVIEW_LINES;++i) room->revbuff[i][0]='\0';
if (room_first==NULL) room_first=room;
else room_last->next=room;
room_last=room;
return room;
}


/*** Construct link object ***/
NL_OBJECT create_netlink()
{
NL_OBJECT nl;

if ((nl=(NL_OBJECT)malloc(sizeof(struct netlink_struct)))==NULL) {
	sprintf(text,"NETLINK: Memory allocation failure in create_netlink().\n");
	write_syslog(text,1);
	return NULL;
	}
if (nl_first==NULL) { 
	nl_first=nl;  nl->prev=NULL;  nl->next=NULL;
	}
else {  
	nl_last->next=nl;  nl->next=NULL;  nl->prev=nl_last;
	}
nl_last=nl;

nl->service[0]='\0';
nl->site[0]='\0';
nl->verification[0]='\0';
nl->mail_to[0]='\0';
nl->mail_from[0]='\0';
nl->mailfile=NULL;
nl->buffer[0]='\0';
nl->ver_major=0;
nl->ver_minor=0;
nl->ver_patch=0;
nl->keepalive_cnt=0;
nl->last_recvd=0;
nl->port=0;
nl->socket=0;
nl->mesg_user=NULL;
nl->connect_room=NULL;
nl->type=UNCONNECTED;
nl->stage=DOWN;
nl->connected=0;
nl->lastcom=-1;
nl->allow=ALL;
nl->warned=0;
return nl;
}


/*** Destruct a netlink (usually a closed incoming one). ***/
destruct_netlink(nl)
NL_OBJECT nl;
{
if (nl!=nl_first) {
	nl->prev->next=nl->next;
	if (nl!=nl_last) nl->next->prev=nl->prev;
	else { nl_last=nl->prev; nl_last->next=NULL; }
	}
else {
	nl_first=nl->next;
	if (nl!=nl_last) nl_first->prev=NULL;
	else nl_last=NULL; 
	}
free(nl);
}


/*** Destroy all clones belonging to given user ***/
destroy_user_clones(user)
UR_OBJECT user;
{
UR_OBJECT u;

for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->owner==user) {
		sprintf(text,"The clone of %s shimmers and vanishes.\n",u->name);
		write_room(u->room,text);
		destruct_user(u);
		}
	}
}


/************ RNUTS PROTOCOL AND LINK MANAGEMENT FUNCTIONS ************/
/* Please don't alter these functions. If you do you may introduce 
   incompatabilities which may prevent other systems connecting or cause
   bugs on the remote site and yours. You may think it looks simple but
   even the newline count is important in some places. */

/*** Accept incoming server connection ***/
accept_server_connection(sock,acc_addr)
int sock;
struct sockaddr_in acc_addr;
{
NL_OBJECT nl,nl2,create_netlink();
RM_OBJECT rm;
char site[81];

/* Send server type id and version number */
sprintf(text,"RNUTS %s, NUTS %s\n",RVERSION,VERSION);
write_sock(sock,text);
strcpy(site,get_ip_address(acc_addr));
sprintf(text,"NETLINK: Received request connection from site %s.\n",site);
write_syslog(text,1);

/* See if legal site, ie site is in config sites list. */
for(nl2=nl_first;nl2!=NULL;nl2=nl2->next) 
	if (!strcmp(nl2->site,site)) goto OK;
write_sock(sock,"DENIED CONNECT 1\n");
close(sock);
write_syslog("NETLINK: Request denied, remote site not in valid sites list.\n",1);
return;

/* Find free room link */
OK:
for(rm=room_first;rm!=NULL;rm=rm->next) {
	if (rm->netlink==NULL && rm->inlink) {
		if ((nl=create_netlink())==NULL) {
			write_sock(sock,"DENIED CONNECT 2\n");  
			close(sock);  
			write_syslog("NETLINK: Request denied, unable to create netlink object.\n",1);
			return;
			}
		rm->netlink=nl;
		nl->socket=sock;
		nl->type=INCOMING;
		nl->stage=VERIFYING;
		nl->connect_room=rm;
		nl->allow=nl2->allow;
		nl->last_recvd=time(0);
		strcpy(nl->service,"<verifying>");
		strcpy(nl->site,site);
		write_sock(sock,"GRANTED CONNECT\n");
		write_syslog("NETLINK: Request granted.\n",1);
		return;
		}
	}
write_sock(sock,"DENIED CONNECT 3\n");
close(sock);
write_syslog("NETLINK: Request denied, no free room links.\n",1);
}
		

/*** Deal with netlink data on link nl ***/
exec_netcom(nl,inpstr)
NL_OBJECT nl;
char *inpstr;
{
int netcom_num,lev;
char w1[ARR_SIZE],w2[ARR_SIZE],w3[ARR_SIZE],*c,ctemp;

/* The most used commands have been truncated to save bandwidth, ie ACT is
   short for action, EMSG is short for end message. Commands that don't get
   used much ie VERIFICATION have been left long for readability. */
char *netcom[]={
"DISCONNECT","TRANS","REL","ACT","GRANTED",
"DENIED","MSG","EMSG","PRM","VERIFICATION",
"VERIFY","REMVD","ERROR","EXISTS?","EXISTS_NO",
"EXISTS_YES","MAIL","ENDMAIL","MAILERROR","KA",
"RSTAT","*"
};

/* The buffer is large (ARR_SIZE*2) but if a bug occurs with a remote system
   and no newlines are sent for some reason it may overflow and this will 
   probably cause a crash. Oh well, such is life. */
if (nl->buffer[0]) {
	strcat(nl->buffer,inpstr);  inpstr=nl->buffer;
	}
nl->last_recvd=time(0);

/* Go through data */
while(*inpstr) {
	w1[0]='\0';  w2[0]='\0';  w3[0]='\0';  lev=0;
	if (*inpstr!='\n') sscanf(inpstr,"%s %s %s %d",w1,w2,w3,&lev);
	/* Find first newline */
	c=inpstr;  ctemp=1; /* hopefully we'll never get char 1 in the string */
	while(*c) {
		if (*c=='\n') {  ctemp=*(c+1); *(c+1)='\0';  break; }
		++c;
		}
	/* If no newline then input is incomplete, store and return */
	if (ctemp==1) {  
		if (inpstr!=nl->buffer) strcpy(nl->buffer,inpstr);  
		return;  
		}
	/* Get command number */
	netcom_num=0;
	while(netcom[netcom_num][0]!='*') {
		if (!strcmp(netcom[netcom_num],w1))  break;
		netcom_num++;
		}
	/* Deal with initial connects */
	if (nl->stage==VERIFYING) {
		if (nl->type==OUTGOING) {
			if (strcmp(w1,"RNUTS")) {
				sprintf(text,"NETLINK: Incorrect connect message from %s.\n",nl->service);
				write_syslog(text,1);
				shutdown_netlink(nl);
				return;
				}	
			/* Store remote version for compat checks */
			nl->stage=UP;
			w2[10]='\0'; 
			sscanf(w2,"%d.%d.%d",&nl->ver_major,&nl->ver_minor,&nl->ver_patch);
			goto NEXT_LINE;
			}
		else {
			/* Incoming */
			if (netcom_num!=9) {
				/* No verification, no connection */
				sprintf(text,"NETLINK: No verification sent by site %s.\n",nl->site);
				write_syslog(text,1);
				shutdown_netlink(nl);  
				return;
				}
			nl->stage=UP;
			}
		}
	/* If remote is currently sending a message relay it to user, don't
	   interpret it unless its EMSG or ERROR */
	if (nl->mesg_user!=NULL && netcom_num!=7 && netcom_num!=12) {
		/* If -1 then user logged off before end of mesg received */
		if (nl->mesg_user!=(UR_OBJECT)-1) write_user(nl->mesg_user,inpstr);   
		goto NEXT_LINE;
		}
	/* Same goes for mail except its ENDMAIL or ERROR */
	if (nl->mailfile!=NULL && netcom_num!=17) {
		fputs(inpstr,nl->mailfile);  goto NEXT_LINE;
		}
	nl->lastcom=netcom_num;
	switch(netcom_num) {
		case  0: 
		if (nl->stage==UP) {
			sprintf(text,"~OLSYSTEM:~FY~RS Disconnecting from service %s in the %s.\n",nl->service,nl->connect_room->name);
			write_room(NULL,text);
			}
		shutdown_netlink(nl);  
		break;

		case  1: nl_transfer(nl,w2,w3,lev,inpstr);  break;
		case  2: nl_release(nl,w2);  break;
		case  3: nl_action(nl,w2,inpstr);  break;
		case  4: nl_granted(nl,w2);  break;
		case  5: nl_denied(nl,w2,inpstr);  break;
		case  6: nl_mesg(nl,w2); break;
		case  7: nl->mesg_user=NULL;  break;
		case  8: nl_prompt(nl,w2);  break;
		case  9: nl_verification(nl,w2,w3,0);  break;
		case 10: nl_verification(nl,w2,w3,1);  break;
		case 11: nl_removed(nl,w2);  break;
		case 12: nl_error(nl);  break;
		case 13: nl_checkexist(nl,w2,w3);  break;
		case 14: nl_user_notexist(nl,w2,w3);  break;
		case 15: nl_user_exist(nl,w2,w3);  break;
		case 16: nl_mail(nl,w2,w3);  break;
		case 17: nl_endmail(nl);  break;
		case 18: nl_mailerror(nl,w2,w3);  break;
		case 19: /* Keepalive signal, do nothing */ break;
		case 20: nl_rstat(nl,w2);  break;
		default: 
			sprintf(text,"NETLINK: Received unknown command '%s' from %s.\n",w1,nl->service);
			write_syslog(text,1);
			write_sock(nl->socket,"ERROR\n"); 
		}
	NEXT_LINE:
	/* See if link has closed */
	if (nl->type==UNCONNECTED) return;
	*(c+1)=ctemp;
	inpstr=c+1;
	}
nl->buffer[0]='\0';
}


/*** Deal with user being transfered over from remote site ***/
nl_transfer(nl,name,pass,lev,inpstr)
NL_OBJECT nl;
char *name,*pass,*inpstr;
int lev;
{
UR_OBJECT u,create_user();

/* link for outgoing users only */
if (nl->allow==OUT) {
	sprintf(text,"DENIED %s 4\n",name);
	write_sock(nl->socket,text);
	return;
	}
if (strlen(name)>USER_NAME_LEN) name[USER_NAME_LEN]='\0';

/* See if user is banned */
if (user_banned(name)) {
	if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=3) 
		sprintf(text,"DENIED %s 9\n",name); /* new error for 3.3.3 */
	else sprintf(text,"DENIED %s 6\n",name); /* old error to old versions */
	write_sock(nl->socket,text);
	return;
	}

/* See if user already on here */
if (u=get_user(name)) {
	sprintf(text,"DENIED %s 5\n",name);
	write_sock(nl->socket,text);
	return;
	}

/* See if user of this name exists on this system by trying to load up
   datafile */
if ((u=create_user())==NULL) {		
	sprintf(text,"DENIED %s 6\n",name);
	write_sock(nl->socket,text);
	return;
	}
u->type=REMOTE_TYPE;
strcpy(u->name,name);
if (load_user_details(u)) {
	if (strcmp(u->pass,pass)) {
		/* Incorrect password sent */
		sprintf(text,"DENIED %s 7\n",name);
		write_sock(nl->socket,text);
		destruct_user(u);
		destructed=0;
		return;
		}
	}
else {
	/* Get the users description */
	if (nl->ver_major<=3 && nl->ver_minor<=3 && nl->ver_patch<1) 
		strcpy(text,remove_first(remove_first(remove_first(inpstr))));
	else strcpy(text,remove_first(remove_first(remove_first(remove_first(inpstr)))));
	text[USER_DESC_LEN]='\0';
	terminate(text);
	strcpy(u->desc,text);
	strcpy(u->in_phrase,"enters");
	strcpy(u->out_phrase,"goes");
	if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=1) {
		if (lev>rem_user_maxlevel) u->level=rem_user_maxlevel;
		else u->level=lev; 
		}
	else u->level=rem_user_deflevel;
	}
/* See if users level is below minlogin level */
if (u->level<minlogin_level) {
	if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=3) 
		sprintf(text,"DENIED %s 8\n",u->name); /* new error for 3.3.3 */
	else sprintf(text,"DENIED %s 6\n",u->name); /* old error to old versions */
	write_sock(nl->socket,text);
	destruct_user(u);
	destructed=0;
	return;
	}
strcpy(u->site,nl->service);
sprintf(text,"%s enters from cyberspace.\n",u->name);
write_room(nl->connect_room,text);
sprintf(text,"NETLINK: Remote user %s received from %s.\n",u->name,nl->service);
write_syslog(text,1);
u->room=nl->connect_room;
u->netlink=nl;
u->read_mail=time(0);
u->last_login=time(0);
num_of_users++;
sprintf(text,"GRANTED %s\n",name);
write_sock(nl->socket,text);
}
		

/*** User is leaving this system ***/
nl_release(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if ((u=get_user(name))!=NULL && u->type==REMOTE_TYPE) {
	sprintf(text,"%s leaves this plain of existence.\n",u->name);
	write_room_except(u->room,text,u);
	sprintf(text,"NETLINK: Remote user %s released.\n",u->name);
	write_syslog(text,1);
	destroy_user_clones(u);
	destruct_user(u);
	num_of_users--;
	return;
	}
sprintf(text,"NETLINK: Release requested for unknown/invalid user %s from %s.\n",name,nl->service);
write_syslog(text,1);
}


/*** Remote user performs an action on this system ***/
nl_action(nl,name,inpstr)
NL_OBJECT nl;
char *name,*inpstr;
{
UR_OBJECT u;
char *c,ctemp;

if (!(u=get_user(name))) {
	sprintf(text,"DENIED %s 8\n",name);
	write_sock(nl->socket,text);
	return;
	}
if (u->socket!=-1) {
	sprintf(text,"NETLINK: Action requested for local user %s from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
inpstr=remove_first(remove_first(inpstr));
/* remove newline character */
c=inpstr; ctemp='\0';
while(*c) {
	if (*c=='\n') {  ctemp=*c;  *c='\0';  break;  }
	++c;
	}
u->last_input=time(0);
if (u->misc_op) {
	if (!strcmp(inpstr,"NL")) misc_ops(u,"\n");  
	else misc_ops(u,inpstr+4);
	return;
	}
if (u->afk) {
	write_user(u,"You are no longer AFK.\n");  
	if (u->vis) {
		sprintf(text,"%s comes back from being AFK.\n",u->name);
		write_room_except(u->room,text,u);
		}
	u->afk=0;
	}
word_count=wordfind(inpstr);
if (!strcmp(inpstr,"NL")) return; 
exec_com(u,inpstr);
if (ctemp) *c=ctemp;
if (!u->misc_op) prompt(u);
}


/*** Grant received from remote system ***/
nl_granted(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;
RM_OBJECT old_room;

if (!strcmp(name,"CONNECT")) {
	sprintf(text,"NETLINK: Connection to %s granted.\n",nl->service);
	write_syslog(text,1);
	/* Send our verification and version number */
	sprintf(text,"VERIFICATION %s %s\n",verification,VERSION);
	write_sock(nl->socket,text);
	return;
	}
if (!(u=get_user(name))) {
	sprintf(text,"NETLINK: Grant received for unknown user %s from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
/* This will probably occur if a user tried to go to the other site , got 
   lagged then changed his mind and went elsewhere. Don't worry about it. */
if (u->remote_com!=GO) {
	sprintf(text,"NETLINK: Unexpected grant for %s received from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
/* User has been granted permission to move into remote talker */
write_user(u,"~FB~OLYou traverse cyberspace...\n");
if (u->vis) {
	sprintf(text,"%s %s to the %s.\n",u->name,u->out_phrase,nl->service);
	write_room_except(u->room,text,u);
	}
else write_room_except(u->room,invisleave,u);
sprintf(text,"NETLINK: %s transfered to %s.\n",u->name,nl->service);
write_syslog(text,1);
old_room=u->room;
u->room=NULL; /* Means on remote talker */
u->netlink=nl;
u->pot_netlink=NULL;
u->remote_com=-1;
u->misc_op=0;  
u->filepos=0;  
u->page_file[0]='\0';
reset_access(old_room);
sprintf(text,"ACT %s look\n",u->name);
write_sock(nl->socket,text);
}


/*** Deny received from remote system ***/
nl_denied(nl,name,inpstr)
NL_OBJECT nl;
char *name,*inpstr;
{
UR_OBJECT u,create_user();
int errnum;
char *neterr[]={
"this site is not in the remote services valid sites list",
"the remote service is unable to create a link",
"the remote service has no free room links",
"the link is for incoming users only",
"a user with your name is already logged on the remote site",
"the remote service was unable to create a session for you",
"incorrect password. Use '.go <service> <remote password>'",
"your level there is below the remote services current minlogin level",
"you are banned from that service"
};

errnum=0;
sscanf(remove_first(remove_first(inpstr)),"%d",&errnum);
if (!strcmp(name,"CONNECT")) {
	sprintf(text,"NETLINK: Connection to %s denied, %s.\n",nl->service,neterr[errnum-1]);
	write_syslog(text,1);
	/* If wiz initiated connect let them know its failed */
	sprintf(text,"~OLSYSTEM:~RS Connection to %s failed, %s.\n",nl->service,neterr[errnum-1]);
	write_level(com_level[CONN],1,text,NULL);
	close(nl->socket);
	nl->type=UNCONNECTED;
	nl->stage=DOWN;
	return;
	}
/* Is for a user */
if (!(u=get_user(name))) {
	sprintf(text,"NETLINK: Deny for unknown user %s received from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
sprintf(text,"NETLINK: Deny %d for user %s received from %s.\n",errnum,name,nl->service);
write_syslog(text,1);
sprintf(text,"Sorry, %s.\n",neterr[errnum-1]);
write_user(u,text);
prompt(u);
u->remote_com=-1;
u->pot_netlink=NULL;
}


/*** Text received to display to a user on here ***/
nl_mesg(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
	sprintf(text,"NETLINK: Message received for unknown user %s from %s.\n",name,nl->service);
	write_syslog(text,1);
	nl->mesg_user=(UR_OBJECT)-1;
	return;
	}
nl->mesg_user=u;
}


/*** Remote system asking for prompt to be displayed ***/
nl_prompt(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
	sprintf(text,"NETLINK: Prompt received for unknown user %s from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
if (u->type==REMOTE_TYPE) {
	sprintf(text,"NETLINK: Prompt received for remote user %s from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
prompt(u);
}


/*** Verification received from remote site ***/
nl_verification(nl,w2,w3,com)
NL_OBJECT nl;
char *w2,*w3;
int com;
{
NL_OBJECT nl2;

if (!com) {
	/* We're verifiying a remote site */
	if (!w2[0]) {
		shutdown_netlink(nl);  return;
		}
	for(nl2=nl_first;nl2!=NULL;nl2=nl2->next) {
		if (!strcmp(nl->site,nl2->site) && !strcmp(w2,nl2->verification)) {
			switch(nl->allow) {
				case IN : write_sock(nl->socket,"VERIFY OK IN\n");  break;
				case OUT: write_sock(nl->socket,"VERIFY OK OUT\n");  break;
				case ALL: write_sock(nl->socket,"VERIFY OK ALL\n"); 
				}
			strcpy(nl->service,nl2->service);

			/* Only 3.2.0 and above send version number with verification */
			sscanf(w3,"%d.%d.%d",&nl->ver_major,&nl->ver_minor,&nl->ver_patch);
			sprintf(text,"NETLINK: Connected to %s in the %s.\n",nl->service,nl->connect_room->name);
			write_syslog(text,1);
			sprintf(text,"~OLSYSTEM:~RS New connection to service %s in the %s.\n",nl->service,nl->connect_room->name);
			write_room(NULL,text);
			return;
			}
		}
	write_sock(nl->socket,"VERIFY BAD\n");
	shutdown_netlink(nl);
	return;
	}
/* The remote site has verified us */
if (!strcmp(w2,"OK")) {
	/* Set link permissions */
	if (!strcmp(w3,"OUT")) {
		if (nl->allow==OUT) {
			sprintf(text,"NETLINK: WARNING - Permissions deadlock, both sides are outgoing only.\n");
			write_syslog(text,1);
			}
		else nl->allow=IN;
		}
	else {
		if (!strcmp(w3,"IN")) {
			if (nl->allow==IN) {
				sprintf(text,"NETLINK: WARNING - Permissions deadlock, both sides are incoming only.\n");
				write_syslog(text,1);
				}
			else nl->allow=OUT;
			}
		}
	sprintf(text,"NETLINK: Connection to %s verified.\n",nl->service);
	write_syslog(text,1);
	sprintf(text,"~OLSYSTEM:~RS New connection to service %s in the %s.\n",nl->service,nl->connect_room);
	write_room(NULL,text);
	return;
	}
if (!strcmp(w2,"BAD")) {
	sprintf(text,"NETLINK: Connection to %s has bad verification.\n",nl->service);
	write_syslog(text,1);
	/* Let wizes know its failed , may be wiz initiated */
	sprintf(text,"~OLSYSTEM:~RS Connection to %s failed, bad verification.\n",nl->service);
	write_level(com_level[CONN],1,text,NULL);
	shutdown_netlink(nl);  
	return;
	}
sprintf(text,"NETLINK: Unknown verify return code from %s.\n",nl->service);
write_syslog(text,1);
shutdown_netlink(nl);
}


/* Remote site only sends REMVD (removed) notification if user on remote site 
   tries to .go back to his home site or user is booted off. Home site doesn't
   bother sending reply since remote site will remove user no matter what. */
nl_removed(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
	sprintf(text,"NETLINK: Removed notification for unknown user %s received from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
if (u->room!=NULL) {
	sprintf(text,"NETLINK: Removed notification of local user %s received from %s.\n",name,nl->service);
	write_syslog(text,1);
	return;
	}
sprintf(text,"NETLINK: %s returned from %s.\n",u->name,u->netlink->service);
write_syslog(text,1);
u->room=u->netlink->connect_room;
u->netlink=NULL;
if (u->vis) {
	sprintf(text,"%s %s\n",u->name,u->in_phrase);
	write_room_except(u->room,text,u);
	}
else write_room_except(u->room,invisenter,u);
look(u);
prompt(u);
}


/*** Got an error back from site, deal with it ***/
nl_error(nl)
NL_OBJECT nl;
{
if (nl->mesg_user!=NULL) nl->mesg_user=NULL;
/* lastcom value may be misleading, the talker may have sent off a whole load
   of commands before it gets a response due to lag, any one of them could
   have caused the error */
sprintf(text,"NETLINK: Received ERROR from %s, lastcom = %d.\n",nl->service,nl->lastcom);
write_syslog(text,1);
}


/*** Does user exist? This is a question sent by a remote mailer to
     verifiy mail id's. ***/
nl_checkexist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
FILE *fp;
char filename[80];

sprintf(filename,"%s/%s.D",USERFILES,to);
if (!(fp=fopen(filename,"r"))) {
	sprintf(text,"EXISTS_NO %s %s\n",to,from);
	write_sock(nl->socket,text);
	return;
	}
fclose(fp);
sprintf(text,"EXISTS_YES %s %s\n",to,from);
write_sock(nl->socket,text);
}


/*** Remote user doesnt exist ***/
nl_user_notexist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;
char filename[80];
char text2[ARR_SIZE];

if ((user=get_user(from))!=NULL) {
	sprintf(text,"~OLSYSTEM:~RS User %s does not exist at %s, your mail bounced.\n",to,nl->service);
	write_user(user,text);
	}
else {
	sprintf(text2,"There is no user named %s at %s, your mail bounced.\n",to,nl->service);
	send_mail(NULL,from,text2,0);
	}
sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,from,to,nl->service);
unlink(filename);
}


/*** Remote users exists, send him some mail ***/
nl_user_exist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;
FILE *fp;
char text2[ARR_SIZE],filename[80],line[82];

sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,from,to,nl->service);
if (!(fp=fopen(filename,"r"))) {
	if ((user=get_user(from))!=NULL) {
		sprintf(text,"~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",to,nl->service);
		write_user(user,text);
		}
	else {
		sprintf(text2,"An error occured during mail delivery to %s@%s.\n",to,nl->service);
		send_mail(NULL,from,text2,0);
		}
	return;
	}
sprintf(text,"MAIL %s %s\n",to,from);
write_sock(nl->socket,text);
fgets(line,80,fp);
while(!feof(fp)) {
	write_sock(nl->socket,line);
	fgets(line,80,fp);
	}
fclose(fp);
write_sock(nl->socket,"\nENDMAIL\n");
unlink(filename);
}


/*** Got some mail coming in ***/
nl_mail(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
char filename[80];

sprintf(text,"NETLINK: Mail received for %s from %s.\n",to,nl->service);
write_syslog(text,1);
sprintf(filename,"%s/IN_%s_%s@%s",MAILSPOOL,to,from,nl->service);
if (!(nl->mailfile=fopen(filename,"w"))) {
	sprintf(text,"ERROR: Couldn't open file %s to write in nl_mail().\n",filename);
	write_syslog(text,0);
	sprintf(text,"MAILERROR %s %s\n",to,from);
	write_sock(nl->socket,text);
	return;
	}
strcpy(nl->mail_to,to);
strcpy(nl->mail_from,from);
}


/*** End of mail message being sent from remote site ***/
nl_endmail(nl)
NL_OBJECT nl;
{
FILE *infp,*outfp;
char c,infile[80],mailfile[80],line[DNL+1];

fclose(nl->mailfile);
nl->mailfile=NULL;

sprintf(mailfile,"%s/IN_%s_%s@%s",MAILSPOOL,nl->mail_to,nl->mail_from,nl->service);

/* Copy to users mail file to a tempfile */
if (!(outfp=fopen("tempfile","w"))) {
	write_syslog("ERROR: Couldn't open tempfile in netlink_endmail().\n",0);
	sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
	write_sock(nl->socket,text);
	goto END;
	}
fprintf(outfp,"%d\r",(int)time(0));

/* Copy old mail file to tempfile */
sprintf(infile,"%s/%s.M",USERFILES,nl->mail_to);
if (!(infp=fopen(infile,"r"))) goto SKIP;
fgets(line,DNL,infp);
c=getc(infp);
while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }
fclose(infp);

/* Copy received file */
SKIP:
if (!(infp=fopen(mailfile,"r"))) {
	sprintf(text,"ERROR: Couldn't open file %s to read in netlink_endmail().\n",mailfile);
	write_syslog(text,0);
	sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
	write_sock(nl->socket,text);
	goto END;
	}
fprintf(outfp,"~OLFrom: %s@%s  %s",nl->mail_from,nl->service,long_date(0));
c=getc(infp);
while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }
fclose(infp);
fclose(outfp);
rename("tempfile",infile);
write_user(get_user(nl->mail_to),"\07~OL~FR-- MAILDAEMON : ~LIYou have new mail!\n");

END:
nl->mail_to[0]='\0';
nl->mail_from[0]='\0';
unlink(mailfile);
}


/*** An error occured at remote site ***/
nl_mailerror(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;

if ((user=get_user(from))!=NULL) {
	sprintf(text,"~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",to,nl->service);
	write_user(user,text);
	}
else {
	sprintf(text,"An error occured during mail delivery to %s@%s.\n",to,nl->service);
	send_mail(NULL,from,text,0);
	}
}


/*** Send statistics of this server to requesting user on remote site ***/
nl_rstat(nl,to)
NL_OBJECT nl;
char *to;
{
char str[80];

gethostname(str,80);
if (nl->ver_major<=3 && nl->ver_minor<2)
	sprintf(text,"MSG %s\n\n*** Remote statistics ***\n\n",to);
else sprintf(text,"MSG %s\n\n~BB*** Remote statistics ***\n\n",to);
write_sock(nl->socket,text);
sprintf(text,"RNUTS version        : %s\nHost                 : %s\n",RVERSION,str);
write_sock(nl->socket,text);
sprintf(text,"Ports (Main/Wiz/Link): %d ,%d, %d\n",port[0],port[1],port[2]);
write_sock(nl->socket,text);
sprintf(text,"Number of users      : %d\nRemote user maxlevel : %s\n",num_of_users,level_name[rem_user_maxlevel]);
write_sock(nl->socket,text);
sprintf(text,"Remote user deflevel : %s\n\nEMSG\nPRM %s\n",level_name[rem_user_deflevel],to);
write_sock(nl->socket,text);
}


/*** Shutdown the netlink and pull any remote users back home ***/
shutdown_netlink(nl)
NL_OBJECT nl;
{
UR_OBJECT u;
char mailfile[80];

if (nl->type==UNCONNECTED) return;

/* See if any mail halfway through being sent */
if (nl->mail_to[0]) {
	sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
	write_sock(nl->socket,text);
	fclose(nl->mailfile);
	sprintf(mailfile,"%s/IN_%s_%s@%s",MAILSPOOL,nl->mail_to,nl->mail_from,nl->service);
	unlink(mailfile);
	nl->mail_to[0]='\0';
	nl->mail_from[0]='\0';
	}
write_sock(nl->socket,"DISCONNECT\n");
close(nl->socket);  
for(u=user_first;u!=NULL;u=u->next) {
	if (u->pot_netlink==nl) {  u->remote_com=-1;  continue;  }
	if (u->netlink==nl) {
		if (u->room==NULL) {
			write_user(u,"~FB~OLYou feel yourself dragged back across the ether...\n");
			u->room=u->netlink->connect_room;
			u->netlink=NULL;
			if (u->vis) {
				sprintf(text,"%s %s\n",u->name,u->in_phrase);
				write_room_except(u->room,text,u);
				}
			else write_room_except(u->room,invisenter,u);
			look(u);  prompt(u);
			sprintf(text,"NETLINK: %s recovered from %s.\n",u->name,nl->service);
			write_syslog(text,1);
			continue;
			}
		if (u->type==REMOTE_TYPE) {
			sprintf(text,"%s vanishes!\n",u->name);
			write_room(u->room,text);
			destruct_user(u);
			num_of_users--;
			}
		}
	}
if (nl->stage==UP) 
	sprintf(text,"NETLINK: Disconnected from %s.\n",nl->service);
else sprintf(text,"NETLINK: Disconnected from site %s.\n",nl->site);
write_syslog(text,1);
if (nl->type==INCOMING) {
	nl->connect_room->netlink=NULL;
	destruct_netlink(nl);  
	return;
	}
nl->type=UNCONNECTED;
nl->stage=DOWN;
nl->warned=0;
}



/*************** START OF COMMAND FUNCTIONS AND THEIR SUBSIDS **************/

/*** Deal with user input ***/
exec_com(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i,len;
char filename[80],*comword=NULL;

com_num=-1;
if (word[0][0]=='.') comword=(word[0]+1);
else comword=word[0];
if (!comword[0]) {
	write_user(user,"Unknown command.\n");  return;
	}

/* get com_num */
if (!strcmp(word[0],">")) strcpy(word[0],"tell");
if (!strcmp(word[0],"<")) strcpy(word[0],"pemote");
if (!strcmp(word[0],"-")) strcpy(word[0],"echo");
if (!strcmp(word[0],"!")) strcpy(word[0],"shout");
if (inpstr[0]==';') strcpy(word[0],"emote");
else if (inpstr[0]=='#') strcpy(word[0],"semote");
	else inpstr=remove_first(inpstr);

i=0;
len=strlen(comword);
while(command[i][0]!='*') {
	if (!strncmp(command[i],comword,len)) {  com_num=i;  break;  }
	++i;
	}
if (user->room!=NULL && (com_num==-1 || com_level[com_num] > user->level)) {
	write_user(user,"Unknown command.\n");  return;
	}
/* See if user has gone across a netlink and if so then intercept certain 
   commands to be run on home site */
if (user->room==NULL) {
	switch(com_num) {
		case HOME: 
		case QUIT:
		case MODE:
		case PROMPT: 
		case COLOUR:
		case REBOOT:
		case SUICIDE:
		case SHUTDOWN: 
		case CHARECHO:
		write_user(user,"~FY~OL*** Home execution ***\n");  break;

		default:
		sprintf(text,"ACT %s %s %s\n",user->name,word[0],inpstr);
		write_sock(user->netlink->socket,text);
		no_prompt=1;
		return;
		}
	}
/* Dont want certain commands executed by remote users */
if (user->type==REMOTE_TYPE) {
	switch(com_num) {
		case PASSWD :
		case ENTPRO :
		case ACCREQ :
		case CONN   :
		case DISCONN:
			write_user(user,"Sorry, remote users cannot use that command.\n");
			return;
		}
	}

/* Main switch */
switch(com_num) {
	case QUIT	: quit(user);  			break;
	case LOOK	: look(user);  			break;
	case MODE	: toggle_mode(user);  		break;
	case SAY : 
		if (word_count<2) {
			write_user(user,"Say what?\n");  return;
			}
		if (nuts_talk_style==1) say_new(user,inpstr);
		else say_old(user,inpstr);
		break;
	case SHOUT 	: shout(user,inpstr);  		break;
	case TELL  	: tell(user,inpstr);   		break;
	case EMOTE 	: emote(user,inpstr);  		break;
	case SEMOTE	: semote(user,inpstr); 		break;
	case PEMOTE	: pemote(user,inpstr); 		break;
	case ECHO  	: echo(user,inpstr);   		break;
	case GO    	: go(user);  			break;
	case IGNALL	: toggle_ignall(user);  	break;
	case PROMPT	: toggle_prompt(user);  	break;
	case DESC  	: set_desc(user,inpstr);  	break;
	case INPHRASE 	: 
	case OUTPHRASE	: set_iophrase(user,inpstr);  	break; 
	case PUBCOM 	:
	case PRIVCOM	: set_room_access(user);  	break;
	case LETMEIN	: letmein(user);  		break;
	case INVITE 	: invite(user);   		break;
	case TOPIC  	: set_topic(user,inpstr);  	break;
	case MOVE   	: move(user);  			break;
	case BCAST  	: bcast(user,inpstr);  		break;
	case WHO    	: who(user,0);  		break;
	case PEOPLE 	: who(user,1);  		break;
	case HELP   	: help(user);  			break;
	case SHUTDOWN	: shutdown_com(user); 		break;
	case WIZZERLIST : wizon(user);			break;
        case WANNABE:
                sprintf(filename,"%s",WANBE);
                switch(more(user,user->socket,filename)) {
                        case 0: write_user(user,"There is no wannabe  file.\n"); break;
                        case 1: user->misc_op=2;
                        }  
                break;
        case VIEWMOTD2:
                sprintf(filename,"%s",MOTD2);
                switch(more(user,user->socket,filename)) {
                        case 0: write_user(user,"There is no news.\n"); break;
                        case 1: user->misc_op=2;
                        }
                break;
	case READ  	: read_board(user);  		break;
	case WRITE 	: write_board(user,inpstr,0);  	break;
	case WIPE  	: wipe_board(user);  		break;
	case SEARCH	: search_boards(user);  	break;
	case REVIEW	: review(user);  		break;
	case HOME  	: home(user);  			break;
	case STATUS	: ustatus(user);  		break;
	case VER:
		sprintf(filename,"%s/versionfile",DATAFILES);
		switch(more(user,user->socket,filename)) {
			case 0: write_user(user,"Unable to find the version file.\n");
			case 1: user->misc_op=2;
			}
		break;
	case RMAIL   	: rmail(user,0);  		break;
	case SMAIL   	: smail(user,inpstr,0);  	break;
	case DMAIL   	: dmail(user,0);  		break;
	case FROM    	: mail_from(user);  		break;
	case ENTPRO  	: enter_profile(user,0);  	break;
	case EXAMINE 	: examine(user);  		break;
	case RMST    	: rooms(user,1);  		break;
	case RMSN    	: rooms(user,0);  		break;
	case NETSTAT 	: netstat(user);  		break;
	case NETDATA 	: netdata(user);  		break;
	case CONN    	: connect_netlink(user);  	break;
	case DISCONN 	: disconnect_netlink(user);  	break;
	case PASSWD  	: change_pass(user);  		break;
	case KILL    	: kill_user(user);  		break;
	case PROMOTE 	: promote(user);  		break;
	case DEMOTE  	: demote(user);  		break;
	case LISTBANS	: listbans(user);  		break;
	case BAN     	: ban(user);  			break;
	case UNBAN   	: unban(user);  		break;
	case VIS     	: visibility(user,1);  		break;
	case INVIS   	: visibility(user,0);  		break;
	case SITE    	: site(user);  			break;
	case WAKE    	: wake(user,inpstr);  		break;
	case WTELL   	: wizshout(user,inpstr,0);  	break;
	case WEMOTE  	: wizshout(user,inpstr,1);  	break;
	case MUZZLE  	: muzzle(user);  		break;
	case UNMUZZLE	: unmuzzle(user);  		break;
	case MAP:
		sprintf(filename,"%s/%s",DATAFILES,MAPFILE);
		switch(more(user,user->socket,filename)) {
			case 0: write_user(user,"There is no map.\n");  break;
			case 1: user->misc_op=2;
			}
		break;
	case LOGGING  	: logging(user); 		break;
	case MINLOGIN 	: minlogin(user);  		break;
	case SYSTEM   	: system_details(user);  	break;
	case CHARECHO 	: toggle_charecho(user);  	break;
	case CLEARLINE	: clearline(user);  		break;
	case FIX      	: change_room_fix(user,1);  	break;
	case UNFIX    	: change_room_fix(user,0);  	break;
	case VIEWLOG  	: view_log(user,inpstr);  	break;
	case ACCREQ   	: account_request(user,inpstr); break;
	case REVCLR   	: revclr(user);  		break;
	case CREATE   	: create_clone(user);  		break;
	case DESTROY  	: destroy_clone(user);  	break;
	case MYCLONES 	: myclones(user);  		break;
	case ALLCLONES	: allclones(user);  		break;
	case SWITCH	: clone_switch(user);  		break;
	case CSAY  	: clone_say(user,inpstr);  	break;
	case CHEAR 	: clone_hear(user);  		break;
	case RSTAT 	: remote_stat(user); 		break;
	case SWBAN 	: swban(user);  		break;
	case AFK   	: afk(user,inpstr);  		break;
	case CLS   	: cls(user);  			break;
	case COLOUR  	: toggle_colour(user);  	break;
	case IGNSHOUT	: toggle_ignshout(user);  	break;
	case IGNTELL 	: toggle_igntell(user);  	break;
	case SUICIDE 	: suicide(user);  		break;
	case NUKE     	: delete_user(user,0);  	break;
	case REBOOT  	: reboot_com(user);  		break;
	case RECOUNT 	: check_messages(user,2);  	break;
	case REVTELL 	: revtell(user);  		break;

	case MATCH	: matchsite(user,0);		break;
	case WHOIS	: siohw(user,inpstr);		break;
	case NSLOOKUP	: pukoolsn(user,inpstr);	break;
	case SYSFINGER	: regnif(user,inpstr);		break;
	case NERF	: nerf(user,inpstr);		break;
	case CNERF	: charge(user);			break;
	case POP	: pop_level(user,inpstr);	break;
        case VERIFY 	: verify_email(user);  		break;
        case FORWARDING:
           switch(forwarding) {
              case 0: write_user(user,"You have turned ~FGon~RS smail auto-forwarding.\n");
                      forwarding=1;
                      sprintf(text,"%s turned ON mail forwarding.\n",user->name);
                      write_syslog(text,1,SYSLOG);
                      break;
              case 1: write_user(user,"You have turned ~FRoff~RS smail auto-forwarding.\n");
                      forwarding=0;
                      sprintf(text,"%s turned OFF mail forwarding.\n",user->name);
                      write_syslog(text,1,SYSLOG);
                      break;
              }
              break;
        case COPYTO	: copies_to(user);  		break;
        case NOCOPIES	: copies_to(user);  		break;
	case HANGMAN	: play_hangman(user);   	break;
        case GUESS	: guess_hangman(user);  	break; 
	case SESSION	: session(user,inpstr);		break;
	case COMMENT	: comment(user,inpstr);		break;
	case FORCE	: force(user,inpstr);		break;
	case REVSHOUT	: revshout(user);		break;
	case CSHOUT	: clear_shouts();
		sprintf(text,"\nShout buffer has now been cleared by %s.\n\n",user->name);
		write_room(NULL,text);
		break;
	case CTELLS	: clear_tells(user);
		write_user(user,"\nYour tells have now been cleared.\n\n");
		break;
        case TICTAC	: tictac(user); 		break;
	case LIST	: list_users(user);		break;
	case CURSE 	: curse(user);			break; 	
	case ENCHANT	: remove_curse(user);		break;
	case LAST   	: last_login(user);		break;
	case ARREST 	: jail_user(user); 		break;
	case RELEASE	: unjail_user(user);		break;
	case JOIN   	: join(user);			break;
	case BRING  	: bring(user);			break;
	case SHOWCOM 	: show(user,inpstr);		break;
	case TIMED 	: timed(user);			break;
	case SET 	: set(user,inpstr);    		break;
	case FAQ 	: ranks_faq(user,1);		break;
	case RANKS 	: ranks_faq(user,0);		break;
	case DELOLD 	: delold_users(user);		break;
	case USERS 	: users(user);       		break;
        case MACROCMD	: macro(&(user->macrolist),inpstr,user,0); break;
	case ACTIONCMD	: list_action_macros(user); break;
        case SYSMACRO: 
		macro(&(system_macrolist),inpstr,user,0);
		sprintf(text,"%s/%s",DATAFILES,SYSMACROFILE);
		save_macros(&(system_macrolist),text);
		break;
	case SYSACTION: 
		macro(&(system_actionlist),inpstr,user,1);
		sprintf(text,"%s/%s",DATAFILES,SYSACTIONFILE);
		save_macros(&(system_actionlist),text);
		break;
	case RSUGGEST	: rsuggest(user);  		break;
	case SUGGEST 	: suggest(user,inpstr); 	break;
        case IGNORE  	: ignore(user,word[1]); 	break;
        case UNIGNORE	: unignore(user,word[1]);	break;
        case SHOWIGNORE	: showignore(user);		break;
	case DUEL 	: duel(user);  			break;
	case PICTURE 	: picture(user,inpstr); 	break;
	case THROW 	: throw(user);  		break;
	case RECENT 	: recent(user);  		break;
	case RESET	: reset(user);			break;
	case CBOT	: connect_bot(user);		break;
	case QBOT	: destruct_bot(user);		break;
	case BOTACT	: bot_action(user,inpstr);	break;
	case BMAIL	: rmail(user,1);		break;
	case BLACKJACK  : blackjack(user);		break;
	case POKER	: sprintf(filename,"%s",POKERFILE);
			  switch(more(user,user->socket,filename)) {
				case 0: write_user(user,"There are no rules to poker!\n");  break; 
				case 1: user->misc_op=2;
				}
			  break;
	case STARTPO	: if (user->room->access==FIXED_PUBLIC) {
				write_user(user,"~OL~FY~BRYou are not permitted to start poker games here, goto a private room please!\n");
				}
			  else { 
				start_poker(user);
				}
			  break;
	case JOINPO	: if (user->room->access==FIXED_PUBLIC) {
				write_user(user,"~OL~FY~BRYou are not permitted to start poker games here, goto a private room please!\n");
				}
			  else { 
				join_poker(user);
				}
			  break;
	case LEAVE	: leave_poker(user); break;
	case GAMES	: list_poker_games(user); break;
	case SCORE	: show_poker_players(user); break;
	case DEAL	: deal_poker(user); break;
	case FOLD	: fold_poker(user); break;
	case BET	: bet_poker(user); break;
	case CHECK	: check_poker(user); break;
	case RAISE	: raise_poker(user); break;
	case SEE	: see_poker(user); break;
	case DISCPO	: disc_poker(user); break;
	case HAND	: hand_poker(user); break;
	case CHIPS	: chips_casino(user); break;
	case CASINO	: rank_casino(user); break;
	case CONNECT4	: connect_four(user); break;
	default		: write_user(user,"Command not executed in exec_com().\n");
	}	
}

/* get poker game_struct pointer from room */
struct poker_game *get_poker_game_here(room)
RM_OBJECT room;
{
struct poker_game *game;

/* Search for game in the room */
for(game=poker_game_first;game!=NULL;game=game->next) {
	if (game->room==room) return game;
	}
return NULL;
}

connect_bot(user)
UR_OBJECT user;
{
UR_OBJECT u;
        
if(bot_bot!=NULL) {
        write_user(user,"The Bot is already active.\n"); return;
        }
if ((u=create_user())==NULL) {
        write_user(user,"Unable to create bot...\n");
        return;
        }        
strcpy(u->name,BOTNAME);
u->name[0]=toupper(u->name[0]);
if (!load_user_details(u)){
        write_user(user,"Bot details not loaded...\n");
        destruct_user(u);
        destructed=0;
        return;
        }
u->socket=99;
u->port=port[1];
strcpy(u->site,BOTSITE);
u->room=room_first;
u->last_login=time(0);
u->last_input=time(0);
u->login=0;
u->level=BOT;
u->type=BOT_TYPE;
num_of_users++;
bot_bot=u;

sprintf(text,"~OL[ ENTERING ]~RS %s %s ~OL~FY<%s>\n",bot_bot->name,bot_bot->desc,MAINROOM);
write_room(NULL,text);
sprintf(text,"%s activated the Bot \"%s\"\n",user->name,bot_bot->name);
write_syslog(text,1);
}

destruct_bot(user)
UR_OBJECT user;
{
if (bot_bot==NULL) {
    write_user(user,"The Bot isn't active.\n"); return;
    }
sprintf(text,"~OL[ LEAVING ]~RS %s %s\n",bot_bot->name,bot_bot->desc);
write_room(NULL,text);
sprintf(text,"%s deactivated the Bot \"%s\"\n",user->name,bot_bot->name);
write_syslog(text,1);
save_user_details(bot_bot,1);
destruct_user(bot_bot);
destructed=0;
num_of_users--;
bot_bot=NULL;
}

bot_action(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char action[WORD_LEN+1];

if(word_count < 3){
        write_user(user,"Usage : bact <operation> <text>\n");
	write_user(user,"+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
	write_user(user,"| acceptable <operations> are |\n");
	write_user(user,"|                             |\n");
	write_user(user,"|  say       tell     emote   |\n");
	write_user(user,"|  semote    shout    desc    |\n");
        write_user(user,"|  go        kill     set     |\n");
        write_user(user,"|  write                      |\n");
if (user->level==EIG) {
        write_user(user,"|  ~FTpromote   demote   force~RS   |\n");
        write_user(user,"|  ~FTdmail~RS                      |\n");
        }
	write_user(user,"|                             |\n");
	write_user(user,"+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
        return;
        }
if (bot_bot==NULL) { 
        write_user(user,"The Bot isn't active.\n");
        return;
        }
strcpy(action,word[1]);
inpstr=remove_first(inpstr);
strcpy(word[1],word[2]) ;
if(!strcmp(action,"say")) { 
	if (nuts_talk_style == 1) say_new(bot_bot,inpstr);
	else say_old(bot_bot,inpstr);
	}
if(!strcmp(action,"tell"))    tell(bot_bot,inpstr);
if(!strcmp(action,"emote"))   emote(bot_bot,inpstr);
if(!strcmp(action,"shout"))   shout(bot_bot,inpstr);
if(!strcmp(action,"semote"))  semote(bot_bot,inpstr);
if(!strcmp(action,"desc"))    set_desc(bot_bot,inpstr);
if(!strcmp(action,"go"))      go(bot_bot);
if(!strcmp(action,"set"))     set(bot_bot,inpstr);
if(!strcmp(action,"kill"))    kill_user(bot_bot);
if(!strcmp(action,"write"))   write_board(bot_bot,inpstr,0);

if ((!strcmp(action,"promote"))&&(user->level==EIG))
        promote(bot_bot);
if ((!strcmp(action,"demote"))&&(user->level==EIG))
        demote(bot_bot);
if ((!strcmp(action,"dmail"))&&(user->level==EIG))
	dmail(user,1);
if ((!strcmp(action,"force"))&&(user->level==EIG))
        force(bot_bot,inpstr);
}


/*** Display details of room ***/
look(user)
UR_OBJECT user;
{
RM_OBJECT rm;
UR_OBJECT u;
char temp[81],*ptr;
int i,exits,users;
char *bjack="~OL~FT(21)";
        
rm=user->room;
write_user(user,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
write_user(user,user->room->desc);
write_user(user,"\n");

users=0;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->room!=rm || u==user || (!u->vis && u->level>user->level))
                continue;
        if (!users++) write_user(user,"~FTYou see :\n");
        if (u->afk) ptr="~OL~FR(AFK)\n"; 
	else if (u->blackjack_status) ptr=bjack;
	else ptr="\n";
        if (!u->vis) sprintf(text,"     ~FR*~RS%s %s~RS %s",u->name,u->desc,ptr);
        else sprintf(text,"      %s %s~RS  %s",u->name,u->desc,ptr);
        write_user(user,text);
        }
if (!users) write_user(user,"~FTYou see :  ~RSAbsolutely nobody\n");
else write_user(user,"\n");

exits=0; 
strcpy(text,"~FTExits   :");  
for(i=0;i<MAX_LINKS;++i) {
        if (rm->link[i]==NULL) break;
        if (rm->link[i]->access & PRIVATE)
                sprintf(temp,"  ~FR%s",rm->link[i]->name);
        else sprintf(temp,"  ~FY%s",rm->link[i]->name);
        strcat(text,temp); 
        ++exits;
        }
if (rm->netlink!=NULL && rm->netlink->stage==UP) {
        if (rm->netlink->allow==IN) sprintf(temp," ~FR%s*",rm->netlink->service);
        else sprintf(temp,"  ~FG%s*",rm->netlink->service);
        strcat(text,temp);
        }
else if (!exits) strcpy(text,"~FTExits   :  ~FGThere are no exits.");
strcat(text,"\n");  
write_user(user,text); 
         
if (rm->access & PRIVATE) sprintf(text,"~FTRoom    :  ~FR%-20s",rm->name);
else sprintf(text,"~FTRoom    :  ~FG%-20s",rm->name);
write_user(user,text);

if (rm->topic[0]) {
        sprintf(text,"~FTTopic   :  ~FY%s\n",rm->topic);
        write_user(user,text);
        }
else write_user(user,"~FTTopic   :  ~FGNo topic has been set yet.\n");

strcpy(text,"~FTAccess  :");
switch(rm->access) {  
        case PUBLIC:        strcat(text,"  ~FGPublic~RS       ");  break;
        case PRIVATE:       strcat(text,"  ~FRPrivate~RS      ");  break;
        case FIXED_PUBLIC:  strcat(text,"  ~FRFixed~RS:~FGPublic~RS "); break;
        case FIXED_PRIVATE: strcat(text,"  ~FRFixed~RS:~FRPrivate~RS"); break;
	}

sprintf(temp,"       ~FTMessages:  ~FG%d\n",rm->mesg_cnt);
strcat(text,temp);
write_user(user,text);

write_user(user,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
} 



/*** Switch between command and speech mode ***/
toggle_mode(user)
UR_OBJECT user;
{
if (user->command_mode) {
	write_user(user,"Now in SPEECH mode.\n");
	user->command_mode=0;  return;
	}
write_user(user,"Now in COMMAND mode.\n");
user->command_mode=1;
}


/*** Shutdown the talker ***/
talker_shutdown(user,str,reboot)
UR_OBJECT user;
char *str;
int reboot;
{
UR_OBJECT u;
NL_OBJECT nl;
int i;
char *ptr;
char *args[]={ progname,confile,NULL };

if (user!=NULL) ptr=user->name; else ptr=str;
if (reboot) {
	write_room(NULL,"\07\n~OL~FR-- SYSTEM : ~LIRebooting now!\n\n");
	sprintf(text,"*** REBOOT initiated by %s ***\n",ptr);
	}
else {
	write_room(NULL,"\07\n~OL~FR-- SYSTEM : ~LIShutting down now!\n\n");
	sprintf(text,"*** SHUTDOWN initiated by %s ***\n",ptr);
	}
write_syslog(text,0);
for(nl=nl_first;nl!=NULL;nl=nl->next) shutdown_netlink(nl);
for(u=user_first;u!=NULL;u=u->next) disconnect_user(u);
for(i=0;i<3;++i) close(listen_sock[i]); 
if (reboot) {
	sprintf(text,"*** REBOOT compleste %s\n\n",long_date(1));
	write_syslog(text,0);
	talker_exit(0);
	}
sprintf(text,"*** SHUTDOWN complete %s ***\n\n",long_date(1));
write_syslog(text,0);
talker_exit(1);
}


talker_exit(shutdown)
int shutdown;
{
UR_OBJECT user;
FILE *fp;

if (shutdown) {
	fp=fopen("bootexit","w");
	fprintf(fp,"SHUTDOWN\n");        
	fclose (fp);
	}
else {
	fp=fopen("bootexit","w");
	fprintf(fp,"REBOOT\n");
	fclose(fp);
	}
exit(0);
}


/*** Say user speech. ***/
say_old(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i=0,num=0;
char type[10],*name,*str;

if (user->muzzled) {
        write_user(user,"You are muzzled, you cannot speak.\n");  return;
        }
if (user->room==NULL) {
        sprintf(text,"ACT %s say %s\n",user->name,inpstr);
        write_sock(user->netlink->socket,text);
        no_prompt=1;
        return;
        }
if (word_count<2 && user->command_mode) {
        write_user(user,"Say what?\n");  return;
        }
switch(inpstr[strlen(inpstr)-1]) {
     	case '?': strcpy(type,"ask");  break;
     	case '!': strcpy(type,"exclaim");  break;
     	default : strcpy(type,"say");
      	}
if (user->type==CLONE_TYPE) {
        sprintf(text,"Clone of %s %ss: %s\n",user->name,type,inpstr);
        write_room(user->room,text);
        record(user->room,text);
        return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
if (user->vis) name=user->name; else name=invisname;
if (user->cursed) {
        num=rand()%num_scurse_text;
        sprintf(text,scurse_text[num],user->color,user->name);
        write_user(user,text);
        sprintf(text,scurse_text[num],user->color,name);
        write_room_except(user->room,text,user);
	}
else {
        sprintf(text,"%sYou %s: %s\n",user->color,type,inpstr);
        write_user(user,text);
        sprintf(text,"%s%s %ss: %s\n",user->color,name,type,inpstr); 
        write_room_except(user->room,text,user);  
	}
record(user->room,text);
BotCheck(user,inpstr);
}


say_new(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i=0,num=0;
char type[10],*name,*str;
        
if (user->muzzled) {
        write_user(user,"You are muzzled, you cannot speak.\n");  return;
        }
if (user->room==NULL) {
        sprintf(text,"ACT %s say %s\n",user->name,inpstr);
        write_sock(user->netlink->socket,text);
        no_prompt=1;
        return;
        }
if ((word_count<2 && user->command_mode)||(!strcmp(inpstr, ""))) {
        write_user(user,"Say what?\n");  return;
        }
if (user->type==CLONE_TYPE) {
        sprintf(text,"[Clone of %s] %s\n",user->name,inpstr);
        write_room(user->room,text);
        record(user->room,text);
        return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
if (user->vis) name=user->name; else name=invisname;
if (user->cursed) {
        num=rand()%num_scurse_text;
        sprintf(text,scurse_text[num],user->color,user->name);
        write_user(user,text);
        sprintf(text,scurse_text[num],user->color,name);
        write_room_except(user->room,text,user);
        }
else {
        sprintf(text,"%s[%s] %s\n",user->color,user->name,inpstr);
        write_user(user,text);
        sprintf(text,"%s[%s] %s\n",user->color,name,inpstr);
        write_room_except(user->room,text,user);
        }
record(user->room,text);
BotCheck(user,inpstr);
}


wake(user,inpstr)
UR_OBJECT user;
char* inpstr;
{
UR_OBJECT u;
char *name;

if (word_count<3) {
        write_user(user,"Usage: wake <user> <message>\n");  return;
        }
if (user->muzzled) {
        write_user(user,"You are muzzled, you cannot wake anyone.\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"Trying to wake yourself up is the eighth sign of madness.\n");
        return;
        }
if (u->afk) {
        write_user(user,"You cannot wake someone who is AFK.\n");  return;
        }
inpstr=remove_first(inpstr);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"\07\n~BR*** ~OL~LIWAKE UP!!!~RS~BR ***\nFrom %s: %s\n\n",name,inpstr);
write_user(u,text);
sprintf(text,"Wake up call sent to %s.\n",u->name);
write_user(user,text);
}


/** Broadcast an important message **/
bcast(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (word_count<2) {
        write_user(user,"Usage: bcast <message>\n");  return;
        }
if (user->muzzled) {
        write_user(user,"You are muzzled, you cannot broadcast anything.\n");
        return;   
        } 
force_listen=1;
if (user->vis)
        sprintf(text,"\07\n~BR*** Broadcast message from %s ***\n%s\n\n",user->name,inpstr);
else sprintf(text,"\07\n~BR*** Broadcast message ***\n%s\n\n",inpstr);
write_room(NULL,text);
}


/*** Shout something ***/
shout(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;
int num=0;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot shout.\n");  return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW...noone can hear you.\n",JAILROOM);
        write_user(user,text); return;
        }       
if ((word_count<2)||(!strcmp(inpstr,""))) {
	write_user(user,"Shout what?\n");  return;
	}
if (ban_swearing && contains_swearing(inpstr)) {
	write_user(user,noswearing);  return;
	}
if (user->vis) name=user->name; else name=invisname;
if (user->cursed) {
        num=rand()%num_shcurse_text;
        sprintf(text,shcurse_text[num],user->color,user->name);
        write_user(user,text);
        sprintf(text,shcurse_text[num],user->color,name);
        write_room_except(NULL,text,user);
	}
else {
	sprintf(text,"~OL%sYou shout:~RS %s\n",user->color,inpstr);
	write_user(user,text);
	sprintf(text,"~OL%s%s shouts:~RS %s\n",user->color,name,inpstr);
	write_room_except(NULL,text,user);
	}
record_shout(text);
}


/*** Tell another user something ***/
tell(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char type[5],*name;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot tell anyone anything.\n");  
	return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....noone can here your words.\n",JAILROOM);
        write_user(user,text);
        return;
        }
if ((word_count<3)||(!strcmp(word[2],""))) {
	write_user(user,"Tell who what?\n");  return;
	}
if (!(u=get_user(word[1]))) {
	write_user(user,notloggedon);  return;
	}
if (u==user) {
	write_user(user,"Talking to yourself is the first sign of madness.\n");
	return;
	}
if (u->afk) {
	if (u->afk_mesg[0])
		sprintf(text,"%s is AFK, message is: %s\n",u->name,u->afk_mesg);
	else sprintf(text,"%s is AFK at the moment.\n",u->name);
	write_user(user,text);
	return;
	}
if (u->ignall && (user->level<FIV || u->level>user->level)) {
	if (u->malloc_start!=NULL) 
		sprintf(text,"%s is using the editor at the moment.\n",u->name);
	else sprintf(text,"%s is ignoring everyone at the moment.\n",u->name);
	write_user(user,text);  
	return;
	}
if (u->igntell && (user->level<FIV || u->level>user->level)) {
	sprintf(text,"%s is ignoring tells at the moment.\n",u->name);
	write_user(user,text);
	return;
	}
if (u->room==NULL) {
	sprintf(text,"%s is offsite and would not be able to reply to you.\n",u->name);
	write_user(user,text);
	return;
	}
/* if ( isignored(u,user->name) ) {  
        sprintf(text,"~FR%s is ignoring tells from you.\n",u->name);
        write_user(user,text);
        return;
	}
*/
inpstr=remove_first(inpstr);
sprintf(text,"~OL%s>> To %s: %s~RS\n",user->color,u->name,inpstr);
write_user(user,text);
record_tell(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~OL%s>> From %s: %s~RS\n",user->color,name,inpstr);
write_user(u,text);
record_tell(u,text);
}


/*** Emote something ***/
emote(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;
int num=0;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot emote.\n");  return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....noone can here your words.\n",JAILROOM);
        write_user(user,text);
	return;
        }
if ((word_count<2 && inpstr[1]<33)||(!strcmp(inpstr, ""))) {
	write_user(user,"Emote what?\n");  return;
	}
if (ban_swearing && contains_swearing(inpstr)) {
	write_user(user,noswearing);  return;
	}
if (user->vis) name=user->name; else name=invisname;
if (user->cursed) {
        num=rand()%num_ecurse_text; 
        if (inpstr[0]==';')
                sprintf(text,ecurse_text[num],user->color,name);
        else sprintf(text,ecurse_text[num],user->color,name);
	}
else {
	if (inpstr[0]==';') 
		sprintf(text,"%s%s %s\n",user->color,name,inpstr+1);
	else if (!strcmp(word[1],"'s")) {
		inpstr=remove_first(inpstr);
		sprintf(text,"%s%s's %s\n",user->color,name,inpstr);
		}
	else sprintf(text,"%s%s %s\n",user->color,name,inpstr);
	}
write_room(user->room,text);
record(user->room,text);
BotCheck(user,inpstr);
}


/*** Do a shout emote ***/
semote(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;
int num=0;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot emote.\n");  return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....noone can here your words.\n",JAILROOM);
        write_user(user,text);
	return;
        }
if ((word_count<2 && inpstr[1]<33)||(!strcmp(inpstr, ""))) {
	write_user(user,"Shout emote what?\n");  return;
	}
if (user->vis) name=user->name; else name=invisname;
if (user->cursed) {
        num=rand()%num_shecurse_text;
        if (inpstr[0]=='#')
                sprintf(text,shecurse_text[num],user->color,name);
        else sprintf(text,shecurse_text[num],user->color,name);
        }
else {
	if (inpstr[0]=='#') 
		sprintf(text,"~OL%s!!~RS%s %s%s\n",user->color,user->color,name,inpstr+1);
	else if (!strcmp(word[1],"'s")) {
		inpstr=remove_first(inpstr);
		sprintf(text,"~OL%s!!~RS%s %s's %s\n",user->color,user->color,name,inpstr);
		}
	else sprintf(text,"~OL%s!!~RS%s %s %s\n",user->color,user->color,name,inpstr);
	}
write_room(NULL,text);
record_shout(text);
}


/*** Do a private emote ***/
pemote(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;
UR_OBJECT u;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot emote.\n");  return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW...noone can hear you.\n",JAILROOM);
	write_user(user,text);
	return;
        }
if ((word_count<3)||(!strcmp(word[2],""))) {
	write_user(user,"Private emote what?\n");  return;
	}
word[1][0]=toupper(word[1][0]);
if (!strcmp(word[1],user->name)) {
	write_user(user,"Pemoting to yourself is the second sign of madness.\n");
	return;
	}
if (!(u=get_user(word[1]))) {
	write_user(user,notloggedon);  return;
	}
if (u->afk) {
	if (u->afk_mesg[0])
		sprintf(text,"%s is AFK, message is: %s\n",u->name,u->afk_mesg);
	else sprintf(text,"%s is AFK at the moment.\n",u->name);
	write_user(user,text);
	return;
	}
if (u->ignall && (user->level<FIV || u->level>user->level)) {
	if (u->malloc_start!=NULL) 
		sprintf(text,"%s is using the editor at the moment.\n",u->name);
	else sprintf(text,"%s is ignoring everyone at the moment.\n",u->name);
	write_user(user,text);  return;
	}
if (u->igntell && (user->level<FIV || u->level>user->level)) {
	sprintf(text,"%s is ignoring private emotes at the moment.\n",u->name);
	write_user(user,text);
	return;
	}
if (u->room==NULL) {
	sprintf(text,"%s is offsite and would not be able to reply to you.\n",u->name);
	write_user(user,text);
	return;
	}
/* if ( isignored(u,user->name) ) {  
        sprintf(text,"~FR%s is ignoring private emotes from you.\n",u->name);
        write_user(user,text);
        return;
	}
*/
if (user->vis) name=user->name; else name=invisname;
inpstr=remove_first(inpstr);
if (!strcmp(word[2],"'s")) {
	inpstr=remove_first(inpstr);
	sprintf(text,"~OL%s>> To %s: %s's %s\n",user->color,u->name,name,inpstr);
	write_user(user,text);
	record_tell(user,text);
	sprintf(text,"~OL%s>> %s's %s~RS\n",user->color,name,inpstr);
	write_user(u,text); 
	record_tell(u,text);
	}
else {
	sprintf(text,"~OL%s>> To %s: %s %s~RS\n",user->color,u->name,name,inpstr);
	write_user(user,text);
	record_tell(user,text);
	sprintf(text,"~OL%s>> %s %s~RS\n",user->color,name,inpstr);
	write_user(u,text);
	record_tell(u,text);
	}
}


/*** Echo something to screen ***/
echo(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot echo.\n");  return;
	}
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW...noone can hear you.\n",JAILROOM);
	write_user(user,text);
	return;
        }    
if (word_count<2) {
	write_user(user,"Echo what?\n");  return;
	}
if (!strcmp(word[1],"")) {
        write_user(user,"Echo what?\n");  return;
        }
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login
            || u->room==NULL
            || (u->room!=user->room && user->room!=NULL)
            || (u->ignall && !force_listen)
            || (u->ignshout && (com_num==SHOUT || com_num==SEMOTE))
            || u==user) continue;
        if (u->type==CLONE_TYPE) {
                if (u->clone_hear==CLONE_HEAR_NOTHING || u->owner->ignall)
                        continue;
                if (user->room!=u->room) continue;
                if (u->clone_hear==CLONE_HEAR_SWEARS) {
                        if (!contains_swearing(inpstr)) continue;
                        }
                sprintf(text,"~FT[ %s ]:~RS %s",u->room->name,inpstr);
                write_user(u->owner,text);
                }
        else { 
                if (u->level>=FOU)
                        sprintf(text,"~OL~FR(~FW%s~OL~FR)~RS %s\n",user->name,inpstr);
                else
                        sprintf(text,"%s\n",inpstr);
                write_user(u,text);
                }   
        }
sprintf(text,"%s\n",inpstr);
write_user(user,text);
record(user->room,text);
}



/*** Move to another room ***/
go(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;
int i;

if (word_count<2) {
	write_user(user,"Go where?\n");  return;
	}
if (user->jailed) {
        sprintf(text,"~OLSorry, you are in ~FR%s~FW, you cannot move!\n",JAILROOM);
	write_user(user,text);
	return;
        }
if (!(strcmp(user->room->name,BLKJKROOM))) {
	write_user(user,"~OLSorry, to leave the game room use .blackjack quit.\n");
	return;
	}
if (user->pop !=NULL) {
        write_user(user,"~OL~FRYou are playing poker, please leave the game first!\n");
        return;
        }
nl=user->room->netlink;
if (nl!=NULL && !strncmp(nl->service,word[1],strlen(word[1]))) {
	if (user->pot_netlink==nl) {
		write_user(user,"The remote service may be lagged, please be patient...\n");
		return;
		}
	rm=user->room;
	if (nl->stage<2) {
		write_user(user,"The netlink is inactive.\n");
		return;
		}
	if (nl->allow==IN && user->netlink!=nl) {
		/* Link for incoming users only */
		write_user(user,"Sorry, link is for incoming users only.\n");
		return;
		}
	/* If site is users home site then tell home system that we have removed
	   him. */
	if (user->netlink==nl) {
		write_user(user,"~FB~OLYou traverse cyberspace...\n");
		sprintf(text,"REMVD %s\n",user->name);
		write_sock(nl->socket,text);
		if (user->vis) {
			sprintf(text,"%s goes to the %s\n",user->name,nl->service);
			write_room_except(rm,text,user);
			}
		else write_room_except(rm,invisleave,user);
		sprintf(text,"NETLINK: Remote user %s removed.\n",user->name);
		write_syslog(text,1);
		destroy_user_clones(user);
		destruct_user(user);
		reset_access(rm);
		num_of_users--;
		no_prompt=1;
		return;
		}
	/* Can't let remote user jump to yet another remote site because this will 
	   reset his user->netlink value and so we will lose his original link.
	   2 netlink pointers are needed in the user structure to allow this
	   but it means way too much rehacking of the code and I don't have the 
	   time or inclination to do it */
	if (user->type==REMOTE_TYPE) {
		write_user(user,"Sorry, due to software limitations you can only traverse one netlink.\n");
		return;
		}
	if (nl->ver_major<=3 && nl->ver_minor<=3 && nl->ver_patch<1) {
		if (!word[2][0]) 
			sprintf(text,"TRANS %s %s %s\n",user->name,user->pass,user->desc);
		else sprintf(text,"TRANS %s %s %s\n",user->name,(char *)crypt(word[2],"NU"),user->desc);
		}
	else {
		if (!word[2][0]) 
			sprintf(text,"TRANS %s %s %d %s\n",user->name,user->pass,user->level,user->desc);
		else sprintf(text,"TRANS %s %s %d %s\n",user->name,(char *)crypt(word[2],"NU"),user->level,user->desc);
		}	
	write_sock(nl->socket,text);
	user->remote_com=GO;
	user->pot_netlink=nl;  /* potential netlink */
	no_prompt=1;
	return;
	}
/* If someone tries to go somewhere else while waiting to go to a talker
   send the other site a release message */
if (user->remote_com==GO) {
	sprintf(text,"REL %s\n",user->name);
	write_sock(user->pot_netlink->socket,text);
	user->remote_com=-1;
	user->pot_netlink=NULL;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
if (rm==user->room) {
	sprintf(text,"You are already in the %s!\n",rm->name);
	write_user(user,text);
	return;
	}
if (rm==get_room(BLKJKROOM)) {
	write_user(user,"~OLSorry, to join a game of 21, use .blackjack join.\n");
	return;
	}

/* See if link from current room */
for(i=0;i<MAX_LINKS;++i) {
	if (user->room->link[i]==rm) {
		move_user(user,rm,0);  return;
		}
	}
if (user->level<FOU) {
	sprintf(text,"The %s is not adjoined to here.\n",rm->name);
	write_user(user,text);  
	return;
	}
move_user(user,rm,1);
}


/*** Called by go() and move() ***/
move_user(user,rm,teleport)
UR_OBJECT user;
RM_OBJECT rm;
int teleport;
{
RM_OBJECT old_room;

old_room=user->room;
if (teleport!=2 && !has_room_access(user,rm)) {
	write_user(user,"That room is currently private, you cannot enter.\n");  
	return;
	}
/* Reset invite room if in it */
if (user->invite_room==rm) user->invite_room=NULL;
if (!user->vis) {
	write_room(rm,invisenter);
	write_room_except(user->room,invisleave,user);
	goto SKIP;
	}
if (teleport==1) {
	sprintf(text,"~FT~OL%s appears in an explosion of blue magic!\n",user->name);
	write_room(rm,text);
	sprintf(text,"~FT~OL%s chants a spell and vanishes into a magical blue vortex!\n",user->name);
	write_room_except(old_room,text,user);
	goto SKIP;
	}
if (teleport==2) {
	write_user(user,"\n~FT~OLA giant hand grabs you and pulls you into a magical blue vortex!\n");
	sprintf(text,"~FT~OL%s falls out of a magical blue vortex!\n",user->name);
	write_room(rm,text);
	if (old_room==NULL) {
		sprintf(text,"REL %s\n",user->name);
		write_sock(user->netlink->socket,text);
		user->netlink=NULL;
		}
	else {
		sprintf(text,"~FT~OLA giant hand grabs %s who is pulled into a magical blue vortex!\n",user->name);
		write_room_except(old_room,text,user);
		}
	goto SKIP;
	}
if (teleport==3) {
        sprintf(text,"\n~FT~OLYou have been ~FBTHROWN~FT out of the %s\n",old_room);
        write_user(user,text);
        sprintf(text,"~FT~OL%s has been ~FBTHROWN~FT in from the %s\n",user->name,old_room);
        write_room(rm,text);
        if (old_room==NULL) {
                sprintf(text,"REL %s\n",user->name);
                write_sock(user->netlink->socket,text);
                user->netlink=NULL;
                }
        else {   
                sprintf(text,"~FT~OL%s is ~FBTHROWN~FT outta here!!\n",user->name);
                write_room_except(old_room,text,user);
                }
        goto SKIP;
        }
sprintf(text,"%s %s.\n",user->name,user->in_phrase);
write_room(rm,text);
sprintf(text,"%s %s to the %s.\n",user->name,user->out_phrase,rm->name);
write_room_except(user->room,text,user);

SKIP:
check_fight_status(user);
user->room=rm;
look(user);
reset_access(old_room);
}

/*** Test if user is involved in combat.  Reset the user's fight status if
     so. This is primarily used when the user leaves the room. ***/
check_fight_status(UR_OBJECT u)
{
char buff[OUT_BUFF_SIZE+1];
        
if (u->melee_status == FIGHTING) {
    sprintf(buff, "\n%s leaves the fight.\n", u->name);
    reset_duel();  /* Duel is reset if user leaves the room. */
    write_room(u->room, buff);
    }
return;                                
}

/*** Switch ignoring all on and off ***/
toggle_ignall(user)
UR_OBJECT user;
{
if (!user->ignall) {
	write_user(user,"You are now ignoring everyone.\n");
	sprintf(text,"%s is now ignoring everyone.\n",user->name);
	write_room_except(user->room,text,user);
	user->ignall=1;
	return;
	}
write_user(user,"You will now hear everyone again.\n");
sprintf(text,"%s is listening again.\n",user->name);
write_room_except(user->room,text,user);
user->ignall=0;
}


/*** Switch prompt on and off ***/
toggle_prompt(user)
UR_OBJECT user;
{
if (user->prompt) {
	write_user(user,"Prompt ~FROFF.\n");
	user->prompt=0;  return;
	}
write_user(user,"Prompt ~FGON.\n");
user->prompt=1;
}


/*** Set user description ***/
set_desc(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (word_count<2) {
	sprintf(text,"Your current description is: %s\n",user->desc);
	write_user(user,text);
	return;
	}
if (strstr(word[1],"(CLONE)")) {
	write_user(user,"You cannot have that description.\n");  return;
	}
if (strlen(inpstr)>USER_DESC_LEN) {
	write_user(user,"Description too long.\n");  return;
	}
if (!strcmp(inpstr, " ")) {
	write_user(user, "Try actually entering something.\n");
	return;
}
if (!strcmp(inpstr, "")) {
	write_user(user, "Try actually entering something.\n");
	return;
}
strcpy(user->desc,inpstr);
write_user(user,"Description set.\n");
}


/*** Set in and out phrases ***/
set_iophrase(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (strlen(inpstr)>PHRASE_LEN) {
	write_user(user,"Phrase too long.\n");  return;
	}
if (com_num==INPHRASE) {
	if (word_count<2) {
		sprintf(text,"Your current in phrase is: %s\n",user->in_phrase);
		write_user(user,text);
		return;
		}
	strcpy(user->in_phrase,inpstr);
	write_user(user,"In phrase set.\n");
	return;
	}
if (word_count<2) {
	sprintf(text,"Your current out phrase is: %s\n",user->out_phrase);
	write_user(user,text);
	return;
	}
strcpy(user->out_phrase,inpstr);
write_user(user,"Out phrase set.\n");
}


/*** Set rooms to public or private ***/
set_room_access(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
int cnt;

rm=user->room;
if (word_count<2) rm=user->room;
else {
	if (user->level<gatecrash_level) {
		write_user(user,"You are not a high enough level to use the room option.\n");  
		return;
		}
	if ((rm=get_room(word[1]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	}
if (user->vis) name=user->name; else name=invisname;
if (rm->access>PRIVATE) {
	if (rm==user->room) 
		write_user(user,"This room's access is fixed.\n"); 
	else write_user(user,"That room's access is fixed.\n");
	return;
	}
if (com_num==PUBCOM && rm->access==PUBLIC) {
	if (rm==user->room) 
		write_user(user,"This room is already public.\n");  
	else write_user(user,"That room is already public.\n"); 
	return;
	}
if (user->vis) name=user->name; else name=invisname;
if (com_num==PRIVCOM) {
	if (rm->access==PRIVATE) {
		if (rm==user->room) 
			write_user(user,"This room is already private.\n");  
		else write_user(user,"That room is already private.\n"); 
		return;
		}
	cnt=0;
	for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
	if (cnt<min_private_users && user->level<ignore_mp_level) {
		sprintf(text,"You need at least %d users/clones in a room before it can be made private.\n",min_private_users);
		write_user(user,text);
		return;
		}
	write_user(user,"Room set to ~FRPRIVATE.\n");
	if (rm==user->room) {
		sprintf(text,"%s has set the room to ~FRPRIVATE.\n",name);
		write_room_except(rm,text,user);
		}
	else write_room(rm,"This room has been set to ~FRPRIVATE.\n");
	rm->access=PRIVATE;
	return;
	}
write_user(user,"Room set to ~FGPUBLIC.\n");
if (rm==user->room) {
	sprintf(text,"%s has set the room to ~FGPUBLIC.\n",name);
	write_room_except(rm,text,user);
	}
else write_room(rm,"This room has been set to ~FGPUBLIC.\n");
rm->access=PUBLIC;

/* Reset any invites into the room & clear review buffer */
for(u=user_first;u!=NULL;u=u->next) {
	if (u->invite_room==rm) u->invite_room=NULL;
	}
clear_revbuff(rm);
}


/*** Ask to be let into a private room ***/
letmein(user)
UR_OBJECT user;
{
RM_OBJECT rm;
int i;

if (word_count<2) {
	write_user(user,"Let you into where?\n");  return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
if (rm==user->room) {
	sprintf(text,"You are already in the %s!\n",rm->name);
	write_user(user,text);
	return;
	}
for(i=0;i<MAX_LINKS;++i) 
	if (user->room->link[i]==rm) goto GOT_IT;
sprintf(text,"The %s is not adjoined to here.\n",rm->name);
write_user(user,text);  
return;

GOT_IT:
if (!(rm->access & PRIVATE)) {
	sprintf(text,"The %s is currently public.\n",rm->name);
	write_user(user,text);
	return;
	}
sprintf(text,"You shout asking to be let into the %s.\n",rm->name);
write_user(user,text);
sprintf(text,"%s shouts asking to be let into the %s.\n",user->name,rm->name);
write_room_except(user->room,text,user);
sprintf(text,"%s shouts asking to be let in.\n",user->name);
write_room(rm,text);
}


/*** Invite a user into a private room ***/
invite(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;

if (word_count<2) {
	write_user(user,"Invite who?\n");  return;
	}
rm=user->room;
if (!(rm->access & PRIVATE)) {
	write_user(user,"This room is currently public.\n");
	return;
	}
if (!(u=get_user(word[1]))) {
	write_user(user,notloggedon);  return;
	}
if (u==user) {
	write_user(user,"Inviting yourself to somewhere is the third sign of madness.\n");
	return;
	}
if (u->room==rm) {
	sprintf(text,"%s is already here!\n",u->name);
	write_user(user,text);
	return;
	}
if (u->invite_room==rm) {
	sprintf(text,"%s has already been invited into here.\n",u->name);
	write_user(user,text);
	return;
	}
sprintf(text,"You invite %s in.\n",u->name);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has invited you into the %s.\n",name,rm->name);
write_user(u,text);
u->invite_room=rm;
}


/*** Set the room topic ***/
set_topic(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
RM_OBJECT rm;
char *name;

rm=user->room;
if (word_count<2) {
	if (!strlen(rm->topic)) {
		write_user(user,"No topic has been set yet.\n");  return;
		}
	sprintf(text,"The current topic is: %s\n",rm->topic);
	write_user(user,text);
	return;
	}
if (strlen(inpstr)>TOPIC_LEN) {
	write_user(user,"Topic too long.\n");  return;
	}
if ((!strcmp(word[1],"-c"))&&(user->level>FOU)) {
	strcpy(rm->topic,"\0");
	sprintf(text,"You have cleared the topic for room: %s\n",rm->name);
	write_user(user,text);
	return;
	}
sprintf(text,"Topic set to: %s\n",inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has set the topic to: %s\n",name,inpstr);
write_room_except(rm,text,user);
strcpy(rm->topic,inpstr);
}


/*** Wizard moves a user to another room ***/
move(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;

if (word_count<2) {
	write_user(user,"Usage: move <user> [<room>]\n");  return;
	}
if (!(u=get_user(word[1]))) {
	write_user(user,notloggedon);  return;
	}
if (word_count<3) rm=user->room;
else {
	if ((rm=get_room(word[2]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	}
if (user==u) {
	write_user(user,"Trying to move yourself this way is the fourth sign of madness.\n");  return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot move a user of equal or higher level than yourself.\n");
	return;
	}
if (u->room == get_room(BLKJKROOM)) {
	sprintf(text,"~OLSorry, %s is playing 21, you cannot move the user.\n",u->name); 
	write_user(user,text);
	return;
	}
if (u->pop !=NULL) {
        write_user(user,"~OL~FRSorry that user is playing poker, you can't move them!\n");
        return;
        }
if (rm==u->room) {
	sprintf(text,"%s is already in the %s.\n",u->name,rm->name);
	write_user(user,text);
	return;
	}
if (rm==get_room(BLKJKROOM)) {
	sprintf(text,"You can't move someone to the blackjack room.\n");
	write_user(user,text);
	return;
	}
if (!has_room_access(user,rm)) {
	sprintf(text,"The %s is currently private, %s cannot be moved there.\n",rm->name,u->name);
	write_user(user,text);  
	return;
	}
write_user(user,"~FT~OLYou chant an ancient spell...\n");
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~FT~OL%s chants an ancient spell...\n",name);
write_room_except(user->room,text,user);
move_user(u,rm,2);
prompt(u);
}



/*** Do the help ***/
help(user)
UR_OBJECT user;
{
int ret;
char filename[80];
char *c;

if ((word_count<2)||(!strcmp(word[1],""))) { help_commands(user,0); return; }
if ((!strcmp(word[1],"-w"))&&(user->level>=FOU)) { help_commands(user,1); return; }
if (!strcmp(word[1],"commands")) {  help_commands(user,0);  return;  }
if (!strcmp(word[1],"credits")) {  help_credits(user);  return;  }
if (!strcmp(word[1],"games")) {  help_games(user);  return;  }

/* Check for any illegal crap in searched for filename so they cannot list 
   out the /etc/passwd file for instance. */
c=word[1];
while(*c) {
	if (*c=='.' || *c++=='/') {
		write_user(user,"Sorry, there is no help on that topic.\n");
		return;
		}
	}
sprintf(filename,"%s/%s",HELPFILES,word[1]);
if (!(ret=more(user,user->socket,filename)))
	write_user(user,"Sorry, there is no help on that topic.\n");
if (ret==1) user->misc_op=2;
}


is_game(com)
int com;
{
int game=0;

while(is_game_name[game][0]!='*') {
	if (!strcmp(command[com],is_game_name[game])) return 1;
	game++;  
	}
return 0;
}



/*** Show the command available ***/
help_commands(user,wiz)
UR_OBJECT user;
int wiz;
{
int com,cnt,lev,total_commands,start,nwiz=0;
char temp[20],*c,null[1];

sprintf(text,"\n~OL~FB*** Commands available for level: %s ***\n\n",level_name[user->level]);
write_user(user,text);

com=0; start=0; null[0]='\0';
if (wiz) lev=FOU;
else lev=ZER;

while(lev<=user->level && !nwiz) {
	cnt=0;  text[0]='\0';
	if (!start) {
		sprintf(temp,"~OL~FR%c)~RS ",level_name[lev][0]);
		write_user(user,temp);
		c="~FT";
		}
	else {
		c=null;
		sprintf(text,"   ");
		}
	while(command[com][0]!='*') {
		if (com_level[com]!=lev) {  com++;  continue;  }
		if (is_game(com)) {  com++;  continue;  }
		sprintf(temp,"%s%-11s ",c,command[com]);
		strcat(text,temp);
		c=null;
		cnt++;
		if (cnt==6) {  
			strcat(text,"\n");  
			write_user(user,text);  
			text[0]='\0';  cnt=0;  
			}
		if (!cnt) strcat(text,"   ");
		com++; 
		}
	if (cnt>0 & cnt<6) {
		strcat(text,"\n");  write_user(user,text);
		}
	com=0;
	start=0;
	lev++;
	if (lev>=FOU && !wiz) nwiz=1;
	}
total_commands = 0;
com=0;
while(command[com][0]!='*') {
        if (com_level[com] <= user->level) {
                total_commands++;
                com++;
                continue;
        }
        com++;
}
if (!wiz && user->level>=FOU)
	write_user(user,"\nType '~FR.help -w~RS' to see your wiz commands.\n");
if (wiz && user->level>=FOU)
	write_user(user,"\nType '~FR.help~RS' to see your regular commands.\n");

sprintf(text,"\nTotal of ~FT%d~RS commands available to you including games listed in '~FG.help games~RS'.\n",total_commands); 
write_user(user,text);
write_user(user,"Type '~FG.help <command name>~RS' for specific help on a command.\nRemember, you can use a '.' on its own to repeat your last command or speech.\n\n");
}



help_games(user)
UR_OBJECT user;
{
int com,cnt,type,start;
char temp[20];

write_user(user,"\n~OL~FB*** Commands available for Games ***\n\n");

com=0; start=0; 
for(type=CAR;type<=BOA;type++) {
	cnt=0;  text[0]='\0';
	if (!start) {
		sprintf(temp,"~OL~FR%s~RS ",game_type[type]);
		write_user(user,temp);
		}
	else {
		sprintf(text,"       ");
		}
	while(is_game_name[com][0]!='*') {
		if (game_type_ref[com]!=type) {  com++;  continue;  }
		sprintf(temp,"%-11s ",is_game_name[com]);
		strcat(text,temp);
		cnt++;
		if (cnt==5) {  
			strcat(text,"\n");  
			write_user(user,text);  
			text[0]='\0';  cnt=0;  
			}
		if (!cnt) strcat(text,"       ");
		com++; 
		}
	if (cnt>0 & cnt<5) {
		strcat(text,"\n");  write_user(user,text);
		}
	com=0;
	start=0;
	}
write_user(user,"\n");
}


/*** Show the credits. Add your own credits here if you wish but PLEASE leave
     my credits intact. Thanks. */
help_credits(user)
UR_OBJECT user; 
{
write_user(user,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
sprintf(text,"~FT   The Credits                                          RNUTS Version %s\n",RVERSION);
write_user(user,text);
write_user(user,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n\n");
write_user(user,"~FG   The rNUTS code is a beastly configuration made up of re-vamped, re-written,\n");
write_user(user,"~FG   and in all other ways modified NUTS3.3.3 code by (c)Neil Robertson - 1996.\n\n");
write_user(user,"~FG   Special thanks to those who helped Neil:\n");
write_user(user,"~FG		Darren Seryck, Steve Guest, Dave Temple, Satish Bedi, Tim Bernhardt\n");
write_user(user,"~FG		Kien Tran, Jesse Walton, Pak Chan, Scott MacKenzie, and Brian McPhail\n");
write_user(user,"~FY   Main Code Modifications by:~OL  Engi and Slugger\n");
write_user(user,"~FG   Macros Provided by:          Knightshade\n\n");
write_user(user,"~FT   Special Thanks to: Akira, Crusier, House, Pres, Emit, Fubar, Starlight\n");
write_user(user,"~FT                      and anyone else who has helped us along the way. =)\n\n");
write_user(user,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
}

/*** Read the message board ***/
read_board(user)
UR_OBJECT user;
{
RM_OBJECT rm;
char filename[80],*name;
int ret,flag=0,i=0;

if (word_count<2) {
	rm=user->room;
	strcpy(user->boardwrite,user->room->name);
	}
else {
	if ((rm=get_room(word[1]))==NULL) {  
		while (board_words[i][0]!='*') {
			if (!strcmp(board_words[i],word[1]))
				flag=1;
			i++;
			}
		}
	else { 
		flag=1;
		if (!has_room_access(user,rm)) {
			write_user(user,"That room is currently private, you cannot read the board.\n");
			return;
			}
		}
	if (flag==0) {
		if (user->level<FOU) { 
			write_user(user,nosuchroom); 
			return; 
			}
		else {
			write_user(user,"Sorry, that board does not exist.\n\n");
			return;
			}
		}
	if ((!strcmp("admnote",word[1]))&&(user->level<SEV)) {
		write_user(user,"Sorry, you do not have access to that board.\n\n");
		return;
		}
	if ((!strcmp("wiznote",word[1]))&&(user->level<FOU)) {
		write_user(user,"Sorry, you do not have access to that board.\n\n");
		return;
		}
	strcpy(user->boardwrite,word[1]);
	}	
sprintf(text,"\n~OL~FR*** The %s message board ***\n\n",user->boardwrite);
write_user(user,text);
sprintf(filename,"%s/%s.B",BOARDFILES,user->boardwrite);
if (!(ret=more(user,user->socket,filename))) 
	write_user(user,"There are no messages on the board.\n\n");
else if (ret==1) user->misc_op=2;
if (user->vis) name=user->name; else name=invisname;
if (!strcmp(user->room->name,user->boardwrite)) {
	sprintf(text,"%s reads the message board.\n",name);
	write_room_except(user->room,text,user);
	}
}


/*** Write on the message board ***/
write_board(user,inpstr,done_editing)
UR_OBJECT user;
char *inpstr;
int done_editing;
{
RM_OBJECT rm=user->room;
FILE *fp;
int cnt,inp,flag=0,i=0;
char *ptr,*name,filename[80];

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot write on the board.\n");  
	return;
	}
if (!done_editing) {
	if (word_count<2) {
		if (user->type==REMOTE_TYPE) {
			/* Editor won't work over netlink cos all the prompts will go
			   wrong, I'll address this in a later version. */
			write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nUse the '.write <mesg>' method instead.\n");
			return;
			}
		rm=user->room;		
		sprintf(text,"\n~OL~FB*** Writing board message for %s ***\n\n",user->room->name);
		write_user(user,text);
		strcpy(user->boardwrite,user->room->name);
		user->misc_op=3;
		editor(user,NULL);
		return;
		}
	if ((word_count==2)&&(user->level>=FOU)) {
		if (user->type==REMOTE_TYPE) {
			write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nUse the '.write <mesg>' method instead.\n");
			return;
			}
		if ((rm=get_room(word[1]))==NULL) {  
		        while (board_words[i][0]!='*') { 
				if (!strcmp(board_words[i],word[1])) 
						flag=1;
				i++;
				}
			}
		else  flag=1;
		if (flag==0) {
			write_user(user,"Sorry, that board does not exist.\n\n");
			return;
			}
		if ((!strcmp("admnote",word[1]))&&(user->level<SEV)) {
			write_user(user,"Sorry, you do not have access to that board\n\n");
			return;
			}
		sprintf(text,"\n~OL~FB*** Writing a message for %s ***\n\n",word[1]);
		write_user(user,text);
		strcpy(user->boardwrite,word[1]);
		user->misc_op=3;
		editor(user,NULL);
		return;
		}
	strcpy(user->boardwrite,user->room->name);
	ptr=inpstr;
	inp=1;
	}
else {
	ptr=user->malloc_start;  inp=0;
	}

sprintf(filename,"%s/%s.B",BOARDFILES,user->boardwrite);
if (!(fp=fopen(filename,"a"))) {
	sprintf(text,"%s: cannot write to file.\n",syserror);
	write_user(user,text);
	sprintf(text,"ERROR: Couldn't open file %s to append in write_board().\n",filename);
	write_syslog(text,0);
	return;
	}
if (user->vis) name=user->name; else name=invisname;
/* The posting time (PT) is the time its written in machine readable form, this 
   makes it easy for this program to check the age of each message and delete 
   as appropriate in check_messages() */
if (user->type==REMOTE_TYPE) 
	sprintf(text,"PT: %d\r~OL~FBFrom: %s@%s %s\n",(int)(time(0)),name,user->netlink->service,long_date(0));
else sprintf(text,"PT: %d\r~OL~FBFrom: %s  %s\n",(int)(time(0)),name,long_date(0));
fputs(text,fp);
cnt=0;
while(*ptr!='\0') {
	putc(*ptr,fp);
	if (*ptr=='\n') cnt=0; else ++cnt;
	if (cnt==80) { putc('\n',fp); cnt=0; }
	++ptr;
	}
if (inp) fputs("\n\n",fp); else putc('\n',fp);
fclose(fp);
sprintf(text,"You write a message on the board %s..",user->boardwrite);
write_user(user,text);
if (!strcmp(user->boardwrite,user->room->name)) {
	sprintf(text,"%s writes a message on the board.\n",name);
	write_room_except(user->room,text,user);
	}
write_user(user,"...Message Stored.\n");
if ((rm=get_room(user->boardwrite))!=NULL)
	rm->mesg_cnt++;
}


/*** Wipe some messages off the board ***/
wipe_board(user)
UR_OBJECT user;
{
int num,cnt,valid,flag=0,magiflag=0,i=0;
char infile[80],line[82],id[82],*name;
FILE *infp,*outfp;
RM_OBJECT rm;

if (user->level>=SEV) {
	if (word_count<3) {
		write_user(user,"Usage: wipe <board name> <# of messages>/all\n");                
		return;
		}
	if ((rm=get_room(word[1]))==NULL) {
		while (board_words[i][0]!='*') {
			if (!strcmp(board_words[i],word[1]))
				flag=1;
			i++;
			}
		}
	else  flag=1;
	if (flag==0) {
		write_user(user,"Sorry, that board does not exist.\n\n");
		return;
		}
	strcpy(user->boardwrite,word[1]);
	if ((num=atoi(word[2]))<1 && strcmp(word[2],"all")) {	
		write_user(user,"Usage: wipe <board name> <# of messages>/all\n");                
		return;
		}
	magiflag=1;	
	}	
else {
	if (word_count<2 || ((num=atoi(word[1]))<1 && strcmp(word[1],"all"))) {
        	write_user(user,"Usage: wipe <number of messages>/all\n");  return;
        	}
	
	rm=user->room;
	strcpy(user->boardwrite,rm->name);
	}
if (user->vis) name=user->name; else name=invisname;
sprintf(infile,"%s/%s.B",BOARDFILES,user->boardwrite);
if (!(infp=fopen(infile,"r"))) {
        write_user(user,"The message board is empty.\n");
        return;
        }
if (((magiflag==1)&&(!strcmp(word[2],"all")))||(!strcmp(word[1],"all"))) {
        fclose(infp);
        unlink(infile);
	sprintf(text,"All messages deleted on board %s.\n",user->boardwrite);
        write_user(user,text);
	if (!strcmp(user->boardwrite,user->room->name)) {
        	sprintf(text,"%s wipes all messages from the board.\n",name);
        	write_room_except(user->room,text,user);
        	}
        sprintf(text,"%s wiped all messages from the board %s.\n",user->name,user->boardwrite);
        write_syslog(text,1);
	if ((rm=get_room(user->boardwrite))!=NULL)
	        rm->mesg_cnt=0;
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile in wipe_board().\n",0);
        fclose(infp);
        return;
        }
cnt=0; valid=1;
fgets(line,82,infp); /* max of 80+newline+terminator = 82 */
while(!feof(infp)) {
        if (cnt<=num) {
                if (*line=='\n') valid=1;
                sscanf(line,"%s",id);
                if (valid && !strcmp(id,"PT:")) {
                        if (++cnt>num) fputs(line,outfp);
                        valid=0;
                        }
                }
        else fputs(line,outfp);
        fgets(line,82,infp);
        }
fclose(infp);
fclose(outfp);
unlink(infile);
if (cnt<num) {
        unlink("tempfile");
        sprintf(text,"There were only %d messages on the board %s, all now deleted.\n",cnt,user->boardwrite);
        write_user(user,text);
	if (!strcmp(user->boardwrite,user->room->name)) {
	        sprintf(text,"%s wipes the message board.\n",name);
        	write_room_except(user->room,text,user);
        	}
        sprintf(text,"%s wiped all messages from the board %s.\n",user->name,user->boardwrite);
        write_syslog(text,1);
	if ((rm=get_room(user->boardwrite))!=NULL)
	        rm->mesg_cnt=0;
        return;
        }
if (cnt==num) {
        unlink("tempfile"); /* cos it'll be empty anyway */
        write_user(user,"All messages deleted.\n");
	if ((rm=get_room(user->boardwrite))!=NULL)
        	rm->mesg_cnt=0;
        sprintf(text,"%s wiped all messages from the board %s.\n",user->name,user->boardwrite);
        }
else {
        rename("tempfile",infile);
        sprintf(text,"%d messages deleted.\n",num);
        write_user(user,text);
	if ((rm=get_room(user->boardwrite))!=NULL)
        	rm->mesg_cnt-=num;
        sprintf(text,"%s wiped %d messages from the board %s.\n",user->name,num,user->boardwrite);
        }
write_syslog(text,1);  
if (!strcmp(user->boardwrite,user->room->name)) {
        sprintf(text,"%s wipes some messages from the board.\n",name);
        write_room_except(user->room,text,user);
        }
}
	

/*** Search all the boards for the words given in the list. Rooms fixed to
	private will be ignore if the users level is less than gatecrash_level ***/
search_boards(user)
UR_OBJECT user;
{
RM_OBJECT rm;
FILE *fp;
char filename[80],line[82],buff[(MAX_LINES+1)*82],w1[81];
int w,cnt,message,yes,room_given;

if (word_count<2) {
	write_user(user,"Usage: search <word list>\n");  return;
	}
/* Go through rooms */
cnt=0;
for(rm=room_first;rm!=NULL;rm=rm->next) {
	sprintf(filename,"%s/%s.B",BOARDFILES,rm->name);
	if (!(fp=fopen(filename,"r"))) continue;
	if (!has_room_access(user,rm)) {  fclose(fp);  continue;  }

	/* Go through file */
	fgets(line,81,fp);
	yes=0;  message=0;  
	room_given=0;  buff[0]='\0';
	while(!feof(fp)) {
		if (*line=='\n') {
			if (yes) {  strcat(buff,"\n");  write_user(user,buff);  }
			message=0;  yes=0;  buff[0]='\0';
			}
		if (!message) {
			w1[0]='\0';  
			sscanf(line,"%s",w1);
			if (!strcmp(w1,"PT:")) {  
				message=1;  
				strcpy(buff,remove_first(remove_first(line)));
				}
			}
		else strcat(buff,line);
		for(w=1;w<word_count;++w) {
			if (!yes && strstr(line,word[w])) {  
				if (!room_given) {
					sprintf(text,"~BB*** %s ***\n\n",rm->name);
					write_user(user,text);
					room_given=1;
					}
				yes=1;  cnt++;  
				}
			}
		fgets(line,81,fp);
		}
	if (yes) {  strcat(buff,"\n");  write_user(user,buff);  }
	fclose(fp);
	}
if (cnt) {
	sprintf(text,"Total of %d matching messages.\n\n",cnt);
	write_user(user,text);
	}
else write_user(user,"No occurences found.\n");
}



/*** See review of conversation ***/
review(user)
UR_OBJECT user;
{
RM_OBJECT rm=user->room;
int i,line,cnt;

if (word_count<2) rm=user->room;
else {
	if ((rm=get_room(word[1]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	if (user->level<FOU) {
		write_user(user,"We are sorry but you can only review that room which you are in.\n");
		return;
		}
	if (!has_room_access(user,rm)) {
		write_user(user,"That room is currently private, you cannot review the conversation.\n");
		return;
		}
	}
cnt=0;
for(i=0;i<REVIEW_LINES;++i) {
	line=(rm->revline+i)%REVIEW_LINES;
	if (rm->revbuff[line][0]) {
		cnt++;
		if (cnt==1) {
			sprintf(text,"\n~BB~FG*** Review buffer for the %s ***\n\n",rm->name);
			write_user(user,text);
			}
		write_user(user,rm->revbuff[line]); 
		}
	}
if (!cnt) write_user(user,"Review buffer is empty.\n");
else write_user(user,"\n~BB~FG*** End ***\n\n");
}


/*** Return to home site ***/
home(user)
UR_OBJECT user;
{
if (user->room!=NULL) {
	write_user(user,"You are already on your home system.\n");
	return;
	}
write_user(user,"~FB~OLYou traverse cyberspace...\n");
sprintf(text,"REL %s\n",user->name);
write_sock(user->netlink->socket,text);
sprintf(text,"NETLINK: %s returned from %s.\n",user->name,user->netlink->service);
write_syslog(text,1);
user->room=user->netlink->connect_room;
user->netlink=NULL;
if (user->vis) {
	sprintf(text,"%s %s\n",user->name,user->in_phrase);
	write_room_except(user->room,text,user);
	}
else write_room_except(user->room,invisenter,user);
look(user);
}


/*** Show some user stats -lj1 ***/
ustatus(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
FILE *fp;
char filename[80],ir[ROOM_NAME_LEN+1],igntext[10],mode[15];
char vemail[EMAIL_LEN+1],vurl[URL_LEN+1],name[USER_NAME_LEN+USER_DESC_LEN+1];
int days,hours,mins,hs,cnt;

if ((word_count<2)||(!strcmp(word[1],""))) {
        strcpy(word[1], user->name);
	}
if (!(u=get_user(word[1]))) {
        if ((u=create_user())==NULL) {
                sprintf(text,"%s: unable to create temporary user object.\n",syserror);
                write_user(user,text);
                write_syslog("ERROR: Unable to create temporary user object in status().\n");
                return;
                }
        strcpy(u->name,word[1]);
        if (!load_user_details(u)) {
                write_user(user,nosuchuser);
                destruct_user(u);
                destructed=0;
                return;
                }
        u2=NULL;
	}
else u2=u;

if ((u->hide_email==1)&&(user->level<FOU))
	strcpy(vemail,"<email address only viewable by wizards>");
else strcpy(vemail,u->email);
if (user==u) strcpy(vemail,user->email);

if ((u->hide_url==1)&&(user->level<FOU))
	strcpy(vurl,"<homepage url only viewable by wizards>");
else strcpy(vurl,u->url);
if (user==u) strcpy(vurl,user->url);

if (u->command_mode==1) strcpy(mode,"COMMAND");
else strcpy(mode,"SPEECH");

sprintf(name,"%s %s~RS",u->name,u->desc);
cnt=colour_com_count(name);

if (u2==NULL) {
	sprintf(text,"~FT+----- User Info -- (Not Currently logged on) ---------------------------------+\n"); 
	write_user(user,text);
	mins=(int)(time(0) - u->last_login)/60;
	sprintf(text,"~FTName   ~RS: %-*.*s~FTLevel ~RS: %-10s\n",45+cnt*3,45+cnt*3,name,level_name[u->level]);
	write_user(user,text);
	sprintf(text,"~FTGender ~RS: %-10s      ~FTAge ~RS: %-17s ~FTOnline for ~RS: (null)\n",u->gend,u->age);
	write_user(user,text);
	sprintf(text,"~FTEmail Address ~RS: %-50s\n",vemail);
	write_user(user,text);
	sprintf(text,"~FTHomepage URL  ~RS: %-50s\n",vurl);
	write_user(user,text);
	sprintf(text,"~FT+----- Game Info --------------------------------------------------------------+\n");
	write_user(user,text);
	sprintf(text,"~FTTicTacToe Stats~RS  : %d - %d - %d (win/lose/draw)\n",u->tic_win,u->tic_lose,u->tic_draw);
	write_user(user,text);
	sprintf(text,"~FTHangMan Stats~RS    : %d - %d (win/lose)\n",u->hang_win,u->hang_lose);
	write_user(user,text);
	sprintf(text,"~FTNerf Stats~RS       : %d - %d - %d - %d (win/lose/life/charge)\n",u->nerf_win,u->nerf_lose,u->nerflife,u->nerfcharge);
	write_user(user,text);
	sprintf(text,"~FT+----- General Info -----------------------------------------------------------+\n");
	write_user(user,text);
	sprintf(text,"~FTEnter Msg  ~RS: %s\n",u->in_phrase);
	write_user(user,text);
	sprintf(text,"~FTExit Msg   ~RS: %s\n",u->out_phrase);
	write_user(user,text);

	if ((noyes2[u->ignall]=="YES")||((noyes2[u->ignshout]=="YES")&&(noyes2[u->igntell]))) 
	strcpy(igntext,"ALL");
	else if (noyes2[u->ignshout]=="YES") strcpy(igntext,"SHOUTS");
	else if (noyes2[u->igntell]=="YES") strcpy(igntext,"TELLS");
	else strcpy(igntext,"NO");
	
	if (u->invite_room==NULL) strcpy(ir,"<nowhere>");
	else strcpy(ir,u->invite_room->name);

	sprintf(text,"~FTInvited to ~RS: %-18s ~FTMuzzled  ~RS: %-15s ~FTIgnoring ~RS: %s\n",ir,noyes2[(u->muzzled>0)],igntext); 
	write_user(user,text);
	sprintf(text,"~FTIn Room    ~RS: %-18s ~FTArrested ~RS: %-15s ~FTNew Mail ~RS: %s\n",u->room,noyes2[(u->jailed>0)],noyes2[has_unread_mail(u)]);
	write_user(user,text);

	if (u->type==REMOTE_TYPE || u->room==NULL) hs=0; else hs=1;
	sprintf(text,"~FTAt Home    ~RS: %-18s ~FTCursed   ~RS: %s\n","(null)",noyes2[(u->cursed>0)]);
	write_user(user,text);

	if ((u==user)||(user->level>THR)) {
		sprintf(text,"~FB+-=-=- User Only Info -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
		write_user(user,text);
		sprintf(text,"~FTPrompt       ~RS: %-16s ~FTMode    ~RS: %-15s ~FTAutoread   ~RS: %s\n",offon[u->prompt],mode,offon[user->autoread]);
		write_user(user,text);
		sprintf(text,"~FTChar echo    ~RS: %-16s ~FTColours ~RS: %-15s ~FTHide Email ~RS: %s\n",offon[u->charmode_echo],offon[u->colour],noyes2[u->hide_email]);
		write_user(user,text);
		sprintf(text,"~FTLogin Room   ~RS: %-16s ~FTVisible ~RS: %-15s ~FTHide URL   ~RS: %s\n",u->pref_rm,noyes2[u->vis],noyes2[u->hide_url]);
		write_user(user,text);
		sprintf(text,"~FTOn From Site ~RS: %s\n",u->last_site);
		write_user(user,text);

		days=u->total_login/86400;
		hours=(u->total_login%86400)/3600;
		mins=(u->total_login%3600)/60;
		sprintf(text,"~FTTotal login  ~RS: %d days, %d hours, %d minutes\n",days,hours,mins);
		write_user(user,text);
		}
sprintf(text,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n\n");
	write_user(user,text);
        destruct_user(u);     
        destructed=0;   
        return;
	}
sprintf(text,"~FB+-=-=- User Info -=- (Currently logged on) -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
write_user(user,text);
mins=(int)(time(0) - u->last_login)/60;
sprintf(text,"~FTName   ~RS: %-*.*s~FTLevel ~RS: %-10s\n",45+cnt*3,45+cnt*3,name,level_name[u->level]);
write_user(user,text);
sprintf(text,"~FTGender ~RS: %-10s      ~FTAge ~RS: %-17s ~FTOnline for ~RS: %d mins\n",u->gend,u->age,mins);
write_user(user,text);
sprintf(text,"~FTEmail Address ~RS: %-50s\n",vemail);
write_user(user,text);
sprintf(text,"~FTHomepage URL  ~RS: %-50s\n",vurl);
write_user(user,text);
sprintf(text,"~FT+----- Game Info --------------------------------------------------------------+\n");
write_user(user,text);
sprintf(text,"~FTTicTacToe Stats~RS  : %d - %d - %d (win/lose/draw)\n",u->tic_win,u->tic_lose,u->tic_draw);
write_user(user,text);
sprintf(text,"~FTHangMan Stats~RS    : %d - %d (win/lose)\n",u->hang_win,u->hang_lose);
write_user(user,text);
sprintf(text,"~FTNerf Stats~RS       : %d - %d - %d - %d (win/lose/life/charge)\n",u->nerf_win,u->nerf_lose,u->nerflife,u->nerfcharge);
write_user(user,text);
sprintf(text,"~FT+----- General Info -----------------------------------------------------------+\n");
write_user(user,text);
sprintf(text,"~FTEnter Msg  ~RS: %s\n",u->in_phrase);
write_user(user,text);
sprintf(text,"~FTExit Msg   ~RS: %s\n",u->out_phrase);
write_user(user,text);

if ((noyes2[u->ignall]=="YES")||((noyes2[u->ignshout]=="YES")&&(noyes2[u->igntell]))) 
	strcpy(igntext,"ALL");
else if (noyes2[u->ignshout]=="YES") strcpy(igntext,"SHOUTS");
else if (noyes2[u->igntell]=="YES") strcpy(igntext,"TELLS");
else strcpy(igntext,"NO");

if (u->invite_room==NULL) strcpy(ir,"<nowhere>");
else strcpy(ir,u->invite_room->name);

sprintf(text,"~FTInvited to ~RS: %-18s ~FTMuzzled  ~RS: %-15s ~FTIgnoring ~RS: %s\n",ir,noyes2[(u->muzzled>0)],igntext); 
write_user(user,text);
sprintf(text,"~FTIn Room    ~RS: %-18s ~FTArrested ~RS: %-15s ~FTNew Mail ~RS: %s\n",u->room,noyes2[(u->jailed>0)],noyes2[has_unread_mail(u)]);
write_user(user,text);

if (u->type==REMOTE_TYPE || u->room==NULL) hs=0; else hs=1;
sprintf(text,"~FTAt Home    ~RS: %-18s ~FTCursed   ~RS: %s\n",noyes2[hs],noyes2[(u->cursed>0)]);
write_user(user,text);

if ((u2==user)||(user->level>THR)) {
	sprintf(text,"~FT+----- User Only Info ---------------------------------------------------------+\n");
	write_user(user,text);
	sprintf(text,"~FTPrompt       ~RS: %-16s ~FTMode    ~RS: %-15s ~FTAutoread   ~RS: %s\n",offon[u->prompt],mode,offon[user->autoread]);
	write_user(user,text);
	sprintf(text,"~FTChar echo    ~RS: %-16s ~FTColours ~RS: %-15s ~FTHide Email ~RS: %s\n",offon[u->charmode_echo],offon[u->colour],noyes2[u->hide_email]);
	write_user(user,text);
	sprintf(text,"~FTLogin Room   ~RS: %-16s ~FTVisible ~RS: %-15s ~FTHide URL   ~RS: %s\n",u->pref_rm,noyes2[u->vis],noyes2[u->hide_url]);
	write_user(user,text);
	sprintf(text,"~FTOn From Site ~RS: %s\n",u->site);
	write_user(user,text);

	days=u->total_login/86400;
	hours=(u->total_login%86400)/3600;
	mins=(u->total_login%3600)/60;
	sprintf(text,"~FTTotal login  ~RS: %d days, %d hours, %d minutes\n",days,hours,mins);
	write_user(user,text);
	}
sprintf(text,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n\n");
write_user(user,text);

}



/*** Read your mail ***/
rmail(user,bot)
UR_OBJECT user;
{
FILE *infp,*outfp;
int ret;
char c,filename[80],line[DNL+1],temp[USER_NAME_LEN+1];

if (bot) {
	sprintf(filename,"%s/%s.M",USERFILES,bot_bot->name);
	strcpy(temp,BOTNAME);
	toupper(temp[0]);
	}
else sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(infp=fopen(filename,"r"))) {
        if (bot) {
                sprintf(text,"%s has no mail.\n",temp);
                write_user(user,text);
                }
        else write_user(user,"You have no mail.\n");
        return;
        }
/* Update last read / new mail received time at head of file */
if (outfp=fopen("tempfile","w")) {
	fprintf(outfp,"%d\r",(int)(time(0)));
	/* skip first line of mail file */
	fgets(line,DNL,infp);

	/* Copy rest of file */
	c=getc(infp);
	while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }

	fclose(outfp);
	rename("tempfile",filename);
	}
user->read_mail=time(0);
fclose(infp);
/* Just reading the one message */
if (word_count>1) {
        read_specific_mail(user);
        return;
        }
/* Readong the whole mail box */
if (bot) {
        sprintf(text,"\n~OL~FB*** %s's mail ***\n\n",temp);
        write_user(user,text);
        }
else write_user(user,"\n~OL~FB*** Your mail ***\n\n");
ret=more(user,user->socket,filename);
if (ret==1) user->misc_op=2;
}


/*************************************************************
*** allows a user to choose what message to read
*************************************************************/
read_specific_mail(user)
UR_OBJECT user;
{
FILE *fp;
int valid,cnt,total,smail_number;
char w1[ARR_SIZE],line[ARR_SIZE],filename[80];
        
if (word_count>2) {
        write_user(user,"Usage: rmail [message #]\n");
        return;
        }
sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(total=mail_count(user))) {
        write_user(user,"You currently have no mail.\n");
        return;
        }
smail_number=atoi(word[1]);
if (!smail_number) {
        write_user(user,"Usage: rmail [message #]\n");
        return;
        }
if (smail_number>total) {
        sprintf(text,"You only have %d messages in your mailbox.\n",total);
        write_user(user,text);
        return;
        }
sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) {
        write_user(user,"There was an error trying to read your mailbox.\n");
        sprintf(text,"Unable to open %s's mailbox in read_mail_specific.\n",user->name);
        write_syslog(text,0);
        return;
        }
valid=1;  cnt=1;
fgets(line,DNL,fp); 
fgets(line,ARR_SIZE-1,fp);
while(!feof(fp)) {
        if (*line=='\n') valid=1;
        sscanf(line,"%s",w1);
        if (valid && (!strcmp(w1,"~OLFrom:") || !strcmp(w1,"From:"))) {
                if (smail_number==cnt) {
                        write_user(user,"\n");
                        while(*line!='\n') {
                                write_user(user,line);
                                fgets(line,ARR_SIZE-1,fp);
                                }
                        }
                valid=0;  cnt++;
                /* no point carrying on if read already */
                if (cnt>smail_number) goto SKIP;
                }
        fgets(line,ARR_SIZE-1,fp);
        }

SKIP:
fclose(fp);
sprintf(text,"\nMessage number ~FM~OL%d~RS out of ~FM~OL%d~RS.\n\n",smail_number,total);
write_user(user,text);
}



/*** Send mail message ***/
smail(user,inpstr,done_editing)
UR_OBJECT user;
char *inpstr;
int done_editing;
{
UR_OBJECT u;
FILE *fp;
int remote,has_account;
char *c,filename[80];

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot mail anyone.\n");  return;
	}
if (done_editing) {
        if (*user->malloc_end--!='\n') *user->malloc_end--='\n';
        send_mail(user,user->mail_to,user->malloc_start,0);
        send_copies(user,user->malloc_start);
	user->mail_to[0]='\0';
	return;
	}
if (word_count<2) {
	write_user(user,"Smail who?\n");  return;
	}
/* See if its to another site */
remote=0;
has_account=0;
c=word[1];
while(*c) {
	if (*c=='@') {  
		if (c==word[1]) {
			write_user(user,"Users name missing before @ sign.\n");  
			return;
			}
		remote=1;  break;  
		}
	++c;
	}
word[1][0]=toupper(word[1][0]);
/* See if user exists */
if (!remote) {
	u=NULL;
	if (!(u=get_user(word[1]))) {
		sprintf(filename,"%s/%s.D",USERFILES,word[1]);
		if (!(fp=fopen(filename,"r"))) {
			write_user(user,nosuchuser);  return;
			}
		has_account=1;
		fclose(fp);
		}
        if (u==user && user->level<FOU) {
		write_user(user,"Trying to mail yourself is the fifth sign of madness.\n");
		return;
		}
	if (u!=NULL) strcpy(word[1],u->name); 
	if (!has_account) {
		/* See if user has local account */
		sprintf(filename,"%s/%s.D",USERFILES,word[1]);
		if (!(fp=fopen(filename,"r"))) {
			sprintf(text,"%s is a remote user and does not have a local account.\n",u->name);
			write_user(user,text);  
			return;
			}
		fclose(fp);
		}
	}
if (word_count>2) {
	/* One line mail */
	strcat(inpstr,"\n"); 
	send_mail(user,word[1],remove_first(inpstr),0);
        send_copies(user,remove_first(inpstr));
	return;
	}
if (user->type==REMOTE_TYPE) {
	write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nUse the '.smail <user> <mesg>' method instead.\n");
	return;
	}
sprintf(text,"\n~OL~FB*** Writing mail message to %s ***\n\n",word[1]);
write_user(user,text);
user->misc_op=4;
strcpy(user->mail_to,word[1]);
editor(user,NULL);
}


/*** Delete some or all of your mail. A problem here is once we have deleted
     some mail from the file do we mark the file as read? If not we could
     have a situation where the user deletes all his mail but still gets
     the YOU HAVE UNREAD MAIL message on logging on if the idiot forgot to 
     read it first. ***/
dmail(user,bot)
UR_OBJECT user;
{
FILE *infp,*outfp;
int num,cnt,i;
char filename[80],line[ARR_SIZE],temp[USER_NAME_LEN+1];
 
strcpy(temp,BOTNAME);
toupper(temp[0]);
if (word_count<2) {
        write_user(user,"Usage: dmail all\n");
        write_user(user,"Usage: dmail <#>\n");
        write_user(user,"Usage: dmail to <#>\n");
        write_user(user,"Usage: dmail from <#> to <#>\n");
        return;
        }
if (get_wipe_parameters(user)==-1) return;
num=mail_count(user);
if (bot) sprintf(filename,"%s/%s.M",USERFILES,bot_bot->name);
else sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (user->wipe_from==-1) {
        if (bot) {
                sprintf(text,"All of %s's mail deleted.\n",temp);
                write_user(user,text);
                unlink(filename);
                return;
                }  
        write_user(user,"\n\07~FR~OL~LI*** WARNING - This will delete all your smail! ***\n\nAre you sure about this (y/n)? ");
        user->misc_op=10;
        no_prompt=1;
        return;
        }
if (user->wipe_from>num) {
        if (bot) sprintf(text,"%s only has %d mail messages.\n",temp,num);
        else sprintf(text,"You only have %d mail messages.\n",num);
        write_user(user,text);
        return;
        }
if (!(infp=fopen(filename,"r"))) {
        if (bot) {
                sprintf(text,"%s has no mail to delete.\n");
                write_user(user,text);
                }
	else write_user(user,"You have no mail to delete.\n");  
	return;
	}
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile in dmail().\n",0);
        fclose(infp);
        return;
        }
fprintf(outfp,"%d\r",(int)time(0));
user->read_mail=time(0);
cnt=0;  
fgets(line,DNL,infp); /* Get header date */
for (i=1;i<user->wipe_from;i++) {
        fgets(line,82,infp);
        while(line[0]!='\n') {
                if (feof(infp)) goto SKIP_WIPE;
                fputs(line,outfp);  fgets(line,82,infp);
                }
        fputs(line,outfp);
        }
for (;i<=user->wipe_to;i++) { 
        fgets(line,82,infp);
        if (i==num) { cnt++; goto SKIP_WIPE; }
        while(line[0]!='\n') fgets(line,82,infp);
        cnt++;
        }
fgets(line,82,infp);
while(!feof(infp)) {
        fputs(line,outfp);
        if (line[0]=='\n') i++;  
        fgets(line,82,infp);
        }
SKIP_WIPE:
fclose(infp);
fclose(outfp);
unlink(filename);
if (user->wipe_from==0 && i<user->wipe_to) {
        unlink("tempfile");
        if (bot) sprintf(text,"There were only %d messages in %s's mailbox, all now deleted.\n",cnt,temp);
        else sprintf(text,"There were only %d mail messages, all now deleted.\n",cnt);
        write_user(user,text);
        return;
        }
if ((user->wipe_from==0 && i==user->wipe_to) || cnt==num) {
        unlink("tempfile"); /* cos it'll be empty anyway */
        if (bot) {
                sprintf(text,"All %s's messages deleted.\n",temp);
                write_user(user,text);
                }
        else write_user(user,"All mail messages deleted.\n");
        }
else {
        rename("tempfile",filename);
        if (bot) sprintf(text,"%d of %s's messages deleted.\n",num,temp);
        else sprintf(text,"%d mail messages deleted.\n",cnt);
        write_user(user,text);
        }
}

/*** Show list of people your mail is from without seeing the whole lot ***/
mail_from(user)
UR_OBJECT user;
{
FILE *fp;
int valid,cnt;
char w1[ARR_SIZE],line[ARR_SIZE],filename[80];

sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) {
	write_user(user,"~OL~FT-- MAILDAEMON : You have no mail!\n");
	return;
	}
write_user(user,"\n~OL~FT-- MAILDAEMON : Mail from...\n\n");
valid=1;  cnt=0;
fgets(line,DNL,fp); 
fgets(line,ARR_SIZE-1,fp);
while(!feof(fp)) {
	if (*line=='\n') valid=1;
	sscanf(line,"%s",w1);
	if (valid && (!strcmp(w1,"~OLFrom:") || !strcmp(w1,"From:"))) {
		write_user(user,remove_first(line));  
		cnt++;  valid=0;
		}
	fgets(line,ARR_SIZE-1,fp);
	}
fclose(fp);
sprintf(text,"\nTotal of %d messages.\n\n",cnt);
write_user(user,text);
}


/*************************************************************
*** count number of messages in user's mail box
*************************************************************/
mail_count(user)
UR_OBJECT user;
{
FILE *fp;
int valid,cnt=0;
char w1[ARR_SIZE],line[ARR_SIZE],filename[80],*str,*colour_com_strip();
        
sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) return cnt;
valid=1;
fgets(line,DNL,fp);
fgets(line,ARR_SIZE-1,fp);
while(!feof(fp)) {
        if (*line=='\n') valid=1;
        sscanf(line,"%s",w1);
        str=colour_com_strip(w1);
        if (valid && !strcmp(str,"From:")) {
                cnt++;  valid=0;
                }
        fgets(line,ARR_SIZE-1,fp);
        }
fclose(fp);
return cnt;
}    


/*************************************************************
*** delete lines from boards or mail or suggestions, etc
*************************************************************/
get_wipe_parameters(user)  
UR_OBJECT user;
{
int retval=-1;   
/* get delete paramters */
strtolower(word[1]);
if (!strcmp(word[1],"all")) {
        user->wipe_from=-1; user->wipe_to=-1;
	}
else if (!strcmp(word[1],"from")) {
        if (word_count<4 || strcmp(word[3],"to")) {
                write_user(user,"Usage: <command> from <#> to <#>\n");
                return(retval);
                }
        user->wipe_from=atoi(word[2]);
        user->wipe_to=atoi(word[4]);
        }
else if (!strcmp(word[1],"to")) {
        if (word_count<2) {
                write_user(user,"Usage: <command> to <#>\n");
                return(retval);
                }
        user->wipe_from=0;
        user->wipe_to=atoi(word[2]);
        }
else {
        user->wipe_from=atoi(word[1]);
        user->wipe_to=atoi(word[1]);
        }
if (user->wipe_from>user->wipe_to) {
        write_user(user,"The first number must be smaller than the second number.\n");
        return(retval);
        }
retval=1;
return(retval);
}



/*** Enter user profile ***/
enter_profile(user,done_editing)
UR_OBJECT user;
int done_editing;
{
FILE *fp;
char *c,filename[80];

if (!done_editing) {
	write_user(user,"\n~BB*** Writing profile ***\n\n");
	user->misc_op=5;
	editor(user,NULL);
	return;
	}
sprintf(filename,"%s/%s.P",USERFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
	sprintf(text,"%s: couldn't save your profile.\n",syserror);
	write_user(user,text);
	sprintf("ERROR: Couldn't open file %s to write in enter_profile().\n",filename);
	write_syslog(text,0);
	return;
	}
c=user->malloc_start;
while(c!=user->malloc_end) putc(*c++,fp);
fclose(fp);
write_user(user,"Profile stored.\n");
}


/*** Examine a user ***/
examine(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
FILE *fp;
char filename[80],line[82];
char vemail[EMAIL_LEN+1],vurl[URL_LEN+1],name[ARR_SIZE];
int new_mail,days,hours,mins,timelen,days2,hours2,mins2,idle,cnt;

if ((word_count<2)||(!strcmp(word[1],""))) {
	strcpy(word[1], user->name);
	}
if (!(u=get_user(word[1]))) {
	if ((u=create_user())==NULL) {
		sprintf(text,"%s: unable to create temporary user object.\n",syserror);
		write_user(user,text);
		write_syslog("ERROR: Unable to create temporary user object in examine().\n",0);
		return;
		}
	strcpy(u->name,word[1]);
	if (!load_user_details(u)) {
		write_user(user,nosuchuser);   
		destruct_user(u);
		destructed=0;
		return;
		}
	u2=NULL;
	}
else u2=u;

if ((u->hide_email==1)&&(user->level<FOU))
	strcpy(vemail,"<email address only viewable by wizards>");
else strcpy(vemail,u->email);
if (user==u) strcpy(vemail,u->email);

if ((u->hide_url==1)&&(user->level<FOU))
	strcpy(vurl,"<homepage url only viewable by wizards>");
else strcpy(vurl,u->url);
if (user==u) strcpy(vurl,u->url);

sprintf(name,"%s %s~RS",u->name,u->desc);
cnt=colour_com_count(name);

sprintf(text,"\n~FT+---- General Info ------------------------------------------------------------+\n");
write_user(user,text);
sprintf(text,"~FTName         : ~RS%-*.*s~RS ~FTLevel : ~FR%s\n",45+cnt*3,45+cnt*3,name,level_name[u->level]);
write_user(user,text);

days=u->total_login/86400;
hours=(u->total_login%86400)/3600;
mins=(u->total_login%3600)/60;
timelen=(int)(time(0) - u->last_login);
days2=timelen/86400;
hours2=(timelen%86400)/3600;
mins2=(timelen%3600)/60;

if (u2==NULL) {
        if (user->level>=FOU) {
                sprintf(text,"~FTLast site    : ~OL~FR%s\n",u->last_site);
                write_user(user,text);
                }
	sprintf(text,"~FTLast Login   : ~FG%s",ctime((time_t*)&(u->last_login)));
	write_user(user,text);
	sprintf(text,"~FTWhich was    :~FG %d days, %d hours, %d minutes ago\n",days2,hours2,mins2);
	write_user(user,text);
	sprintf(text,"~FTWas on for   :~FG %d hours, %d minutes\n",u->last_login_len/3600,(u->last_login_len%3600)/60);     
	write_user(user,text);
	sprintf(text,"~FTTotal login  :~FB %d days, %d hours, %d minutes\n",days,hours,mins);
	write_user(user,text);

	sprintf(text,"~FT+-=-=- Personal Info ----------------------------------------------------------+\n");
	write_user(user,text);
	sprintf(text,"~FTGender : ~FM%-10s~FTEmail : ~FM%-46s\n",u->gend,vemail);
	write_user(user,text);
	sprintf(text,"~FTAge : ~FY%-10s   ~FTHomepage : ~FY%-46s\n",u->age,vurl);
	write_user(user,text);

	sprintf(text,"~FT+-=-=- Profile ----------------------------------------------------------------+\n");
	write_user(user,text);
	
	sprintf(filename,"%s/%s.P",USERFILES,u->name);
	if (!(fp=fopen(filename,"r"))) write_user(user,"No Profile.\n");
	else {
		fgets(line,81,fp);
		while(!feof(fp)) {
			sprintf(text,line);
			write_user(user,text);
			fgets(line,81,fp);
			}
		fclose(fp);
	}

sprintf(text,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
	write_user(user,text);
	destruct_user(u);
	destructed=0;
	return;
	}
idle=(int)(time(0) - u->last_input)/60;

if (user->level>=FOU) {
	sprintf(text,"~FTSite         :~FR %s:%d\n",u->site,u->site_port);
        write_user(user,text);
        }
sprintf(text,"~FTOn since     :~FG %s",ctime((time_t *)&u->last_login));
write_user(user,text);
sprintf(text,"~FTOn for       :~FG %d hours, %d minutes\n",hours2,mins2);
write_user(user,text);
if (u->afk) {
	sprintf(text,"~FTIdle for     :~FG %d minutes ~BR(AFK)\n",idle);
	write_user(user,text);
	if (u->afk_mesg[0]) {
		sprintf(text,"~FTAFK message  :~RS %s\n",u->afk_mesg);
		write_user(user,text);
		}
	}
else {
	sprintf(text,"~FTIdle for     :~FG %d minutes\n",idle);
	write_user(user,text);
	}
sprintf(text,"~FTTotal login  :~FB %d days, %d hours, %d minutes\n",days,hours,mins);
write_user(user,text);
if (u->socket==-1) {
	sprintf(text,"~FTHome service : %s\n",u->netlink->service);
	write_user(user,text);
	}
sprintf(text,"~FT+---- Personal Info -----------------------------------------------------------+\n");
write_user(user,text);
sprintf(text,"~FTGender : ~FM%-10s~FTEmail : ~FM%-46s\n",u->gend,vemail);
write_user(user,text);
sprintf(text,"~FTAge:~FY %-10s    ~FTHomepage :~FY %-46s\n",u->age,vurl);
write_user(user,text);
sprintf(text,"~FT+---- Profile -----------------------------------------------------------------+\n");
write_user(user,text);

sprintf(filename,"%s/%s.P",USERFILES,u->name);
if (!(fp=fopen(filename,"r"))) write_user(user,"No Profile.\n");
else {
	fgets(line,81,fp);
        while(!feof(fp)) {
        	sprintf(text,line);
                write_user(user,text);
                fgets(line,81,fp);
                }
        fclose(fp);
        }
sprintf(text,"~FB+-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-+\n");
write_user(user,text);
write_user(user,"\n");
}


/*** Show talker rooms ***/
rooms(user,show_topics)
UR_OBJECT user;
int show_topics;
{
RM_OBJECT rm;
UR_OBJECT u;
NL_OBJECT nl;
char access[9],stat[9],serv[SERV_NAME_LEN+1];
int cnt;

if (show_topics) 
	write_user(user,"\n~OL~FB*** Rooms data ***\n\n~FTRoom name            : Access  Users  Mesgs  Topic\n\n");
else write_user(user,"\n~OL~FB*** Rooms data ***\n\n~FTRoom name            : Access  Users  Mesgs  Inlink  LStat  Service\n\n");
for(rm=room_first;rm!=NULL;rm=rm->next) {
	if (rm->access & PRIVATE) strcpy(access," ~FRPRIV");
	else strcpy(access,"  ~FGPUB");
	if (rm->access & FIXED) access[0]='*';
	cnt=0;
	for(u=user_first;u!=NULL;u=u->next) 
		if (u->type!=CLONE_TYPE && u->room==rm) ++cnt;
	if (show_topics)
		sprintf(text,"%-20s : %9s~RS    %3d    %3d  %s\n",rm->name,access,cnt,rm->mesg_cnt,rm->topic);
	else {
		nl=rm->netlink;  serv[0]='\0';
		if (nl==NULL) {
			if (rm->inlink) strcpy(stat,"~FRDOWN");
			else strcpy(stat,"   -");
			}
		else {
			if (nl->type==UNCONNECTED) strcpy(stat,"~FRDOWN");
				else if (nl->stage==UP) strcpy(stat,"  ~FGUP");
					else strcpy(stat," ~FYVER");
			}
		if (nl!=NULL) strcpy(serv,nl->service);
		sprintf(text,"%-20s : %9s~RS    %3d    %3d     %s   %s~RS  %s\n",rm->name,access,cnt,rm->mesg_cnt,noyes1[rm->inlink],stat,serv);
		}
	write_user(user,text);
	}
write_user(user,"\n");
}


/*** List defined netlinks and their status ***/
netstat(user)
UR_OBJECT user;
{
NL_OBJECT nl;
UR_OBJECT u;
char *allow[]={ "  ?","ALL"," IN","OUT" };
char *type[]={ "  -"," IN","OUT" };
char portstr[6],stat[9],vers[8];
int iu,ou,a;

if (nl_first==NULL) {
	write_user(user,"No remote connections configured.\n");  return;
	}
write_user(user,"\n~BB*** Netlink data & status ***\n\n~FTService name    : Allow Type Status IU OU Version  Site\n\n");
for(nl=nl_first;nl!=NULL;nl=nl->next) {
	iu=0;  ou=0;
	if (nl->stage==UP) {
		for(u=user_first;u!=NULL;u=u->next) {
			if (u->netlink==nl) {
				if (u->type==REMOTE_TYPE)  ++iu;
				if (u->room==NULL) ++ou;
				}
			}
		}
	if (nl->port) sprintf(portstr,"%d",nl->port);  else portstr[0]='\0';
	if (nl->type==UNCONNECTED) {
		strcpy(stat,"~FRDOWN");  strcpy(vers,"-");
		}
	else {
		if (nl->stage==UP) strcpy(stat,"  ~FGUP");
		else strcpy(stat," ~FYVER");
		if (!nl->ver_major) strcpy(vers,"3.?.?"); /* Pre - 3.2 version */  
		else sprintf(vers,"%d.%d.%d",nl->ver_major,nl->ver_minor,nl->ver_patch);
		}
	/* If link is incoming and remoter vers < 3.2 we have no way of knowing 
	   what the permissions on it are so set to blank */
	if (!nl->ver_major && nl->type==INCOMING && nl->allow!=IN) a=0; 
	else a=nl->allow+1;
	sprintf(text,"%-15s :   %s  %s   %s~RS %2d %2d %7s  %s %s\n",nl->service,allow[a],type[nl->type],stat,iu,ou,vers,nl->site,portstr);
	write_user(user,text);
	}
write_user(user,"\n");
}



/*** Show type of data being received down links (this is usefull when a
     link has hung) ***/
netdata(user)
UR_OBJECT user;
{
NL_OBJECT nl;
char from[80],name[USER_NAME_LEN+1];
int cnt;

cnt=0;
write_user(user,"\n~BB*** Mail receiving status ***\n\n");
for(nl=nl_first;nl!=NULL;nl=nl->next) {
	if (nl->type==UNCONNECTED || nl->mailfile==NULL) continue;
	if (++cnt==1) write_user(user,"To              : From                       Last recv.\n\n");
	sprintf(from,"%s@%s",nl->mail_from,nl->service);
	sprintf(text,"%-15s : %-25s  %d seconds ago.\n",nl->mail_to,from,(int)(time(0)-nl->last_recvd));
	write_user(user,text);
	}
if (!cnt) write_user(user,"No mail being received.\n\n");
else write_user(user,"\n");

cnt=0;
write_user(user,"\n~BB*** Message receiving status ***\n\n");
for(nl=nl_first;nl!=NULL;nl=nl->next) {
	if (nl->type==UNCONNECTED || nl->mesg_user==NULL) continue;
	if (++cnt==1) write_user(user,"To              : From             Last recv.\n\n");
	if (nl->mesg_user==(UR_OBJECT)-1) strcpy(name,"<unknown>");
	else strcpy(name,nl->mesg_user->name);
	sprintf(text,"%-15s : %-15s  %d seconds ago.\n",name,nl->service,(time(0)-nl->last_recvd));
	write_user(user,text);
	}
if (!cnt) write_user(user,"No messages being received.\n\n");
else write_user(user,"\n");
}


/*** Connect a netlink. Use the room as the key ***/
connect_netlink(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;
int ret,tmperr;

if (word_count<2) {
	write_user(user,"Usage: connect <room service is linked to>\n");  return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
if ((nl=rm->netlink)==NULL) {
	write_user(user,"That room is not linked to a service.\n");
	return;
	}
if (nl->type!=UNCONNECTED) {
	write_user(user,"That rooms netlink is already up.\n");  return;
	}
write_user(user,"Attempting connect (this may cause a temporary hang)...\n");
sprintf(text,"NETLINK: Connection attempt to %s initiated by %s.\n",nl->service,user->name);
write_syslog(text,1);
errno=0;
if (!(ret=connect_to_site(nl))) {
	write_user(user,"~FGInitial connection made...\n");
	sprintf(text,"NETLINK: Connected to %s (%s %d).\n",nl->service,nl->site,nl->port);
	write_syslog(text,1);
	nl->connect_room=rm;
	return;
	}
tmperr=errno; /* On Linux errno seems to be reset between here and sprintf */
write_user(user,"~FRConnect failed: ");
write_syslog("NETLINK: Connection attempt failed: ",1);
if (ret==1) {
	sprintf(text,"%s.\n",sys_errlist[tmperr]);
	write_user(user,text);
	write_syslog(text,0);
	return;
	}
write_user(user,"Unknown hostname.\n");
write_syslog("Unknown hostname.\n",0);
}



/*** Disconnect a link ***/
disconnect_netlink(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;

if (word_count<2) {
	write_user(user,"Usage: disconnect <room service is linked to>\n");  return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
nl=rm->netlink;
if (nl==NULL) {
	write_user(user,"That room is not linked to a service.\n");
	return;
	}
if (nl->type==UNCONNECTED) {
	write_user(user,"That rooms netlink is not connected.\n");  return;
	}
/* If link has hung at verification stage don't bother announcing it */
if (nl->stage==UP) {
	sprintf(text,"~OLSYSTEM:~RS Disconnecting from %s in the %s.\n",nl->service,rm->name);
	write_room(NULL,text);
	sprintf(text,"NETLINK: Link to %s in the %s disconnected by %s.\n",nl->service,rm->name,user->name);
	write_syslog(text,1);
	}
else {
	sprintf(text,"NETLINK: Link to %s disconnected by %s.\n",nl->service,user->name);
	write_syslog(text,1);
	}
shutdown_netlink(nl);
write_user(user,"Disconnected.\n");
}


/*** Change users password. Only THRes and above can change another users 
	password and they do this by specifying the user at the end. When this is 
	done the old password given can be anything, the wiz doesnt have to know it
	in advance. ***/
change_pass(user)
UR_OBJECT user;
{
UR_OBJECT u;
char *name;

if (word_count<3) {
	if (user->level<SEV)
		write_user(user,"Usage: passwd <old password> <new password>\n");
	else write_user(user,"Usage: passwd <old password> <new password> [<user>]\n");
	return;
	}
if (strlen(word[2])<3) {
	write_user(user,"New password too short.\n");  return;
	}
if (strlen(word[2])>PASS_LEN) {
	write_user(user,"New password too long.\n");  return;
	}
/* Change own password */
if (word_count==3) {
	if (strcmp((char *)crypt(word[1],"NU"),user->pass)) {
		write_user(user,"Old password incorrect.\n");  return;
		}
	if (!strcmp(word[1],word[2])) {
		write_user(user,"Old and new passwords are the same.\n");  return;
		}
	strcpy(user->pass,(char *)crypt(word[2],"NU"));
	save_user_details(user,0);
	cls(user);
	write_user(user,"Password changed.\n");
	return;
	}
/* Change someone elses */
if (user->level<SEV) {
	write_user(user,"You are not a high enough level to use the user option.\n");  
	return;
	}
word[3][0]=toupper(word[3][0]);
if (!strcmp(word[3],user->name)) {
	/* security feature  - prevents someone coming to a wizes terminal and 
	   changing his password since he wont have to know the old one */
	write_user(user,"You cannot change your own password using the <user> option.\n");
	return;
	}
if (u=get_user(word[3])) {
	if (u->type==REMOTE_TYPE) {
		write_user(user,"You cannot change the password of a user logged on remotely.\n");
		return;
		}
	if (u->level>=user->level) {
		write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
		return;
		}
	strcpy(u->pass,(char *)crypt(word[2],"NU"));
	cls(user);
	sprintf(text,"%s's password has been changed.\n",u->name);
	write_user(user,text);
	if (user->vis) name=user->name; else name=invisname;
	sprintf(text,"~FR~OLYour password has been changed by %s!\n",name);
	write_user(u,text);
	sprintf(text,"%s changed %s's password.\n",user->name,u->name);
	write_syslog(text,1);
	return;
	}
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in change_pass().\n",0);
	return;
	}
strcpy(u->name,word[3]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);   
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
strcpy(u->pass,(char *)crypt(word[2],"NU"));
save_user_details(u,0);
destruct_user(u);
destructed=0;
cls(user);
sprintf(text,"%s's password changed to \"%s\".\n",word[3],word[2]);
write_user(user,text);
sprintf(text,"%s changed %s's password.\n",user->name,u->name);
write_syslog(text,1);
}


/*** Kill a user ***/
kill_user(user)
UR_OBJECT user;
{
UR_OBJECT victim;
RM_OBJECT rm;
char *name;

if (word_count<2) {
	write_user(user,"Usage: kill <user>\n");  return;
	}
if (!(victim=get_user(word[1]))) {
	write_user(user,notloggedon);  return;
	}
if (victim->pop !=NULL) {
        write_user(user,"~OL~FRSorry that user is playing poker ATM, type .force (user) .leave\n");
        return;
        }
if (user==victim) {
	write_user(user,"Trying to commit suicide this way is the sixth sign of madness.\n");
	return;
	}
if (victim->level>=user->level) {
	write_user(user,"You cannot kill a user of equal or higher level than yourself.\n");
	sprintf(text,"%s tried to kill you!\n",user->name);
	write_user(victim,text);
	return;
	}
sprintf(text,"%s KILLED %s.\n",user->name,victim->name);
write_syslog(text,1);
write_user(user,"~FM~OLYou Look upon your prey with despise.\n");
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~FM~OL%s noisily draws their sword...\n",name);
write_room(NULL,text);
sprintf(text,"~FM~OL%s's cold blade of steel slices through the air, and slaughters you!!!\n",name);
write_user(victim,text);
sprintf(text,"~FM~OL%s's cold blade of steel slices through the air, slaughtering %s!\n",name,victim->name);
write_room(NULL,text);
sprintf(text,"~FM~OL%s loudly sheathes their sword proclaiming triumph over the evil %s\n",name,victim->name);
write_room(NULL,text);
disconnect_user(victim);
}


/*** Promote a user ***/
promote(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char text2[80],*name;

if (word_count<2) {
	write_user(user,"Usage: promote <user>\n");  return;
	}
/* See if user is on atm */
if ((u=get_user(word[1]))!=NULL) {
	if (u->level>=user->level) {
		write_user(user,"You cannot promote a user to a level higher than your own.\n");
		return;
		}
	if (user->vis) name=user->name; else name=invisname;
	u->level++;
	sprintf(text,"~FG~OLYou promote %s to level: ~RS~OL%s.\n",u->name,level_name[u->level]);
	write_user(user,text);
	rm=user->room;
	user->room=NULL;
	sprintf(text,"~FG~OL%s promotes %s to level: ~RS~OL%s.\n",name,u->name,level_name[u->level]);
	write_room_except(NULL,text,u);
	user->room=rm;
	sprintf(text,"~FG~OL%s has promoted you to level: ~RS~OL%s!\n",name,level_name[u->level]);
	write_user(u,text);
	sprintf(text,"~OL~FY%s PROMOTED %s to level: ~FW%s.\n",name,u->name,level_name[u->level]);
	write_log(text,1,4);
	return;
	}
/* Create a temp session, load details, alter , then save. This is inefficient
   but its simpler than the alternative */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in promote().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot promote a user to a level higher than your own.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
u->level++;  
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"You promote %s to level: ~OL%s.\n",u->name,level_name[u->level]);
write_user(user,text);
sprintf(text2,"~FG~OLYou have been promoted to level: ~RS~OL%s.\n",level_name[u->level]);
send_mail(user,word[1],text2,0);
sprintf(text,"~OL~FY%s PROMOTED %s to level: ~FW%s.\n",user->name,word[1],level_name[u->level]);
write_log(text,1,4);
destruct_user(u);
destructed=0;
}


/*** Demote a user ***/
demote(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char text2[80],*name;

if (word_count<2) {
	write_user(user,"Usage: demote <user>\n");  return;
	}
/* See if user is on atm */
if ((u=get_user(word[1]))!=NULL) {
	if (u->level==ZER) {
		sprintf(text,"You cannot demote a user of level %s.\n",level_name[ZER]);
		write_user(user,text);
		return;
		}
	if (u->level>=user->level) {
		write_user(user,"You cannot demote a user of an equal or higher level than yourself.\n");
		return;
		}
	if (user->vis) name=user->name; else name=invisname;
	u->level--;
	sprintf(text,"~FR~OLYou demote %s to level: ~RS~OL%s.\n",u->name,level_name[u->level]);
	write_user(user,text);
	rm=user->room;
	user->room=NULL;
	sprintf(text,"~FR~OL%s demotes %s to level: ~RS~OL%s.\n",name,u->name,level_name[u->level]);
	write_room_except(NULL,text,u);
	user->room=rm;
	sprintf(text,"~FR~OL%s has demoted you to level: ~RS~OL%s!\n",name,level_name[u->level]);
	write_user(u,text);
	sprintf(text,"~OL~FY%s DEMOTED %s to level: ~FW%s.\n",user->name,u->name,level_name[u->level]);
	write_log(text,1,4);
	return;
	}
/* User not logged on */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in demote().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level==ZER) {
	sprintf(text,"You cannot demote a user of level %s.\n",level_name[ZER]);
	write_user(user,text);
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot demote a user of an equal or higher level than yourself.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
u->level--;
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"You demote %s to level: ~OL%s.\n",u->name,level_name[u->level]);
write_user(user,text);
sprintf(text2,"~FR~OLYou have been demoted to level: ~RS~OL%s.\n",level_name[u->level]);
send_mail(user,word[1],text2,0);
sprintf(text,"~OL~FY%s DEMOTED %s to level: ~FW%s.\n",user->name,word[1],level_name[u->level]);
write_log(text,1,4);
destruct_user(u);
destructed=0;
}


/*** List banned sites or users ***/
listbans(user)
UR_OBJECT user;
{
int i;
char filename[80];

if (!strcmp(word[1],"sites")) {
	write_user(user,"\n~BB*** Banned sites/domains ***\n\n"); 
	sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
	switch(more(user,user->socket,filename)) {
		case 0:
		write_user(user,"There are no banned sites/domains.\n\n");
		return;

		case 1: user->misc_op=2;
		}
	return;
	}
if (!strcmp(word[1],"users")) {
	write_user(user,"\n~BB*** Banned users ***\n\n");
	sprintf(filename,"%s/%s",DATAFILES,USERBAN);
	switch(more(user,user->socket,filename)) {
		case 0:
		write_user(user,"There are no banned users.\n\n");
		return;

		case 1: user->misc_op=2;
		}
	return;
	}
if (!strcmp(word[1],"swears")) {
	write_user(user,"\n~BB*** Banned swear words ***\n\n");
	i=0;
	while(swear_words[i][0]!='*') {
		write_user(user,swear_words[i]);
		write_user(user,"\n");
		++i;
		}
	if (!i) write_user(user,"There are no banned swear words.\n");
	if (ban_swearing) write_user(user,"\n");
	else write_user(user,"\n(Swearing ban is currently off)\n\n");
	return;
	}
write_user(user,"Usage: listbans sites/users/swears\n"); 
}


/*** Ban a site/domain or user ***/
ban(user)
UR_OBJECT user;
{
char *usage="Usage: ban site/user <site/user name>\n";

if (word_count<3) {
	write_user(user,usage);  return;
	}
if (!strcmp(word[1],"site")) {  ban_site(user);  return;  }
if (!strcmp(word[1],"user")) {  ban_user(user);  return;  }
write_user(user,usage);
}


ban_site(user)
UR_OBJECT user;
{
FILE *fp;
char filename[80],host[81],site[80];

gethostname(host,80);
if (!strcmp(word[2],host)) {
	write_user(user,"You cannot ban the machine that this program is running on.\n");
	return;
	}
sprintf(filename,"%s/%s",DATAFILES,SITEBAN);

/* See if ban already set for given site */
if (fp=fopen(filename,"r")) {
	fscanf(fp,"%s",site);
	while(!feof(fp)) {
		if (!strcmp(site,word[2])) {
			write_user(user,"That site/domain is already banned.\n");
			fclose(fp);  return;
			}
		fscanf(fp,"%s",site);
		}
	fclose(fp);
	}

/* Write new ban to file */
if (!(fp=fopen(filename,"a"))) {
	sprintf(text,"%s: Can't open file to append.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Couldn't open file to append in ban_site().\n",0);
	return;
	}
fprintf(fp,"%s\n",word[2]);
fclose(fp);
write_user(user,"Site/domain banned.\n");
sprintf(text,"%s BANNED site/domain %s.\n",user->name,word[2]);
write_syslog(text,1);
}


ban_user(user)
UR_OBJECT user;
{
UR_OBJECT u;
FILE *fp;
char filename[80],filename2[80],p[20],name[USER_NAME_LEN+1];
int a,b,c,d,level;

word[2][0]=toupper(word[2][0]);
if (!strcmp(user->name,word[2])) {
	write_user(user,"Trying to ban yourself is the seventh sign of madness.\n");
	return;
	}

/* See if ban already set for given user */
sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (fp=fopen(filename,"r")) {
	fscanf(fp,"%s",name);
	while(!feof(fp)) {
		if (!strcmp(name,word[2])) {
			write_user(user,"That user is already banned.\n");
			fclose(fp);  return;
			}
		fscanf(fp,"%s",name);
		}
	fclose(fp);
	}

/* See if already on */
if ((u=get_user(word[2]))!=NULL) {
	if (u->level>=user->level) {
		write_user(user,"You cannot ban a user of equal or higher level than yourself.\n");
		return;
		}
	}
else {
	/* User not on so load up his data */
	sprintf(filename2,"%s/%s.D",USERFILES,word[2]);
	if (!(fp=fopen(filename2,"r"))) {
		write_user(user,nosuchuser);  return;
		}
	fscanf(fp,"%s\n%d %d %d %d %d",p,&a,&b,&c,&d,&level);
	fclose(fp);
	if (level>=user->level) {
		write_user(user,"You cannot ban a user of equal or higher level than yourself.\n");
		return;
		}
	}

/* Write new ban to file */
if (!(fp=fopen(filename,"a"))) {
	sprintf(text,"%s: Can't open file to append.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Couldn't open file to append in ban_user().\n",0);
	return;
	}
fprintf(fp,"%s\n",word[2]);
fclose(fp);
write_user(user,"User banned.\n");
sprintf(text,"%s BANNED user %s.\n",user->name,word[2]);
write_syslog(text,1);
if (u!=NULL) {
	write_user(u,"\n\07~FR~OL~LIYou have been banned from here!\n\n");
	disconnect_user(u);
	}
}

	

/*** uban a site (or domain) or user ***/
unban(user)
UR_OBJECT user;
{
char *usage="Usage: unban site/user <site/user name>\n";

if (word_count<3) {
	write_user(user,usage);  return;
	}
if (!strcmp(word[1],"site")) {  unban_site(user);  return;  }
if (!strcmp(word[1],"user")) {  unban_user(user);  return;  }
write_user(user,usage);
}


unban_site(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
char filename[80],site[80];
int found,cnt;

sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
if (!(infp=fopen(filename,"r"))) {
	write_user(user,"That site/domain is not currently banned.\n");
	return;
	}
if (!(outfp=fopen("tempfile","w"))) {
	sprintf(text,"%s: Couldn't open tempfile.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Couldn't open tempfile to write in unban_site().\n",0);
	fclose(infp);
	return;
	}
found=0;   cnt=0;
fscanf(infp,"%s",site);
while(!feof(infp)) {
	if (strcmp(word[2],site)) {  
		fprintf(outfp,"%s\n",site);  cnt++;  
		}
	else found=1;
	fscanf(infp,"%s",site);
	}
fclose(infp);
fclose(outfp);
if (!found) {
	write_user(user,"That site/domain is not currently banned.\n");
	unlink("tempfile");
	return;
	}
if (!cnt) {
	unlink(filename);  unlink("tempfile");
	}
else rename("tempfile",filename);
write_user(user,"Site ban removed.\n");
sprintf(text,"%s UNBANNED site %s.\n",user->name,word[2]);
write_syslog(text,1);
}


unban_user(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
char filename[80],name[USER_NAME_LEN+1];
int found,cnt;

sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (!(infp=fopen(filename,"r"))) {
	write_user(user,"That user is not currently banned.\n");
	return;
	}
if (!(outfp=fopen("tempfile","w"))) {
	sprintf(text,"%s: Couldn't open tempfile.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Couldn't open tempfile to write in unban_user().\n",0);
	fclose(infp);
	return;
	}
found=0;  cnt=0;
word[2][0]=toupper(word[2][0]);
fscanf(infp,"%s",name);
while(!feof(infp)) {
	if (strcmp(word[2],name)) {
		fprintf(outfp,"%s\n",name);  cnt++;
		}
	else found=1;
	fscanf(infp,"%s",name);
	}
fclose(infp);
fclose(outfp);
if (!found) {
	write_user(user,"That user is not currently banned.\n");
	unlink("tempfile");
	return;
	}
if (!cnt) {
	unlink(filename);  unlink("tempfile");
	}
else rename("tempfile",filename);
write_user(user,"User ban removed.\n");
sprintf(text,"%s UNBANNED user %s.\n",user->name,word[2]);
write_syslog(text,1);
}



/*** Set user visible or invisible ***/
visibility(user,vis)
UR_OBJECT user;
int vis;
{
if (vis) {
	if (user->vis) {
		write_user(user,"You are already visible.\n");  return;
		}
	write_user(user,"~FB~OLYou recite a melodic incantation and reappear.\n");
	sprintf(text,"~FB~OLYou hear a melodic incantation chanted and %s materialises!\n",user->name);
	write_room_except(user->room,text,user);
	user->vis=1;
	return;
	}
if (!user->vis) {
	write_user(user,"You are already invisible.\n");  return;
	}
write_user(user,"~FB~OLYou recite a melodic incantation and fade out.\n");
sprintf(text,"~FB~OL%s recites a melodic incantation and disappears!\n",user->name);
write_room_except(user->room,text,user);
user->vis=0;
}


/*** Site a user ***/
site(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
	write_user(user,"Usage: site <user>\n");  return;
	}
/* User currently logged in */
if (u=get_user(word[1])) {
	if (u->type==REMOTE_TYPE) sprintf(text,"%s is remotely connected from %s.\n",u->name,u->site);
	else sprintf(text,"%s is logged in from %s:%d.\n",u->name,u->site,u->site_port);
	write_user(user,text);
	return;
	}
/* User not logged in */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in site().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);
	destruct_user(u);
	destructed=0;
	return;
	}
sprintf(text,"%s was last logged in from %s.\n",word[1],u->last_site);
write_user(user,text);
destruct_user(u);
destructed=0;
}


/*** Wake up some sleepy herbert ***/


/*** Shout something to other wizes and gods. If the level isnt given it
	defaults to WIZ level. ***/
wizshout(user,inpstr,emotion)
UR_OBJECT user;
char *inpstr;
int emotion;
{
int lev;

if (user->muzzled) {
        write_user(user,"You are muzzled, you cannot wtell or wemote.\n");
        return;
        }
if (user->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....noone can here you.\n",JAILROOM);
        write_user(user,text);
        return;
        }
if ((word_count<2)&&(emotion==0)) {
        write_user(user,"Usage: wtell [<superuser level>] <message>\n");
        return;
        }
if ((word_count<2)&&(emotion==1)) {
        write_user(user,"Usage: wemote [<superuser level>] <message>\n");
        return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
strtoupper(word[1]);
if ((lev=get_level(word[1]))==-1) lev=FOU;
else {
        if ((lev<FOU || word_count<3)&&(emotion==0)) {
                write_user(user,"Usage: wtell [<superuser level>] <message>\n");
                return;
                }
        if ((lev<FOU || word_count<3)&&(emotion==1)) {
                write_user(user,"Usage: wemote [<superuser level>] <message>\n");
                return;
                }
        if (lev>user->level) {
                write_user(user,"You cannot specifically talk to users of a higher level than yourself.\n");
                return;
                }
        if (emotion==0) {
                inpstr=remove_first(inpstr);
                sprintf(text,"~OL[wtell<%s> ] %s:~RS %s\n",level_name[lev],user->name,inpstr);
                write_user(user,text);
                sprintf(text,"~OL[wtell<%s> ] %s:~RS %s\n",level_name[lev],user->name,inpstr);
                write_level(lev,1,text,user);
                return;
                }
        else {   
                inpstr=remove_first(inpstr);
                sprintf(text,"~OL[wemote<%s>]~RS %s %s\n",level_name[lev],user->name,inpstr);
                write_user(user,text);
                sprintf(text,"~OL[wemote<%s>]~RS %s %s\n",level_name[lev],user->name,inpstr);
                write_level(lev,1,text,user);
                return;
                }
        }
if (emotion==0) {
        sprintf(text,"~OL[wtell ] %s:~RS %s\n",user->name,inpstr);
        write_user(user,text);
        sprintf(text,"~OL[wtell ] %s:~RS %s\n",user->name,inpstr);
        write_level(lev,1,text,user);
        }
else {
        sprintf(text,"~OL[wemote]~RS %s %s\n",user->name,inpstr);
        write_user(user,text);
        sprintf(text,"~OL[wemote]~RS %s %s\n",user->name,inpstr);
        write_level(lev,1,text,user);
        }
}

/*** Muzzle an annoying user so he cant speak, emote, echo, write, smail
	or bcast. Muzzles have levels from WIZ to GOD so for instance a wiz
     cannot remove a muzzle set by a god.  ***/
muzzle(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
	write_user(user,"Usage: muzzle <user>\n");  return;
	}
if ((u=get_user(word[1]))!=NULL) {
	if (u==user) {
		write_user(user,"Trying to muzzle yourself is the ninth sign of madness.\n");
		return;
		}
	if (u->level>=user->level) {
		write_user(user,"You cannot muzzle a user of equal or higher level than yourself.\n");
		return;
		}
	if (u->muzzled>=user->level) {
		sprintf(text,"%s is already muzzled.\n",u->name);
		write_user(user,text);  return;
		}
	sprintf(text,"~FR~OL%s now has a muzzle of level: ~RS~OL%s.\n",u->name,level_name[user->level]);
	write_user(user,text);
	write_user(u,"~FR~OLYou have been muzzled!\n");
	sprintf(text,"%s muzzled %s.\n",user->name,u->name);
	write_syslog(text,1);
	u->muzzled=user->level;
	return;
	}
/* User not logged on */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in muzzle().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot muzzle a user of equal or higher level than yourself.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->muzzled>=user->level) {
	sprintf(text,"%s is already muzzled.\n",u->name);
	write_user(user,text); 
	destruct_user(u);
	destructed=0;
	return;
	}
u->socket=-2;
u->muzzled=user->level;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"~FR~OL%s given a muzzle of level: ~RS~OL%s.\n",u->name,level_name[user->level]);
write_user(user,text);
send_mail(user,word[1],"~FR~OLYou have been muzzled!\n",0);
sprintf(text,"%s muzzled %s.\n",user->name,u->name);
write_syslog(text,1);
destruct_user(u);
destructed=0;
}



/*** Umuzzle the bastard now he's apologised and grovelled enough via email ***/
unmuzzle(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
	write_user(user,"Usage: unmuzzle <user>\n");  return;
	}
if ((u=get_user(word[1]))!=NULL) {
	if (u==user) {
		write_user(user,"Trying to unmuzzle yourself is the tenth sign of madness.\n");
		return;
		}
	if (!u->muzzled) {
		sprintf(text,"%s is not muzzled.\n",u->name);  return;
		}
	if (u->muzzled>user->level) {
		sprintf(text,"%s's muzzle is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->muzzled]);
		write_user(user,text);  return;
		}
	sprintf(text,"~FG~OLYou remove %s's muzzle.\n",u->name);
	write_user(user,text);
	write_user(u,"~FG~OLYou have been unmuzzled!\n");
	sprintf(text,"%s unmuzzled %s.\n",user->name,u->name);
	write_syslog(text,1);
	u->muzzled=0;
	return;
	}
/* User not logged on */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in unmuzzle().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->muzzled>user->level) {
	sprintf(text,"%s's muzzle is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->muzzled]);
	write_user(user,text);  
	destruct_user(u);
	destructed=0;
	return;
	}
u->socket=-2;
u->muzzled=0;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"~FG~OLYou remove %s's muzzle.\n",u->name);
write_user(user,text);
send_mail(user,word[1],"~FG~OLYou have been unmuzzled.\n",0);
sprintf(text,"%s unmuzzled %s.\n",user->name,u->name);
write_syslog(text,1);
destruct_user(u);
destructed=0;
}



/*** Switch system logging on and off ***/
logging(user)
UR_OBJECT user;
{
if (system_logging) {
	write_user(user,"System logging ~FROFF.\n");
	sprintf(text,"%s switched system logging OFF.\n",user->name);
	write_syslog(text,1);
	system_logging=0;
	return;
	}
system_logging=1;
write_user(user,"System logging ~FGON.\n");
sprintf(text,"%s switched system logging ON.\n",user->name);
write_syslog(text,1);
}


/*** Set minlogin level ***/
minlogin(user)
UR_OBJECT user;
{
UR_OBJECT u,next;
char *usage="Usage: minlogin NONE/<user level>\n";
char levstr[5],*name;
int lev,cnt;

if (word_count<2) {
	write_user(user,usage);  return;
	}
strtoupper(word[1]);
if ((lev=get_level(word[1]))==-1) {
	if (strcmp(word[1],"NONE")) {
		write_user(user,usage);  return;
		}
	lev=-1;
	strcpy(levstr,"NONE");
	}
else strcpy(levstr,level_name[lev]);
if (lev>user->level) {
	write_user(user,"You cannot set minlogin to a higher level than your own.\n");
	return;
	}
if (minlogin_level==lev) {
	write_user(user,"It is already set to that.\n");  return;
	}
minlogin_level=lev;
sprintf(text,"Minlogin level set to: ~OL%s.\n",levstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has set the minlogin level to: ~OL%s.\n",name,levstr);
write_room_except(NULL,text,user);
sprintf(text,"%s set the minlogin level to %s.\n",user->name,levstr);
write_syslog(text,1);

/* Now boot off anyone below that level */
cnt=0;
u=user_first;
while(u) {
	next=u->next;
	if (!u->login && u->type!=CLONE_TYPE && u->level<lev) {
		write_user(u,"\n~FY~OLYour level is now below the minlogin level, disconnecting you...\n");
		disconnect_user(u);
		++cnt;
		}
	u=next;
	}
sprintf(text,"Total of %d users were disconnected.\n",cnt);
destructed=0;
write_user(user,text);
}


/*** Show talker system parameters etc ***/
system_details(user)
UR_OBJECT user;
{
NL_OBJECT nl;
RM_OBJECT rm;
UR_OBJECT u;
char bstr[40],minlogin[5];
char *ca[]={ "NONE  ","IGNORE","REBOOT" };
int days,hours,mins,secs;
int netlinks,live,inc,outg;
int rms,inlinks,num_clones,mem,size;

sprintf(text,"\n~OL~FB*** RNUTS version %s - System status ***\n",RVERSION);
write_user(user,text);
        
/* Get some values */
strcpy(bstr,ctime(&boot_time));
secs=(int)(time(0)-boot_time);
days=secs/86400;
hours=(secs%86400)/3600;
mins=(secs%3600)/60;
secs=secs%60;
num_clones=0;
mem=0;
size=sizeof(struct user_struct);
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE) num_clones++;
        mem+=size;
        }

rms=0;
inlinks=0;
size=sizeof(struct room_struct);
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if (rm->inlink) ++inlinks;
        ++rms;  mem+=size;
        }

netlinks=0;  
live=0;
inc=0;
outg=0;
size=sizeof(struct netlink_struct);  
for(nl=nl_first;nl!=NULL;nl=nl->next) {
        if (nl->type!=UNCONNECTED && nl->stage==UP) live++;
        if (nl->type==INCOMING) ++inc;
        if (nl->type==OUTGOING) ++outg;
        ++netlinks;  mem+=size;
        } 
if (minlogin_level==-1) strcpy(minlogin,"NONE");
else strcpy(minlogin,level_name[minlogin_level]);
        
/* Show header parameters */
sprintf(text,"~FTProcess ID    : ~FG%d\n",getpid());
write_user(user,text);
sprintf(text,"~FTTalker booted : ~FG%s",bstr);
write_user(user,text);
sprintf(text,"~FTUptime        : ~FG%d days, %d hours, %d minutes, %d seconds\n",days,hours,mins,secs);
write_user(user,text);
sprintf(text,"~FTPorts (M/W/L) : ~FG%d,  %d, %d\n\n",port[0],port[1],port[2]);
write_user(user,text);

/* Show others */
sprintf(text,"Max users              : %-12d ",max_users);
write_user(user,text);
sprintf(text,"Current num. of users  : %d\n",num_of_users);
write_user(user,text);

sprintf(text,"Max clones             : %-12d ",max_clones);
write_user(user,text);
sprintf(text,"Current num. of clones : %d\n",num_clones);
write_user(user,text);

sprintf(text,"Current minlogin level : %-12s ",minlogin);
write_user(user,text);
sprintf(text,"Login idle time out    : %d secs.\n",login_idle_time);
write_user(user,text);

sprintf(text,"User idle time out     : %-4d secs.   ",user_idle_time);
write_user(user,text);
sprintf(text,"Heartbeat              : %d\n",heartbeat);
write_user(user,text);

sprintf(text,"Remote user maxlevel   : %-12s ",level_name[rem_user_maxlevel]);
write_user(user,text);
sprintf(text,"Remote user deflevel   : %s\n",level_name[rem_user_deflevel]);
write_user(user,text);

sprintf(text,"Wizport min login level: %-12s ",level_name[wizport_level]);
write_user(user,text);
sprintf(text,"Gatecrash level        : %s\n",level_name[gatecrash_level]);
write_user(user,text);

sprintf(text,"Time out maxlevel      : %-12s ",level_name[time_out_maxlevel]);
write_user(user,text);
sprintf(text,"Private room min count : %d\n",min_private_users);
write_user(user,text);

sprintf(text,"Message lifetime       : %-3d days     ",mesg_life);
write_user(user,text);
sprintf(text,"Message check time     : %02d:%02d\n",mesg_check_hour,mesg_check_min);
write_user(user,text);

sprintf(text,"Net idle time out      : %-4d secs.   ",net_idle_time);
write_user(user,text);
sprintf(text,"Number of rooms        : %d\n",rms);
write_user(user,text);

sprintf(text,"Num. accepting connects: %-12d ",inlinks);
write_user(user,text);
sprintf(text,"Total netlinks         : %d\n",netlinks);
write_user(user,text);

sprintf(text,"Number which are live  : %-12d ",live);
write_user(user,text);
sprintf(text,"Number incoming        : %d\n",inc);
write_user(user,text);

sprintf(text,"Number outgoing        : %-12d ",outg);
write_user(user,text);
sprintf(text,"Ignoring sigterm       : %s\n",noyes2[ignore_sigterm]);
write_user(user,text);

sprintf(text,"Echoing passwords      : %-12s ",noyes2[password_echo]);
write_user(user,text);
sprintf(text,"Swearing banned        : %s\n",noyes2[ban_swearing]);
write_user(user,text);

sprintf(text,"Time out afks          : %-12s ",noyes2[time_out_afks]);
write_user(user,text);
sprintf(text,"Allowing caps in name  : %s\n",noyes2[allow_caps_in_name]);
write_user(user,text);

sprintf(text,"New user prompt default: %-12s ",offon[prompt_def]);
write_user(user,text);
sprintf(text,"New user colour default: %s\n",offon[colour_def]);
write_user(user,text);

sprintf(text,"New user charecho def. : %-12s ",offon[charecho_def]);
write_user(user,text);
sprintf(text,"System logging         : %s\n",offon[system_logging]);
write_user(user,text);

sprintf(text,"Crash action           : %-12s ",ca[crash_action]);
write_user(user,text);
sprintf(text,"Object memory allocated: %d\n\n",mem);
write_user(user,text);
}



/*** Set the character mode echo on or off. This is only for users logging in
     via a character mode client, those using a line mode client (eg unix
     telnet) will see no effect. ***/
toggle_charecho(user)
UR_OBJECT user;
{
if (!user->charmode_echo) {
	write_user(user,"Echoing for character mode clients ~FGON.\n");
	user->charmode_echo=1;
	}
else {
	write_user(user,"Echoing for character mode clients ~FROFF.\n");
	user->charmode_echo=0;
	}
if (user->room==NULL) prompt(user);
}


/*** Free a hung socket ***/
clearline(user)
UR_OBJECT user;
{
UR_OBJECT u;
int sock;

if (word_count<2 || !isnumber(word[1])) {
	write_user(user,"Usage: clearline <line>\n");  return;
	}
sock=atoi(word[1]);

/* Find line amongst users */
for(u=user_first;u!=NULL;u=u->next) 
	if (u->type!=CLONE_TYPE && u->socket==sock) goto FOUND;
write_user(user,"That line is not currently active.\n");
return;

FOUND:
if (!u->login) {
	write_user(user,"You cannot clear the line of a logged in user.\n");
	return;
	}
write_user(u,"\n\nThis line is being cleared.\n\n");
disconnect_user(u); 
sprintf(text,"%s cleared line %d.\n",user->name,sock);
write_syslog(text,1);
sprintf(text,"Line %d cleared.\n",sock);
write_user(user,text);
destructed=0;
no_prompt=0;
}


/*** Change whether a rooms access is fixed or not ***/
change_room_fix(user,fix)
UR_OBJECT user;
int fix;
{
RM_OBJECT rm;
char *name;

if (word_count<2) rm=user->room;
else {
	if ((rm=get_room(word[1]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	}
if (user->vis) name=user->name; else name=invisname;
if (fix) {	
	if (rm->access & FIXED) {
		if (rm==user->room) 
			write_user(user,"This room's access is already fixed.\n");
		else write_user(user,"That room's access is already fixed.\n");
		return;
		}
	sprintf(text,"Access for room %s is now ~FRFIXED.\n",rm->name);
	write_user(user,text);
	if (user->room==rm) {
		sprintf(text,"%s has ~FRFIXED~RS access for this room.\n",name);
		write_room_except(rm,text,user);
		}
	else {
		sprintf(text,"This room's access has been ~FRFIXED.\n");
		write_room(rm,text);
		}
	sprintf(text,"%s FIXED access to room %s.\n",user->name,rm->name);
	write_syslog(text,1);
	rm->access+=2;
	return;
	}
if (!(rm->access & FIXED)) {
	if (rm==user->room) 
		write_user(user,"This room's access is already unfixed.\n");
	else write_user(user,"That room's access is already unfixed.\n");
	return;
	}
sprintf(text,"Access for room %s is now ~FGUNFIXED.\n",rm->name);
write_user(user,text);
if (user->room==rm) {
	sprintf(text,"%s has ~FGUNFIXED~RS access for this room.\n",name);
	write_room_except(rm,text,user);
	}
else {
	sprintf(text,"This room's access has been ~FGUNFIXED.\n");
	write_room(rm,text);
	}
sprintf(text,"%s UNFIXED access to room %s.\n",user->name,rm->name);
write_syslog(text,1);
rm->access-=2;
reset_access(rm);
}

/* Show the filter log */
view_log(user,inpstr)
UR_OBJECT user;
char *inpstr;
{

FILE *fp;
char c,emp[40],del[40],usage[80],title[40],filename[80],log[15];
int lines,cnt,cnt2,ok=0;

if (word_count<2) {
        write_user(user,"\nUsage: view <filename> [<#lines");
	if (user->level==EIG) write_user(user,"/-c>]\n");
	else write_user(user,">]\n");
	write_user(user,"Filenames: system,filter,account,boot,promo,delold\n\n");
	return;
	}
if (!strcmp(word[1],"filter")) {
	strcpy(emp,"The filter log is empty\n");
	strcpy(del,"\nFilter log cleared\n");
	strcpy(log,"Filter");
	strcpy(filename,FILTERLOG);
	strcpy(title,"\n~OL~FR***~RS~OL Filter Log ~FR***\n");
	ok=1;
	}
else if (!strcmp(word[1],"account")) {
	strcpy(emp,"The account log is empty\n");
	strcpy(del,"\nAccount log cleared\n");
	strcpy(log,"Account");
	strcpy(filename,ACCOUNTLOG);
	strcpy(title,"\n~OL~FR***~RS~OL Account Log ~FR***\n\n");
	ok=1;
	}
else if (!strcmp(word[1],"system")) {
	strcpy(emp,"The system log is empty\n");
	strcpy(del,"\nSystem log cleared\n");
	strcpy(log,"system");
	strcpy(filename,SYSLOG);
	strcpy(title,"\n~OL~FR***~RS~OL System Log ~FR***\n\n");
	ok=1;
	}
else if (!strcmp(word[1],"delold")) {
	strcpy(emp,"The delold log is empty\n");
	strcpy(del,"\nDelold log cleared\n");
	strcpy(log,"delold");
	strcpy(filename,"logfiles/users_removed");
	strcpy(title,"\n~OL~FR***~RS~OL Delold Log ~FR***\n\n");
	ok=1;
	}
else if (!strcmp(word[1],"boot")) {
	strcpy(emp,"The boot log is empty\n");
	strcpy(del,"\nBoot log cleared\n");
	strcpy(log,"boot");
	strcpy(filename,BOOTLOG);
	strcpy(title,"\n~OL~FR***~RS~OL Boot Log ~FR***\n\n");
	ok=1;
	}
else if (!strcmp(word[1],"promo")) {
	strcpy(emp,"The Promotion log is empty\n");
	strcpy(del,"\nPromotion log cleared\n");
	strcpy(log,"Promotion");
	strcpy(filename,PROMOLOG);
	strcpy(title,"\n~OL~FR***~RS~OL Promotion Log ~FR***\n\n");
	ok=1;
	}

if ((word_count==2)&&(ok==1)) {
        write_user(user,title);
        switch(more(user,user->socket,filename)) {
                case 0: write_user(user,emp);  return;
                case 1: user->misc_op=2;
                }
        return;
        }
/* Count total lines */
if (word_count<3) {
        write_user(user,"\nUsage: view <filename> [<#lines");
	if (user->level==EIG) write_user(user,"/-c>]\n");
	else write_user(user,">]\n");
	write_user(user,"Filenames: system,filter,account,boot,promo,delold\n\n");
	return;
	}
if (!(fp=fopen(filename,"r"))) {  write_user(user,emp);  return;  }
if ((!strcmp(word[2],"-c"))&&(user->level==EIG)) {
	if (!strcmp(word[1],"account")) {  
		write_user(user,"\nERROR: you may not clear that log.\n");
		return;
		}
        fclose(fp);
        unlink(filename);
        write_user(user,del);
        return;
        }
if (!isnumber(word[2])) {
        fclose(fp);
        write_user(user,"\nUsage: view <filename> [<#lines");
	if (user->level==EIG) write_user(user,"/-c>]\n");
	else write_user(user,">]\n");
	write_user(user,"Filenames: system,filter,account,boot,promo,delold\n\n");
        return;
        }
cnt=0;
lines=atoi(word[2]);
  
c=getc(fp);
while(!feof(fp)) {
        if (c=='\n') ++cnt;
        c=getc(fp);
        } 
if (cnt<lines) {
        sprintf(text,"There are only %d lines in the log.\n",cnt);
        write_user(user,text);
        fclose(fp);
        return;
        }
if (cnt==lines) {
        write_user(user,title);
        fclose(fp);  more(user,user->socket,filename);  return;
        }
 
/* Find line to start on */
fseek(fp,0,0);
cnt2=0;
c=getc(fp);
while(!feof(fp)) {  
        if (c=='\n') ++cnt2;
        c=getc(fp);
        if (cnt2==cnt-lines) {
                sprintf(text,"\n~BB*** %s Log (last %d lines) ***\n\n",log,lines);
                write_user(user,text);
                user->filepos=ftell(fp)-1;
                fclose(fp);
                if (more(user,user->socket,filename)!=1) user->filepos=0;
                else user->misc_op=2;
                return;
                }  
        }
fclose(fp);
sprintf(text,"%s: Line Count Error.\n",syserror);
write_user(user,text);
sprintf(text,"ERROR: Line count Error in %s().\n",log);
write_log(text,0,0);
}



/*** A newbie is requesting an account. Get his email address off him so we
     can validate who he is before we promote him and let him loose as a 
     proper user. ***/
account_request(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (user->level>ZER) {
	write_user(user,"This command is for new users only, you already have a full account.\n");
	return;
	}
/* This is so some pillock doesnt keep doing it just to fill up the syslog */
if (user->accreq) {
	write_user(user,"You have already requested an account.\n");
	return;
	}
if (!strcmp(user->desc,"hasn't used .desc yet")) {
	write_user(user,"Set your descrition first.\n");
	return;
	}
if (!strcmp(user->gend,"\0")) {
	write_user(user,"Set your gender first.\n");
	return;
	}
if (word_count<2) {
	write_user(user,"Usage: accreq <an email address we can contact you on + any relevent info>\n");
	return;
	}
/* Could check validity of email address I guess but its a waste of time.
   If they give a duff address they don't get an account, simple. ***/
sprintf(text,"ACCOUNT REQUEST from %s: %s.\n",user->name,inpstr);
write_log(text,1,1);
sprintf(text,"~OLSYSTEM:~RS %s has made a request for an account.\n",user->name);
write_level(FOU,1,text,NULL);
write_user(user,"Account request logged.\n");
user->accreq=1;
user->level++;
sprintf(text,"~OL%s has been AUTO-Promoted to level: ~FW%s\n",user->name,level_name[user->level]);
write_room(user->room,text);
sprintf(text,"~OL~FR%s AUTO-Promoted to level: ~FW%s\n",user->name,level_name[user->level]);
write_log(text,1,4);
}


/*** Clear the review buffer ***/
revclr(user)
UR_OBJECT user;
{
char *name;

clear_revbuff(user->room); 
write_user(user,"Review buffer cleared.\n");
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has cleared the review buffer.\n",name);
write_room_except(user->room,text,user);
}


/*** Clone a user in another room ***/
create_clone(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
int cnt;

/* Check room */
if (word_count<2) rm=user->room;
else {
	if ((rm=get_room(word[1]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	}	
/* If room is private then nocando */
if (!has_room_access(user,rm)) {
	write_user(user,"That room is currently private, you cannot create a clone there.\n");  
	return;
	}
/* Count clones and see if user already has a copy there , no point having 
   2 in the same room */
cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->owner==user) {
		if (u->room==rm) {
			sprintf(text,"You already have a clone in the %s.\n",rm->name);
			write_user(user,text);
			return;
			}	
		if (++cnt==max_clones) {
			write_user(user,"You already have the maximum number of clones allowed.\n");
			return;
			}
		}
	}
/* Create clone */
if ((u=create_user())==NULL) {		
	sprintf(text,"%s: Unable to create copy.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create user copy in clone().\n",0);
	return;
	}
u->type=CLONE_TYPE;
u->socket=user->socket;
u->room=rm;
u->owner=user;
strcpy(u->name,user->name);
strcpy(u->desc,"~BR(CLONE)");

if (rm==user->room)
	write_user(user,"~FB~OLYou whisper a haunting spell and a clone is created here.\n");
else {
	sprintf(text,"~FB~OLYou whisper a haunting spell and a clone is created in the %s.\n",rm->name);
	write_user(user,text);
	}
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~FB~OL%s whispers a haunting spell...\n",name);
write_room_except(user->room,text,user);
sprintf(text,"~FB~OLA clone of %s appears in a swirling magical mist!\n",user->name);
write_room_except(rm,text,user);
}


/*** Destroy user clone ***/
destroy_clone(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
RM_OBJECT rm;
char *name;

/* Check room and user */
if (word_count<2) rm=user->room;
else {
	if ((rm=get_room(word[1]))==NULL) {
		write_user(user,nosuchroom);  return;
		}
	}
if (word_count>2) {
	if ((u2=get_user(word[2]))==NULL) {
		write_user(user,notloggedon);  return;
		}
	if (u2->level>=user->level) {
		write_user(user,"You cannot destroy the clone of a user of an equal or higher level.\n");
		return;
		}
	}
else u2=user;
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->room==rm && u->owner==u2) {
		destruct_user(u);
		reset_access(rm);
		write_user(user,"~FM~OLYou whisper a sharp spell and the clone is destroyed.\n");
		if (user->vis) name=user->name; else name=invisname;
		sprintf(text,"~FM~OL%s whispers a sharp spell...\n",name);
		write_room_except(user->room,text,user);
		sprintf(text,"~FM~OLThe clone of %s shimmers and vanishes.\n",u2->name);
		write_room(rm,text);
		if (u2!=user) {
			sprintf(text,"~OLSYSTEM: ~FR%s has destroyed your clone in the %s.\n",user->name,rm->name);
			write_user(u2,text);
			}
		destructed=0;
		return;
		}
	}
if (u2==user) sprintf(text,"You do not have a clone in the %s.\n",rm->name);
else sprintf(text,"%s does not have a clone the %s.\n",u2->name,rm->name);
write_user(user,text);
}


/*** Show users own clones ***/
myclones(user)
UR_OBJECT user;
{
UR_OBJECT u;
int cnt;

cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type!=CLONE_TYPE || u->owner!=user) continue;
	if (++cnt==1) 
		write_user(user,"\n~BB*** Rooms you have clones in ***\n\n");
	sprintf(text,"  %s\n",u->room);
	write_user(user,text);
	}
if (!cnt) write_user(user,"You have no clones.\n");
else {
	sprintf(text,"\nTotal of %d clones.\n\n",cnt);
	write_user(user,text);
	}
}


/*** Show all clones on the system ***/
allclones(user)
UR_OBJECT user;
{
UR_OBJECT u;
int cnt;

cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type!=CLONE_TYPE) continue;
	if (++cnt==1) {
		sprintf(text,"\n~BB*** Current clones %s ***\n\n",long_date(1));
		write_user(user,text);
		}
	sprintf(text,"%-15s : %s\n",u->name,u->room);
	write_user(user,text);
	}
if (!cnt) write_user(user,"There are no clones on the system.\n");
else {
	sprintf(text,"\nTotal of %d clones.\n\n",cnt);
	write_user(user,text);
	}
}


/*** User swaps places with his own clone. All we do is swap the rooms the
	objects are in. ***/
clone_switch(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;

if (word_count<2) {
	write_user(user,"Usage: switch <room clone is in>\n");  return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) {
		write_user(user,"\n~FB~OLYou experience a strange sensation...\n");
		u->room=user->room;
		user->room=rm;
		sprintf(text,"The clone of %s comes alive!\n",u->name);
		write_room_except(user->room,text,user);
		sprintf(text,"%s turns into a clone!\n",u->name);
		write_room_except(u->room,text,u);
		look(user);
		return;
		}
	}
write_user(user,"You do not have a clone in that room.\n");
}


/*** Make a clone speak ***/
clone_say(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
RM_OBJECT rm;
UR_OBJECT u;

if (user->muzzled) {
	write_user(user,"You are muzzled, your clone cannot speak.\n");
	return;
	}
if ((word_count<3)||(!strcmp(word[2], ""))) {
	write_user(user,"Usage: csay <room clone is in> <message>\n");
	return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) {
		if (nuts_talk_style==1) say_new(u,remove_first(inpstr));
		else say_old(u,remove_first(inpstr));
		return;
		}
	}
write_user(user,"You do not have a clone in that room.\n");
}


/*** Set what a clone will hear, either all speach , just bad language
	or nothing. ***/
clone_hear(user)
UR_OBJECT user;
{
RM_OBJECT rm;
UR_OBJECT u;

if (word_count<3  
    || (strcmp(word[2],"all") 
	    && strcmp(word[2],"swears") 
	    && strcmp(word[2],"nothing"))) {
	write_user(user,"Usage: chear <room clone is in> all/swears/nothing\n");
	return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
for(u=user_first;u!=NULL;u=u->next) {
	if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) break;
	}
if (u==NULL) {
	write_user(user,"You do not have a clone in that room.\n");
	return;
	}
if (!strcmp(word[2],"all")) {
	u->clone_hear=CLONE_HEAR_ALL;
	write_user(user,"Clone will now hear everything.\n");
	return;
	}
if (!strcmp(word[2],"swears")) {
	u->clone_hear=CLONE_HEAR_SWEARS;
	write_user(user,"Clone will now only hear swearing.\n");
	return;
	}
u->clone_hear=CLONE_HEAR_NOTHING;
write_user(user,"Clone will now hear nothing.\n");
}


/*** Stat a remote system ***/
remote_stat(user)
UR_OBJECT user;
{
NL_OBJECT nl;
RM_OBJECT rm;

if (word_count<2) {
	write_user(user,"Usage: rstat <room service is linked to>\n");  return;
	}
if ((rm=get_room(word[1]))==NULL) {
	write_user(user,nosuchroom);  return;
	}
if ((nl=rm->netlink)==NULL) {
	write_user(user,"That room is not linked to a service.\n");
	return;
	}
if (nl->stage!=2) {
	write_user(user,"Not (fully) connected to service.\n");
	return;
	}
if (nl->ver_major<=3 && nl->ver_minor<1) {
	write_user(user,"The RNUTS version running that service does not support this facility.\n");
	return;
	}
sprintf(text,"RSTAT %s\n",user->name);
write_sock(nl->socket,text);
write_user(user,"Request sent.\n");
}


/*** Switch swearing ban on and off ***/
swban(user)
UR_OBJECT user;
{
if (!ban_swearing) {
	write_user(user,"Swearing ban ~FGON.\n");
	sprintf(text,"%s switched swearing ban ON.\n",user->name);
	write_syslog(text,1);
	ban_swearing=1;  return;
	}
write_user(user,"Swearing ban ~FROFF.\n");
sprintf(text,"%s switched swearing ban OFF.\n",user->name);
write_syslog(text,1);
ban_swearing=0;
}


/*** Do AFK ***/
afk(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (word_count>1) {
	if (!strcmp(word[1],"lock")) {
		if (user->type==REMOTE_TYPE) {
			/* This is because they might not have a local account and hence
			   they have no password to use. */
			write_user(user,"Sorry, due to software limitations remote users cannot use the lock option.\n");
			return;
			}
		inpstr=remove_first(inpstr);
		if (strlen(inpstr)>AFK_MESG_LEN) {
			write_user(user,"AFK message too long.\n");  return;
			}
		write_user(user,"You are now AFK with the session locked, enter your password to unlock it.\n");
		if (inpstr[0]) {
			strcpy(user->afk_mesg,inpstr);
			write_user(user,"AFK message set.\n");
			}
		user->afk=2;
		}
	else {
		if (strlen(inpstr)>AFK_MESG_LEN) {
			write_user(user,"AFK message too long.\n");  return;
			}
		write_user(user,"You are now AFK, press <return> to reset.\n");
		if (inpstr[0]) {
			strcpy(user->afk_mesg,inpstr);
			write_user(user,"AFK message set.\n");
			}
		user->afk=1;
		}
	}
else {
	write_user(user,"You are now AFK, press <return> to reset.\n");
	user->afk=1;
	}
if (user->vis) {
	if (user->afk_mesg[0]) 
		sprintf(text,"%s goes AFK: %s\n",user->name,user->afk_mesg);
	else sprintf(text,"%s goes AFK...\n",user->name);
	write_room_except(user->room,text,user);
	}
}


/*** Toggle user colour on and off ***/
toggle_colour(user)
UR_OBJECT user;
{
int col;

/* A hidden "feature" , notalot of practical use but lets see if any users
   stumble across it :) */
if (user->command_mode && user->ignall && user->charmode_echo) {
	for(col=1;col<NUM_COLS;++col) {
		sprintf(text,"%s: ~%sRNUTS 3 VIDEO TEST~RS\n",colcom[col],colcom[col]);
		write_user(user,text);
		}
	return;
	}
if (user->colour) {
	write_user(user,"Colour ~FROFF.\n");  
	user->colour=0;
	}
else {
	user->colour=1;  
	write_user(user,"Colour ~FGON.\n");
	}
if (user->room==NULL) prompt(user);
}


toggle_ignshout(user)
UR_OBJECT user;
{
if (user->ignshout) {
	write_user(user,"You are no longer ignoring shouts and shout emotes.\n");  
	user->ignshout=0;
	return;
	}
write_user(user,"You are now ignoring shouts and shout emotes.\n");
user->ignshout=1;
}


toggle_igntell(user)
UR_OBJECT user;
{
if (user->igntell) {
	write_user(user,"You are no longer ignoring tells and private emotes.\n");  
	user->igntell=0;
	return;
	}
write_user(user,"You are now ignoring tells and private emotes.\n");
user->igntell=1;
}


suicide(user)
UR_OBJECT user;
{
if (word_count<2) {
	write_user(user,"Usage: suicide <your password>\n");  return;
	}
if (strcmp((char *)crypt(word[1],"NU"),user->pass)) {
	write_user(user,"Password incorrect.\n");  return;
	}
write_user(user,"\n\07~FR~OL~LI*** WARNING - This will delete your account! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=6;  
no_prompt=1;
}


/*** Delete a user ***/
delete_user(user,this_user)
UR_OBJECT user;
int this_user;
{
UR_OBJECT u;
char filename[80],name[USER_NAME_LEN+1];

if (this_user) {
	/* User structure gets destructed in disconnect_user(), need to keep a
	   copy of the name */
	strcpy(name,user->name); 
	write_user(user,"\n~FR~LI~OLACCOUNT DELETED!\n");
	sprintf(text,"~OL~LI%s commits suicide!\n",user->name);
	write_room_except(user->room,text,user);
	sprintf(text,"%s SUICIDED.\n",name);
	write_syslog(text,1);
	disconnect_user(user);
	sprintf(filename,"%s/%s.D",USERFILES,name);
	unlink(filename);
	sprintf(filename,"%s/%s.M",USERFILES,name);
	unlink(filename);
	sprintf(filename,"%s/%s.P",USERFILES,name);
	unlink(filename);
	sprintf(filename,"%s/%s.A",USERFILES,name);
	unlink(filename);
	sprintf(filename,"%s/%s.R",USERFILES,name);
	unlink(filename);
	return;
	}
if (word_count<2) {
	write_user(user,"Usage: nuke <user>\n");  return;
	}
word[1][0]=toupper(word[1][0]);
if (!strcmp(word[1],user->name)) {
	write_user(user,"Trying to nuking yourself is the eleventh sign of madness.\n");
	return;
	}
if (get_user(word[1])!=NULL) {
	/* Safety measure just in case. Will have to .kill them first */
        write_user(user,"You cannot delete a user who is currently logged on.\n");
        return;
	}
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in delete_user().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot delete a user of an equal or higher level than yourself.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
destruct_user(u);
destructed=0;
sprintf(filename,"%s/%s.D",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.M",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.P",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.A",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.R",USERFILES,word[1]);
unlink(filename);
sprintf(text,"\07~FR~OL~LIUser %s deleted!\n",word[1]);
write_user(user,text);
sprintf(text,"%s DELETED %s.\n",user->name,word[1]);
write_syslog(text,1);
}


/*** Shutdown talker interface func. Countdown time is entered in seconds so
	we can specify less than a minute till reboot. ***/
shutdown_com(user)
UR_OBJECT user;
{
if (rs_which==1) {
	write_user(user,"The reboot countdown is currently active, you must cancel it first.\n");
	return;
	}
if (!strcmp(word[1],"cancel")) {
	if (!rs_countdown || rs_which!=0) {
		write_user(user,"The shutdown countdown is not currently active.\n");
		return;
		}
	if (rs_countdown && !rs_which && rs_user==NULL) {
		write_user(user,"Someone else is currently setting the shutdown countdown.\n");
		return;
		}
	write_room(NULL,"~OLSYSTEM:~RS~FG Shutdown cancelled.\n");
	sprintf(text,"%s cancelled the shutdown countdown.\n",user->name);
	write_syslog(text,1);
	rs_countdown=0;
	rs_announce=0;
	rs_which=-1;
	rs_user=NULL;
	return;
	}
if (word_count>1 && !isnumber(word[1])) {
	write_user(user,"Usage: shutdown [<secs>/cancel]\n");  return;
	}
if (rs_countdown && !rs_which) {
	write_user(user,"The shutdown countdown is currently active, you must cancel it first.\n");
	return;
	}
if (word_count<2) {
	rs_countdown=0;  
	rs_announce=0;
	rs_which=-1; 
	rs_user=NULL;
	}
else {
	rs_countdown=atoi(word[1]);
	rs_which=0;
	}
write_user(user,"\n\07~FR~OL~LI*** WARNING - This will shutdown the talker! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=1;  
no_prompt=1;  
}


/*** Reboot talker interface func. ***/
reboot_com(user)
UR_OBJECT user;
{
if (!rs_which) {
	write_user(user,"The shutdown countdown is currently active, you must cancel it first.\n");
	return;
	}
if (!strcmp(word[1],"cancel")) {
	if (!rs_countdown) {
		write_user(user,"The reboot countdown is not currently active.\n");
		return;
		}
	if (rs_countdown && rs_user==NULL) {
		write_user(user,"Someone else is currently setting the reboot countdown.\n");
		return;
		}
	write_room(NULL,"~OLSYSTEM:~RS~FG Reboot cancelled.\n");
	sprintf(text,"%s cancelled the reboot countdown.\n",user->name);
	write_syslog(text,1);
	rs_countdown=0;
	rs_announce=0;
	rs_which=-1;
	rs_user=NULL;
	return;
	}
if (word_count>1 && !isnumber(word[1])) {
	write_user(user,"Usage: reboot [<secs>/cancel]\n");  return;
	}
if (rs_countdown) {
	write_user(user,"The reboot countdown is currently active, you must cancel it first.\n");
	return;
	}
if (word_count<2) {
	rs_countdown=0;  
	rs_announce=0;
	rs_which=-1; 
	rs_user=NULL;
	}
else {
	rs_countdown=atoi(word[1]);
	rs_which=1;
	}
write_user(user,"\n\07~FY~OL~LI*** WARNING - This will reboot the talker! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=7;  
no_prompt=1;  
}



/*** Show recorded tells and pemotes ***/
revtell(user)
UR_OBJECT user;
{
int i,cnt,line;

cnt=0;
for(i=0;i<REVTELL_LINES;++i) {
	line=(user->revline+i)%REVTELL_LINES;
	if (user->revbuff[line][0]) {
		cnt++;
		if (cnt==1) write_user(user,"\n~BB~FG*** Your revtell buffer ***\n\n");
		write_user(user,user->revbuff[line]); 
		}
	}
if (!cnt) write_user(user,"Revtell buffer is empty.\n");
else write_user(user,"\n~BB~FG*** End ***\n\n");
}



/**************************** EVENT FUNCTIONS ******************************/

void do_events()
{
set_date_time();
check_reboot_shutdown();
check_idle_and_timeout();
check_nethangs_send_keepalives(); 
check_messages(NULL,0);
reset_alarm();
}


reset_alarm()
{
signal(SIGALRM,do_events);
alarm(heartbeat);
}



/*** See if timed reboot or shutdown is underway ***/
check_reboot_shutdown()
{
int secs;
char *w[]={ "~FRShutdown","~FYRebooting" };

if (rs_user==NULL) return;
rs_countdown-=heartbeat;
if (rs_countdown<=0) talker_shutdown(rs_user,NULL,rs_which);

/* Print countdown message every minute unless we have less than 1 minute
   to go when we print every 10 secs */
secs=(int)(time(0)-rs_announce);
if (rs_countdown>=60 && secs>=60) {
	sprintf(text,"~OLSYSTEM: %s in %d minutes, %d seconds.\n",w[rs_which],rs_countdown/60,rs_countdown%60);
	write_room(NULL,text);
	rs_announce=time(0);
	}
if (rs_countdown<60 && secs>=10) {
	sprintf(text,"~OLSYSTEM: %s in %d seconds.\n",w[rs_which],rs_countdown);
	write_room(NULL,text);
	rs_announce=time(0);
	}
}



/*** login_time_out is the length of time someone can idle at login, 
     user_idle_time is the length of time they can idle once logged in. 
     Also ups users total login time. ***/
check_idle_and_timeout()
{
UR_OBJECT user,next;
int tm;

/* Use while loop here instead of for loop for when user structure gets
   destructed, we may lose ->next link and crash the program */
user=user_first;
while(user) {
	next=user->next;
	if (user->type==CLONE_TYPE) {  user=next;  continue;  }
	user->total_login+=heartbeat; 
	if (user->level>time_out_maxlevel) {  user=next;  continue;  }

	tm=(int)(time(0) - user->last_input);
	if (user->login && tm>=login_idle_time) {
		write_user(user,"\n\n*** Time out ***\n\n");
		disconnect_user(user);
		user=next;
		continue;
		}
	if (user->warned) {
		if (tm<user_idle_time-60) {  user->warned=0;  continue;  }
		if (tm>=user_idle_time) {
			write_user(user,"\n\n\07~FR~OL~LI*** You have been timed out. ***\n\n");
			disconnect_user(user);
			user=next;
			continue;
			}
		}
	if ((!user->afk || (user->afk && time_out_afks)) 
	    && !user->login 
	    && !user->warned
	    && tm>=user_idle_time-60) {
		write_user(user,"\n\07~FY~OL~LI*** WARNING - Input within 1 minute or you will be disconnected. ***\n\n");
		user->warned=1;
		}
	user=next;
	}
}
	


/*** See if any net connections are dragging their feet. If they have been idle
     longer than net_idle_time the drop them. Also send keepalive signals down
     links, this saves having another function and loop to do it. ***/
check_nethangs_send_keepalives()
{
NL_OBJECT nl;
int secs;

for(nl=nl_first;nl!=NULL;nl=nl->next) {
	if (nl->type==UNCONNECTED) {
		nl->warned=0;  continue;
		}

	/* Send keepalives */
	nl->keepalive_cnt+=heartbeat;
	if (nl->keepalive_cnt>=keepalive_interval) {
		write_sock(nl->socket,"KA\n");
		nl->keepalive_cnt=0;
		}

	/* Check time outs */
	secs=(int)(time(0) - nl->last_recvd);
	if (nl->warned) {
		if (secs<net_idle_time-60) nl->warned=0;
		else {
			if (secs<net_idle_time) continue;
			sprintf(text,"~OLSYSTEM:~RS Disconnecting hung netlink to %s in the %s.\n",nl->service,nl->connect_room->name);
			write_room(NULL,text);
			shutdown_netlink(nl);
			nl->warned=0;
			}
		continue;
		}
	if (secs>net_idle_time-60) {
		sprintf(text,"~OLSYSTEM:~RS Netlink to %s in the %s has been hung for %d seconds.\n",nl->service,nl->connect_room->name,secs);
		write_level(SIX,1,text,NULL);
		nl->warned=1;
		}
	}
destructed=0;
}



/*** Remove any expired messages from boards unless force = 2 in which case
	just do a recount. ***/
check_messages(user,force)
UR_OBJECT user;
int force;
{
RM_OBJECT rm;
FILE *infp,*outfp;
char id[82],filename[80],line[82];
int valid,pt,write_rest;
int board_cnt,old_cnt,bad_cnt,tmp;
static int done=0;

switch(force) {
	case 0:
	if (mesg_check_hour==thour && mesg_check_min==tmin) {
		if (done) return;
		}
	else {  done=0;  return;  }
	break;

	case 1:
	printf("-- BOOT : Checking boards...\n");
	}
done=1;
board_cnt=0;
old_cnt=0;
bad_cnt=0;

for(rm=room_first;rm!=NULL;rm=rm->next) {
	tmp=rm->mesg_cnt;  
	rm->mesg_cnt=0;
	sprintf(filename,"%s/%s.B",BOARDFILES,rm->name);
	if (!(infp=fopen(filename,"r"))) continue;
	if (force<2) {
		if (!(outfp=fopen("tempfile","w"))) {
			if (force) fprintf(stderr,"RNUTS: Couldn't open tempfile.\n");
			write_syslog("ERROR: Couldn't open tempfile in check_messages().\n",0);
			fclose(infp);
			return;
			}
		}
	board_cnt++;
	/* We assume that once 1 in date message is encountered all the others
	   will be in date too , hence write_rest once set to 1 is never set to
	   0 again */
	valid=1; write_rest=0;
	fgets(line,82,infp); /* max of 80+newline+terminator = 82 */
	while(!feof(infp)) {
		if (*line=='\n') valid=1;
		sscanf(line,"%s %d",id,&pt);
		if (!write_rest) {
			if (valid && !strcmp(id,"PT:")) {
				if (force==2) rm->mesg_cnt++;
				else {
					/* 86400 = num. of secs in a day */
					if ((int)time(0) - pt < mesg_life*86400) {
						fputs(line,outfp);
						rm->mesg_cnt++;
						write_rest=1;
						}
					else old_cnt++;
					}
				valid=0;
				}
			}
		else {
			fputs(line,outfp);
			if (valid && !strcmp(id,"PT:")) {
				rm->mesg_cnt++;  valid=0;
				}
			}
		fgets(line,82,infp);
		}
	fclose(infp);
	if (force<2) {
		fclose(outfp);
		unlink(filename);
		if (!write_rest) unlink("tempfile");
		else rename("tempfile",filename);
		}
	if (rm->mesg_cnt!=tmp) bad_cnt++;
	}
switch(force) {
	case 0:
	if (bad_cnt) 
		sprintf(text,"CHECK_MESSAGES: %d files checked, %d had an incorrect message count, %d messages deleted.\n",board_cnt,bad_cnt,old_cnt);
	else sprintf(text,"CHECK_MESSAGES: %d files checked, %d messages deleted.\n",board_cnt,old_cnt);
	write_syslog(text,1);
	break;

	case 1:
	printf("          %d board files checked, %d out of date messages found.\n",board_cnt,old_cnt);
	break;

	case 2:
	sprintf(text,"%d board files checked, %d had an incorrect message count.\n",board_cnt,bad_cnt);
	write_user(user,text);
	sprintf(text,"%s forced a recount of the message boards.\n",user->name);
	write_syslog(text,1);
	}
}
/**************************** Made in England *******************************/

/****************************************************************************/
/*			Commands Added By RobinHood			    */
/****************************************************************************/

set(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
RM_OBJECT rm;
int i=0;

	word_count = wordfind(inpstr);
	strtolower(inpstr);

	if (word_count<1) {
		write_user(user,"Set What?\n");
		return;
	}
	if (!strcmp (word[0],"desc")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"\n~OLDesc set to: ~RS%s\n\n",user->desc);
			write_user(user,text);
			return;
		}
		inpstr=remove_first(inpstr);
		set_desc(user,inpstr);
		return;
	}
	if (!strcmp(word[0],"autofwd")) {
		if (!user->fwdaddy[0] || !strcmp(user->fwdaddy,"#UNSET")) {
			write_user(user,"You have not yet set your email address - autofwd cannot be used until you do.\n");
            		return;
            	}
        	if (!user->mail_verified) {
           		write_user(user,"You have not yet verified your email - autofwd cannot be used until you do.\n");
            		return;
            	}
		if (!strcmp(word[1],"yes")) {
			if (user->autofwd==0) {
				user->autofwd=1;
                   		write_user(user,"You will also receive smails via email.\n");
				return;
			} else {
				write_user(user,"You are already recieving smail via email.\n");
				return;
			}
		}
		if (!strcmp(word[1],"no")) {
			if (user->autofwd==1) {
				user->autofwd=0;
                    		write_user(user,"You will no longer receive smails via email.\n");
				return;
			} else {
				write_user(user,"You are not recieving smails via email.\n");
				return;
			}
            	}
		write_user(user,"Usage: set autofwd <'no'/'yes'>\n\n");
        	return;
	}
	if (!strcmp(word[0],"fmail")) {
        	inpstr=remove_first(inpstr);
        	inpstr=colour_com_strip(inpstr);
        	if (!inpstr[0]) strcpy(user->fwdaddy,"#UNSET");
        	else {
                	for (i=0;i<strlen(inpstr);++i) {
                       		if (!isalpha(inpstr[i]) && inpstr[i]!='.' && inpstr[i]!='@') {
                               		write_user(user,"Not a valid email address.\n");
                               		return;
                               	}
                       	}
                	strcpy(user->fwdaddy,inpstr);
                }
        	if (!strcmp(user->email,"#UNSET")) sprintf(text,"Email set to :~FRunset\n");
        	else sprintf(text,"Forwading address set to: ~FT%s\n",user->fwdaddy);
        	write_user(user,text);
        	set_forward_email(user);  
        	return;
	}

	if (!strcmp(word[0],"autoread")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"\n~OLAutoread set to: ~RS%s\n\n",noyes2[user->autoread]);
			write_user(user,text);
			return;
		}
		if (!strcmp(word[1],"yes")) {
			if (user->autoread==0) {
				user->autoread=1;
				write_user(user,"You will now autoread mail on login.\n\n");
				return;
			}
			else {
				write_user(user,"You already have autoread enabled.\n\n");
				return;
			}
		}
		if (!strcmp(word[1],"no")) {
			if (user->autoread==1) {
				user->autoread=0;
				write_user(user,"You will no longer autoread mail on login.\n\n");
				return;
			}
			else {
				write_user(user,"You do not have autoread enabled.\n\n");
				return;
			}
		}
		write_user(user,"Usage: set autoread <on/off>\n\n");
		return;
	}
		
	if (!strcmp(word[0],"hide")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			write_user(user,"You are currently hidding: \n");
			sprintf(text,"~OL     Email: ~RS%s\n",noyes2[user->hide_email]);
			write_user(user,text);
			sprintf(text,"~OL     URL  : ~RS%s\n\n",noyes2[user->hide_url]);
			write_user(user,text);
			return;
		}
		if (!strcmp(word[1],"email")) {
			if (user->hide_email==0) { 
				user->hide_email=1;
 				write_user(user,"You have hidden your email.\n\n");
				return;
			}
			else { 
				user->hide_email=0;
				write_user(user,"Your email is now visible.\n\n");
				return;
			}
		}
		if (!strcmp(word[1],"url")) {
			if (user->hide_url==0) { 
				user->hide_url=1;
				write_user(user,"You have hidden your url.\n\n");
				return;
			}
			else {
				user->hide_url=0;
				write_user(user,"Your url is now visible.\n\n");
				return;
			}
		}
		write_user(user,"Usage: set hide <email/url>\n\n");
		return;
	}

	if (!strcmp(word[0],"login")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"Your current login room is the ~OL%s.\n",user->pref_rm);
			write_user(user,text);
			return;
		}
		if (!strcmp(word[1],JAILROOM))  {
			write_user(user,"You cannot log into that room.\n");
			return;
		}
	        for(rm=room_first;rm!=NULL;rm=rm->next) {
	                if (!strncmp(rm->name,word[1],strlen(word[1]))) {
				if (!has_room_access(user,rm)) {
					write_user(user,"You do not have access to that room.\n");
					return;
					}
				else {
					strcpy(user->pref_rm,word[1]);
					write_user(user,"Login Room Set.\n");
					return;
					}
			}
		}
		write_user(user,"That room does not exist.\n");
		return;
	}

	if (!strcmp(word[0],"gend")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"Gender Set to: %s\n",user->gend);
			write_user(user,text);
			return;
		}
		if (strlen(word[1]) > GEND_LEN) {
			write_user(user,"Gender too long.\n");
			return;
		}
		if (!strcmp(user->desc,"hasn't used .desc yet")) {
			write_user(user,"Set your description first.\n");
			return;
		}
		strcpy(user->gend,word[1]);
		write_user(user,"Gender Set.\n");
		return;
	}

        if (!strcmp(word[0],"age")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"Age Set to: %s\n",user->age);
			write_user(user,text);
			return;
		}
                if (strlen(word[1]) > AGE_LEN) {
                        write_user(user,"Age too long.\n");
                        return;
                }
                strcpy(user->age,word[1]);
                write_user(user,"Age Set.\n");
                return;
        }

        if (!strcmp(word[0],"email")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"Email Set to: %s\n",user->email);
			write_user(user,text);
			return;
		}
                if (strlen(word[1]) > EMAIL_LEN) {
                        write_user(user,"Email too long.\n");
                        return;
                }
		sprintf(text,"%s",word[1]);
                strcpy(user->email,text);
                write_user(user,"Email Set.\n");
                return;
        }

        if (!strcmp(word[0],"url")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"Homepage Set to: %s\n",user->url);
			write_user(user,text);
			return;
		}
                if (strlen(word[1]) > URL_LEN) {
                        write_user(user,"URL too long.\n");
                        return;
                }
		sprintf(text,"%s",word[1]);
                strcpy(user->url,text);
                write_user(user,"Homepage Set.\n");
                return;
        }

	if (!strcmp(word[0],"color")) {
		if ((word_count<2) || (!strcmp(word[1],""))) {
			sprintf(text,"%sThe color of this text is your speech color\n",user->color);
			write_user(user,text);
			return;
		}
		if ((!strcmp(word[1],"white"))||(!strcmp(word[1],"fw"))) {	
			strcpy(user->color,"~FW");
			write_user(user,"\n~FWThe color of this text is your speech color\n");
			return;
		}
		if ((!strcmp(word[1],"purple"))||(!strcmp(word[1],"fm"))) {
                        strcpy(user->color,"~FM");
	                write_user(user,"\n~FMThe color of this text is your speech color\n");
                        return;
		}
		if ((!strcmp(word[1],"red"))||(!strcmp(word[1],"fr"))) {
                        strcpy(user->color,"~FR");
                        write_user(user,"\n~FRThe color of this text is your speech color\n");
                        return;
		}
		if ((!strcmp(word[1],"yellow"))||(!strcmp(word[1],"fy"))) {
                        strcpy(user->color,"~FY");
	                write_user(user,"\n~FYThe color of this text is your speech color\n");
                        return;
		}
		if ((!strcmp(word[1],"green"))||(!strcmp(word[1],"fg"))) {
                        strcpy(user->color,"~FG");
	                write_user(user,"\n~FGThe color of this text is your speech color\n");
                        return;
		}
		if ((!strcmp(word[1],"blue"))||(!strcmp(word[1],"fb"))) {
                        strcpy(user->color,"~FB");
	                write_user(user,"\n~FBThe color of this text is your speech color\n");
                        return;
		}
		if ((!strcmp(word[1],"turquoise"))||(!strcmp(word[1],"ft"))) {
                        strcpy(user->color,"~FT");
	                write_user(user,"\n~FTThe color of this text is your speech color\n");
                        return;
		}
		if (!strcmp(word[1],"wiz")&&(user->level>=FOU)) {
			strcpy(user->color,word[2]);
			sprintf(text,"\n%sThe color of this text is your speech color.\n",word[2]);
			write_user(user,text);
			return;
		}

	write_user(user,"That is not a valid color setting.\n");
	return;
		
	}
	if ((!strcmp(word[0],"says"))&&(user->level>=EIG)) {
		if (nuts_talk_style==1) {
			nuts_talk_style=0;
			write_user(user,"Say Style is OLD\n\n");
			sprintf(text,"\n~OL~FRSYSTEM:~RS Changing to OLD say style.\n\n");
			write_room_except(user->room,text,user);
			return;
			}
		else {
			nuts_talk_style=1;
			write_user(user,"Say Style is NEW\n\n");
			sprintf(text,"\n~OL~FRSYSTEM:~RS Changing to NEW say style.\n\n");
			write_room_except(user->room,text,user);
			return;
			}
		return;
	}
	if (strcmp(word[0],"")) {
		sprintf(text,"\n~OL~FR%s ~FWis not a valid SET parameter.\n\n",word[0]);
		write_user(user,text);
	}
	write_user(user,"~FTValid ~OLSET ~RS~FTParameters are:\n\n");  
	write_user(user,"~OL    desc~RS <your description>      ");
	write_user(user,"~OL    gend~RS <your gender>           \n");
	write_user(user,"~OL     age~RS <your age>              ");
	write_user(user,"~OL   email~RS <email address>         \n");
	write_user(user,"~OL     url~RS <homepage address>      ");
	write_user(user,"~OL   login~RS <room name>             \n");
	write_user(user,"~OL   color~RS <speech color>          ");
	write_user(user,"~OL   fmail~RS <forwarding address>    \n");
	write_user(user,"~OL autofwd~RS <'no'/'yes'>            ");
	write_user(user,"~OLautoread~RS <'no'/'yes'>            \n");
	write_user(user,"~OL    hide~RS <'email'/'url'>         ");
	if (user->level>=EIG) {
	write_user(user,"~FT    says~RS <--toggle               \n");
	}
	write_user(user,"\n\n");
	return;
}


/* Count the total number of users */
users(user)  
UR_OBJECT user; 
{
FILE *fp;
char temp[80], file[80], sys[80],filename[80];
int count=0;
int days=0;
                
sprintf(sys, "find %s/*.D -print > logfiles/users_list",USERFILES,days);
system(sys);
sprintf(filename,"%s/%s",LOGFILES,"users_list");
fp=fopen(filename, "r");    
fscanf(fp, "%s", temp);
while(!feof(fp))
        {
        count++;
        fscanf(fp, "%s", temp);
        }
sprintf(text,"\n~OLTotal Number of ~FGDwellers~FW:~FB %d\n",count);
write_user(user,text);
}

rsuggest(user)
UR_OBJECT user;
{
FILE *fp;
char c,*emp="\nThe Suggestion Log is empty.\n";
int lines,cnt,cnt2;
                
if (word_count==1) {   
        write_user(user,"\n~BB*** Suggestion Log ***\n\n");
        switch(more(user,user->socket,SUGGESTIONS)) {
                case 0: write_user(user,emp);  return;
                case 1: user->misc_op=2;
                }
        return;
        }
if (!isnumber(word[1])) {
        write_user(user,"Usage: rsuggest [<lines from the end>]\n");  return;
        }   
/* Count total lines */
if (!(fp=fopen(SUGGESTIONS,"r"))) {  write_user(user,emp);  return;  }
cnt=0;   
lines=atoi(word[1]);

c=getc(fp);
while(!feof(fp)) {  
        if (c=='\n') ++cnt;
        c=getc(fp);
        }
if (cnt<lines) {
        sprintf(text,"There are only %d lines in the log.\n",cnt);
        write_user(user,text);
        fclose(fp);
        return;
        }
if (cnt==lines) {
        write_user(user,"\n~BB*** Suggestion Log ***\n\n");
        fclose(fp);  more(user,user->socket,SUGGESTIONS);  return;
        }

/* Find line to start on */
fseek(fp,0,0);
cnt2=0;
c=getc(fp); 
while(!feof(fp)) {
        if (c=='\n') ++cnt2;
        c=getc(fp);
        if (cnt2==cnt-lines) {
                sprintf(text,"\n~BB*** Suggestion Log (last %d lines) ***\n\n",lines);
                write_user(user,text);
                user->filepos=ftell(fp)-1;
                fclose(fp);
                if (more(user,user->socket,SUGGESTIONS)!=1) user->filepos=0;
                else user->misc_op=2;
                return;
                }
        }
fclose(fp);
sprintf(text,"%s: Line Count Error.\n",syserror);
write_user(user,text);
write_syslog("ERROR: Line Count Error in suggest().\n",0);
}         


/* Write suggestion to file */
suggest(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if ((word_count<2)||(!strcmp(word[0],""))) {
	write_user(user,"Usage: suggest <any suggestions you might have, for commands, rooms, etc...>\n");
	return;
	}

sprintf(text,"SUGGESTION FROM ~OL%s~RS:\n~OL~FT %s\n\n",user->name,inpstr);
write_suggest(text,1);
write_user(user,"\nSuggestion Noted.\n");
}

/*** Write a string to Suggestion Log ***/
write_suggest(str,write_time)
char *str;
int write_time;  
{
FILE *fp;
                
if (!system_logging || !(fp=fopen(SUGGESTIONS,"a"))) return;
if (!write_time) fputs(str,fp);
else fprintf(fp,"%02d/%02d %02d:%02d:%02d: %s",tmday,tmonth+1,thour,tmin,tsec,str);
fclose(fp);
}

/* User quits */
quit(user)
UR_OBJECT user;
{
	if (!strcmp(user->room->name,BLKJKROOM)) {
		sprintf(text,"You cannot quit in the middle of a game.\n");
		write_user(user,text);
		return;
		}
        if (user->pop !=NULL) {
                write_user(user,"~OL~FRYOU HAVE TO LEAVE POKER FIRST!!\n");
                return;
                }
	if (user->level==ZER) {
		write_user(user,"~OLMinimum requirements not completed.  Account will be ~FRdeleted.\n");
		delete_user(user->name,1);
		}
	else {
		disconnect_user(user);
	}
}

/** Show an ascii art picture ***/
        
picture(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
int ret,pic,col;  
FILE *fp;
char filename[80], *c, temp[20];
                
if (user->muzzled) {
        write_user(user,"~OLYou cannot do pictures while muzzled.\n");
        return;
        }
            
if ((word_count<2)||(word_count>2)) {
    sprintf(text,"Usage : picture <picture name>            -CeJaY\n");
    sprintf(text, "\n~FG** Pictures Available **\n\n");
        write_user(user, text);
        pic = 0; col = 0;
        text[0] = '\0';
        while (picnames[pic][0] != '*') {
            sprintf(temp, "~OL~FB%-11s", picnames[pic]);
            strcat(text, temp);
            col++;

            if (col == 7) {
                strcat(text, "\n");
                write_user(user, text);
                text[0] = '\0';
               col = 0;
                }
            pic++;
        }
    if (text[0] != '\0') {    /* Print out remainder of line. */
        strcat(text, "\n");
        write_user(user, text);
        }
    return;
    }
            
/* Check for any illegal crap in searched for file so they cannot list
   out the /etc/passwd file for instance. */
c=word[1];
while(*c) {
    if (*c=='.' || *c=='/') {
        write_user(user,"~FRSorry, that picture is not available.\n");
        return;
        }
    ++c;
    }
sprintf(filename,"%s/%s",PICTUREFILES,word[1]);
if ((fp=fopen(filename,"r"))==NULL)
        {
        write_user(user,"~FRSorry, that picture is not available.\n");
        return;
        }
            
/* for(u=user_first;u!=NULL;u=u->next) { */

sprintf(text,"~OL%s shows :\n\n",user->name);
write_room(user->room,text);
while (fgets(text,80,fp)!=NULL)
	write_room(user->room,text);
rewind(fp);
/*        } */
fclose(fp);
return;
}

/* Throw a user to another room */
/* If thrower is a wiz, user can be thrown anywhere inside talker */
/* If not, can only be thrown to an ajoining room */
throw(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
         
if (word_count<2) {
        write_user(user,"Usage: throw <user> [<room>]\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (word_count<3) rm=user->room;
else {  
        if ((rm=get_room(word[2]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }
if (user==u) {
        write_user(user,"Trying to throw yourself eh?  I don't think so... :P\n");
	return;
        }
if (u->level>user->level) {
        write_user(user,"You cannot throw a user of higher level than yourself.\n");
        return;
        }
if ((u->room!=user->room) && (user->level<FIV)) {
	write_user(user,"You cannot throw someone that is not in the room!\n");
	return;
	}
if (rm==u->room) {
        sprintf(text,"%s is already in the %s.\n",u->name,rm->name);
        write_user(user,text);
        return;
        };
if (!has_room_access(user,rm)) {
        sprintf(text,"The %s is currently private, %s cannot be thrown there.\n",rm->name,u->name);
        write_user(user,text);
        return;  
        }
sprintf(text,"~FT~OLYou heft %s up into the air...\n",u->name);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~FT~OL%s hefts %s up into the air...\n",name,u->name);
write_room_except(user->room,text,user);
move_user(u,rm,3);
prompt(u);
}

/* Display the logged_users array for the .recent command */
recent(user)
UR_OBJECT user;
{
int q;
	write_user(user,"\n~OL~FT*** ~FWRecent Logins ~FT***\n\n");

	for (q=0;q<=num_recent_users;q++) {
		sprintf(text,"%s",logged_users[q]);
		write_user(user,text);
	}
write_user(user,"\n");
return;
}

/****************************************************************************/
/*			Commands Added By LittleJohn			    */
/****************************************************************************/

/*** Do the ranks ***/

ranks_faq(user,faq)
UR_OBJECT user;
int faq;
{
int ret;
char filename[80];
char *c;
        
if (word_count<2) {
	if (faq) sprintf(filename,"%s/mainfaq",FAQFILES);
        else sprintf(filename,"%s/mainranks",RANKFILES);
        if (!(ret=more(user,user->socket,filename))) {
		if (faq) write_user(user,"There are no faqz at the moment.\n");
                else write_user(user,"There are no ranks at the moment.\n");
                return;
                }
	write_user(user,"\n");
        if (ret==1) user->misc_op=2;
        return;
	}
/* Check for any illegal crap in searched for filename so they cannot list
   out the /etc/passwd file for instance. */
c=word[1];
while(*c) {
        if (*c=='.' || *c=='/') {
                write_user(user,"Sorry, there is no information about that level at the moment.\n\n");
                if (ret==1) user->misc_op=2;
                return;
                }
	if ((*c=='*')&&(user->level<FIV)) {
		write_user(user,"Sorry, there is no file called that.\n");
		if (ret==1) user->misc_op=2;
		return;
		}
        ++c;
        }
if (faq) sprintf(filename,"%s/%s",FAQFILES,word[1]);
else sprintf(filename,"%s/%s",RANKFILES,word[1]);
if (!(ret=more(user,user->socket,filename)))
        if (faq) write_user(user,"Sorry, there is no information about that faq at the moment.\n\n");
        else write_user(user,"Sorry, there is no information about that level at the moment.\n\n");
if (ret==1) user->misc_op=2;
}


timed(user)
UR_OBJECT user;
{
FILE *fp;
int days,hours,mins,secs,count=0;
char bstr[40],temp[80],filename[80],sys[80];
        
strcpy(bstr,ctime(&boot_time));
secs=(int)(time(0)-boot_time);
days=secs/86400;
hours=(secs%86400)/3600;
mins=(secs%3600)/60;
secs=secs%60;

sprintf(sys,"find %s/*.D -print > logfiles/users_list",USERFILES);
system(sys);
sprintf(filename,"%s/%s",LOGFILES,"users_list");
fp=fopen(filename, "r");
fscanf(fp, "%s", temp);
while(!feof(fp))
        {
        count++;
        fscanf(fp, "%s", temp);
        }
sprintf(text,"\n~OL~FB*** RNUTS version %s - Time status ***\n\n",RVERSION);
write_user(user,text);
sprintf(text,"~FTServer Time  : ~FY%s EST\n",long_date(2));
write_user(user,text);
sprintf(text,"~FTTalker Booted: ~FG%s",bstr);
write_user(user,text);
sprintf(text,"~FTUptime       : ~FG%d days, %d hours, %d minutes, %d seconds\n",days,hours,mins,secs);
write_user(user,text);
sprintf(text,"~FTTotal Users  : ~FG%d Dwellers\n\n",count);
write_user(user,text);
}


/* delete old users of inpstr length or older */
delold_users(user)
UR_OBJECT user;
{
int days=PURGEDAYS;
int c=0;
FILE *fp;
char temp[80], file[80], sys[80],filename[80];

sprintf(sys, "find %s/*.D -mtime +%d -print > logfiles/users_removed",USERFILES,days);
system(sys);
sprintf(filename,"%s/%s",LOGFILES,"users_removed");
fp=fopen(filename, "r");
fscanf(fp, "%s", temp);
while(!feof(fp))
        {
        temp[strlen(temp)-2]='\0';
	sprintf(file,"rm %s.*",temp);
	system(file);
	c++;
        fscanf(fp, "%s", temp);
        }
sprintf(temp,"\n~OLTotal Number of ~FGDwellers~FW removed:~FB %d\n",c);
write_user(user,temp);
}


wizon(user)
UR_OBJECT user;
{
UR_OBJECT u;
int i=0;
char filename[80];

write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
sprintf(text,"~FT   Wiz List                                             RNUTS Version %s\n",RVERSION);
write_user(user,text);
write_user(user,"~FT+-----------------------------------------------------------------------------+\n\n");

sprintf(filename,"%s/%s",DATAFILES,WIZLISTFILE);
switch(more(user,user->socket,filename)) {
	case 0: write_user(user,"There is no Wizlist.\n"); break;
        case 1: user->misc_op=2;
	}
write_user(user,"\n");
write_user(user,"~FT+-----------------------------------------------------------------------------+\n\n");

for(u=user_first;u!=NULL;u=u->next) {
    if (u->type==CLONE_TYPE) {
        i++;
        continue;
        }
    if (u->level>=FOU) {
        sprintf(text,"   %-14s %-12s %-12s\n",u->name,who_wiz[u->level],u->room);
        write_user(user,text);
        }
    i++;
    }
write_user(user,"\n");
write_user(user,"~FT+-----------------------------------------------------------------------------+\n\n");
}

/*** Show something to user ***/
show(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;

if (user->muzzled) {
	write_user(user,"You are muzzled, you cannot Show.\n");  return;
	}
if (word_count<3) {
	write_user(user,"Show who what?\n");  return;
	}
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"Trying to show yourself is the eighth sign of madness.\n");
        return;
        }
inpstr=remove_first(inpstr);
sprintf(text,"~OL~FB(%s) -->Shows: %s <to:%s>\n",user->name,inpstr,u->name);
write_level(FOU,1,text,NULL);
sprintf(text,"~OL~FB-->Type:~RS %s\n",inpstr);
write_user(u,text);
sprintf(text,"~OL~FB-->You Show: %s <to:%s>\n",inpstr,u->name);
write_user(user,text);
}


bring(UR_OBJECT u)
{
UR_OBJECT target;
char buff[OUT_BUFF_SIZE];

if (u->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....you cannot bring anyone.\n",JAILROOM);
	write_user(u,text);
        return;
        }
if (word_count < 2) {
      	write_user(u, "~FRSyntax: bring <user>\n");
      	return;
	}
if ((target=get_user(word[1]))==NULL)
      	write_user(u,notloggedon);
else {
	if (target->name == u->name)
        	write_user(u,"~FRWhy are you trying to bring yourself?\n");
        else if (target->room == u->room) {
        	sprintf(text,"~OLThat user is already in this room!\n");
        	write_user(u,text);
        	}
	else if (target->room == get_room(BLKJKROOM)) { 
		sprintf(text,"~OLThat user is playing blackjack.\n");
		write_user(u,text);
		}
        else if (target->pop!=NULL) {
                write_user(u,"~OL~FRThat user is playing poker, you can't bring them anywhere!\n");
                return;
                }
        else if (target->level>u->level) {
	        write_user(u,"~OLSorry, you can't ~FTBRING~FW a user of greater level than your own.\n");
        	}
	else move_user(target,u->room,2);
	}
return;
}


join(UR_OBJECT u)
{
UR_OBJECT target;
char buff[OUT_BUFF_SIZE];   

if (u->jailed) {
	sprintf(text,"~OLYou are in ~FR%s~FW....you cannot join anyone.\n",JAILROOM);
        write_user(u,text);
        return;
        }
if (word_count < 2) {
        write_user(u, "~FRSyntax: join <user>\n");
        return;
        }
if ((target=get_user(word[1]))==NULL)
        write_user(u,notloggedon);
else {
        if (target->name == u->name)
                write_user(u,"~FRWhy are you trying to join yourself?\n");
        else if (target->room == u->room) {
                sprintf(text,"~OLYou are already in the same room as %s!\n",target->name);
                write_user(u,text);
                }
        else if (u->pop!=NULL) {
                write_user(u,"~OL~FRYou are playing poker, leave the game first!\n");
                return;
                }
        else if (target->room == get_room(BLKJKROOM)) {
		write_user(u,"~OLSorry, to join a game of 21, use .blackjack join.\n");
	return;
	}

        else move_user(u,target->room,1);
        }
return;
}


jail_user(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
RM_OBJECT rm;

if (word_count<2) {
    write_user(user,"~OLUsage: arrest <user>\n");
    return;                                 
    }
if (!(u=get_user(word[1]))) { 
        write_user(user,notloggedon);
        return;
        }
if ((u=get_user(word[1]))!=NULL) {
        if (u->level>=user->level) {
                write_user(user,"~OLSorry, you canot arrest a user of greater level than your own.\n");
                return;
                }
        if (u->jailed) {
		sprintf(text,"~OLThat user is already in %s!\n",JAILROOM);
                write_user(user,text);
                return;
                }
        if (user==u)
                {
                write_user(user,"~OLWhy are you trying to arrest yourself??\n");
                return;
                }
        rm=get_room(JAILROOM);
        move_user(u,rm,4);
        u->jailed=user->level;
        return;
        }
/* User not logged on */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text); 
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in jail_user().\n",0);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);
        destruct_user(u);
        destructed=0;
        return;
        }
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
rm=get_room(JAILROOM);
move_user(u,rm,4);
u->jailed=user->level;
destruct_user(u);
destructed=0;
return;
}

unjail_user(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;

if (word_count<2) {
    write_user(user,"~OLUsage: release <user>\n");
    return;
    }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;   
	}
if ((u=get_user(word[1]))!=NULL) {
        if (user==u) {
		sprintf(text,"~OLYou cant remove yourself from %s.\n",JAILROOM);
                write_user(user,text);
                return;
                }
        if (u->level<user->jailed) {
                write_user(user,"~OLSorry, you dont have the power to release this user.\n");
                return;
                }
        if (!(u->jailed)) {
		sprintf(text,"~OLThat person is not in ~FR%s~FW.\n",JAILROOM);
                write_user(user,text);
                return;
                }
        move_user(u,user->room,2);
        u->jailed=0;
        return;            
        }
/* User not logged on */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in unjail_user().\n",0);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);
        destruct_user(u);
        destructed=0;
        return;
        }
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
u->jailed=0;
destruct_user(u);
destructed=0;
return;
}
         

last_login(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
int timelen,days2,hours2,mins2;
        
if (word_count<2) {
        write_user(user,"Last who?\n");  return;
        }
if (!(u=get_user(word[1]))) {
        if ((u=create_user())==NULL) {
                sprintf(text,"%s: unable to create temporary user object.\n",syserror);
                write_user(user,text);
                write_syslog("ERROR: Unable to create temporary user object in  last_known_login().\n");
                return;
                }
        strcpy(u->name,word[1]);
        if (!load_user_details(u)) {
                write_user(user,nosuchuser);
                destruct_user(u);
                destructed=0;
                return;
                }
        u2=NULL;
        }
else u2=u;

timelen=(int)(time(0) - u->last_login);
days2=timelen/86400;
hours2=(timelen%86400)/3600;
mins2=(timelen%3600)/60;

if (u2==NULL) {
	sprintf(text,"\n~OL%s was last logged in: ~FG%s",u->name,ctime((time_t*)&(u->last_login)));
	write_user(user,text);
	sprintf(text,"~OLWhich was: ~FG%d ~FWdays,~FG %d ~FWhours, and ~FG%d ~FWminutes ago\n\n",days2,hours2,mins2);
	write_user(user,text);
	destruct_user(u);
	destructed=0;
	return;
	}
sprintf(text,"~OL%s has been logged in since: ~FG%s",u->name,ctime((time_t*)&(u->last_login)));
write_user(user,text);
}


curse(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
	write_user(user,"Usage: curse <user>\n");  return;
	}
if ((u=get_user(word[1]))!=NULL) {
	if (u==user) {
		write_user(user,"Trying to curse yourself is the ninth sign of madness.\n");
		return;
		}
	if (u->level>=user->level) {
		write_user(user,"You cannot curse a user of equal or higher level than yourself.\n");
		return;
		}
	if (u->cursed>=user->level) {
		sprintf(text,"%s is already cursed.\n",u->name);
		write_user(user,text);  return;
		}
	sprintf(text,"~FR~OL%s now has a curse of level: ~RS~OL%s.\n",u->name,level_name[user->level]);
	write_user(user,text);
	write_user(u,"~FR~OLYou have been cursed!\n");
	sprintf(text,"%s cursed %s.\n",user->name,u->name);
	write_syslog(text,1);
	u->cursed=user->level;
	return;
	}
/* User not logged on */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in curse().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->level>=user->level) {
	write_user(user,"You cannot curse a user of equal or higher level than yourself.\n");
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->cursed>=user->level) {
	sprintf(text,"%s is already cursed.\n",u->name);
	write_user(user,text); 
	destruct_user(u);
	destructed=0;
	return;
	}
u->socket=-2;
u->cursed=user->level;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"~FR~OL%s given a curse of level: ~RS~OL%s.\n",u->name,level_name[user->level]);
write_user(user,text);
send_mail(user,word[1],"~FR~OLYou have been cursed!\n",0);
sprintf(text,"%s cursed %s.\n",user->name,u->name);
write_syslog(text,1);
destruct_user(u);
destructed=0;
}



/*** Enchant the bastard now he's apologised and grovelled enough via email ***/
remove_curse(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
	write_user(user,"Usage: enchant <user>\n");  return;
	}
if ((u=get_user(word[1]))!=NULL) {
	if (u==user) {
		write_user(user,"Trying to enchant yourself is the tenth sign of madness.\n");
		return;
		}
	if (!u->cursed) {
		sprintf(text,"%s is not cursed.\n",u->name);  return;
		}
	if (u->cursed>user->level) {
		sprintf(text,"%s's curse is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->cursed]);
		write_user(user,text);  return;
		}
	sprintf(text,"~FG~OLYou remove %s's curse.\n",u->name);
	write_user(user,text);
	write_user(u,"~FG~OLYou have been enchanted!\n");
	sprintf(text,"%s enchanted %s.\n",user->name,u->name);
	write_syslog(text,1);
	u->cursed=0;
	return;
	}
/* User not logged on */
if ((u=create_user())==NULL) {
	sprintf(text,"%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
	write_syslog("ERROR: Unable to create temporary user object in remove_curse().\n",0);
	return;
	}
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);  
	destruct_user(u);
	destructed=0;
	return;
	}
if (u->cursed>user->level) {
	sprintf(text,"%s's curse is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->cursed]);
	write_user(user,text);  
	destruct_user(u);
	destructed=0;
	return;
	}
u->socket=-2;
u->cursed=0;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"~FG~OLYou remove %s's curse.\n",u->name);
write_user(user,text);
send_mail(user,word[1],"~FG~OLYou have been enchanted.\n",0);
sprintf(text,"%s enchanted %s.\n",user->name,u->name);
write_syslog(text,1);
destruct_user(u);
destructed=0;
}


list_users(user)  
UR_OBJECT user; 
{
FILE *fp;
char sys[80];
int i=0;

if ((word_count<2)||(!strcmp(word[1],""))) {                
	write_user(user,"Usage: ulist <letter/-a>\n\n");
	return;
	}  
strtolower(word[1]);
if (!strcmp(word[1],"-a")) {
	sprintf(sys,"find %s/*.D -print > logfiles/ulist_list",USERFILES);
	system(sys);
	view_ulist(user,word[1],1);
	}
else {
	strtoupper(word[1]);
	sprintf(sys,"find %s/%s*.D -print > logfiles/ulist_list",USERFILES,word[1]);
	system(sys);
	view_ulist(user,word[1],0);
	}
}


view_ulist(user,str,all)
UR_OBJECT user;
char *str;
int all;
{
FILE *fp;
char line[80],temp[80]="\0";
int c=0,j,i;

write_user(user,"\n~OL~FT*** ~FWUser List ~FT***\n\n");
fp=fopen("logfiles/ulist_list","r");
fscanf(fp,"%s",line);
while(!feof(fp)) {
	j=0;
	for (i=10;i<strlen(line);i++) {
		temp[j]=line[i];
		j++;
		}
	c++;
	temp[strlen(temp)-2]='\0';
	sprintf(text,"%s\n",temp);
	write_user(user,text);
	for (i=0;i<80;i++)
		temp[i]='\0';
	fscanf(fp,"%s",line);
	}
if (all==1) sprintf(text,"\nTotal number of users = %d\n\n",c);
else sprintf(text,"\nTotal number of %s's = %d\n\n",str,c);
write_user(user,text);
}


/* Force someone to do something.*/
force(user, inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
if (word_count<3) {
        write_user(user,"Force who to do what?\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
	write_user(user, "Um.. just do it yourself.\n");
	return;
	}
if (user->level<=u->level) {
   	write_user(user, "I wouldn't do that if I were you.\n"); 
	return; 
	}
inpstr=remove_first(inpstr);
clear_words();
word_count=wordfind(inpstr);
sprintf(text, "~FW%s~RS is forcing you to: ~FW%s~RS\n", user->name,inpstr);
write_user(u, text);
sprintf(text, "You force ~FW%s~RS to: ~FW%s~RS\n", u->name, inpstr);
write_user(user, text);
parse(u, inpstr); 
return;
}
     
/* get users which to send copies of smail to */
copies_to(user)
UR_OBJECT user;
{
FILE *fp;
int remote,i=0,docopy,found,cnt;
char *c,filename[80];

if (com_num==NOCOPIES) {
    for (i=0; i<MAX_COPIES; i++) user->copyto[i][0]='\0';
    write_user(user,"Sending no copies of your next smail.\n");  return;
    }
if (word_count<2) {
    text[0]='\0';  found=0;
    for (i=0; i<MAX_COPIES; i++) {
        if (!user->copyto[i][0]) continue;
        if (++found==1) write_user(user,"Sending copies of your next smail to...\n");
        strcat(text,"   ");  strcat(text,user->copyto[i]);
        }
    strcat(text,"\n\n");
    if (!found) write_user(user,"You are not sending a copy to anyone.\n");
    else write_user(user,text);
    return;
    }
if (word_count>MAX_COPIES+1) {      /* +1 because 1 count for command */
    sprintf(text,"You can only copy to a maximum of %d people.\n",MAX_COPIES);
    write_user(user,text);  return;
    }
write_user(user,"\n");
cnt=0;
for (i=0; i<MAX_COPIES; i++) user->copyto[i][0]='\0';
for (i=1; i<word_count; i++) {
    remote=0;  docopy=1;
    /* See if its to another site */
    c=word[i];
    while(*c) {
        if (*c=='@') {
            if (c==word[i]) {
                sprintf(text,"Name missing before @ sign for copy to name '%s'.\n",word[i]);
                write_user(user,text);  docopy=0; goto SKIP;
                }
            remote=1;  docopy=1;  goto SKIP;
            }
        ++c;
        }
    /* See if user exists */
    if (get_user(word[i])==user && user->level<SEV) {
        write_user(user,"You cannot send yourself a copy.\n");
        docopy=0;  goto SKIP;
        }
    word[i][0]=toupper(word[i][0]);
    if (!remote) {
        sprintf(filename,"%s/%s.D",USERFILES,word[i]);
        if (!(fp=fopen(filename,"r"))) {
            sprintf(text,"There is no such user with the name '%s' to copy to.\n",word[i]);
            write_user(user,text);
            docopy=0;
            }
        else docopy=1;
        }
    fclose(fp);
    SKIP:
    if (docopy) {
        strcpy(user->copyto[cnt],word[i]);  cnt++;
        }
    }
text[0]='\0';  i=0;  found=0;
for (i=0; i<MAX_COPIES; i++) {
    if (!user->copyto[i][0]) continue;
    if (++found==1) write_user(user,"Sending copies of your next smail to...\n");
    strcat(text,"   ");  strcat(text,user->copyto[i]);
    }
strcat(text,"\n\n");
if (!found) write_user(user,"You are not sending a copy to anyone.\n");
else write_user(user,text);
}

/* send out copy of smail to anyone that is in user->copyto */
send_copies(user,ptr)
UR_OBJECT user;
char *ptr;
{
int sent, i, found=0;

for (i=0; i<MAX_COPIES; i++) {
    if (!user->copyto[i][0]) continue;
    if (++found==1) write_user(user,"Attempting to send copies of smail...\n");
    if (send_mail(user,user->copyto[i],ptr,1)) {
        sprintf(text,"%s sent a copy of mail to %s.\n",user->name,user->copyto[i]);
        write_syslog(text,1,SYSLOG);
        }
    }
for (i=0; i<MAX_COPIES; i++) user->copyto[i][0]='\0';
}


/* verify user's email when it is set specified */
set_forward_email(user)
UR_OBJECT user;
{
FILE *fp;
char cmd[ARR_SIZE],filename[80];

if (!user->fwdaddy[0] || !strcmp(user->fwdaddy,"#UNSET")) {
  write_user(user,"Your email address is currently ~FRunset~RS.  If you wish to use
the\nauto-forwarding function then you must set your forwarding address.\n\n");
  user->autofwd=0;
  return;
  }
if (!forwarding) {
  write_user(user,"Even though you have set your email, the auto-forwarding function is currently unavaliable.\n");
  user->mail_verified=0;
  user->autofwd=0;
  return;
  }
user->mail_verified=0;
user->autofwd=0;
/* Let them know by email */
sprintf(filename,"%s/%s.FWD",MAILSPOOL,user->name);
if (!(fp=fopen(filename,"w"))) { 
  write_syslog("Unable to open forward mail file in set_forward_email()\n",0,SYSLOG);
  return;
  }
sprintf(user->verify_code,"%s%d",VERIFYCODE,rand()%9999);
/* email header */
fprintf(fp,"From: %s Talker\n",TALKERNAME);
fprintf(fp,"To: %s <%s>\n",user->name,user->fwdaddy);
fprintf(fp,"Subject: Verification of auto-mail.\n");   
fprintf(fp,"\n");
/* email body */
fprintf(fp,"Hello, %s.\n\n",user->name);
fprintf(fp,"Thank you for setting your email address, and now that you have do so you are able to\n");
fprintf(fp,"use the auto-forwarding function on %s to have any smail sent to your email address.\n",TALKERNAME);
fprintf(fp,"To be able to do this though you must verify that you have received this email.\n\n");
fprintf(fp,"Your verification code is: %s\n\n",user->verify_code);
fprintf(fp,"Use this code with the 'verify' command when you next log onto the talker.\n");
fprintf(fp,"Thank you for coming to the talker - we hope you enjoy it!\n\n RobinHood & LittleJohn.\n\n");
fputs(talker_signature,fp);
fclose(fp);
/* send the mail */
sprintf(cmd,"mail %s < %s",user->fwdaddy,filename);
if (system(cmd)==-1) {
  write_syslog("Mail not sent in set_forward_email()\n",1,SYSLOG);
  unlink(filename);
  write_user(user,"\nThe email could not be sent to you.  Please check the address supplied.\n");
  write_user(user,"If you gave the wrong address, please use the ~FTset~RS command again.\n\n");
  strcpy(user->verify_code,"#NONE");
  return;
  }
unlink(filename);
sprintf(text,"%s had mail sent to them by set_forward_email().\n",user->name);
write_syslog(text,1,SYSLOG);
/* Inform them online */
write_user(user,"Now that you have set your email you can use the auto-forward functions.\n");
write_user(user,"You must verify your address with the code you will receive shortly, via email.\n"); 
write_user(user,"If you do not receive any email, then use ~FTset fmail <address>~RS again, making\n");
write_user(user,"sure you have the correct address.\n\n");
}



/* verify that mail has been sent to the address supplied */
verify_email(user)
UR_OBJECT user;
{
if (word_count<2) {
  write_user(user,"Usage: verify <verification code>\n");
  return;
  }
if (!user->fwdaddy[0] || !strcmp(user->fwdaddy,"#UNSET")) {
  write_user(user,"You have not yet set your forwarding address.  You must do this first.\n");
  return;
  }
if (!strcmp(user->verify_code,"#EMAILSET")) {
  write_user(user,"You have already verified your current email address.\n");
  return;
  }
if (strcmp(user->verify_code,word[1]) || !strcmp(user->verify_code,"#NONE")) {
  write_user(user,"That does not match your verification code.  Please check your code and try again.\n");
  return;
  }
strcpy(user->verify_code,"#EMAILSET");
user->mail_verified=1;
write_user(user,"\nThe verification code you gave was correct.\nYou may now use the auto-forward functions.\n\n");
}


/* send smail to the email ccount */
forward_email(name,from,message,user)
char *name,*from,*message;
UR_OBJECT user;
{
FILE *fp;
UR_OBJECT u;
char filename[80],cmd[ARR_SIZE];
int on=0;
  
if (!forwarding) return;
if (u=get_user(name)) {
  on=1;
  goto SKIP;
  }
/* Have to create temp user if not logged on to check if email verified, etc */
if ((u=create_user())==NULL) {
    write_syslog("ERROR: Unable to create temporary user object in forward_email().\n",0,SYSLOG);
    return;
    }
strcpy(u->name,name);
if (!load_user_details(u)) {
    destruct_user(u);
    destructed=0;
    return;
    }
on=0;
SKIP:
if (!u->mail_verified) {
    if (!on) { destruct_user(u);  destructed=0; }
    return;
    }
if (!u->autofwd){
    if (!on) { destruct_user(u);  destructed=0; }
    return; 
    }

sprintf(filename,"%s/%s.FWD",MAILSPOOL,u->name);
if (!(fp=fopen(filename,"w"))) {
  write_syslog("Unable to open forward mail file in set_forward_email()\n",0,SYSLOG);
  return;
  }
fprintf(fp,"From: %s@%s\n",user->name,TALKERNAME);
fprintf(fp,"To: %s <%s>\n",u->name,u->fwdaddy);
fprintf(fp,"Subject: Auto-forward of smail.\n");
fprintf(fp,"\n");
from=colour_com_strip(from);
fputs(from,fp);
fputs("\n",fp);
message=colour_com_strip(message);
fputs(message,fp);
fputs("\n\n",fp);
fputs(talker_signature,fp);
fclose(fp);
sprintf(cmd,"mail %s < %s",u->fwdaddy,filename);
if (system(cmd)==-1) {
  write_syslog("Mail not sent in forward_email()\n",1,SYSLOG);
  unlink(filename);
  if (!on) {
    destruct_user(u);
    destructed=0;
    }
  return;
  }
unlink(filename);
sprintf(text,"%s had mail sent to their email address.\n",u->name);
write_syslog(text,1,SYSLOG);
if (!on) {
    destruct_user(u);
    destructed=0;
    }
return;
}

pop_level(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char level[10];

word_count = wordfind(inpstr);
if (word_count < 2) {
        write_user(user,"Usage: pop <user> <level>\n");
        return;
        }
if ( ! (u=get_user(word[0]) ) ) {
        write_user(user,"There is no one of that name logged in.\n");
        return;
        }
if (u == user) {
        write_user(user,"Nice try but you can't change your own level.\n");
        return;
        }
if (u->level >= user->level) {
        write_user(user,"You cannot change the level of someone of equal or higher level than yourself.\n");
        return;
	}
strtoupper(word[1]);
if (!strcmp(word[1],who_wiz[EIG])) { u->level=EIG+1; --u->level; }
if (!strcmp(word[1],who_wiz[SEV])) { u->level=SEV+1; --u->level; }
if (!strcmp(word[1],who_wiz[SIX])) { u->level=SIX+1; --u->level; }
if (!strcmp(word[1],who_wiz[FIV])) { u->level=FIV+1; --u->level; }
if (!strcmp(word[1],who_wiz[FOU])) { u->level=FOU+1; --u->level; }
if (!strcmp(word[1],who_wiz[THR])) { u->level=THR+1; --u->level; }
if (!strcmp(word[1],who_wiz[TWO])) { u->level=TWO+1; --u->level; }
if (!strcmp(word[1],who_wiz[ONE])) { u->level=ONE+1; --u->level; }
if (!strcmp(word[1],who_wiz[ZER])) { u->level=ZER+1; --u->level; }
sprintf(text,"~OL%s ~RSpops~OL %s ~RSto level: ~FR~OL%s\n",user->name,u->name,u->level[level_name]);
write_room_except(NULL, text, u);
write_log(text,1,4);
sprintf(text,"~OL%s ~RSpops you to level: ~FR~OL%s\n",user->name,u->level[level_name]);
write_user(u, text);
}


/* Display all the people logged on from the same site as user */
matchsite(user,stage)
UR_OBJECT user;
int stage;
{
UR_OBJECT u,u_loop;
int j=0,i,found,cnt,same,on;
char line[82],temp[82],name[USER_NAME_LEN+1];
FILE *fpi;
    
if (!stage) {
	if (word_count<2) {
		write_user(user,"Usage: matchsite user/site [-a]\n");
		return;
		}
	strtolower(word[1]); strtolower(word[2]);
	if (word_count==3 && !strcmp(word[2],"-a")) 
		user->matchsite_all_store=1;
	else user->matchsite_all_store=0;
	if (!strcmp(word[1],"user")) {
		write_user(user,"Enter the name of the user to be matched against: ");
		user->misc_op=8;
		return;
		}
	if (!strcmp(word[1],"site")) {
		write_user(user,"~OL~FRNOTE:~RS Partial site strings can be given, but NO wildcards.\n");
		write_user(user,"Enter the site to be matched against: ");
		user->misc_op=9;
		return;
		}
	write_user(user,"Usage: matchsite user/site [-a]\n");
	return;
	}

/* check for users of matching sites - 'user' supplied */
if (stage==1) {
	/* check just those logged on */
	if (!user->matchsite_all_store) {
		found=cnt=same=0;
		if ((u=get_user(user->matchsite_check_store))==NULL) {
			write_user(user,notloggedon);
			return;
			}
		for (u_loop=user_first;u_loop!=NULL;u_loop=u_loop->next) {
			cnt++;
			if (u_loop==u) continue;  
			if (strstr(u->site,u_loop->site)) {
				same++;
				if (++found==1) {
					sprintf(text,"\nUsers logged on from the match site as ~OL%s~RS\n\n",u->name);
					write_user(user,text);
					}
				sprintf(text,"    %-11s %s\n",u_loop->name,u_loop->site);
				if (u_loop->type==REMOTE_TYPE) text[2]='@';
				if (!u_loop->vis) text[3]='*';
				write_user(user,text);
				}
			}
		if (!found) {
			sprintf(text,"No users currently logged on have that match site as %s.\n",u->name);
			write_user(user,text);
			}
		else { 
			sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s~RS ~FG(%s)\n\n",cnt,same,u->name,u->site);
			write_user(user,text);
			}
		return;
		}

	/* check all the users..  First, load the name given */
	if (!(u=get_user(user->matchsite_check_store))) {
		if ((u=create_user())==NULL) {
			sprintf(text,"%s: unable to create temporary user session.\n",syserror);
			write_user(user,text);
			write_syslog("ERROR: Unable to create temporary user session in matchsite() - stage 1/all.\n",0);
			return;
			}
		strcpy(u->name,user->matchsite_check_store);
		if (!load_user_details(u)) {
			destruct_user(u); destructed=0;
			sprintf(text,"Sorry, unable to load user file for %s.\n",user->matchsite_check_store);
			write_user(user,text);
			write_syslog("ERROR: Unable to load user details in matchsite() - stage 1/all.\n",0);
			return;
			}  
		on=0;
		}
	else on=1;

	/* open userlist to check against all users */
	if (!(fpi=fopen("logfiles/users_list","r"))) {
		write_syslog("ERROR: Unable to open userlist in matchsite() - stage 1/all.\n",0);
		write_user(user,"Sorry, you are unable to use the ~OLall~RS option at this time.\n");
		return;
		}
	found=cnt=same=0;
	fscanf(fpi,"%s",line);
	while (!feof(fpi)) {
	        j=0; 
	        for (i=10;i<strlen(line);i++) {
        	        temp[j]=line[i];
                	j++;
                	}
	        temp[strlen(temp)-2]='\0';
	        sprintf(name,"%s",temp);
		for (i=0;i<80;i++)   
                	temp[i]='\0';
		name[0]=toupper(name[0]);
		/* create a user object if user not already logged on */  
		if ((u_loop=create_user())==NULL) {
			write_syslog("ERROR: Unable to create temporary user session in matchsite().\n",0);
			goto SAME_SKIP1;
			}  
		strcpy(u_loop->name,name);
		if (!load_user_details(u_loop)) {
			destruct_user(u_loop); destructed=0;
			goto SAME_SKIP1;
			}
		cnt++; 
		if ((on && !strcmp(u->site,u_loop->last_site)) || (!on && !strcmp(u->last_site,u_loop->last_site))) {
			same++;
			if (++found==1) {
				sprintf(text,"\nAll users from the same site as ~OL%s~RS\n\n",u->name);
				write_user(user,text);
				}
			sprintf(text,"    %-11s %s\n",u_loop->name,u_loop->site);
			write_user(user,text);
			}
		destruct_user(u_loop);
		destructed=0;

	SAME_SKIP1:
		fscanf(fpi,"%s",line);
		}
	fclose(fpi);
	if (!found) {
		sprintf(text,"No users have the same site as %s.\n",u->name);
		write_user(user,text);
		}    
	else { 
		if (!on) sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s~RS ~FG(%s)\n\n",cnt,same,u->name,u->last_site);
		else sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s~RS ~FG(%s)\n\n",cnt,same,u->name,u->site);
		write_user(user,text);
		}
	if (!on) { destruct_user(u);  destructed=0; }
	return;
	} /* end of stage 1 */

/* check for users of matching sites - site supplied */
if (stage==2) {
	/* check just those logged on */
	if (!user->matchsite_all_store) {
		found=cnt=same=0;
		for (u=user_first;u!=NULL;u=u->next) {
			cnt++;
			if (!strstr(u->site,user->matchsite_check_store)) continue;
			same++;
			if (++found==1) {
				sprintf(text,"\nUsers logged on from the match site as ~OL%s~RS\n\n",user->matchsite_check_store);  
				write_user(user,text);
				}
			sprintf(text,"    %-11s %s\n",u->name,u->site);
			if (u->type==REMOTE_TYPE) text[2]='@';
			if (!u->vis) text[3]='*';
			write_user(user,text);
			}
		if (!found) {
			sprintf(text,"No users currently logged on have that match site as %s.\n",user->matchsite_check_store);
			write_user(user,text);
			}   
		else {
			sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s\n\n",cnt,same,user->matchsite_check_store);
			write_user(user,text);
			}		
		return;
		}

	/* check all the users.. */
	/* open userlist to check against all users */
	if (!(fpi=fopen("logfiles/users_list","r"))) {
		write_syslog("ERROR: Unable to open userlist in matchsite() - stage 2/all.\n",0);
		write_user(user,"Sorry, you are unable to use the ~OLall~RS option at this time.\n");
		return;
		}
	found=cnt=same=0;
	fscanf(fpi,"%s",line);   
	while (!feof(fpi)) {
                j=0;
                for (i=10;i<strlen(line);i++) {
                        temp[j]=line[i];
                        j++;
                        }
                temp[strlen(temp)-2]='\0';
                sprintf(name,"%s",temp);
                for (i=0;i<80;i++)
                        temp[i]='\0';
		name[0]=toupper(name[0]);
		/* create a user object if user not already logged on */
		if ((u_loop=create_user())==NULL) {
			write_syslog("ERROR: Unable to create temporary user session in matchsite() - stage 2/all.\n",0);
			goto SAME_SKIP2;
			}
		strcpy(u_loop->name,name);
		if (!load_user_details(u_loop)) {
			destruct_user(u_loop); destructed=0;
			goto SAME_SKIP2;
			}
		cnt++;
		if (strstr(u_loop->last_site,user->matchsite_check_store)) {
			same++;
			if (++found==1) {
				sprintf(text,"\nAll users that have the site ~OL%s~RS\n\n",user->matchsite_check_store);
				write_user(user,text);
				}
			sprintf(text,"    %-11s %-50s\n",u_loop->name,u_loop->last_site);
			write_user(user,text);
			}
		destruct_user(u_loop);
		destructed=0;

	SAME_SKIP2:
		fscanf(fpi,"%s",line);  
		}     
	fclose(fpi);

	if (!found) {
		sprintf(text,"No users have the same site as %s.\n",user->matchsite_check_store);
		write_user(user,text);  
		}  
	else {   
		if (!on) sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s~RS\n\n",cnt,same,user->matchsite_check_store);
		else sprintf(text,"\nChecked ~OL%d~RS users, ~OL%d~RS had the site as ~OL%s~RS\n\n",cnt,same,user->matchsite_check_store);
		write_user(user,text);
		}
	return;
	} /* end of stage 2 */
}



/*** End of File ***/


