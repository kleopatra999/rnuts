/****************************************************************************/
/* Please note there is a small bug that still occurs...even with my
bandaids, but if a user winds up logging off while playing poker, it causes
a segmentation fault on the next move because it does not destruct the player
properly, i have no idea on how to fix this i have tried pretty much every-
thing...so if you can let me know my email addy is squirt@idigital.net
or squirt@funcity.ml.org or squirt@talker.com */
/****************************************************************************/
extern UR_OBJECT create_user();
extern UR_OBJECT get_user(char[]);
extern struct poker_game *get_poker_game_here(RM_OBJECT);

/***************Poker, derived from Robb Thomas' version********************/
/*** Get poker_game struct pointer from name ***/
struct poker_game *get_poker_game(name)
char *name;
{
struct poker_game *game;
name[0]=toupper(name[0]);
/* Search for exact name */
for(game=poker_game_first;game!=NULL;game=game->next) {
	if (!strcmp(game->name,name))  return game;
	}
/* Search for close match name */
for(game=poker_game_first;game!=NULL;game=game->next) {
	if (strstr(game->name,name))  return game;
	}
return NULL;
}

/*** Create a poker game ***/
struct poker_game *create_poker_game()
{
struct poker_game *game;
int i;

if ((game=(struct poker_game *)malloc(sizeof(struct poker_game)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_poker_game().\n",0);
	return NULL;
	}

/* Append object into linked list. */
if (poker_game_first==NULL) {  
        poker_game_first=game;  game->prev=NULL;  
        }
   else {  
        poker_game_last->next=game;  game->prev=poker_game_last;  
	}
game->next=NULL;
poker_game_last=game;

/* initialise the game */
game->name[0]='\0';
game->room=NULL;
game->players=NULL;
game->dealer=NULL;
game->newdealer=0;
game->num_players=0;
game->num_raises=0;
game->top_card=0;
game->bet=0;
game->pot=0;
game->state=0;
game->curr_player=NULL;
game->first_player=NULL;
game->last_player=NULL;

return game;
}

/*** Destruct a poker game. ***/
destruct_poker_game(game)
struct poker_game *game;
{
/* Remove from linked list */
if (game==poker_game_first) {
        poker_game_first=game->next;
        if (game==poker_game_last) poker_game_last=NULL;
        else poker_game_first->prev=NULL;
	}
   else {
	game->prev->next=game->next;
        if (game==poker_game_last) { 
                poker_game_last=game->prev;  poker_game_last->next=NULL; 
		}
                else game->next->prev=game->prev;
                }
sprintf(text, "~OL->~FT Game~FM %s~FT has ended.\n", game->name);
write_room(game->room, text);
free(game);
}

/*** Create a poker player ***/
struct poker_player *create_poker_player(game)
struct poker_game *game;
{
struct poker_player *player;
int i;

if ((player=(struct poker_player *)malloc(sizeof(struct poker_player)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_poker_player().\n",0);
	return NULL;
	}

/* Append object into linked list. */
if (game->first_player==NULL) {
	game->first_player=player;  player->prev=NULL;  
	}
   else {  
	game->last_player->next=player;  player->prev=game->last_player;  
	}
player->next=NULL;
game->last_player=player;

/* Keep track of num players */
game->num_players++;

/* initialise the player */
player->hand[0] = -1;
player->touched = 0;
player->putin = 0;
player->rank = 0;
player->user = NULL;
player->game = game;
return player;
}

/*** Destruct a poker player. ***/
destruct_poker_player(player)
struct poker_player *player;
{
  struct poker_game *game = player->game;
   /* if there are other players   */
   /* pass the turn before leaving */

  /* Keep track of num players */
  game->num_players--;

/*  if (game->num_players<=0){destruct_poker_game(game);} */

  sprintf(text, "~OL-> You leave the game~FG %s.\n", game->name);
  write_user(player->user, text);
  sprintf(text, "~OL-> Player~FG %s~FW leaves the game~FG %s.\n", player->user->name,game->name);
  write_room_except(player->user->room, text, player->user);

if (game->num_players > 1) {
	if (player->hand[0] != -1) {
        word_count = 1;
        fold_poker(player->user);
	}
	
	/* Pass the turn */
	if (game->curr_player == player)
        next_poker_player(game);   
	
	/* Pass the honor of dealing */
	if (game->dealer == player)
        pass_the_deal(game);
}

/* Remove from linked list */
  if (player==game->first_player) {
	game->first_player=player->next;
	if (player==game->last_player) game->last_player=NULL;
	else game->first_player->prev=NULL;
        }
   else {
	player->prev->next=player->next;
	if (player==game->last_player) { 
        game->last_player=player->prev; game->last_player->next=NULL; 
	}
	else player->next->prev=player->prev;
  }
/* record(game->room, text); */

if (game->state == 0) {
	game->curr_player = game->dealer;
        sprintf(text, "~OL# It's your turn to deal.\n");
	write_user(game->dealer->user, text);
        sprintf(text, "~OL-> It's~FG %s's~FW turn to deal.\n",game->dealer->user->name);
	write_room_except(game->room, text, game->dealer->user);
	/* record(game->room, text); */
}
free(player);
/* If the last player left, destruct the game */
if (game->first_player==NULL)
	destruct_poker_game(game);
	}

/*** Shuffle cards ***/
shuffle_cards(int deck[])
{
int i, j, k, tmp;
  
/* init the deck */
  for (i = 0; i < 52; i++) 
	deck[i] = i;

/* do this 7 times */
  for (k = 0; k < 7; k++) {
	/* Swap a random card below */
	/* the ith card with the ith card */
	for (i = 0; i < 52; i++) {
        j = myRand(52-i);
        tmp = deck[j];   
        deck[j] = deck[i];
        deck[i] = tmp;
	}
   }
}

/*** look at hand ***/
hand_poker(user)
UR_OBJECT user;
{
if (user->pop == NULL) {
        write_user(user, "~OL~FMYou gotta be in a game silly\n");
	return;
        }
if (user->pop->hand[0] == -1) {
        write_user(user, "~OL~FTYou need to have some cards first.\n");
	return;
        }
print_hand(user, user->pop->hand);
}

/*** Print hand ***/
print_hand(user, hand)
UR_OBJECT user;
int hand[];
{
int i, j, k;
sprintf(text, "");
        for (i = 0; i < CARD_LENGTH; i++) {
	for (j = 0; j < 5; j++) {
                strcat(text, cards[hand[j]][i]);
                strcat(text, " ");
                }
                strcat(text, "\n");
                write_user(user,text);
                sprintf(text, "");
                }
/** Print the labels */
write_user(user, "~OL<-- ~FG1~FW--> <-- ~FG2~FW--> <-- ~FG3~FW--> <-- ~FG4~FW--> <-- ~FG5~FW-->\n");
}

/*** Print hand for the room ***/
room_print_hand(user, hand)
UR_OBJECT user;
int hand[];
{
int i, j, k;
sprintf(text, "");
        for (i = 0; i < CARD_LENGTH; i++) {
	for (j = 0; j < 5; j++) {
        strcat(text, cards[hand[j]][i]);
        strcat(text, " ");
	}
	strcat(text, "\n");
	write_room(user->room,text);
	/* record(user->room, text); */
	sprintf(text, "");
        }
}

/*** Start a poker Game ***/
start_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
struct cw_game *cwgame;
int hist_index;

if ((hist_index = get_casino_player_hist_index(user)) == -1) {
        write_user(user,"~OL~FRYou need some $$$$$$$$$$\n");
	return;
        }
if (word_count < 2) {
        write_user(user,"~OL~FTYou need to name the game.\n");
	return;
        }
if (strlen(word[1])>=15) {
        write_user(user,"~OLYou kinda need a shorter game name.\n");
        return;
        }
if ((game=get_poker_game(word[1]))!=NULL) {
        write_user(user,"~OL~FTThere is already a game with that name.\n");
	return;
        }
if ((game=get_poker_game_here(user->room))!=NULL) {
        write_user(user,"~OL~FTThere is already a game in this room.\n");
	return;
        }
/*if ((cwgame=get_cw_game(word[1]))!=NULL) {
	write_user(user, "~FRThere is already a game with that name.\n");
	return;
        }
if ((cwgame=get_cw_game_here(user->room))!=NULL) {
	write_user(user, "~FRThere is already a game in this room.\n");
	return;
        } */
if (user->pop == NULL) { 
        if ((game=create_poker_game())==NULL) {
        write_syslog("ERROR: Memory allocation failure in start_poker().\n",0);
        write_user(user, "~FRSorry, can't start a game\n");
        return;
	}
   else {
        if ((user->pop=create_poker_player(game))==NULL) {
                write_syslog("ERROR: Memory allocation failure in start_poker().\n",0);
                write_user(user, "~OL~FRSorry, can't start a game\n");
		return;
                }
        user->pop->user = user;
        user->pop->hist = casino_hist[hist_index];
        game->players = user->pop;
        game->curr_player = user->pop;
        game->dealer = user->pop;
        strcpy(game->name, word[1]);
        game->room = user->room;
        sprintf(text, "~OL-> You start a game of Poker called~FM %s.\n", game->name);
        write_user(user,text);
        sprintf(text, "~OL<~FG %s~FW started a game of Poker called~FG %s~FW >\n",user->name, game->name);
        write_room_except(user->room,text, user);
        write_user(user, "~OL-> It's your turn to deal.\n");
        }
}
   else {
        write_user(user, "~OL~FTYou are already in a game.\n");
        }
}

/*** Join a poker game ***/
join_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
int hist_index;
if ((hist_index = get_casino_player_hist_index(user)) == -1) {
        write_user(user, "~OL~FTYou need $$$$$$$$$$$ ask a wiz\n");
	return;
        }
/*if ((user->pop != NULL)&&(user->cwp != NULL)) {
	write_user(user, "~FRYou are already in a game.\n");
	return;
        }*/
if ((user->pop != NULL)&&(user->pop != NULL)) {
	write_user(user, "~FRYou are already in a game.\n");
	return;
        }
if ((game=get_poker_game_here(user->room))==NULL) {
        write_user(user, "~OL~FTYou gotta be in the same room as the game.\n");
	return;
        }
if (game->num_players == 6) {
        write_user(user, "~OL~FTThis Poker game is full.\n");
	return;
        }
if ((user->pop=create_poker_player(game))==NULL) {
	write_syslog("ERROR: Memory allocation failure in join_po().\n",0);
        write_user(user, "~OL~FRSorry, can't join a game\n");
	return;
        }
   else {
	user->pop->user = user;
        user->pop->hist = casino_hist[hist_index];
        sprintf(text, "~OL-> You join the game~FG %s.\n", game->name);
	write_user(user, text);
        sprintf(text, "~OL<~FG %s~FW joins the game~FG %s~FW >\n", user->name, game->name);
	write_room_except(user->room, text, user);
	/* record(user->room, text); */
        }
}

/*** Leave a PO Game ***/
leave_poker(user)
UR_OBJECT user;
{
if (user->pop != NULL) {
        destruct_poker_player(user->pop);
	user->pop = NULL;
        }
   else {
        write_user(user, "~OL~FTYou aren't in a Poker game.\n");
        }
}

/*** List PO Games ***/
list_poker_games(user)
UR_OBJECT user;
{
struct poker_game *game;
int count = 0;
	write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
        write_user(user,"\n~FT   Current Poker games being played\n");
	write_user(user,"~FT+-----------------------------------------------------------------------------+\n\n");
        write_user(user,"~FY~OLName            : Room         : # of Players\n");
        for(game=poker_game_first;game!=NULL;game=game->next) {
        sprintf(text,"~OL%-15s : %-12s : %d players\n",game->name,game->room->name,game->num_players);
	write_user(user, text);
	count++;
        }
sprintf(text,"\n~OLTotal of~FM %d~FW games.\n\n", count);
write_user(user, text);
}

/*** add_casino_player_hist ***/
int add_casino_player_hist(u, total, given)
UR_OBJECT u;
int total, given;
{
struct casino_player_hist *player_hist;

if ((player_hist=(struct casino_player_hist *)malloc(sizeof(struct casino_player_hist)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in add_casino_player_hist().\n",0);
	return 0;
        }

/* create a new po player history */
strcpy(player_hist->name, u->name);
player_hist->total = total;
player_hist->given = given;

/* add it to the list */
casino_hist[max_casino_hist] = player_hist;
max_casino_hist++;
sort_casino_hist();
return 1;
}

/*** get index for a player hist */
int get_casino_player_hist_index(u)
UR_OBJECT u;
{
int i;
for (i = 0; i < max_casino_hist; i++) {
        if (strcmp(casino_hist[i]->name, u->name) == 0) {
        return i;
	}
}
return -1;
}

/*** casino player history compare ***/
int casino_player_hist_cmp(a,b)
struct casino_player_hist **a;
struct casino_player_hist **b;
{
int a_winnings;
int b_winnings;

a_winnings = (*a)->total;
b_winnings = (*b)->total;
        if (a_winnings < b_winnings) {
                 return 1; }
        else if (a_winnings > b_winnings) {
        return -1; }
   else {
	return 0;
        }
} 									 

/*** sort the casino history ***/
sort_casino_hist()
{
if (max_casino_hist > 0) {
        qsort(casino_hist, max_casino_hist, sizeof(casino_hist[0]),
                  (int (*) (const void *, const void *))casino_player_hist_cmp);
                  }
}

/*** show casino player history ***/
rank_casino(user)
UR_OBJECT user;
{
int i;
int many = 0;

if (word_count == 2) {
	many = atoi(word[1]);
        }
        if (many < 5) many = 5;
        if (many > max_casino_hist) many = max_casino_hist;

write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
write_user(user,"~FT   Casino Ranking\n");
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
write_user(user,"~FTRank: Name         : Total             \n");
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
        for (i = 0; i < many; i++) {
        sprintf(text,"  %-2d: %-12s : $%-17d\n",i+1,casino_hist[i]->name,casino_hist[i]->total,casino_hist[i]->given);
	write_user(user, text);
        }
for (i = 0; i < max_casino_hist; i++) {
        if (!strcmp(user->name, casino_hist[i]->name)) {
        if (i == many) {
        sprintf(text,"  %-2d: %-12s : $%-17d\n",i+1,casino_hist[i]->name,casino_hist[i]->total,casino_hist[i]->given);
		write_user(user, text);
		break;
                }
        else if (i > many) {
                sprintf(text, "           :\n  %-2d: %-12s : $%-17d\n",i+1,casino_hist[i]->name,casino_hist[i]->total,casino_hist[i]->given);
		write_user(user, text);
		break;
                }
           else {
		break;
                }
	}
  }	
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
}

/*** Give chips to all the little people ***/
chips_casino(user)
UR_OBJECT user;
{
UR_OBJECT u;
char text2[80];
int amount;
int hist_index;
  
if (word_count<2) {
        write_user(user,"Usage: chips <user> <ammount>\n");  return;
        }
amount = atoi(word[2]);
if (amount > 500) {
        write_user(user,"An amount lower then 500 please!\n");
        return;
        }
/* User currently logged in */
if (u=get_user(word[1])) {
	/* add to player history */
        if ((hist_index = get_casino_player_hist_index(u)) == -1) {
        /* not in casino_hist */
        if (add_casino_player_hist(u,amount,amount))
                hist_index = get_casino_player_hist_index(u);
            else
		return;
                }
           else {
/*sprintf(text, "hist_index = %d\n", hist_index);
  write_user(user, text);*/

/* already in hist */
        casino_hist[hist_index]->total += amount;
        casino_hist[hist_index]->given += amount;
        sort_casino_hist();
        }
sprintf(text,"~FG~OL%s~FW has~FG $%d~FW poker chips.\n",u->name,casino_hist[hist_index]->total);
	write_user(user,text);
        sprintf(text,"~FG~OL%s~FW has given you~FG $%s~FW worth of poker chips.\n",user->name,word[2]);
	write_user(u,text);
	sprintf(text,"%s gave %s $%s worth of poker chips.\n",user->name,u->name,word[2]);
	write_syslog(text,1);
	return;
        }
/* User not logged in */
if ((u=create_user())==NULL) {
	sprintf(text,"~FR%s: unable to create temporary user object.\n",syserror);
	write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in chips_casino().\n",0);
	return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
	write_user(user,nosuchuser);
	destruct_user(u);
	destructed=0;
	return;
        }
/* add to player history */
        if ((hist_index = get_casino_player_hist_index(u)) == -1) {
/* not in casino_hist */
        if (add_casino_player_hist(u,amount,amount))
        hist_index = get_casino_player_hist_index(u);
   else {
        destruct_user(u);
        destructed=0;
        return;
        }
        } else {
/* already in hist */
        casino_hist[hist_index]->total += amount;
        casino_hist[hist_index]->given += amount;
        sort_casino_hist();
        }
sprintf(text,"~FG~OL%s~FW has~FG $%d~FW poker chips.\n",word[1],casino_hist[hist_index]->total);
write_user(user,text);
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text2,"~OLYou have been given~FG $%s~FW worth of poker chips.~RS\n",word[2]);
send_mail(user,word[1],text2);
sprintf(text,"%s gave %s $%s poker chips.\n",user->name,u->name,word[2]);
write_syslog(text,1);
destruct_user(u);
destructed=0;
}

/*** save casino history to disk ***/
int save_casino_hist()
{
FILE *fp;
char filename[80];
int i;
sprintf(filename,"%s/casinohist",LOGFILES);
if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"SAVE_CASINO_HIST: Failed to save casino history.\n");
	write_syslog(text,1);
	return 0;
        }
fprintf(fp,"%d\n",max_casino_hist);
for (i = 0; i < max_casino_hist; i++) {
        fprintf(fp,"%s\n",casino_hist[i]->name);
        fprintf(fp,"%d %d\n",casino_hist[i]->total,casino_hist[i]->given);
        }
fclose(fp);
return 1;
}

/*** load poker history from disk ***/
int load_casino_hist()
{
FILE *fp;
char filename[80];
int i;
struct casino_player_hist *player_hist;
  
sprintf(filename,"%s/casinohist",LOGFILES);
        if (!(fp=fopen(filename,"r"))) {
        printf("LOAD_CASINO_HIST: Failed to load casino history.\n");
	return 0;
        }
fscanf(fp,"%d",&max_casino_hist); 
        printf("max_casino_hist = %d\n", max_casino_hist);
        for (i = 0; i < max_casino_hist; i++) {
        if ((player_hist=(struct casino_player_hist *)malloc(sizeof(struct casino_player_hist)))==NULL) {
        printf("ERROR: Memory allocation failure in load_casino_hist().\n");
        return 0;
	}
        fscanf(fp,"%s",player_hist->name);
        fscanf(fp,"%d %d",&(player_hist->total),&(player_hist->given));
        printf("name = %s, total = %d\n",player_hist->name,player_hist->total);
/* add it to the list */
        casino_hist[i] = player_hist;
        }
fclose(fp);
sort_casino_hist();
        printf("Loaded poker history.\n");
        return 1;
        }

/*** Deal cards to all players ***/
deal_poker(user)
UR_OBJECT user;
{
int i;
struct poker_game *game;
struct poker_player *tmp_player;

if (user->pop == NULL) {
        write_user(user,"~OL~FTYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;
if (user->pop != game->dealer) {
        sprintf(text,"~OL~FG%s~FW is the dealer.\n",game->dealer->user->name);
	write_user(user, text);
	return;
        }
if (game->state != 0) {
        write_user(user,"~OL-> You can't deal now.\n");
	return;
        } 
if (game->num_players < 2) {
        write_user(user,"~OL-> You need two people in to deal poker.\n");
	return;
        } 
/* Reset game state */
        game->top_card = 0;
        game->bet = 0;
        game->pot = 0;
        game->num_raises = 0;
        game->in_players = 0;
        game->opened = NULL;
        game->newdealer = 0;
  
shuffle_cards(game->deck);

/* Reset players */
for (tmp_player = game->first_player; tmp_player != NULL;
        tmp_player = tmp_player->next) {
        if (tmp_player->hist->total >= 5) {
        tmp_player->hand[0] = -1;
        tmp_player->touched = 0;
        tmp_player->putin = 5;
        tmp_player->rank = 0;
        game->in_players++;
	  
write_user(tmp_player->user,"~OL-> You ante $5.\n");
        } else {
        write_user(tmp_player->user,"~OL-> You need more poker chips!\n");
	}
}
sort_casino_hist();
if (game->in_players < 2) {
        write_room(user->room,"~OL~FTNot enough people have chips to ante.  You need at least two.\n");
	/* record(user->room,  "~FRNot enough people have chips to ante.  You need at least two.\n"); */
	return;
        }
write_room(user->room,"~OL~FT Everyone has anted $5.\n");
        /* record(user->room, "# Everyone has anted $5.\n"); */
        write_user(user, "~OL-> You shuffle and deal the cards.\n");
        sprintf(text, "~OL~FG %s~FW shuffles and deals the cards.\n", user->name);
        write_room_except(user->room,text,user);
        /* record(user->room,text); */

/* Start with the player to the left of the dealer */
game->curr_player = game->dealer;
next_poker_player(game);
tmp_player = game->curr_player;

/* deal five to each player */
for (i = 0; i < 5; i++) {
	do {
	  if (game->curr_player->putin == 5) {
	  game->curr_player->hand[i] = game->deck[game->top_card];
	  game->top_card++;
if (i == 4) {
	game->curr_player->hist->total -= 5;
	game->pot += 5;
	game->curr_player->putin = 0;	
	print_hand(game->curr_player->user,game->curr_player->hand);
	}
}
/* Next Player */
next_poker_player(game);
	}
	while (game->curr_player != tmp_player);
	}
/* Make this guy the default opened guy */
game->opened = game->curr_player;
write_user(game->curr_player->user,"~OL-> The first round of betting starts with you.\n");
sprintf(text,"~OL~FTThe first round of betting starts with %s.\n",game->curr_player->user->name);
write_room_except(user->room,text,game->curr_player->user);
/* record(user->room,text); */
game->state = 1;
}

/*** Next poker player ***/
next_poker_player(game)
struct poker_game *game;
{
if (game->curr_player->next == NULL) {
	game->curr_player = game->first_player;
        }
   else {
	game->curr_player = game->curr_player->next;
        }  
}

/*** Next in player ***/
next_in_player(game)
struct poker_game *game;
{
do {
	if (game->curr_player->next == NULL) {
        game->curr_player = game->first_player;
	}
   else {
        game->curr_player = game->curr_player->next;
	}  
    } while (game->curr_player->hand[0] == -1);
}

/*** Bet in poker ***/
bet_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
int player_bet;
if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;

/* Check if it's possible to bet */
if ((game->state != 1)&&(game->state != 3)) {
        write_user(user,"~OLYou can't bet now.\n");
	return;
        } 
if (game->curr_player != user->pop) {
        write_user(user,"~OLYou can't bet unless it's your turn.\n");
	return;
        }
if (word_count < 2) {
        write_user(user,"~OLHow much do you want to bet?\n");
	return;
        }
player_bet = atoi(word[1]);
if ((player_bet == 0) && (word[1][0] != '0')) {
        write_user(user,"~OLPlease use numbers.\n");
	return;
        }
if (player_bet < 0) {
        write_user(user,"~OLPositive bets please.\n");
	return;
        }
if (player_bet%5 != 0) {
        write_user(user,"~OLMake your bet a multiple of $5 please.\n");
	return;
        }
if (player_bet == 0) {
        check_poker(user);
	return;
        }
if (player_bet < game->bet) {
        sprintf(text,"~OLYou must bet at least~FG $%d~FW or fold.\n",game->bet);
	write_user(user, text);
	return;
        }
if (player_bet - game->bet > 100) {
	if (game->bet == 0) {
        write_user(user,"~OLThe largest opening bet is $100.\n");
        return;
	}
   else {
        write_user(user,"~OLThe largest raise is $100.\n");
        return;
	}
}
if ((player_bet > game->bet) && (game->bet != 0)) {
        if (game->num_raises > 5) {
        write_user(user,"~OLThere is a limit of five raises.  Please see the bet, or fold.\n");
        return;
	}
	game->num_raises++;
        }
if (user->pop->hist->total < player_bet) {
        sprintf(text,"~OLYou don't have enough chips to make that bet.\n");
	write_user(user, text);
	return;
        } 
bet_poker_aux(user,player_bet);
}

/*** Aux bet function ***/
bet_poker_aux(user,player_bet)
UR_OBJECT user;
int player_bet;
{
struct poker_game *game;
game = user->pop->game;
user->pop->touched = 1;
if (player_bet == game->bet) {
        sprintf(text, "~OL-> You see the bet of~FG $%d\n", game->bet);
	write_user(user, text);
        sprintf(text, "~OL~FG %s~FT sees the bet of~FG $%d\n",user->name,game->bet);
	write_room_except(user->room, text, user);
	/* record(game->room,text); */
        }
else if (game->bet == 0) {
	game->opened = user->pop;
        sprintf(text,"~OL-> You open the betting with~FG $%d\n",player_bet);
	write_user(user, text);
        sprintf(text,"~OL~FG %s~FT opens the betting with~FG $%d\n",user->name,player_bet);
        write_room_except(user->room,text,user);
	/* record(game->room,text); */
        }
   else {
        sprintf(text, "~OL-> You raise the bet to~FG $%d.\n",player_bet);
	write_user(user, text);
        sprintf(text, "~OL~FG%s~FW raises the bet to~FG $%d\n",user->name,player_bet);
	write_room_except(user->room, text, user);
	/* record(game->room,text); */
        }
game->bet = player_bet;
game->pot += (player_bet - user->pop->putin);
user->pop->hist->total -= (player_bet - user->pop->putin);
user->pop->putin = player_bet;
sort_casino_hist();

sprintf(text, "~OL~FT The pot is now~FG $%d.\n",game->pot);
write_user(user, text);

/* Go to next elegible player */
next_in_player(game);

/* Check if all players have called */
all_called_check(game);
bet_message(game);
}

/*** Raise the bet in poker ***/
raise_poker(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
struct poker_game *game;
int player_raise;
if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;

/* Check if it's possible to raise */
if ((game->state != 1)&&(game->state != 3)) {
        write_user(user,"~OLYou can't raise now.\n");
	return;
        } 
if (game->curr_player != user->pop) {
        write_user(user,"~OLYou can't raise unless it's your turn.\n");
	return;
        }
if (word_count < 2) {
        write_user(user,"~OLBy how much do you want to raise?\n");
	return;
        }
player_raise = atoi(word[1]);
if ((player_raise == 0) && (word[1][0] != '0')) {
        write_user(user,"~OLPlease use numbers.\n");
	return;
        }
if (player_raise < 0) {
        write_user(user,"~OLPositive raises please.\n");
	return;
        }
if (player_raise%5 != 0) {
        write_user(user,"~OLMake your raise a multiple of $5 please.\n");
	return;
        }
if (player_raise < 10) {
        write_user(user,"~OLThe smallest raise is $10.\n");
	return;
        }
if (player_raise > 100) {
        write_user(user,"~OLThe largest raise is $100.\n");
	return;
        }
if (player_raise == 0) {
        write_user(user,"~OLThe smallest raise is $5.\n");
	return;
        }
if (game->num_raises > 5) {
        write_user(user,"~OLThere is a limit of five raises.  Please see the bet or fold.\n");
	return;
        }
player_raise += game->bet;
game->num_raises++;
if (user->pop->hist->total < player_raise) {
        write_user(user,"~OLYou don't have enough chips to make that raise.\n");
	return;
        }
bet_poker_aux(user, player_raise);
}

/*** see a bet ***/
see_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
int player_bet;
if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;

/* Check if it's possible to see a bet */
if ((game->state != 1)&&(game->state != 3)) {
        write_user(user,"~OLYou can't see a bet now.\n");
	return;
        } 
if (game->curr_player != user->pop) {
        write_user(user,"~OLYou can't see a bet unless it's your turn.\n");
	return;
        }
player_bet = game->bet;
if (user->pop->hist->total < player_bet) {
        write_user(user,"~OLYou don't have enough chips to see that bet.\n");
	return;
        }
if (player_bet == 0)
        check_poker(user);
else
        bet_poker_aux(user, player_bet);
        }

/*** All called check ***/
all_called_check(game)
struct poker_game *game;
{
struct poker_player *tmp_player;
if (game->curr_player->touched && game->curr_player->putin == game->bet) {
	/* Everyone has called */
	/* reset the touched flags and putin ammts */
for (tmp_player = game->first_player; tmp_player != NULL;
          tmp_player = tmp_player->next) {
	  tmp_player->touched = 0;
	  tmp_player->putin = 0;
          }
game->state++; /* next state */
switch (game->state) {
    case 2:
	  /* Start with the player to the left of the dealer */
	  game->curr_player = game->dealer;
	  next_in_player(game);
          hand_poker(game->curr_player->user);
          sprintf(text,"~OL-> It's your turn to discard.\n");
	  write_user(game->curr_player->user, text);
          sprintf(text,"~OL~FT It's~FG %s's~FT turn to discard.\n",game->curr_player->user->name);
          write_room_except(game->room,text,game->curr_player->user);
	  /* record(game->room, text); */
	  break;
    case 4:
          write_room(game->room,"~OL~BG~FM*****SHOWDOWN*****\n");
          showdown_poker(game);
	  break;
          }
     }
}

/*** pass the deal to the next player ***/
pass_the_deal(game)
struct poker_game *game;
{
if (game->dealer->next == NULL) {
	game->dealer = game->first_player;
        }
   else {
	game->dealer = game->dealer->next;
        }
if (game->state != 0) {
	game->newdealer = 1;  /* mark that we have a new dealer */
        }
}

/*** Fold and pass turn to next player ***/
fold_poker(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
struct poker_game *game;
struct poker_player *tmp_player;

if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
if (user->pop->hand[0] == -1) {
        write_user(user,"~OLYou don't have any cards.\n");
	return;
        }
game = user->pop->game;

/* Mark folding by making the first card in hand -1 */
user->pop->hand[0] = -1;
game->in_players--;
if (word_count < 2) {
        sprintf(text,"~OL-> You fold.\n");
	write_user(user, text);
        sprintf(text,"~OL~FG %s~FT folds.\n", user->name);
        }
   else {
        sprintf(text,"~OL-> You say, \"~FM%s~FW\" and fold.\n", inpstr);
	write_user(user, text);
        sprintf(text,"~OL~FG %s~FT says, \"~FM%s~FT\" and folds.\n",user->name,inpstr);
        }
write_room_except(user->room, text, user);
/* record(user->room,text); */
/* Check if there is 1 player in */
if (game->in_players == 1) {
	/* The in person wins */
	next_in_player(game);
	/* add to players total_bux */
	game->curr_player->hist->total += game->pot;
        sort_casino_hist();
        sprintf(text,"~OL-> You win~FG $%d~FW!!!\n", game->pot);
	write_user(game->curr_player->user, text);
        sprintf(text,"~OL~FG %s~FT wins~FG $%d~FT!!!\n", 
                game->curr_player->user->name, game->pot);
                write_room_except(game->room,text,game->curr_player->user);
                /* record(game->room, text); */
	game->pot = 0;
	game->bet = 0;
	game->state = 0;  /* reset and deal cards */
	for (tmp_player = game->first_player; tmp_player != NULL;
          tmp_player = tmp_player->next) {
	  /* clear players' hands */
	  tmp_player->hand[0] = -1;
          } 

/* pass the deal if it hasn't already */
if (!game->newdealer)
        pass_the_deal(game);
        game->curr_player = game->dealer;
        sprintf(text,"~OL-> It's your turn to deal.\n");
	write_user(game->dealer->user, text);
        sprintf(text,"~OL~FT It's~FG %s's~FT turn to deal.\n",game->dealer->user->name);
                        write_room_except(game->room,text,game->dealer->user);
                        /* record(game->room, text); */
                        }
                   else {
/* check if it's my turn */
                        if (game->curr_player == user->pop) {
/* Go to next elegible player */
                        next_in_player(game);
/* Check what state we're in */
                        all_called_check(game);
                        bet_message(game);
                        }
           }
}

/*** Check poker (kinda lame command, but I decided to keep it in ***/
check_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
int player_bet;
if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;

/* Check if it's possible to check */
if ((game->state != 1)&&(game->state != 3)) {
        write_user(user,"~OLYou can't check now.\n");
	return;
        } 
if (game->curr_player != user->pop) {
        write_user(user,"~OLYou can't check unless it's your turn.\n");
	return;
        }
if (game->bet > 0) {
        sprintf(text,"~OLYou must bet at least~FG $%d~FW or fold.\n",game->bet);
	write_user(user, text);
	return;
        }
/* We've checked! */
user->pop->touched = 1;
sprintf(text,"~OL-> You check.\n");
        write_user(user, text);
        sprintf(text,"~OL~FG %s~FT checks.\n",user->name);
        write_room_except(user->room,text, user);
        /* record(game->room,text); */

/* Go to next elegible player */
next_in_player(game);

/* Check what state we're in */
        all_called_check(game);
        bet_message(game);
        }

