/* blackjack.c file stuff */

PLR_OBJECT players;          	/* Contains the players.    */
void pop_player();	 	/* Removes the first player from the list. */
void push_player();	 	/* Places a player on the head of the list. */
PLR_OBJECT player_turn;	     	/* Player whose turn it is. */
int deck[52];		     	/* Sorted deck of cards.    */
CRD_OBJECT shuffle_deck;       	/* Shuffled deck.           */
CRD_OBJECT pop_card();       	/* Removes the first card from the list. */
CRD_OBJECT push_card();      	/* Places a card on the head of the list. */
void calculate_results();     	/* Calculates the results at end of game. */
void display_hand();		/* Display the user's hand  */
int num_players;              	/* Holds number of players. */
extern RM_OBJECT get_room(char *name);

/* actual game code */
blackjack(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
	PLR_OBJECT temp_plr,curr;
	RM_OBJECT rm;
	CRD_OBJECT temp_crd;
	UR_OBJECT u;
	int i,lineno,cardno,randno,card_total;
	char card_type;

	if (word_count<2) /*show current game stats*/	
	{
		write_user(user,"~OL--- BLACKJACK Stats ---\n\n");
		if (players==NULL)
		{
			write_user(user,"~OLThere is no game in progress.\n");
		}
		else
		{
			write_user(user,"~OLPlayer        Total Wins\n");
			for(temp_plr=players;temp_plr!=NULL;temp_plr=temp_plr->next)
			{
				sprintf(text,"%s %d\n",temp_plr->user->name,temp_plr->user->blackjack_total);
				write_user(user,text);
			}
			write_user(user,"\n~OL------------------------\n");
		}
		write_user(user,"~OLUsage : blackjack hand/join/deal/hit/stand/quit\n");
		return;
	}

	if (user->jailed)
	{
		write_user(user,"~OLSorry, you are in jail.\n");
		return;
	}

	if (!strcmp(word[1],"join"))
	{
		if (!strcmp(user->room->name,BLKJKROOM))
		{
			write_user(user,"~OLYou are already in the game room.\n");
			return;
		}
		if (blackjack_status==1)
		{
			write_user(user,"~OLSorry, there is a game in progress.\n");
			return;
		}
		if (num_players == MAX_PLAYERS)
		{
			write_user(user,"~OLSorry, there is a maximum of 5 players.\n");
			return;
		}
		if (user->muzzled)
		{
			write_user(user,"~OLSorry, you are muzzled.\n");
			return;
		}
		if (user->cursed)
		{
			write_user(user,"~OLSorry, you are cursed.\n");
			return;
		}
		if (user->jailed)
		{
			write_user(user,"~OLSorry, you are in jail.\n");
			return;
		}
		rm=get_room(BLKJKROOM);
		move_user(user,rm,5);
		push_player(user);	
		num_players=num_players+1;
		return;
	}	

	if (!strcmp(word[1],"hand"))
	{
		if (blackjack_status==0)
		{
			write_user(user,"~OLSorry, no game is in progress.\n");
			return;
		}
		if (user->blackjack_status==0)
		{
			write_user(user,"~OLSorry, you are not playing a game.\n");
			return;
		}
		display_hand(user);
		return;
	}		
	if (!strcmp(word[1],"deal"))
	{
		if (strcmp(user->room->name,BLKJKROOM))
		{
			write_user(user,"~OLSorry, you have not joined a game.\n");
			return;
		}    
		if (blackjack_status)
		{
			write_user(user,"~OLSorry, there is a game in progress.\n");
			return;
		}
		write_room(user->room,"~OLShuffling deck...\n");
		for (i=0;i<52;i++) deck[i] = i+1;
		i = 1;
		while (i<=52)
		{
			randno= rand() % 52;
			while (deck[randno] == 0) randno = rand() % 52;
			deck[randno] = 0;
			if (randno<13) card_type = 'h';
			else if (randno<26) card_type = 'd';
			else if (randno<39) card_type = 's';
			else if (randno<52) card_type = 'c';
			card_total = (randno % 13) + 1;
			shuffle_deck = push_card(shuffle_deck,card_total,card_type);
			i++;
		}
		for(temp_plr=players;temp_plr!=NULL;temp_plr=temp_plr->next)
		{
			sprintf(text,"~OLDealing %s's cards.\n",temp_plr->user->name);
			write_room(user->room,text);
			for (i=0;i<2;i++)
			{
				if (shuffle_deck==NULL)
				{
					write_room(user->room,"~OLError : Run out of cards.\n");
					return;
				}
				if ((shuffle_deck->i>10)&&(shuffle_deck->i<14))
					temp_plr->total+=10;
				else
					temp_plr->total+=shuffle_deck->i;
				if (temp_plr->ace)
					temp_plr->total2+=shuffle_deck->i;
				if ((shuffle_deck->i == 1)&&(!(temp_plr->ace)))
				{
					temp_plr->total2+=10;
					temp_plr->total2+=temp_plr->total;
					temp_plr->ace=1;
				}
				temp_plr->cards = push_card(temp_plr->cards,shuffle_deck->i,shuffle_deck->card_type);		
				shuffle_deck = pop_card(shuffle_deck);
			}
			temp_plr->user->blackjack_status = 1;
			display_hand(temp_plr->user);
		}	
		write_room(user->room,"~OLFinished dealing.\n");
		player_turn = players;
		sprintf(text,"~OL%s's turn.\n",player_turn->user->name);
		write_room(user->room,text);
		blackjack_status = 1;
		return;
	}

	if (!strcmp(word[1],"hit"))
	{
		if (!(user->blackjack_status))
		{
			write_user(user,"~OLSorry, you are not playing a game.\n");
			return;
		}
		if (user!=player_turn->user)
		{
			write_user(user,"~OLSorry, it is not your turn.\n");
			return;
		}
		if (shuffle_deck==NULL)
		{
			write_room(user->room,"~OLError : Run out of cards.\n");
			return;
		}
		temp_plr = player_turn;
		if ((shuffle_deck->i>10)&&(shuffle_deck->i<14))
        		temp_plr->total+=10;
        	else
			temp_plr->total+=shuffle_deck->i;
		if (temp_plr->ace)
			temp_plr->total2+=shuffle_deck->i;
		if ((shuffle_deck->i == 1)&&(!(temp_plr->ace)))
		{
			temp_plr->total2+=10;
			temp_plr->total2+=temp_plr->total;
			temp_plr->ace=1;
		}
		temp_plr->cards = push_card(temp_plr->cards,shuffle_deck->i,shuffle_deck->card_type);
		shuffle_deck = pop_card(shuffle_deck);
		sprintf(text,"~OL%s takes another card.\n",temp_plr->user->name);
		write_room(user->room,text);
		display_hand(user);
		if ((temp_plr->total>21) || (temp_plr->total2>21))
		{
			sprintf(text,"~OL~FY%s has BUSTED!!!\n",
				temp_plr->user->name);
			write_room(user->room,text);
			if (player_turn->next==NULL)
			{
				calculate_results();
				for (temp_plr=players;temp_plr!=NULL;temp_plr=temp_plr->next)
				{
					while (temp_plr->cards!=NULL)
					{
						temp_plr->cards = pop_card(temp_plr->cards);
					}
					temp_plr->total = 0;
					temp_plr->total2 = 0;
					temp_plr->ace = 0;
				}
				while (shuffle_deck!=NULL) shuffle_deck = pop_card(shuffle_deck);
				blackjack_status = 0;
				return;
			}
			player_turn = player_turn->next;
			sprintf(text,"~OL%s's turn.\n",player_turn->user->name);
			write_room(user->room,text);
			return;
		}
		return;
	}
	if (!strcmp(word[1],"stand"))
	{
		if (!(user->blackjack_status))
		{
			write_user(user,"~OLSorry, you are not playing a game.\n");
			return;
		}
		if (player_turn->user!=user)
		{
			write_user(user,"~OLSorry, it is not your turn.\n");
			return;
		}
		sprintf(text,"~OL%s has finished their turn.\n",user->name);
		write_room(user->room,text);
		if (player_turn->next==NULL)
		{
			calculate_results();
			for (temp_plr=players;temp_plr!=NULL;temp_plr=temp_plr->next)
			{
				while (temp_plr->cards!=NULL)
				{
 					temp_plr->cards = pop_card(temp_plr->cards);
				}
				temp_plr->total = 0;
				temp_plr->total2 = 0;
				temp_plr->ace = 0;
			}
			while (shuffle_deck!=NULL) shuffle_deck = pop_card(shuffle_deck);
			blackjack_status = 0;
			return;
		}
		player_turn = player_turn->next;
		sprintf(text,"~OL%s's turn.\n",player_turn->user->name);
		write_room(user->room,text);
		return;
	}

	if (!strcmp(word[1],"quit")) {
		if (blackjack_status) {
			write_user(user,"~OLSorry, a game is in progress.\n");
			return;
		}
		if (!(user->room==get_room(BLKJKROOM))) {
			write_user(user,"~OLSorry, u are not in the blackjack  room.\n");
			return;
		}
 		for (temp_plr=players;temp_plr->user!=user;temp_plr=temp_plr->next) {}
			while (temp_plr->cards!=NULL) {
				temp_plr->cards = pop_card(temp_plr->cards);
			}
			pop_player(temp_plr);
			num_players=num_players-1;
			rm=get_room(MAINROOM);
			move_user(user,rm,6);
			return;
		}
		else {
			write_user(user,"~OLUsage : blackjack hand/join/deal/hit/stand/quit\n");
			return;
		}
	}

