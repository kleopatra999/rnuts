#include "bot_quote.h"
#include "bot_fortune.h"

instr(s1,s2)
char *s1,*s2;
{
int f,g;
for (f=0;*(s1+f);++f) {
    for (g=0;;++g) {
        if (*(s2+g)=='\0' && g>0) return f;
        if (*(s2+g)!=*(s1+f+g)) break;
        }
    }
return -1;
}

RM_OBJECT get_the_room(name)
char *name;
{
RM_OBJECT rm;

for(rm=room_first;rm!=NULL;rm=rm->next)
     if (!strncmp(rm->name,name,strlen(name))) return rm;
return NULL;
}

BotCheck(user,inpstr)
UR_OBJECT user;   
char *inpstr;  
{
RM_OBJECT rm;
char *name;

if (user->vis) name=user->name; else name=invisname;
strtolower(inpstr); 
if(bot_bot==NULL) return;
if(user->room==bot_bot->room&&(instr(inpstr,BOTNAME)!=-1||instr(inpstr," tc")!=-1) ) {
        if (instr(inpstr, "kill") != -1) {
                if ((rand() % 10) >= 7) {
			sprintf(text,botkill_text[rand()%num_botkill_text],bot_bot->color,bot_bot->name,name);
			write_room(user->room,text);
                        write_user (user, "Kiss my ass barf bag :P...\n");
                        write_user (user, "    ... when he runs his blade right through you.\n");
                        disconnect_user(user);
			return;
                        }
                else {
			sprintf(text,botlive_text[rand()%num_botlive_text],bot_bot->color,bot_bot->name,name);
                        write_room(user->room,text);
			return;
			}
                }
	else if ((instr(inpstr,"hit")!=-1)||(instr(inpstr,"bop")!=-1)) {
		if ((rand() % 10) >= 7) {
			if ((rand() % 10) >= 9)
				rm=get_the_room("northRoad");
			if ((rand() % 10) <= 2)
				rm=get_the_room("stream");
			if (((rand() % 10) > 4)&&((rand() % 10) < 7))
				rm=get_the_room("forest");
			move_user(user,rm,3);
			return;
			}	
		else {
			if (instr(inpstr,"hit")!=-1)
				sprintf(text,hit_text[rand()%num_hit_text],bot_bot->color,bot_bot->name,name);
                	if (instr(inpstr,"bop")!=-1)
				sprintf(text,bop_text[rand()%num_bop_text],bot_bot->color,bot_bot->name,name);
			}
		}
        else if (instr (inpstr, "bye") != -1)   	 sprintf(text,bye_text[rand()%num_bye_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "quote") != -1)          sprintf(text,quote_text[rand()%num_quote_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "fortune") != -1)        sprintf(text,fortune_text[rand()%num_fortune_text],bot_bot->color,bot_bot->name);
        else if (instr (inpstr, "hug") != -1)    	 sprintf(text,hug_text[rand()%num_hug_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "hi ") != -1)		 sprintf(text,hello_text[rand()%num_hello_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "hello") != -1)		 sprintf(text,hello_text[rand()%num_hello_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "hey") != -1)		 sprintf(text,hello_text[rand()%num_hello_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "look") != -1)   	 sprintf(text,look_text[rand()%num_look_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "lick") != -1)           sprintf(text,lick_text[rand()%num_lick_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "kick") != -1)           sprintf(text,kick_text[rand()%num_kick_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "kiss") != -1)           sprintf(text,kiss_text[rand()%num_kiss_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "poke") != -1)           sprintf(text,poke_text[rand()%num_poke_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "pinch") != -1)          sprintf(text,pinch_text[rand()%num_pinch_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "sing") != -1)           sprintf(text,sing_text[rand()%num_sing_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "topic") != -1)          sprintf(text,topic_text[rand()%num_topic_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "tickle") != -1)         sprintf(text,tickle_text[rand()%num_tickle_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "talk") != -1)           sprintf(text,talk_text[rand()%num_talk_text],bot_bot->color,bot_bot->name,name);
        else if (instr (inpstr, "wave") != -1)           sprintf(text,wave_text[rand()%num_wave_text],bot_bot->color,bot_bot->name,name);
/*      
	else if (instr (inpstr, "bot") != -1)            sprintf(text,bot_text[rand()%num_bot_text],bot_bot->color,bot_bot->name,name);
*/	
	else return;
	write_room(user->room,text);
	return;
	}
}

