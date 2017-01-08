/* This is the 'logic' ;) for playing tictactoe with the BOT -- RobinHood */
/* As it stands, the BOT's move is determined by a random number.  Later, */
/* I will try to make the move actually 'logical', in that it will(and    */
/* should) attempt to block the user attempt at winning.                  */  
tic_bot(user)
UR_OBJECT user;
{
int bot_move_check = 0;

if (!win_tic(user->array)) {
	do
	{
		check_position(pos_array);

		if ((user->opponent->array[bot_move] == 1) || (!bot_check)) {
                	bot_move = (rand() % 9);
 		}
                bot_move_check = legal_tic(user->opponent->array,bot_move+1,1);
	}
	while (!bot_move_check);

        sprintf(text,"\n~RS~OL%s moves in square: %d\n\n",user->opponent->name,bot_move+1);
        write_user(user,text);

	user->array[bot_move] = 2;
	user->opponent->array[bot_move] = 1;
	print_tic(user);

	if (!win_tic(user->array)) return;

        if (win_tic(user->array) == 1) {
                sprintf(text,"~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->name,user->opponent->name);
		user->tic_win++;
		user->opponent->tic_lose++;
                }
        else if (win_tic(user->array) == 2) {
                sprintf(text,"~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->opponent->name,user->name);
		user->opponent->tic_win++;
		user->tic_lose++;
                }
        else {
                sprintf(text,"~OL~FBCat's game between %s and %s.\n",user->name,user->opponent->name);
		user->tic_draw++;
		user->opponent->tic_draw++;
                }

        write_room(user->room,text);
        strcpy(user->array,"000000000");
        strcpy(user->opponent->array,"000000000");
        user->first = 0;
        user->opponent->first = 0;
        user->opponent->opponent = NULL;
        user->opponent = NULL;
	game_with_bot = 0;
	game_on = 0;
	strcpy(pos_array,"000000000");
}
else {
	if (win_tic(user->array) == 1) {
		sprintf(text,"~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->name,user->opponent->name);
		user->tic_win++;
		user->opponent->tic_lose++;
		}
	else if (win_tic(user->array) == 2) {
		sprintf(text,"~OL~FB%s has beaten %s at Tic Tac Toe.\n",user->opponent->name,user->name);
		user->opponent->tic_win++;
		user->tic_lose++;
		}
	else {
		sprintf(text,"~OL~FBCat's game between %s and %s.\n",user->name,user->opponent->name);
		user->tic_draw++;
		user->opponent->tic_draw++;
		}

	write_room(user->room,text);
	strcpy(user->array,"000000000");
	strcpy(user->opponent->array,"000000000");
	user->first = 0;
	user->opponent->first = 0;
	user->opponent->opponent = NULL;
	user->opponent = NULL;
	game_with_bot = 0;
	strcpy(pos_array,"000000000");
	game_on = 0;
	}
}

/* Checks the pos_array to see if the bot is in danger of losing.  If he
is, try to counter the move, if possible. There has GOT to be an easier
way to do this ;) */

check_position(char *game_array)
{
	bot_check = 0;

/* Possible Horizontal winning positions */
	if ((!strcmp(game_array,"XX0000000")) && (game_array[2] != 'B'))
	{
		bot_move = 2;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"0XX000000")) && (game_array[0] != 'B'))
	{
		bot_move = 0;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"X0X000000")) && (game_array[1] != 'B'))
	{
		bot_move = 1;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"000XX0000")) && (game_array[5] != 'B'))
	{
		bot_move = 5;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"0000XX000")) && (game_array[3] != 'B'))
	{
		bot_move = 3;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"000X0X000")) && (game_array[4] != 'B'))
	{
		bot_move = 4;
		bot_check = 1;
		return;
	}

	if ((!strcmp(game_array,"000000XX0")) && (game_array[8] != 'B'))
	{
		bot_move = 8;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"0000000XX")) && (game_array[6] != 'B'))
	{
		bot_move = 6;
		bot_check = 1;
		return;
	}
	if ((!strcmp(game_array,"000000X0X")) && (game_array[7] != 'B'))
	{
		bot_move = 7;
		bot_check = 1;
		return;
	}

/* Possible vertical winning positions */
        if ((!strcmp(game_array,"X00X00000")) && (game_array[6] != 'B'))
	{
		bot_move = 6;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"000X00X00")) && (game_array[0] != 'B'))
        {
	        bot_move = 0;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"X00000X00")) && (game_array[3] != 'B'))
        {
	        bot_move = 3;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"0X00X0000")) && (game_array[7] != 'B'))
        {
	        bot_move = 7;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"0000X00X0")) && (game_array[1] != 'B'))
        {
	        bot_move = 1;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"0X00000X0")) && (game_array[4] != 'B'))
        {
	        bot_move = 4;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"00X00X000")) && (game_array[8] != 'B'))
        {
	        bot_move = 8;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"00000X00X")) && (game_array[2] != 'B'))
        {
	        bot_move = 2;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"00X00000X")) && (game_array[5] != 'B'))
        {
	        bot_move = 5;
		bot_check = 1;
		return;
	}

/* Possible diagonal winning positions */
        if ((!strcmp(game_array,"X000X0000")) && (game_array[8] != 'B'))
        {
	        bot_move = 8;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"0000X000X")) && (game_array[0] != 'B'))
        {
	        bot_move = 0;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"X0000000X")) && (game_array[4] != 'B'))
        {
	        bot_move = 4;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"00X0X0000")) && (game_array[6] != 'B'))
        {
	        bot_move = 6;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"0000X0X00")) && (game_array[2] != 'B'))
        {
	        bot_move = 2;
		bot_check = 1;
		return;
	}
        if ((!strcmp(game_array,"00X000X00")) && (game_array[4] != 'B'))
        {
	        bot_move = 4;
		bot_check = 1;
		return;
	}

/* Other winning position checks */
        if ((!strcmp(game_array,"BXX0X00B0")) && (game_array[6] != 'B'))
        {
                bot_move = 6;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"BX00X0XB0")) && (game_array[2] != 'B'))
        {
                bot_move = 2;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"BX0XX00B0")) && (game_array[5] != 'B'))
        {
                bot_move = 5;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"BX00XX0B0")) && (game_array[3] != 'B'))
        {
                bot_move = 3;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"B0XXX0B00")) && (game_array[5] != 'B'))
        {
                bot_move = 5;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"B0X0XXB00")) && (game_array[3] != 'B'))
        {
                bot_move = 3;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"BXXXXBB00")) && (game_array[7] != 'B'))
        {
                bot_move = 7;
                bot_check = 1;
                return;
        }
        if ((!strcmp(game_array,"B0XXXBBX0")) && (game_array[1] != 'B'))
        {
                bot_move = 1;
                bot_check = 1;
                return;
        }
}
