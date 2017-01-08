write_file(user)
UR_OBJECT user;
{
char filename[80];

sprintf(filename,"logfiles/system_temp.%s",user->name);
switch(more(user,user->socket,filename)) {
	case 0: write_user(user,"There is no info.\n"); break;
	case 1: user->misc_op=2;
	}
}

/*** Use system's finger command to get info on troublesome user ***/
regnif(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
   char filename[80];
   if (!strcmp(inpstr,"-r")) { 
	write_file(user);
	return;
	}
   if (strpbrk(inpstr,";$/+*[]\\") ) {
        write_user(user,"Illegal character in address\n");
        return;
        }
   if ((!strlen(inpstr)) || (strlen(inpstr) < 8)) {
       write_user(user,"You need to specify address in form: user@host\n");
       return;
       }
   if (strlen(inpstr) > 41) {
       write_user(user,"Address specified too long. No greater than 41 chars.\n");
       return;
       }
sprintf(filename,"logfiles/system_temp.%s",user->name);
sprintf(text,"finger %s > %s 2> /dev/null",inpstr,filename);
system(text);
write_user(user,"Done.\n");
}

/*** Use system's nslookup command to resolve an ip address ***/
pukoolsn(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
   char filename[80];
   if (!strcmp(inpstr,"-r")) {
        write_file(user);
        return;
        }
   if (strpbrk(inpstr,";$/+*[]\\") ) {
        write_user(user,"Illegal character in ip address\n");
        return;
        }
   if ((!strlen(inpstr)) || (strlen(inpstr) < 7)) {
       write_user(user,"You need to specify a valid ip address\n");
       return;
       }
   if (strlen(inpstr) > 25) {
       write_user(user,"Address specified too long. No greater than 25 chars.\n");
       return;
       }
sprintf(filename,"logfiles/system_temp.%s",user->name);
sprintf(text,"nslookup %s > %s 2> /dev/null",inpstr,filename);
system(text);
write_user(user,"Done.\n");
}

/*** Use system's whois command to find what a domain name is ***/
siohw(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
   char filename[80];
   if (!strcmp(inpstr,"-r")) {
        write_file(user);
        return;
        }   
   if (strpbrk(inpstr,";:$/+*[]\\") ) {
        write_user(user,"Illegal character in search string\n");
        return;
        }
   if ((!strlen(inpstr)) || (strlen(inpstr) < 3)) {
       write_user(user,"You need to specify a valid search string\n");
       return;
       }
   if (strlen(inpstr) > 45) {
       write_user(user,"String specified too long. No greater than 45 chars.\n");
       return;
       }
sprintf(filename,"logfiles/system_temp.%s",user->name);
sprintf(text,"fwhois %s > %s 2> /dev/null",inpstr,filename);
system(text);
write_user(user,"Done.\n");
}
