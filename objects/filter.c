filter_site(user) 
UR_OBJECT user;
{
   FILE *fp;
   int filter=0;
   char line[82],filename[80];
   char name[80], site[80], real[80];
   int num_matches;

   sprintf(filename,"%s/%s",LOGFILES,"users_sitefilter");
   if ( (fp=fopen(filename, "r")) == NULL) {
	sprintf(text,"Can't open file <%s> in filter.c:filter_site\n",filename); 	
	write_log(text,0,2);
	return;
   }
   while(!feof(fp)) {
        fgets(line,81,fp); 
 	num_matches = sscanf(line,"%s %s ",name,site); 
        if (num_matches < 2)
           continue;
        if (name[0]=='#') /* skip comment lines */ 
		continue;
	if ( strcmp(name,user->name) == 0) {
	   strcpy(real,user->site);
	   strcpy(user->site,site);
	   filter=1;
	   break;
         }
   }
   fclose(fp);
   if (filter) {
      sprintf(text,"Filter site check  : %-11s %s\n",user->name,real);
      write_log(text,1,2);
   }
}

filter_level(user) 
UR_OBJECT user;
{
   FILE *fp;
   int filter=0,num_matches,levelchk=0;
   char line[82],filename[80];
   char name[80], lvl[80];

   sprintf(filename,"%s/%s",LOGFILES,"users_levelfilter");
   if ( (fp=fopen(filename, "r")) == NULL) {
	sprintf(text,"Can't open file <%s> in filter.c:filter_level\n",filename); 	
	write_log(text,0,2);
	return;
   }
   while(!feof(fp)) {
        fgets(line,81,fp); 
 	num_matches = sscanf(line,"%s %s",name,lvl); 
        if (num_matches < 2)
           continue;
        if (name[0]=='#') /* skip comment lines */ 
		continue;
	if (!strcmp(name,user->name)) {
		if (!strcmp(lvl,who_wiz[BOT])) user->level=SEV+1;
		else --user->level;
		if (!strcmp(lvl,who_wiz[EIG])) { user->level=EIG+1; --user->level; }
		if (!strcmp(lvl,who_wiz[SEV])) { user->level=SEV+1; --user->level; }
		if (!strcmp(lvl,who_wiz[SIX])) { user->level=SIX+1; --user->level; }
		if (!strcmp(lvl,who_wiz[FIV])) { user->level=FIV+1; --user->level; }
		if (!strcmp(lvl,who_wiz[FOU])) { user->level=FOU+1; --user->level; }
		if (!strcmp(lvl,who_wiz[THR])) { user->level=THR+1; --user->level; }
		if (!strcmp(lvl,who_wiz[TWO])) { user->level=TWO+1; --user->level; }
		if (!strcmp(lvl,who_wiz[ONE])) { user->level=ONE+1; --user->level; }
		if (!strcmp(lvl,who_wiz[ZER])) { user->level=ZER+1; --user->level; }
		filter=1;
		levelchk=1;
		break;
		}
	}
   if ((!levelchk)&&(user->level>=FOU)) { 
	sprintf(text,"~OL~FYFailed level check :~RS %-11s %s\n",user->name,user->level[who_wiz]);
	write_log(text,1,2);
	user->level=THR+1; --user->level; 
	}
   fclose(fp);
   if (filter) {
	sprintf(text,"Filter level check : %-11s %s\n",user->name,user->level[who_wiz]);
	write_log(text,1,2);
	}
}


filter_passwd(user)
UR_OBJECT user;
{
sprintf(text,"~OL~FRFailed passwd check: ~RS%-11s %s\n",user->name,user->site);
write_log(text,1,2);
}


filter(user,passwd)
UR_OBJECT user;
int passwd;
{

if (passwd) filter_passwd(user);
else {
	filter_site(user);
	filter_level(user);
	}
}

