/****************** Header file for NUTS version 3.3.3 ******************/


extern char text[ARR_SIZE*2];

/* variables added/modified May, 1997 -- KnightShade */
extern int word_count;

#define SYSMACROFILE "sysmacros"   /* name of system macros file */
#define SYSACTIONFILE "sysactions"

#define INPUT_SEP '\\' /*  Use a backslash to separate commands */

#define NUM_MAC_PARAM 10 /* maxium # of macro paramters = 10 */

struct macro_struct {
	char *name; /* the name of the macro */
	char *value; /* the replacement text of the macro */
	int star;	/* the starting word # for a $* expansion */
	int is_running; /* is the macro running or not (boolean flag) */
	struct macro_struct *next; /* pointer to next macro in list, or NULL */
	};

typedef struct macro_struct *MACRO;

MACRO system_macrolist;
MACRO system_actionlist;

struct ignore_struct {
	char name[USER_NAME_LEN+1]; /* name of person to ignore */
	struct ignore_struct *next; /* pointer to next person in list, or NULL */
};

enum result {SUCCESS,FAIL};  

/* function prototypes */

void free_macrolist(MACRO *list);
void free_macro(MACRO macro);

int save_macros(MACRO *list,char *filename);
int load_macros(MACRO *list,char *filename);

void macro(MACRO *list,char *input,UR_OBJECT user,int is_action);

void delete_macro(MACRO *list,char *name);
int update_macro(MACRO macro,char *value);
int add_macro(MACRO *list,char *name,char *value);

MACRO allocate_macro(char *macroname,char *macrovalue);
MACRO ismacro(MACRO *list,char *name);
MACRO findmacro(MACRO *list,char *name);
MACRO findprevmacro(MACRO *list,char *name);

void parse(UR_OBJECT user,char *string);
int macroexpand(MACRO *list,char *expansion,UR_OBJECT user);



void fix_separators(char *input,char *output);
char todigit(char c);

void ignore(UR_OBJECT user, char *name);
void unignore(UR_OBJECT user, char *name);
void showignore(UR_OBJECT user);
int isignored(UR_OBJECT user, char *name);

