/*** Get user struct pointer from name ***/
UR_OBJECT get_opp(name)
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


/*** This function initiates a duel between two users.
    Calls resolve_fight to resolve the fight. --RobinHood  ***/
duel(UR_OBJECT att)
{
UR_OBJECT def;   /* The defender. */
char buff[OUT_BUFF_SIZE+1];
enum win_fl win_stat;
int str1, str2;
        
    if (!(strcmp(att->room->name,MAINROOM))) {
        write_user(att,"~OLNo dueling in the main room :P\n");
        return;  
        }
    if (word_count < 2) {   /* User typed "go" only. */
        write_user(att, "\n~FT*****~OL Duel Status~RS~FT *****\n");
                if (fighting) {
                        sprintf(buff, "~OLLocation:~RS %s\n", fighter[0]->room->name);
                        write_user(att, buff);
                        write_user(att, "~OL~FBAggressor       Defender\n");
                        sprintf(buff, "%-15s %-15s\n\n",fighter[0]->name,fighter[1]->name);
                }
                else sprintf(buff, "Umm... maybe you should start one.\n");
        write_user(att, buff);
        return;
        }
/* At this point, 2 (or more) words have been entered. */
    def = get_opp(word[1]);
        if (att == def) {
                write_user(att, "\nYou like beating yourself, don't you?\n");
                return;
        }
        if (!fighting) {      /* Start new fight. */
                if ((def == NULL) || (att->room != def->room)) {
                       sprintf(buff, "\nYou'd love to start a fight, but %s is not here.\n", word[1]);
                        write_user(att, buff);
                        return;
                }
/* Fight exists so stick them in the fighter's list. */
                fighter[0] = att;  /* Attacker.  */
                fighter[1] = def;  /* Defender.  */
    
/* Next 4 lines added for random message determination --Slugger */
                sprintf(fight_buff, chal_text[ rand() % num_chal_text ], att, def);
                write_room(def->room, fight_buff);
                write_user(def, CHAL_LINE);
                write_user(def, CHAL_LINE2);
                write_user(att, CHAL_ISSUED);
         
                fighting = TRUE;
                fighter[0]->melee_status = FIGHTING;
                fighter[1]->melee_status = FIGHTING;
                return;
        }
/* fight is TRUE */
    if (att->melee_status != FIGHTING) {
        write_user(att, "\nA fight has already been started.\n");
                return;
        }
    if (fighter[1] == att) {   /* Defender typed 'fight'. */
            if ((!strcmp(word[1], "Yes")) || (!strcmp(word[1], "Y"))) {
            reset_duel();
            str1 = get_ftr_str(fighter[0]->level);
            str2 = get_ftr_str(fighter[1]->level);
            win_stat = resolve_fight(str1, str2);
            switch (win_stat) {
    /* All cases have been modified to include the random messages  --Slugger */
                
                            case DRW1 :
                                    sprintf(fight_buff, tie1_text[ rand() % num_tie1_text ], fighter[0]->name,fighter[1]->name);
                                    write_room(fighter[0]->room, fight_buff);
                                    break;
                            case WIN1 : 
                                    sprintf(fight_buff, wins1_text[ rand() % num_wins1_text ], fighter[0]->name,fighter[1]->name);
                                    write_room(fighter[0]->room, fight_buff);
                                    disconnect_user(fighter[1]);
                                    break;
                            case WIN2 :
                                    sprintf(fight_buff, wins2_text[ rand() % num_wins2_text ], fighter[1]->name,fighter[0]->name);
                                    write_room(fighter[1]->room, fight_buff);
                                    disconnect_user(fighter[0]);
                                    break;  /* Move winner to f1. */
                            case DRW2 :
                                    sprintf(fight_buff, tie2_text[ rand() % num_tie2_text ], fighter[0]->name,fighter[1]->name);
                                    write_room(fighter[1]->room, fight_buff);
                                    disconnect_user(fighter[0]);
                                    disconnect_user(fighter[1]);
                                    break;
                            default : write_user(fighter[0], "Error-fight failed\n");
                }
                return;
}
                else if ((!strcmp(word[1], "No")) || (!strcmp(word[1], "N"))) {
                        sprintf(fight_buff, wimp_text[ rand() % num_wimp_text ],fighter[1]);
                        write_room(fighter[1]->room, fight_buff);
                        reset_duel();
                        return;
                }
                else write_user(att, "\nYou are already involved in a fight!\n");
        }
        else /* Aggressor tried to start another fight. */
                write_user(att, "\nYou must wait for a reply to your challenge.\n");
        return;
}

/* Resets the fight status. */
reset_duel()
{
        fighting = FALSE;
        fighter[0]->melee_status = NONE;
        fighter[1]->melee_status = NONE;
    return;
}

/* Resolve the duel */
int resolve_fight(int str1, int str2)
{
    int mult;   /* Multiplier to levels. Used in calc. the winner. */
    int i, dice;    /* The dice value to roll. */
    float temp1, temp2;
    enum win_fl win;
                            
/* Make weaker fighter even weaker for probability reasons. */
/* Don't do this for gang fights bcause we may have an overflow. */
        temp1 = str1;
        temp2 = str2;
        if (str1 > str2) {
            temp2 /= (temp1 / temp2);   
            str2 = temp2 + 1;   /* add 1 bcause temp2 may be 0.2234 etc */
        }
        else if (str2 > str1) {
            temp1 /= (temp2 / temp1);
            str1 = temp1 + 1;
        }
    
/* resolve the fight. */
    win = 0;
    dice = str1 + str2;
    if ((rand() % dice) < str1) win += 1;  /* WINNER1 */
    if ((rand() % dice) < str2) win += 2;  /* WINNER2 */
/* If a draw, then fight again. */
    if ((win == DRW1) || (win == DRW2)) {      /* This makes getting a draw */
        win = 0;                            /* much harder. A draw only  */
        if ((rand() % dice) < str1) win += 1;  /* occurs if the fight has   */
        if ((rand() % dice) < str2) win += 2;  /* two draws.                */
    }
    return win;
}
            
/*** Calc. strength of fighter depending on his/her level. ***/
int get_ftr_str(int lvl)
{   
int str, i;
    
    str = 5;
    for (i = 0; i < lvl; ++i) str *= 2; /* Get strength. */
    return str;
}

