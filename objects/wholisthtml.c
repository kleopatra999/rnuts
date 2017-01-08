/*
	This function uses the who() command to create a file of current
	users.  This file is then moved into the public_html via a system call,
	and is made available(chmod 644) to view from the web.
				RobinHood  --  December 1997
	Modified to write directly to the web directory, because the
	system callls were spawning zombie processes.
				RobinHood  --  January 1998
*/

wholist(user,people)
UR_OBJECT user;
int people;
{
UR_OBJECT u;
int cnt,total,invis,mins,remote,idle,logins;
char name[USER_NAME_LEN+4];
char rname[ROOM_NAME_LEN+1],portstr[5],idlestr[6],sockstr[3];
FILE *fp;
char filename[80],sys[80],*colour_com_strip();

total=0;  invis=0;  remote=0;  logins=0;

sprintf(filename,"/home/rnuts/WWW/%s.html",WHOHTML);

if (!(fp=fopen(filename,"w"))) {
	sprintf(text,"~FRFAILED to save HTML WhoList.\n");
	write_syslog(text,1);
	return 0;
	}
else {
	fprintf(fp,"<html>\n");
        fprintf(fp,"<body background=\"images/back3.gif\">\n");
        fprintf(fp,"<font color=\"#0B8B00\" face=\"MagicMedieval\">\n");
	fprintf(fp,"<center>\n");
	fprintf(fp,"<h2>\n");
        fprintf(fp,"Current users logged in on\n");
	fprintf(fp,"<br>");
	fprintf(fp,"%s\n",long_date(1));
	fprintf(fp,"</h2>\n");
	fprintf(fp,"\n");
	fprintf(fp,"<p>&nbsp</p>\n");
	fprintf(fp,"\n");
        fprintf(fp,"<table border=2>\n");
	fprintf(fp,"<tr>\n");
	fprintf(fp,"	<th><b>Room</b></th>\n");
	fprintf(fp,"	<th><b>Level</b></th>\n");
	fprintf(fp,"	<th><b>Name / Desc</b></th>\n");
	fprintf(fp,"	<th><b>Time On</b></th>\n");
	fprintf(fp,"	<th><b>Status</b></th>\n");
	fprintf(fp,"	<th><b>Idle</b></th>\n");
	fprintf(fp,"</tr>\n");
	fprintf(fp,"\n");
	fprintf(fp,"<tr>\n");
	fprintf(fp,"<font color=\"#0B8B00\" face=\"MagicMedieval\">\n");

	for(u=user_first;u!=NULL;u=u->next) {
        	if (u->type==CLONE_TYPE) continue;
        	mins=(int)(time(0) - u->last_login)/60;
        	idle=(int)(time(0) - u->last_input)/60;
        	if (u->type==REMOTE_TYPE) strcpy(portstr,"   -");
        	else {
                	if (u->port==port[0]) strcpy(portstr,"MAIN");
                	else strcpy(portstr," WIZ");
                }
        	++total;
        	if (u->type==REMOTE_TYPE) ++remote;
        	if (!u->vis) {
                	++invis;
                	if (u->level>user->level) continue;
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
		/* if (user->level>=APPR) */	
			sprintf(text,"	<td>%s</td> <td>%s</td> <td>%s %s</td> <td align=center>%d</td> <td align=center>%s</td> <td align=center>%d</td>\n",rname,u->level[who_wiz],name,colour_com_strip(u->desc),mins,idlestr,idle);

		/* else sprintf(text," %s %d %s %d %s %s %s\n",rname,u->level[who_user],name,colour_com_strip(u->desc),mins,idlestr,idle); */
        	fprintf(fp,text);
		fprintf(fp,"</tr>\n");
        }
	fprintf(fp,"\n");
	fprintf(fp,"<tr>\n");
	sprintf(text,"	<td colspan=6 align=center>There are <b>%d</b> visible, <b>%d</b> invisible, <b>%d</b> remote users.<br>Total of <b>%d</b> users</td>\n",total-invis,invis,remote,total);
	fprintf(fp,text);
	fprintf(fp,"</tr>\n");
	fprintf(fp,"</table>\n");
	fprintf(fp,"<p>&nbsp</p>\n");
	fprintf(fp,"<p>&nbsp</p>\n");
	fprintf(fp,"<P>\n");
	fprintf(fp,"<CENTER>\n");
	fprintf(fp,"<TABLE CELLSPACING=0 CELLPADDING=0 BORDER=5 ALIGN=ABSMIDDLE VALIGN=TOP WIDTH=0>\n");
	fprintf(fp,"<TR>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"index.html\"><IMG SRC=\"images/mainbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"mainbutton.gif\"></A></TD>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"users.html\"><IMG SRC=\"images/usersbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"usersbutton.gif\"></A></TD>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"wizards.html\"><IMG SRC=\"images/wizardbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"wizardbutton.gif\"></A></TD>\n");
	fprintf(fp,"</TR>\n");
	fprintf(fp,"\n");
	fprintf(fp,"<TR>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"mailto:hyde@sherwood.ml.org\"><IMG SRC=\"images/emailbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"emailbutton.gif\"></A></TD>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"about.html\"><IMG SRC=\"images/aboutbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"aboutbutton.gif\"></A></TD>\n");
        fprintf(fp,"	<TD ALIGN=Center><A HREF=\"Terminal2-5/term.html\"><IMG SRC=\"images/chatbutton.gif\" BORDER=0 ALIGN=Middle ALT=\"chatbutton.gif\"></A></TD>\n");
	fprintf(fp,"</TR>\n");
	fprintf(fp,"</TABLE>\n");
	fprintf(fp,"</CENTER>\n");
	fprintf(fp,"</P>\n");
	fprintf(fp,"\n");
	fprintf(fp,"</body>\n");
	fprintf(fp,"</html>\n");
	fclose(fp);
     }
}