/*** Bet message ***/
bet_message(game)
struct poker_game *game;
{
if (game->state == 1 || game->state == 3) {
        sprintf(text,"~OL You've put in~FG $%d~FW this round. The bet is~FG $%d ~FWto you.\n",game->curr_player->putin,game->bet);
	write_user(game->curr_player->user, text);
        sprintf(text,"~OL~FTIt's~FG %s's~FT turn to bet.\n",game->curr_player->user->name);
	write_room_except(game->room, text, game->curr_player->user);
	/* record(game->room,text); */
        }
}

/*** Discard cards in your hand ***/
disc_poker(user)
UR_OBJECT user;
{
struct poker_game *game;
struct poker_player *tmp_player;
int i;
int have_ace;
int disc_these[5];
int choice;
if (user->pop == NULL) {
        write_user(user,"~OLYou have to be in a Poker game to use this command.\n");
	return;
        }
game = user->pop->game;

/* Check if it's possible to discard */
if (game->state != 2) {
        write_user(user,"~OLYou can't discard now.\n");
	return;
        } 
if (game->curr_player != user->pop) {
        write_user(user,"~OLYou can't discard unless it's your turn.\n");
	return;
        }
if (word_count > 5) {
        write_user(user,"~OLYou can't discard five cards.\n");
	return;
        }
if (word_count > 4) {
	/* Look for an ace */
	have_ace = 0;
	i = 0;
     do {
        if (user->pop->hand[i]%13 == 12)
        have_ace = 1;
        i++;
	} while ((i < 5) && (!have_ace));
if (!have_ace) {
        write_user(user,"~OLYou can't discard four cards unless you have an ace.\n");
        return;
        }
}
user->pop->touched = 1;
if ((word_count < 2) || (word[1][0] == '0')) {
	/* No discards */
        sprintf(text,"~OL-> You stand pat.\n");
	write_user(user, text);
        sprintf(text,"~OL~FG %s~FT stands pat.~RS\n",user->name);
	write_room_except(user->room, text, user);
	/* record(game->room,text); */
        }
   else {
	/* discards */
	/** Init the array **/
	for (i = 0; i < 5; i++)
        disc_these[i] = 0;
	/** Get which cards to discard **/
	for (i = 1; i < word_count; i++) {
        choice = atoi(word[i]);
if ((choice <= 0) || (choice > 5)) {
        sprintf(text,"~OLChoose a number 1 through 5 please.\n");
		write_user(user, text);
		return;
                }
           else {
		disc_these[choice - 1] = 1; /* We're not keeping this one */
                }
	}
        /** draw cards **/
	for (i = 0; i < 5; i++) {
        if (disc_these[i]) {
		user->pop->hand[i] = game->deck[game->top_card];
		game->top_card++;
                }
	}
        sprintf(text,"~OL-> You have discarded~FG %d~FW card(s).\n",word_count-1);
	write_user(user, text);
        sprintf(text,"~OL~FG%s~FT has discarded~FG %d~FT card(s)~RS.\n",user->name,word_count-1);
	write_room_except(user->room, text, user);
	/* record(game->room,text); */
        print_hand(user, user->pop->hand);
        }

/* Go to next elegible player */
next_in_player(game);
if (game->curr_player->touched) {
	/* We've dealt cards to everyone */
	/* reset the touched flags */
	for (tmp_player = game->first_player; tmp_player != NULL;
                tmp_player = tmp_player->next) {
                tmp_player->touched = 0;
                }
	/* reset the bet */
	game->bet = 0;
	game->num_raises = 0;
/* Start with the player who opened if still in */
        game->curr_player = game->opened;
	if (game->curr_player->hand[0] == -1) {
        /* That person folded */
        next_in_player(game);
	}
        sprintf(text,"~OL~FTThe current pot is~FG $%d.\n",game->pot);
	write_room(user->room, text);
        sprintf(text,"~OL-> The second round of betting starts with you.\n");
	write_user(game->curr_player->user, text);
        sprintf(text,"~OL~FTThe second round of betting starts with~FG %s\n",game->curr_player->user->name);
	write_room_except(user->room, text, game->curr_player->user);
	/* record(game->room,text); */
        game->state = 3;
        }
   else {
        hand_poker(game->curr_player->user);
        sprintf(text,"~OL-> It's your turn to discard.\n");
	write_user(game->curr_player->user, text);
        sprintf(text,"~OL~FTIt's~FG %s's~FT turn to discard.\n",game->curr_player->user->name);
	write_room_except(user->room, text, game->curr_player->user);
	/* record(game->room,text); */
        }
}