/*** Extra misc funtions for BlackJack - Slugger ***/

void display_hand(user)
UR_OBJECT user;
{
	PLR_OBJECT temp_plr;
	CRD_OBJECT temp_crd;
	int cardno,lineno,i;

		temp_plr = players;
		while (temp_plr->user!=user) 
			temp_plr = temp_plr->next;
		temp_crd = temp_plr->cards;
		i = 0;
		for (temp_crd=temp_plr->cards;temp_crd!=NULL;temp_crd=temp_crd->next)
			i++;		
		for (lineno=0;lineno<11;lineno++)
		{
			cardno=1;
			write_user(user,"        ");
			for (temp_crd=temp_plr->cards;temp_crd!=NULL;temp_crd=temp_crd->next)
			{
				if (lineno==0)
				{
					write_user(user,"+---");
					if (cardno==i)
						write_user(user,"------------+\n");
				}
				if (lineno==1)
				{
					switch (temp_crd->i)
					{
						case 1: write_user(user,"| A ");
							break;
						case 11:write_user(user,"| J ");
							break;
						case 12:write_user(user,"| Q ");
							break;
						case 13:write_user(user,"| K ");
							break; 	
					}
					if ((temp_crd->i<11)&&(temp_crd->i>1))
					{
						if (temp_crd->i==10)
						{
							sprintf(text,"| %d",temp_crd->i);
							write_user(user,text);
						}
						else
						{
							sprintf(text,"| %d ",temp_crd->i);
							write_user(user,text);
						}
					}
				}
				if ((lineno>0)&&(lineno<10)) {
					switch (temp_crd->i)
					{
						case 13:if (lineno == 5)
								write_user(user,"| B ");
							if (lineno == 6)
								write_user(user,"| | ");
							if (lineno == 7)
								write_user(user,"| | ");
							if (lineno == 8)
								write_user(user,"| @ ");
							if (lineno == 9)
								write_user(user,"|   ");
							if ((lineno < 5)&&(lineno > 1))
								write_user(user,"|   ");
							break;
						case 12:if (lineno == 5)
								write_user(user,"| B ");
							if (lineno == 6)
								write_user(user,"| | "); 
							if (lineno == 7)
								write_user(user,"| | ");
							if (lineno == 8)
								write_user(user,"| * ");
							if (lineno == 9)
								write_user(user,"|   ");
							if ((lineno < 5)&&(lineno > 1))
								write_user(user,"|   ");
							break;
						default:if (lineno > 1)
								write_user(user,"|   ");
							break;
					}
					if (cardno==i) 
						switch (temp_crd->i)
						{
					    	case 13:if (lineno == 1)
								write_user(user,"|\\/\\/\\/|    |\n");
						    	if (lineno == 2)
								write_user(user,"|------| @  |\n");
						    	if (lineno == 3)
								write_user(user," c  o o  |  |\n");
						    	if (lineno == 4)
								write_user(user,"     .   |  |\n");
						    	if (lineno == 5)
								write_user(user,"   --    B  |\n");
						    	if (lineno == 6)
								write_user(user,"  .         |\n");
						    	if (lineno == 7)
								write_user(user," o o  a     |\n");
						    	if (lineno == 8)
								write_user(user,"|------|    |\n");
						    	if (lineno == 9)
								write_user(user,"|/\\/\\/\\| K  |\n");
						    	break;
					    	case 12:if (lineno == 1)
								write_user(user,"|\\/\\/\\/|    |\n");
						    	if (lineno == 2)
								write_user(user,"|@@@@@@| *     |\n");
						    	if (lineno == 3)
								write_user(user,"@c  o o  |     |\n");
						    	if (lineno == 4)
								write_user(user,"@    .   |     |\n");
						    	if (lineno == 5)
								write_user(user,"@  --  @ B     |\n");
						    	if (lineno == 6)
								write_user(user,"  .    @       |\n");
						    	if (lineno == 7)
								write_user(user," o o  a@       |\n");
						    	if (lineno == 8)
								write_user(user,"|@@@@@@|       |\n");
						    	if (lineno == 9)
								write_user(user,"|/\\/\\/\\|  Q    |\n");
					            	break;
						case 11:if (lineno == 1)
								write_user(user,".aaaaaaa    |\n");
						    	if (lineno == 2)
								write_user(user,"&&&&&&&&    |\n");
						    	if (lineno == 3)
								write_user(user," c  o o     |\n");
						    	if (lineno == 4)
								write_user(user," |   .      |\n");
						    	if (lineno == 5)
								write_user(user,"   --       |\n");
						    	if (lineno == 6)
								write_user(user,"  .   |     |\n");
						    	if (lineno == 7)
								write_user(user," o o  a     |\n");
						    	if (lineno == 8)
								write_user(user,"&&&&&&&&    |\n");
						    	if (lineno == 9)
								write_user(user,"`^^^^^^^  J |\n");
						    	break;
						default:write_user(user,"            |\n");
							break;
						}
					}
					if (lineno==10)
					{
						write_user(user,"+---");
						if (cardno==i)
						write_user(user,"------------+\n");
					}
					cardno++;
				}
			}
			sprintf(text,"~OLTotal Wins : %d\n",temp_plr->user->blackjack_total);
			write_user(user,text);
			if (temp_plr->ace)
			{
				sprintf(text,"Current Hand (Ace = 1) : %d\n",temp_plr->total);
				write_user(user,text);
				sprintf(text,"             (Ace = 11): %d\n",temp_plr->total2);
				write_user(user,text);
			}	
			else
			{
				sprintf(text,"Current Hand : %d\n",temp_plr->total);
				write_user(user,text);
			}
			return;
}

