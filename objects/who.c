/*** Show who is on ***/
who(user,people)
UR_OBJECT user;
int people;
{
UR_OBJECT u;
int cnt,total,invis,mins,remote,idle,logins;
char name[USER_NAME_LEN+4];
char rname[ROOM_NAME_LEN+1],portstr[5],idlestr[6],sockstr[3];

total=0;  invis=0;  remote=0;  logins=0;
write_user(user,"~FT+-----------------------------------------------------------------------------+~RS\n");
sprintf(text,"~FT    Current users logged in on: %s\n",long_date(1));
write_user(user,text);
write_user(user,"~FT+-----------------------------------------------------------------------------+~RS\n");
if (people) write_user(user,"~FTName       : Level      Line Ignall Visi Idle Mins  Port  Site/Service\n\n\r");
/*                   @kingsCastle 0000 ACTV 0000 MAGI *@Willscarlet Rob's bro - sorta        */
else write_user(user,"~FT  Room        Mins/Stat/Idle Level  Name        Desc ~RS\n\n");

for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE) continue;
        mins=(int)(time(0) - u->last_login)/60;
        idle=(int)(time(0) - u->last_input)/60;
        if (u->type==REMOTE_TYPE) strcpy(portstr,"   -");
        else {
                if (u->port==port[0]) strcpy(portstr,"MAIN");
                else strcpy(portstr," WIZ");
                }
        if (u->login) {
                if (!people) continue;
                sprintf(text,"~FY[Login %2d] : %-10s %4d %6s %4s %4d %4s %5s %s:%d\n",4 - u->login,"-",u->socket,"-","-",idle,"-",portstr,u->site,u->site_port);
                write_user(user,text);
                logins++;
                continue;
                }
        ++total;
        if (u->type==REMOTE_TYPE) ++remote;
        if (!u->vis) {
                ++invis;
                if (u->level>user->level) continue;
                }
        if (people) {   
                if (u->afk) strcpy(idlestr," AFK");
                else sprintf(idlestr,"%4d",idle);
                if (u->type==BOT_TYPE) strcpy(idlestr,"   -");
                if (u->type==REMOTE_TYPE) strcpy(sockstr," -");
                else sprintf(sockstr,"%2d",u->socket);
                sprintf(text,"%-10s : %-10s   %s    %s  %s %s %4d  %s %s\n",u->name,level_name[u->level],sockstr,noyes1[u->ignall],noyes1[u->vis],idlestr,mins,portstr,u->site);
                write_user(user,text);   
                continue;
                }
	if ((!u->vis)&&(u->type==REMOTE_TYPE)) sprintf(name,"*@%s",u->name);
        else if (!u->vis) sprintf(name,"* %s",u->name);
        else if (u->type==REMOTE_TYPE) sprintf(name," @%s",u->name);
	else sprintf(name,"  %s",u->name);
        if (u->room==NULL) sprintf(rname,"@%s",u->netlink->service);
        else sprintf(rname," %s",u->room->name);
        if (u->afk) strcpy(idlestr,"AFK ");
        else if (idle<1) strcpy(idlestr,"ACTV");
	else if (idle==1) strcpy(idlestr,"AWKE");
	else if (idle>1) strcpy(idlestr,"IDLE");
	if (u->inedit) strcpy(idlestr,"EDIT"); 
        if (u->type==BOT_TYPE) { strcpy(idlestr,"AWKE"); idle=0; }
	if (user->level>=FOU)	
		sprintf(text," %-12s %4d %s %-4d %s %s%-13s %-24s\n",rname,mins,idlestr,idle,u->level[who_wiz],u->color,name,u->desc);
	else if (u->name==user->name)
		sprintf(text," %-12s %4d %s %-4d ~OL~FR%s~RS %s%-13s %-24s\n",rname,mins,idlestr,idle,u->level[who_wiz],u->color,name,u->desc);
	else sprintf(text," %-12s %4d %s %-4d %s %s%-13s %-24s\n",rname,mins,idlestr,idle,u->level[who_user],u->color,name,u->desc);
        write_user(user,text);
        }
sprintf(text,"\n~FTThere are %d visible, %d invisible, %d remote users.\n~FTTotal of %d users",num_of_users-invis,invis,remote,total);
if (people) sprintf(text,"%s and %d logins.\n\n",text,logins);
else strcat(text,".\n\n");
write_user(user,text);
}