/*** Showdown ***/
showdown_poker(game)
struct poker_game *game;
{
struct poker_player *tmp_player;
struct poker_player *winners[4];
int num_winners;
int i, j, temp;
int loot;
char rtext[20];
winners[0] = game->first_player;
num_winners = 1;

/* assign ranks to all players hands */
for (tmp_player = game->first_player; tmp_player != NULL;
        tmp_player = tmp_player->next) {
	/* If the player is not folded */
	if (tmp_player->hand[0] != -1) {
        /* sort cards */
        for(i=0;i<5;i++) {
		for(j=0;j<4;j++)
                {
                if(tmp_player->hand[j]%13 < tmp_player->hand[j+1]%13) {
                        temp=tmp_player->hand[j];
                        tmp_player->hand[j]=tmp_player->hand[j+1];
                        tmp_player->hand[j+1]=temp;
                        }
                }
        }
        /*      
        for (i = 0; i < 5; i++) {
		sprintf(text, "%s's hand[%d]mod 13 = %d\n",
                      tmp_player->user->name, i, tmp_player->hand[i]%13);
                      write_room(game->room, text);
                      } */
	  /* check for straight or straight flush */
	  /* Ace low 5432A */
	  if ((tmp_player->hand[0]%13 == (tmp_player->hand[1]%13) + 1) &&
                (tmp_player->hand[1]%13 == (tmp_player->hand[2]%13) + 1) &&
                (tmp_player->hand[2]%13 == (tmp_player->hand[3]%13) + 1) &&
                (tmp_player->hand[4]%13 == 12) && (tmp_player->hand[0]%13 == 0)) {
		tmp_player->rank = 5;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 1);
		swap_cards(tmp_player->hand, 1, 2);
		swap_cards(tmp_player->hand, 2, 3);
		swap_cards(tmp_player->hand, 3, 4);
                }
           else { /* Ace high AKQJ10 or other straight */
		if ((tmp_player->hand[0]%13 == (tmp_player->hand[1]%13) + 1) &&
			(tmp_player->hand[1]%13 == (tmp_player->hand[2]%13) + 1) &&
			(tmp_player->hand[2]%13 == (tmp_player->hand[3]%13) + 1) &&
			(tmp_player->hand[3]%13 == (tmp_player->hand[4]%13) + 1)) {
                        tmp_player->rank = 5;
                        }        
	  }
	
	  /* check for flush */
	  if (((tmp_player->hand[0] < 13) &&
		   (tmp_player->hand[1] < 13) &&
		   (tmp_player->hand[2] < 13) &&
		   (tmp_player->hand[3] < 13) &&
		   (tmp_player->hand[4] < 13)) ||
		  (((tmp_player->hand[0] < 26) && (tmp_player->hand[0] > 12)) &&
		   ((tmp_player->hand[1] < 26) && (tmp_player->hand[1] > 12)) &&
		   ((tmp_player->hand[2] < 26) && (tmp_player->hand[2] > 12)) &&
		   ((tmp_player->hand[3] < 26) && (tmp_player->hand[3] > 12)) &&
		   ((tmp_player->hand[4] < 26) && (tmp_player->hand[4] > 12))) ||
		  (((tmp_player->hand[0] < 39) && (tmp_player->hand[0] > 25)) &&
		   ((tmp_player->hand[1] < 39) && (tmp_player->hand[1] > 25)) &&
		   ((tmp_player->hand[2] < 39) && (tmp_player->hand[2] > 25)) &&
		   ((tmp_player->hand[3] < 39) && (tmp_player->hand[3] > 25)) &&
		   ((tmp_player->hand[4] < 39) && (tmp_player->hand[4] > 25))) ||
		  (((tmp_player->hand[0] < 52) && (tmp_player->hand[0] > 38)) &&
		   ((tmp_player->hand[1] < 52) && (tmp_player->hand[1] > 38)) &&
		   ((tmp_player->hand[2] < 52) && (tmp_player->hand[2] > 38)) &&
		   ((tmp_player->hand[3] < 52) && (tmp_player->hand[3] > 38)) &&
		   ((tmp_player->hand[4] < 52) && (tmp_player->hand[4] > 38)))) {
                   /* We have a flush at least */
                   if (tmp_player->rank > 0) { 
                   /* We have a straight flush */
                   tmp_player->rank = 9;
                   }
              else {
		  /* We have a flush */
		  tmp_player->rank = 6;
                  }
	  }
  	  /* check for four of a kind */
	  if ((tmp_player->rank == 0) && 
		  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
		   (tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
		   (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
		tmp_player->rank = 8;
	  }
	  if ((tmp_player->rank == 0) &&
		   ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
			(tmp_player->hand[2]%13 == tmp_player->hand[3]%13) &&
			(tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
		/* we have four of a kind */
		tmp_player->rank = 8;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 4);
	  }
			   
	  /* check for three of a kind or full house */
	  if ((tmp_player->rank == 0) && 
                ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
		/* we have three of a kind */
		tmp_player->rank = 4;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 3);
	  }
	  if ((tmp_player->rank == 0) && 
                ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                (tmp_player->hand[1]%13 == tmp_player->hand[2]%13))) {
		if (tmp_player->hand[3]%13 == tmp_player->hand[4]%13) {
                /* we have a full house */
                tmp_player->rank = 7;
		} else {
                /* we have three of a kind */
                tmp_player->rank = 4;
		}
	  }
	  if ((tmp_player->rank == 0) && 
                ((tmp_player->hand[2]%13 == tmp_player->hand[3]%13) &&
                (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
		if (tmp_player->hand[0]%13 == tmp_player->hand[1]%13) {
                /* we have a full house */
                tmp_player->rank = 7;
                /* arrange cards */
                swap_cards(tmp_player->hand, 0, 3);
                swap_cards(tmp_player->hand, 1, 4);
		} else {
                /* we have three of a kind */
                tmp_player->rank = 4;
                /* arrange cards */
                swap_cards(tmp_player->hand, 0, 3);
                swap_cards(tmp_player->hand, 1, 4);
		}
	  }

	  /* check for two pair */
	  if ((tmp_player->rank == 0) && 
		  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
		   (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
          /* we have two pair */
          tmp_player->rank = 3;
	  }
	  /* check for two pair */
	  if ((tmp_player->rank == 0) && 
                ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
		/* we have two pair */
		tmp_player->rank = 3;
		/* arrange cards */
		swap_cards(tmp_player->hand, 2, 4);
	  }		
	  /* check for two pair */
	  if ((tmp_player->rank == 0) && 
                ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
		/* We have two pair */
		tmp_player->rank = 3;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 2);
		swap_cards(tmp_player->hand, 2, 4);
	  }
	  /* check for one pair */
	  if ((tmp_player->rank == 0) && 
                (tmp_player->hand[0]%13 == tmp_player->hand[1]%13)) {
		/* we have a pair */
		tmp_player->rank = 2;
	  }
	  /* check for one pair */
	  if ((tmp_player->rank == 0) && 
                (tmp_player->hand[1]%13 == tmp_player->hand[2]%13)) {
		/* we have a pair */
		tmp_player->rank = 2;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 2);
	  }
	  /* check for one pair */
	  if ((tmp_player->rank == 0) && 
                (tmp_player->hand[2]%13 == tmp_player->hand[3]%13)) {
		/* we have a pair */
		tmp_player->rank = 2;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 2);
		swap_cards(tmp_player->hand, 1, 3);
	  }
	  /* check for one pair */
	  if ((tmp_player->rank == 0) && 
                (tmp_player->hand[3]%13 == tmp_player->hand[4]%13)) {
		/* We have a pair */
		tmp_player->rank = 2;
		/* arrange cards */
		swap_cards(tmp_player->hand, 0, 2);
		swap_cards(tmp_player->hand, 1, 3);
		swap_cards(tmp_player->hand, 0, 4);
	  }
	  
	  if (tmp_player->rank == 0) {
		/* We must have a high card */
		tmp_player->rank = 1;
                }
        }
}