void push_player(u)	
UR_OBJECT u;
{	
PLR_OBJECT new_plr;
PLR_OBJECT temp_plr;
PLR_OBJECT curr_plr;

new_plr = ((PLR_OBJECT)malloc(sizeof(struct player_struct)));
new_plr->user = u;
new_plr->total = 0;
new_plr->total2 = 0;
new_plr->ace = 0;
new_plr->nocards = 0;
new_plr->cards = NULL;
new_plr->next = NULL;
if (players==NULL)	
	players=new_plr;
else 
	{
	for(curr_plr=players;curr_plr->next!=NULL;curr_plr=curr_plr->next)
{}
	curr_plr->next=new_plr;
	}
}

CRD_OBJECT push_card(list,i,type)	
CRD_OBJECT list;
int i;
char type;
{	
CRD_OBJECT new_crd;
CRD_OBJECT curr_crd;

new_crd = ((CRD_OBJECT)malloc(sizeof(struct card_struct)));

new_crd->i = i;
new_crd->card_type = type;
new_crd->next = NULL;
if (list==NULL)	
	list=new_crd;
else 
	{
	for(curr_crd=list;curr_crd->next!=NULL;curr_crd=curr_crd->next) {}
	curr_crd->next=new_crd;
	}
return list;
}
	
