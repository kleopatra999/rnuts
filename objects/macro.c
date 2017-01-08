/***************************************************************************
 * Functions dealing with macros, and how to parse them                    *
 **************************************************************************/
void list_user_macros(UR_OBJECT user) {
	MACRO macro;
	write_user(user,"System macros\n");
	for (macro = system_macrolist; macro!=NULL; macro = macro -> next) {
		sprintf(text,"%s=%s\n",macro->name,macro->value);
		write_user(user,text);
	}
	write_user(user,"\nPersonal macros\n");
	for (macro = user->macrolist; macro!=NULL; macro = macro -> next) {
		sprintf(text,"%s=%s\n",macro->name,macro->value);
		write_user(user,text);
	}
} /* list_user_macros */

void list_action_macros(UR_OBJECT user) {
	MACRO macro;
	write_user(user,"System actions\n");
	for (macro = system_actionlist; macro!=NULL; macro=macro->next) {
		sprintf(text,"%s=%s\n",macro->name,macro->value);
		write_user(user,text);
	}
}


void free_macrolist(MACRO *list) { 
	MACRO macro,next;

	macro = *list;

	while ( macro ) {
		next=macro->next;
		free_macro(macro);
		macro=next;
	} 

	*list=NULL;

} /* free_macrolist */

void free_macro(MACRO macro) {
	free(macro->name);
	free(macro->value);
	free(macro);
} /* free_macro */

int save_macros(MACRO *list,char *filename) { 
	FILE *fp;	
	MACRO macro;

	if (! ( fp=fopen( filename,"w" ) ) ) {
		sprintf(text,"Can't open file <%255s> for writing",filename);
		write_syslog(text,1);
		return FAIL;
	}
	for (macro = *list; macro != NULL; macro= macro->next) {
		sprintf(text,"%s=%s\n",macro->name,macro->value);
		fputs(text,fp);
	}

	fclose(fp);
	return SUCCESS;
} /* save_macros */

int load_macros(MACRO *list,char *filename) {
	char macrostring[255],*name,*value,*equals;
	int len;
	FILE *fp;

	if (! ( fp=fopen( filename,"r" ) ) ) {
		return FAIL; /* not everyone has macros */
	}
	while ( fgets(macrostring,255,fp) ) { 
		/* remove trailing \n */	
		macrostring[strlen(macrostring)-1]='\0';

		/* split macro into name/value at = sign */
		equals = strchr(macrostring,'=');
		if (equals)	
			equals[0]='\0'; 
		if (equals && ( equals[1] ) )  /*if theres a value portion */
			;
		else {
			sprintf(text,"In function load_macros, file <%s>\n",filename);	
			write_syslog(text,1);

			sprintf(text,"Read bad value for macro:<%s>\n",macrostring);
			write_syslog(text,1);
			continue;
		}
		name=macrostring;
		value=equals+1; /*  macro value starts one char past the = */
		add_macro(list,name,value);
	}
	fclose(fp);

	return SUCCESS;
} /* load_macros */


void macro(MACRO *list,char *input,UR_OBJECT user,int is_action) {

	char *equals,*value,*name;
	MACRO macro;

	/* If just .macro by itself, list the macros */

	if ((!input[0])&&(is_action==0)) {
		list_user_macros(user);
		return;
	}
	else if ((!input[0])&&(is_action==1)) {
		list_action_macros(user);
		return;
	}

	equals=strchr(input,'=');

	if (!equals) {
		write_user(user,"You need an to put an = sign in\n");
		return;
	}
	*equals='\0'; /* end the string at the equals sign */

	name=input; /* macro name is the part before the equals sign */
	value=equals+1; /* macro value starts after the equals sign */

	if (!value[0]) {		/* if macro has no value portion, delete macro */ 
		delete_macro(list,name);
		return;
	}
	if (macro = findmacro(list,name) ) {
		update_macro(macro,value);
		return;
	}
	add_macro(list,name,value);
} /* macro */

void delete_macro(MACRO *list,char *name) {

	MACRO macro,prev;
	prev = NULL; /* no previous macro on the list */

	for (macro = *list; macro != NULL; macro = macro -> next) {

		if (! strcmp(macro->name,name) )  {
			/* if we don't have a previous macro,change head of list */
			if (prev) 
				prev -> next = macro -> next;
			else
				*list = macro->next;
			free_macro(macro);
			return;
		}
		prev=macro;     
	} /* for */ 

	/* if we exit the loop without deleting finding anything, we didn't
	   have anything to delete anyway :-) */

} /* delete_macro */

int update_macro(MACRO macro,char *value) {
	char *newvalue;
	newvalue=(char *) malloc(strlen(value)+1);
	if (newvalue) {
		strcpy(newvalue,value);
		free(macro->value);
		macro->value=newvalue;
		return SUCCESS;
	}

	write_syslog("Couldn't allocate memory in function update_macro",1);
	return FAIL;
} /* update macro */
  