/* show all hands that are in */
for (tmp_player = game->first_player; tmp_player != NULL;
	   tmp_player = tmp_player->next) {
           if (tmp_player->rank > 0) {
           /* get max rank */
           if (tmp_player->rank > winners[0]->rank) { 
		winners[0] = tmp_player;
		num_winners = 1;
                } 
           if ((tmp_player->rank == winners[0]->rank) &&
                (tmp_player != winners[0])){
		for (i = 0; i < 5; i++) {
                if (tmp_player->hand[i]%13 > winners[0]->hand[i]%13) {
			winners[0] = tmp_player;
			num_winners = 1;
			break;
                        }
                        else if (tmp_player->hand[i]%13 < winners[0]->hand[i]%13) {
			break;
                        } else if (i == 4) {
			/* we have a tie */
			winners[num_winners] = tmp_player;
			num_winners++;
                        }
		}
	  }		  
	room_print_hand(tmp_player->user, tmp_player->hand);
	switch (tmp_player->rank) {
        case 1: sprintf(rtext,"a high card."); break;
        case 2: sprintf(rtext,"a pair."); break;
        case 3: sprintf(rtext,"two pair."); break;
        case 4: sprintf(rtext,"three of a kind."); break;
        case 5: sprintf(rtext,"a straight."); break;
        case 6: sprintf(rtext,"a flush."); break;
        case 7: sprintf(rtext,"a full house."); break;
        case 8: sprintf(rtext,"four of a kind."); break;
        case 9: sprintf(rtext,"a straight flush."); break;
	}
        sprintf(text,"~OL~FG%s~FT has~FG %s\n",tmp_player->user->name,rtext);
	write_room(game->room, text);
	/* record(game->room,text); */
	}
  }