CRD_OBJECT pop_card(list)
CRD_OBJECT list;
{
CRD_OBJECT temp;

if (list == NULL) {
	write_room(get_room(BLKJKROOM),"\nError: Tried to pop empty
list.\n");
	return;
	}
temp = list->next;
free(list);
list = temp;

return list;
}

void pop_player(p)
PLR_OBJECT p;
{
PLR_OBJECT prev, curr;

    if (players == NULL) {
	write_user(p->user, "\nError: Tried to pop empty list.\n");
	return;
    }
    if (p->user == players->user) {    /* delete head. */
	prev = players;
	players = players->next;
	free(prev);
    } else {
	prev = players;
	curr = players->next;
	while ((curr != NULL) && (curr->user != p->user)) {
	    prev = curr;
	    curr = curr->next;    /* Get player to delete. */
	}
	if (curr == NULL) {
	    write_user(p->user, "\nYou have not even joined one.\n");
	}
	else {
	    prev->next = curr->next;
	    free(curr);
	}
    }
}

void calculate_results()
{
PLR_OBJECT currplay,winner;
int i;

for (currplay=players;currplay!=NULL;currplay=currplay->next) {
	if (currplay->total > 21)
		currplay->total = 0;
	if (currplay->total2 > 21)
		currplay->total2 = 0;
	if (currplay->total < currplay->total2)
		currplay->total = currplay->total2; 
	currplay->user->blackjack_status = 0;
	}
winner = players;
for (currplay=players;currplay!=NULL;currplay=currplay->next) {
	if (winner->total < currplay->total)
		winner = currplay;
	}
for (currplay=players;currplay!=NULL;currplay=currplay->next) {	
	if ((winner->total == currplay->total)&&(winner!=currplay)) {
		sprintf(text,"~OLThe game has been drawn on %d.\n",winner->total);
		write_room(winner->user->room,text);
		return;
		}	
	}
winner->user->blackjack_total++;
sprintf(text,"~OL%s is the winner with
%d.\n",winner->user->name,winner->total); 
write_room(winner->user->room,text); 
}

