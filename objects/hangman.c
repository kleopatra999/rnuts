
/* lets a user start, stop or check out their status of a game of hangman */
play_hangman(user)
UR_OBJECT user;
{
int i;
char *get_hang_word();

if (word_count<2) { 
	write_user(user,"Usage: hangman [start/stop/status]\n");
	return;
	}
srand(time(0));
strtolower(word[1]);
i=0;
if (!strcmp("status",word[1])) {
	if (user->hang_stage==-1) {
		write_user(user,"You haven't started a game of hangman yet.\n");
		return;
		}
	write_user(user,"Your current hangman game status is:\n");
	if (strlen(user->hang_guess)<1)
		sprintf(text,hanged[user->hang_stage],user->hang_word_show,"None yet!");
	else sprintf(text,hanged[user->hang_stage],user->hang_word_show,user->hang_guess);
	write_user(user,text);
	write_user(user,"\n");
	return;
	}
if (!strcmp("stop",word[1])) {
	if (user->hang_stage==-1) {
		write_user(user,"You haven't started a game of hangman yet.\n");
		return;
		}
	user->hang_stage=-1;
	user->hang_word[0]='\0';
	user->hang_word_show[0]='\0';
	user->hang_guess[0]='\0';
	write_user(user,"You stop your current game of hangman.\n");
	return;
	}
if (!strcmp("start",word[1])) {
	if (user->hang_stage>-1) {
		write_user(user,"You have already started a game of hangman.\n");
		return;
		}
	strtolower(get_hang_word(user->hang_word));
	strcpy(user->hang_word_show,user->hang_word);
 	for (i=0;i<strlen(user->hang_word_show);++i) user->hang_word_show[i]='-';
	user->hang_stage=0;
	write_user(user,"Your current hangman game status is:\n\n");
	sprintf(text,hanged[user->hang_stage],user->hang_word_show,"None yet!");
	write_user(user,text);
	return;
	}
write_user(user,"Usage: hangman [start/stop/status]\n");
}



/* returns a word from a list for hangman.
   this will save loading words into memory, and the list could be updated as and when
   you feel like it */
char *get_hang_word(aword)
char *aword; 
{
char filename[80];
FILE *fp;
int lines,cnt,i;

lines=cnt=i=0;
sprintf(filename,HANGDICT);
lines=count_lines(filename);
srand(time(0));
cnt=rand()%lines;

if (!(fp=fopen(filename,"r"))) return("hangman");
fscanf(fp,"%s\n",aword);
while (!feof(fp)) {
	if (i==cnt) {
		fclose(fp);
		return aword;
		}
	++i;
	fscanf(fp,"%s\n",aword);
	}
fclose(fp);
/* if no word was found, just return a generic word */
return("hangman");
}


/* counts how many lines are in a file */
int count_lines(filename)
char *filename;
{
int i,c;
FILE *fp;

i=0;
if (!(fp=fopen(filename,"r"))) return i;
c=getc(fp);
while (!feof(fp)) {
	if (c=='\n') i++;
	c=getc(fp);
	}
fclose(fp);
return i;
}


/* Lets a user guess a letter for hangman */
guess_hangman(user)
UR_OBJECT user;
{
int count,i,blanks;

count=blanks=i=0;
if (word_count<2) {
	write_user(user,"Usage: guess <letter>\n");
	return;
	}
if (user->hang_stage==-1) {
	write_user(user,"You haven't started a game of hangman yet.\n");
	return;
	}
if (strlen(word[1])>1) {
	write_user(user,"You can only guess one letter at a time!\n");
	return;
	}
strtolower(word[1]);
if (strstr(user->hang_guess,word[1])) {
	user->hang_stage++;
	write_user(user,"You have already guessed that letter!  And you know what that means...\n\n");
	sprintf(text,hanged[user->hang_stage],user->hang_word_show,user->hang_guess);
	write_user(user,text);
	if (user->hang_stage>=7) {
		write_user(user,"~FR~OLUh-oh!~RS  You couldn't guess the word and died!\n");
		sprintf(text,"The word was: ~OL%s\n",user->hang_word);
		write_user(user,text);
		user->hang_lose++;
		user->hang_stage=-1;
		user->hang_word[0]='\0';
		user->hang_word_show[0]='\0';
		user->hang_guess[0]='\0';
		}
	write_user(user,"\n");
	return;
	}
for (i=0;i<strlen(user->hang_word);++i) {
	if (user->hang_word[i]==word[1][0]) {
		user->hang_word_show[i]=user->hang_word[i];
		++count;
		}
	if (user->hang_word_show[i]=='-') ++blanks;
	}
strcat(user->hang_guess,word[1]);
if (!count) {
	user->hang_stage++;
	write_user(user,"That letter isn't in the word!  And you know what that means...\n");
	sprintf(text,hanged[user->hang_stage],user->hang_word_show,user->hang_guess);
	write_user(user,text);
	if (user->hang_stage>=7) {
    		write_user(user,"~FR~OLUh-oh!~RS  You couldn't guess the word and died!\n");
    		sprintf(text,"The word was: ~OL%s\n",user->hang_word);
    		write_user(user,text);
		user->hang_lose++;
    		user->hang_stage=-1;
    		user->hang_word[0]='\0';
    		user->hang_word_show[0]='\0';
    		user->hang_guess[0]='\0';
    		}
  	write_user(user,"\n");
  	return;
  	}
if (count==1) sprintf(text,"Well done!  There was 1 occurrence of the letter %s\n",word[1]);
else sprintf(text,"Well done!  There were %d occurrences of the letter %s\n",count,word[1]);
write_user(user,text);
sprintf(text,hanged[user->hang_stage],user->hang_word_show,user->hang_guess);
write_user(user,text);
if (!blanks) {
  	write_user(user,"~FY~OLCongratz!~RS  You guessed the word without dying!\n");
	user->hang_win++;
  	user->hang_stage=-1;
  	user->hang_word[0]='\0';
  	user->hang_word_show[0]='\0';
  	user->hang_guess[0]='\0';
  	}
}