/*sprintf(text, "num_winners = %d\n", num_winners);
  write_room(game->room, text); */
/* divide the loot */
/* curr_player == the player who was called */
for (i = 0; i < num_winners; i++) {
	temp = (game->pot / (num_winners * 5));
	loot = 5 * temp;
/*sprintf(text, "temp = %d\n", temp);
        write_room(game->room, text);*/
/* player called gets remainder */
if (winners[i] == game->curr_player) {
        loot += game->pot%(num_winners*5);
	}
	winners[i]->hist->total += loot;
        sprintf(text,"~OL-> You win~FG $%d~FW!!!\n", loot);
	write_user(winners[i]->user, text);
        sprintf(text,"~OL~FG%s~FT wins~FG $%d~FT!!!\n",winners[i]->user->name,loot);
	write_room_except(game->room, text, winners[i]->user);
	/* record(game->room, text); */
        }
sort_casino_hist();
game->pot = 0;
game->bet = 0;
game->state = 0;  /* reset and deal cards */
for (tmp_player = game->first_player; tmp_player != NULL;
        tmp_player = tmp_player->next) {
	/* clear players' hands */
	tmp_player->hand[0] = -1;
        }

/* pass the deal if we haven't already */
if (!game->newdealer)
	pass_the_deal(game);