add_macro(MACRO *list,char *name,char *value) {
	MACRO macro,prev;
	int num,star;
	char *replace,replace_type;

	if ( !( macro = allocate_macro(name,value) ) ) {
		write_syslog("Out of memory in function add_macro",1);
		return FALSE;
	}

	strcpy(macro->name,name);
	strcpy(macro->value,value);

	/* find macro whose name alphabetically preceeds "name" */
	prev = findprevmacro(list,name);

	if (prev == NULL ) { /* no precursor, add to start of list */
		macro->next = *list;
		*list = macro;
	} else {
		macro->next=prev->next; /* point to next entry in list */
		prev->next = macro;		/* insert macro into list */
	}	
	/* Store the starting place of a $* expansion, for speed */ 

	star = 0;

	while ( replace=strchr(value,'$') ){
		replace_type = replace[1];
		if ( isdigit(replace_type) ){
			num = todigit(replace_type); 
			if (num > star)
				star = num;
		}

		if (replace[1])
			value=replace+2;
		else
			break; /* Hit end of string, we're done */

	} /*while*/

	star++; /* we start with first *unused* word */
	macro->star = star;
	
	macro->is_running = FALSE; /* starts out *not* running :-) */
} /* add macro */

MACRO allocate_macro(char *macroname,char *macrovalue) {
	MACRO macro;
	char *name,*value;

	/* try to allocate the space we need */

	name=(char *) malloc(strlen(macroname)+1);
	value=(char *) malloc(strlen(macrovalue)+1);
	macro = (struct macro_struct *) malloc(sizeof(struct macro_struct));

	/* if we can't, free it all, and exit */

	if (! (name && value && macro ) ){
	
		if (!name) free(name);
		if (!value) free(value);
		if (!macro) free(macro);

		return NULL;
	}

	/* assign good defaults to our macro */
	macro->next = NULL;
	macro->name = name;
	macro->value = value;
	macro->star = 0;
	return macro;
} /* allocate macro */
	
/* Macroexpand:
Function: Macroexpand looks at the command word, and
		  checks to see if this is a macro. If so, it expands the macro,
		  and returns the results via the output parameter, expansion.
		  If the system flag is set, macroexpand expands system macros,
		  otherwise, it expands user macros.
Parameters:
	user: pointer to the user structure of the user currently being parsed
	expansion: a buffer where the output results are to be stored
	list: the address of a list of macros

Global Variables: The word array, wordcount

Exit status: macroexpand returns TRUE if a macro was expanded, 0 otherwise.
*/

int macroexpand(MACRO *list,char *expansion, UR_OBJECT user) {
	MACRO macro;
	int i;
	char *replacement,*string,replace_type;

	expansion[0]='\0'; /* initialize properly */

	if ( !( macro = ismacro( list, word[0] ) ) ) 
		return FALSE; /* command not a macro */
	if (macro -> is_running)
		return FALSE; /* we expand macros only once */

	string=macro->value;
	
	while (*string) {
		replacement=strchr(string,'$');
	
		if (!replacement){
			strcat(expansion,string);
			break;
		} 
		strncat(expansion,string,replacement-string);                        
		/* copy macro text up to the point of the replacment */

		replace_type=replacement[1]; /* first char after $ is type */

		if ( isdigit(replace_type) ) /* if $[0-9] */
			strcat(expansion,word[ todigit(replace_type) ]);

		if (replace_type == '*') {     /* if $* */
			for (i=macro->star; i < word_count; i++){ /* copy remaining words */
				strcat(expansion,word[i]);
				strcat(expansion," ");
			}
			if( expansion[0] )	
				expansion[strlen(expansion)-1]='\0'; /* remove final space */
		} 


		if ( !replacement[1] ) /* if we're at end of string, get out */
			break;
		string = replacement+2;

	} /* while */
	macro -> is_running = TRUE;
	parse(user,expansion); /* re-parse the expanded string */
	macro -> is_running = FALSE;
	return TRUE;
} /* macroexpand */

char todigit(char c) {
	 return (c-'0');
}

MACRO ismacro(MACRO *list,char *command) {
	MACRO macro;

	for ( macro= *list; macro != NULL; macro=macro->next ) {
		if ( !strncmp( macro->name,command,strlen(command) ) )
			return macro;
	}
	return NULL;
} /* ismacro */		

/* like "ismacro", but look for an exact match */
MACRO findmacro(MACRO *list,char *command) {
	MACRO macro;

	for ( macro= *list; macro != NULL; macro=macro->next ) {
		if ( !strcmp( macro->name,command) )
			return macro;
	}
	return NULL;
}
/* this function returns the macro whose name alphabetically preceeds
   "name", or NULL if no such macro is found */
