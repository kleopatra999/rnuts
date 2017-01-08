/*** Get user struct pointer from name ***/
UR_OBJECT get_the_user(name)
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


nerf(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
RM_OBJECT rm;
UR_OBJECT u;

if (word_count<2) {
        write_user(user,"Usage: .nerf <person>.\n");
        return; 
        }
if (strstr(user->room->name,NERFROOM)) {
	if (!(u=get_the_user(word[1]))) {
		write_user(user,notloggedon);  return;
        	}
        if (u==user) {
                write_user(user,"Why nerf yourself?!\n");  return;
        	}
        if (!(user->room==u->room)) {
                write_user(user,"That user isn't in this room.\n");
                return;
            	}
        if (user->nerfcharge<1) { 
                write_user(user,"You have no nerf energy\n");
                return;
         	}
        if (u->nerflife-- && user->nerfcharge>0) {	
		if ((rand() % 10) >= 5) {  
	                write_user(user,"You nerf your target.\n");
     	                sprintf(text,"%s is nerfed by %s.\n",u->name,user->name);
             	        write_room_except(u->room,text,user);
              	        user->nerfcharge--;
               	        return;
           		}
		else {
			write_user(user,"You miss your target.\n");
			sprintf(text,"%s's nerf misses %s.\n",user->name,u->name);
			write_room_except(u->room,text,user);
			user->nerfcharge--;
			return;
			}
		}
        if (u->nerflife<1) {
                write_user(u,"Sorry, you have died.\n");
                sprintf(text,"%s is nerfed to death by %s.\n",u->name,user->name);
                write_room_except(user->room,text,u);
                user->nerf_win++;
                u->nerf_lose++;
                u->nerflife=10;
		disconnect_user(u);
                return;
                }
	}
else write_user(user,"Not in here!\n");
}


/** charge **/
charge(user)
UR_OBJECT user;
{
RM_OBJECT rm;
                 
if (!(strstr(user->room->name,NERFROOM))) {
	sprintf(text,"You must be in the %s to recharge.\n",NERFROOM);
	write_user(user,text);
	return;
        }
user->nerfcharge=5;
write_user(user,"You recharge your nerfgun.\n");
sprintf(text,"%s recharges their nerfgun.\n",user->name);
user->nerfcharge=5;
write_room_except(user->room,text,user);  return;
}