game->curr_player = game->dealer;
sprintf(text,"~OL-> It's your turn to deal.\n");
        write_user(game->dealer->user, text);
        sprintf(text,"~OL~FTIt's~FG %s's~FT turn to deal.\n",game->dealer->user->name);
        write_room_except(game->room, text, game->dealer->user);
        /* record(game->room, text); */
        }
  

/*** swap_cards ***/
swap_cards(hand, c1, c2)
int hand[];
int c1;
int c2;
{
int tmp;
tmp = hand[c1];
hand[c1] = hand[c2];
hand[c2] = tmp;
}

magic_poker(user)
UR_OBJECT user;
{
user->pop->hand[0] = atoi(word[1]);
user->pop->hand[1] = atoi(word[2]);
user->pop->hand[2] = atoi(word[3]);
user->pop->hand[3] = atoi(word[4]);
user->pop->hand[4] = atoi(word[5]);
sprintf(text,"~OL~FG%s~FT fiddles the cards.\n",user->name);
write_room(user->room,text);
}

/*** Show the poker players ***/
show_poker_players(user)
UR_OBJECT user;
{
struct poker_game *game;
struct poker_player *player;
char turn_text[80];
char text2[80];
if (word_count < 2) {
	if (user->pop == NULL) {
        if ((game=get_poker_game_here(user->room))==NULL) {
                write_user(user,"~OLWhich game are you interested in?\n");
		return;
                }
/* else game = the game in this room */
	}
   else {
        game = user->pop->game;
	}
  } else {
        if ((game=get_poker_game(word[1]))==NULL) {
                write_user(user,"~OLNo games by that name are being played.\n");
                return;
                }
}

/* show who is in */
/* fix opening bet bug */
/* fix bug in fold and quit */

write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
sprintf(text,"\n~FT  Info for game %s\n",game->name);
write_user(user, text);
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
write_user(user,"~FTName         :  state  : Chips\n");
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
        switch (game->state) {
        case 0: sprintf(turn_text,"~OL<--~FM Turn to deal.~RS\n"); break;
        case 1: sprintf(turn_text,"~OL<--~FM Turn to bet.(1st rnd.)~RS\n"); break;
        case 2: sprintf(turn_text,"~OL<--~FM Turn to discard.~RS\n"); break;
        case 3: sprintf(turn_text,"~OL<--~FM Turn to bet.(2nd rnd.)~RS\n"); break;
        case 4: sprintf(turn_text,"~OL<--\n~RS"); break;
        } 
for (player = game->first_player; player != NULL; player = player->next) {
	sprintf(text, "%-12s : ", player->user->name);
	if (game->state == 0) {
        strcat(text,"~FR~OLwaiting~RS : ");
	} else {
        if (player->hand[0] == -1) {
                strcat(text," ~FR~OLfolded~RS : ");
	  } else {
                strcat(text,"~FG~OLplaying~RS : ");
                }
	}
        sprintf(text2, "$%-6d ",player->hist->total);
	strcat(text, text2);
	if (game->curr_player == player) {
        strcat(text, turn_text);
	} else {
        strcat(text, "\n");
	}
	write_user(user, text);
}
write_user(user,"~FT+-----------------------------------------------------------------------------+\n");
if ((game->state == 1)||(game->state == 3)) {
	if (user->pop != NULL) {
        sprintf(text,"~OL-> You have put~FG $%d into the pot during this betting round.\n",user->pop->putin);
	write_user(user, text); }
        sprintf(text,"~OL-> The current bet is~FG $%d.\n",game->bet);
	write_user(user, text);
        }
        sprintf(text,"~OL-> The current pot is~FG $%d.\n",game->pot);
        write_user(user, text);
        }
