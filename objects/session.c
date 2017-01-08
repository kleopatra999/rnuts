session(user,inpstr)
UR_OBJECT user;
char* inpstr;
{
FILE *sessionfile;
int mins,secs,total,ret,hrs;
if(word_count<2){
  if(thesession[0]!='\0'){
    sprintf(text,"The Session is:  %s\n",thesession);
    write_user(user,text);
    write_user(user,"----------------------------------------------------------------------\n");
    if (!(ret=more(user,user->socket,"logfiles/session")))
     write_user(user,"No one has .comment'ed yet.\n\n");
    else if (ret==1) user->misc_op=2;
    return;
  }
  else{
    sprintf(text,"No session exists.  Would you like to set it?\n");
    write_user(user,text);
    return;  }
}
if(!strcmp(word[1],"-w")){
  if(thesession[0]!='\0'){
  total=time(0)-sessiontime;
  secs=total%60;
  mins=(total-secs)/60;
  if(mins>=60) {
    hrs=mins%60;
    mins=mins-(hrs*60);
    sprintf(text,"The session was set by %s, %d hours, %d minutes %d seconds ago.\n",whosession,hrs,mins,secs);
  } else if(mins<60)
  sprintf(text,"The session was set by %s, %d minutes %d seconds ago.\n",whosession,mins,secs);
  }
  else
   sprintf(text,"No session exists.  Would you like to set it?\n");
  write_user(user,text);
  return;
}
if(time(0) < sessiontime+600 && strcmp(user->name,whosession)){
  total=(sessiontime+600)-time(0);
  secs=total%60;
  mins=(total-secs)/60;
  sprintf(text,"The current session can be reset in: %d minutes and %d seconds.\n",mins,secs);
  write_user(user,text);
  return;
}
if(strlen(inpstr) > 100){
  write_user(user,"Session can only be 100 characters big\n");
  return;
}
strcpy(thesession,inpstr);
unlink("logfiles/session");
sprintf(text,"~OL>>~RS %s has set the session to: %s\n",user->name,inpstr);
write_room(NULL,text);
strcpy(whosession,user->name);
sessiontime=time(0);
}


comment(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
FILE *fp;
int total_len,cur_len,line_pos;
if(thesession[0]=='\0'){
  write_user(user,"No session exists.  Maybe you'd like to set it?\n");
  return;
}
if(word_count<2){   
  write_user(user,"USAGE:  .comment <your comment>\n");
  return;
}
fp=fopen("logfiles/session","a");
sprintf(text,"%-*s : %s\n\r",20,user->name,inpstr);
line_pos=0;
for(cur_len=0,total_len=strlen(text);cur_len<total_len;cur_len++,line_pos++){
  putc(text[cur_len],fp);
  if(line_pos%76==0 && line_pos!=0){
    putc('\n',fp);  putc('\r',fp);
    fprintf(fp,"                       ");
    line_pos=23;
  }
}
fclose(fp);

sprintf(text,"You comment to the session: %s\n",inpstr);
write_user(user,text);
}