MACRO findprevmacro(MACRO *list, char *name) {
	MACRO current,prev;

	prev=NULL;

	for (current = *list; current != NULL; current = current -> next) {
		if ( strcmp(current->name,name) > 0 ) /* alphabetically preceeds */
			 return prev;
		prev = current; /* store previous entry */
	}

	return prev;
}
/* parse:
Function: Parse processes a raw command line into commands to be
		  executed, breaking lines into command lines, and expanding
		  macros as appropriate.

Paramters: user -- a pointer to the user structure of the user running the
				   commands

	   string -- the text to be parsed into commands

Return Value: None (destructively parses string)

Description: Parse strips a command line into one or more commands,
			 separated by the character INPUT_SEP.

			 If, however, there are two consecutive instances of the
			 INPUT_SEP character, they are ignored, and reduced to
			 a single instance using the function separator_fix before
			 the command is executed.
 
			Each command is checked for to see if it has a valid
			user level, or failing that, a system level, macro expansion.
			The results of such an expansion, if any, are placed in
			the string "expansion", and reparsed.
 
*/

void parse(UR_OBJECT user,char *string) {
	char *separator,*rest, *start,*temp;
	char expansion[1024];
	int done=FALSE;

	start=string; /* store starting position of string */
	expansion[0]='\0';	/* clear expansion string */
	
	while (!done) {

		separator = strchr(string, INPUT_SEP); 
		if(separator){
			if (separator[1] == INPUT_SEP){  
				 string = separator+2; /* skip past both separators */
				 continue;
				}
			else {
				separator[0]='\0'; /* turn it into the end of string */
				}
				
		} /* if */

		clear_words();
		word_count=wordfind(start);

		/* if we have macros, re-parse the string */
		if ( strlen(word[0]) > 0 ) { /* if we really have a command */

			if ( !macroexpand(&(user->macrolist),expansion,user) && 
			     !macroexpand(&system_macrolist,expansion,user) &&
			     !macroexpand(&system_actionlist,expansion,user)) {

				/* convert any multiple separators to single for execution */
				fix_separators(start,expansion);
			
				if ( user->command_mode || word[0][0] == '.')
					exec_com(user,expansion);
				else  
					if (nuts_talk_style==1) say_new(user,expansion);
					else say_old(user,expansion);
			}

		}

		if (separator) { 		/* we have more */
			start = separator+1; /* point past separator character */
			string=start;
		} else {
			done=TRUE;
		}
	} /*while*/
} /* parse */


/* Hey, I need at least *one* obfuscated function, and this *is*
   a faster way to write it :-)   
*/
void fix_separators(char *input,char *output) {
	while( *input) {
		 if ( *input == INPUT_SEP && *(input+1) == INPUT_SEP) 
			*input++;
		*output++=*input++;
	}
	*output='\0';
} /* fix_separators */

/****************************************************************************
 Code for the "ignore" and related functions

This code needs to be thoroughly tested to make sure it is not causing
SIGSEGV crashes of Sherwood
****************************************************************************/

void ignore(UR_OBJECT user,char *name) {
  	struct ignore_struct *node;		

	if (word_count<2) {
		write_user(user,"Usage: ignore <username>\n");
		return;
	}
	node = (struct ignore_struct *) malloc(sizeof(struct ignore_struct));

	if (! node) {
		sprintf(text,"In function ignore: Out of memory!\n");
		write_syslog(text,1); 
		write_user(user,"Ignore ran out of memory: .tell KnightShade\n");
		return;
	}
	strcpy(node->name,name);
	node->next = user->ignorelist;
	user->ignorelist = node;
} /* ignore */

void unignore(UR_OBJECT user,char *name) {
	struct ignore_struct *prev, *current;	

	prev=NULL;
	for (current = user->ignorelist; current !=NULL; current=current->next) {
		if ( !strcmp(current->name,name) ) {
			
			if (prev) 
				prev->next = current->next; 	
			else
				user->ignorelist = current -> next;

			free(current);
			return;
		}
		prev = current;
	}
} /* unignore */			

void showignore(UR_OBJECT user) {
	struct ignore_struct *node;
	int linewidth=0;
	write_user(user,"People you are ignoring:\n");
	for (node = user->ignorelist; node != NULL; node = node-> next ) {
		sprintf(text,"%s\t",node->name);
		write_user(user,text);
		if (linewidth == 4)
			write_user(user,"\n");
		linewidth= (linewidth + 1 ) % 5;
	}
	if (linewidth)
		write_user(user,"\n");
} /* showignore */

/* is user ignoring name? */
int isignored(UR_OBJECT user,char *name) {
	struct ignore_struct *node;
	for (node = user->ignorelist; node != NULL; node = node -> next) {
		if (!strcmp(node->name, name) )
			return TRUE;
	}
	return FALSE;
}