check_casino_hist(user)
UR_OBJECT user;
{
static int done=0;

if (mesg_check_hour==thour && mesg_check_min==tmin) {
        if (done) return;
        }
   else {
        done=0;  return;  }
        done=1;
        save_casino_hist();
if (user)
        write_user(user,"~OL~FGSaved poker rankings.\n");
        }
/************************************************************************/
/* My random function **/
int myRand(int max)
{
int n;
int mask;
/* Mask out as many bits as possible */
for (mask = 1; mask < max; mask *= 2);
mask -= 1;
/* Reroll until a number <= max is returned */
do {
        n = random()&mask;
        }
        while (n >= max);
        return(n);
        }


/****************************************************************************/
/******************************The Connect 4 game ***************************/
/* Please note there is a small bug with it...for example if a column is full,
it still lets a player go there...haven't really worked on it that much, but
I figured if someone is stupid enough to try it then they are a complete
idiot! */

/* Base code from Chris, AKA, Slugger */
/* modified to NUTS by Rob, AKA Squirt */
connect_four(user)
UR_OBJECT user;
{
UR_OBJECT u;
char *c4_opponent;
int x,y,winner,key,dropped,f;

if (word_count<2) {
        write_user(user,"Usage: .connect4 <user> --either lets you offer a challenge to another\n");
        write_user(user,"                          user or accept the challenge.\n");
        write_user(user,"Usage: .connect4 #      --Lets you pick where you want to drop your piece\n");
        write_user(user,"Usage: .connect4 quit   --Quits your current game.\n");
        write_user(user,"Usage: .connect4 board  --Shows your current game boards status\n");
        return;
        }
if (!isdigit(word[1][0])) {
if (!strcmp(word[1],"board")) {
        if (!user->c4_opponent) {
                write_user(user,"Try playing against someone...that might work ya think!\n");
                return;
                }
        print_board(user);
        return;
        }
if (!strcmp(word[1],"quit")) {
        if (!user->c4_opponent) {
                write_user(user,"You wanna quit a game when your not playing??\n");
                return;
                }
        sprintf(text,"~OL~FM%s~FG calls the game quits.\n",user->name);
        write_user(user,text);
        write_user(user->c4_opponent,text);
	for (y=1;y<7;y++) {
		for(x=1;x<8;x++) {
		user->board[x][y]=0;
		user->c4_opponent->board[x][y]=0;
		}
	}
        user->c4_opponent->c4_opponent=NULL;
        user->c4_opponent=NULL;
        return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) {
        write_user(user,"~OL~FMYou like playing with yourself?\n");
        return;
        }
        user->c4_opponent=u;
        if (user->c4_opponent->c4_opponent) {
                if (user->c4_opponent->c4_opponent!=user) {
                        write_user(user, "Sorry, that person is already playing a game of Connect 4\n");
                        return;
                        }
                sprintf(text, "~OL~FG%s ~FMagrees to play a game of connect 4 with you\n",user->name);
                write_user(user->c4_opponent, text);
		user->c4_opponent->turn=1;
                sprintf(text, "~OL~FMYou agree to a game of connect 4 with ~FG%s\n",user->c4_opponent->name);  
                write_user(user,text);
                print_board(user);
       		print_board(user->c4_opponent);
		write_user(user->c4_opponent,"~OL~FMIt is now your turn!\n");
		sprintf(text,"~OL~FMIt is now ~FG%s's ~FMturn.\n",user->c4_opponent->name);
		write_user(user,text);
                return;
                }
           else {
                sprintf(text, "~OL~FG%s ~FMwants to play a game of Connect 4 with you\n",user->name);
                write_user(user->c4_opponent,text);
                sprintf(text,"~OL~FMYou ask ~FG%s ~FMto play a game of Connect 4.\n",user->c4_opponent->name);  
                write_user(user,text);
                return;
                }
        }
if (!user->c4_opponent) {
        write_user(user,"You can't really move if your not playing with anyone you know!\n");
        return;
        }
if (user->c4_opponent->c4_opponent!=user) {
        write_user(user,"Wait till they accept first eh!\n");
        return;
        }
key=word[1][0]-'0';
if (user->turn) {
if (key == 27) return;
        if ((key > 0) && (key < 8)) {
                for (y=6;y>0;y--) {
                    f = y;
     if (user->board[key][f]==0 || user->c4_opponent->board[key][f]==0)
     break;
     }
     user->board[key][f] = 1;
     user->c4_opponent->board[key][f] = 2;
     print_board(user);
     print_board(user->c4_opponent);
     dropped++;
     f = 0;
     user->turn=0;
     user->c4_opponent->turn=1;
     sprintf(text,"~OL~FYIt is now ~FR%s's ~FYturn\n",user->c4_opponent->name);
     write_user(user,text);
     write_user(user->c4_opponent,"~OL~FYIt is now your turn\n");
     }
}
else { write_user(user,"It is not your turn\n");
        return;
        }
          if (!connect_win(user)) return;
          if (connect_win(user)) {
                sprintf(text,"~OL~FM%s ~FYkicked some ass at this game...\n",user->name);
                }
     else if (connect_win(user->c4_opponent)) {
                sprintf(text,"~OL~FM%s~FY kicked some ass at this game...\n",user->c4_opponent->name);
                }
          else if (dropped==42) {
                sprintf(text,"~OL~FTDag nab it, it was a tie between you 2!\n");
                }
print_board(user);
print_board(user->c4_opponent);
write_user(user,text);
write_user(user->c4_opponent,text);
for (y=1;y<7;y++) {
	for(x=1;x<8;x++) {
	user->board[x][y]=0;
	user->c4_opponent->board[x][y]=0;
	}
    }
user->c4_opponent->c4_opponent=NULL;
user->c4_opponent=NULL;
}

