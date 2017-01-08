tictac(user)
UR_OBJECT user;
{
char temp_s[81];
int move;
UR_OBJECT u;

if ((word_count<2)||(!strcmp(word[1],""))) {
        write_user(user,"Play Tic Tac Toe with who?\n");
        return;
        }
if (!isdigit(word[1][0])) {
	if (game_with_bot) {
		word[1][0]=toupper(word[1][0]); 
        	if (strstr("Blinkin",word[1])) {
			sprintf(text,"~OL~FBBlinkin is already involved in a game.\n");
        		write_user(user,text);
			return;
			}
	}
	u = get_opp(word[1]);
	if (u == bot_bot) {
		if (bot_bot != NULL) {
			game_with_bot = 1;
			game_on = 1;
			sprintf(text,"~OL~FB%s has accepted your TicTacToe game.\n",u->name);
			write_user(user,text);
                	sprintf(temp_s,"~OL~FB%s has started playing Tic Tac Toe with %s\n",user->name,u->name);
                	write_room(user->room,temp_s);
			user->opponent = u;
			user->opponent->opponent = user;
			strcpy(user->array,"000000000");
			strcpy(pos_array,"000000000");
			print_tic(user);
			return;
			}
		else {
			write_user(user,"~OL~FBBlinkin is not here at the moment.\n");
			return;
			}
		}
	if (u == NULL) {
        	write_user(user,notloggedon);
        	return;
        	}
	if (u==user) {
		write_user(user,"Sorry you can't play yourself.\n"); return;
		}
	user->opponent=u;
	if (user->opponent->opponent) {
                if (user->opponent->opponent!=user) {
                        write_user(user, "Sorry, that person is already engaged in Tic Tac Toe.\n");
                        return;
                        }
                if (user->opponent->level<ONE) {
                        write_user(user,"That user doesn't have that command.\n");
                        return;
                        }
		game_on = 1;
                sprintf(temp_s, "%s has accepted your Tic Tac Toe game.\n",user->name);
                write_user(user->opponent, temp_s);
                sprintf(temp_s, "You accept a game of Tic Tac Toe with %s\n",user->opponent->name);
                write_user(user,temp_s);
                sprintf(temp_s,"~OL~FB%s has started playing Tic Tac Toe with %s\n",user->name,user->opponent->name);
                write_room(user->room,temp_s);
                print_tic(user); print_tic(user->opponent);
                return;
                }
else {
         sprintf(temp_s, "~OL~FB%s wants to play Tic Tac Toe with you.\n",user->name);   
         write_user(user->opponent,temp_s);
         sprintf(temp_s,"You ask %s to a game of Tic Tac Toe.\n",user->opponent->name);
         write_user(user,temp_s);
         return;
         }
}   
if (!user->opponent) {
        write_user(user,"You are not playing with anyone.\n");
        return;
        }
if (user->opponent->opponent!=user) {
        write_user(user,"That user has not accepted yet.\n");
        return;
        }
if (strcmp(user->array,"000000000")==0&&!user->opponent->first) {
        user->first=1; 
        }
move=word[1][0]-'0';
if (legal_tic(user->array,move,user->first)) {
        user->array[move-1] = 1;
        user->opponent->array[move-1] = 2;
	pos_array[move-1] = 'X';
        print_tic(user);
	if (game_with_bot) {
		tic_bot(user);
		return;
		}
        print_tic(user->opponent);
        }
else {
        write_user(user,"That is not a legal move.\n");
        return;
        }   
if (!win_tic(user->array)) return;
if (win_tic(user->array)==1) {
        sprintf(temp_s, "~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->name,user->opponent->name);
	user->tic_win++;
	user->opponent->tic_lose++;
        }
else if (win_tic(user->array) == 2) {
        sprintf(temp_s, "~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->opponent->name, user->name);
	user->opponent->tic_win++;
	user->tic_lose++;
        }
else {
        sprintf(temp_s,"~OL~FBCat's game between %s and %s.\n",user->name,user->opponent->name);;
	user->tic_draw++;
	user->opponent->tic_draw++;
       }   
/* print_tic(user);
print_tic(user->opponent);
*/
write_room(user->room, temp_s);
strcpy(user->array, "000000000");
strcpy(user->opponent->array,"000000000");
user->first=0;
game_on = 0;
user->opponent->first=0;
user->opponent->opponent=NULL;
user->opponent=NULL;
if (game_with_bot) {
	strcpy(pos_array,"000000000");
	game_with_bot = 0;
	}
}

legal_tic(char *array,int move,int first) {          
int count1=0,count2=0;
int i;
       
if (array[move-1]==1||array[move-1]==2) return 0;
for (i=0;i<9;i++) {
        if (array[i]==1) count1++;
        else if (array[i]==2) count2++;
        }
if (count1>count2) return 0;
if (first!=1)
        if (count1==count2) return 0; 
return 1;
}

win_tic(char *array) {
int i,j;
int person;
    
for (person=1;person<3;person++) {        
        for (i=0;i<3;i++) 
                for (j=0;j<3;j++) {
                        if (array[i*3+j]!=person) break;
                        if (j==2) return person;
                        }
        for (i=0;i<3;i++) 
                for (j=0;j<3;j++) {
                        if (array[j*3+i]!=person) break;
                        if (j==2) return person;
                        }  
        if (array[0]==person&&array[4]==person&&array[8]==person)
                return person;
        if (array[2]==person&&array[4]==person&&array[6]==person)
                return person;
        }
for (i=0,j=0;i<9;i++) {
        if (array[i]==1||array[i]==2) j++;
        }
if (j==9) return 3;
return 0;
}

print_tic(user)
UR_OBJECT user;
{          
char temp_s[81];
char array[10];
int i;
          
for (i = 0; i < 9; i++) {
        if (user->array[i] == 1)
                array[i] = 'X';
        else if (user->array[i] == 2)
                array[i] = 'O';
        else   
                array[i] = ' ';
        }
write_user(user,"\n");
write_user(user,"~FM1~RS     ~OL |~RS~FM2~RS      ~OL|~RS~FM3       \n");
sprintf(temp_s, "   %c  ~OL |~RS   %c  ~OL |~RS   %c   \n",array[0],array[1],array[2]);
write_user(user,temp_s);
write_user(user,"~OL_______|_______|_______\n");
write_user(user,"~FM4~RS     ~OL |~RS~FM5~RS     ~OL |~RS~FM6       \n");
sprintf(temp_s, "   %c   ~OL|~RS   %c   ~OL|~RS   %c \n",array[3],array[4],array[5]);
write_user(user,temp_s);
write_user(user,"~OL_______|_______|_______\n");
write_user(user,"~FM7~RS      ~OL|~RS~FM8~RS      ~OL|~RS~FM9       \n");
sprintf(temp_s, "   %c   ~OL|~RS   %c   ~OL|~RS   %c \n",array[6],array[7],array[8]);
write_user(user,temp_s);
write_user(user,"       ~OL|~RS       ~OL|~RS       \n");
write_user(user,"\n");
}
 
reset(user)
UR_OBJECT user;
{          
if (game_on) {
	sprintf(text,"~OL~FBTic-tac-toe between %s and %s has now been reset.\n",user->name,user->opponent->name);
	write_room(user->room,text);
	strcpy(user->array,"000000000");
	strcpy(user->opponent->array,"000000000");
	user->first = 0;
	user->opponent = 0;
	game_on = 0;
	if (game_with_bot) {
		strcpy(pos_array,"000000000");
		game_with_bot = 0;
		}
	}
else {
	write_user(user,"~OL~FBYou have no game in progress.\n");
	return;
	}
}