/* Draws the board, and shows where each users pieces are.*/
print_board(user)
UR_OBJECT user;
{
int x,y;
write_user(user,"\n\n");
write_user(user,"~FT+---+---+---+---+---+---+---+\n");
for (y=1;y<7;y++) {
        for (x=1;x<8;x++) {
               write_user(user,"~FT|");
               if (user->board[x][y] == 0) write_user(user,"   ");
               if (user->board[x][y] == 1) write_user(user,"~OL~FR X ~RS");
               if (user->board[x][y] == 2) write_user(user,"~OL~FW O ~RS");
               if (x == 7) write_user(user,"~FT|\n");
          }
          write_user(user,"~FT+---+---+---+---+---+---+---+\n");
     }
     write_user(user,"~OL~FM  1   2   3   4   5   6   7\n");
}

/* Scans through the board and checks for a winner. */
connect_win(user)
UR_OBJECT user;
{
int x,y,p;
for (y=1;y<7;y++) {
        for (x=1;x<8;x++) {
                for (p=1;p<3;p++) {
                    if (x+3<8) {
                    if ((user->board[x][y] == p) && (user->board[x+1][y] == p) && (user->board[x+2][y] == p) && (user->board[x+3][y] == p)) return p; 
                    }
                    if (y+3<7) {
                    if ((user->board[x][y] == p) && (user->board[x][y+1] == p)  && (user->board[x][y+2] == p) && (user->board[x][y+3] == p)) return p; 
                    }
                    if ((x+3<8) && (y+3<7)) {
                    if ((user->board[x][y] == p) && (user->board[x+1][y+1] == p)  && (user->board[x+2][y+2] == p) && (user->board[x+3][y+3] == p)) return p;
                    }
                    if ((x-3>0) && (y+3<7)) {
                    if ((user->board[x][y] == p) && (user->board[x-1][y+1] == p)  && (user->board[x-2][y+2] == p) && (user->board[x-3][y+3] == p)) return p;
                    }
               }
          }
     }
     return 0;
}

