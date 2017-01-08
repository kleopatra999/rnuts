/******************************************************************
rNUTS autorun, Created by:  fck
******************************************************************/

#include <strings.h>
#include <stdio.h>

main(int argc, char *argv[])
{
FILE *fp;
int is_active,go=1;
char temp[30], temp2[30];

printf("\n\033[1m\033[33m-- BOOT\033[0m : Booting autorun.ex for rNUTS 3.0.2\n");
printf("        : Running with \033[31m<pid: %d>\n",getpid());

/** Run in background **/
switch(fork()) {
	case -1: go=0;
                 printf("          - Boot Failed...Shuting down autorun.ex\n");
	case  0: break;
	default: sleep(1); 
                 exit(0);
                 printf("          - Continuing in Background\n");
	}
sprintf(temp2,"./%s",argv[1]);
while(go) {
	is_active = 0;
	if (fp=fopen("bootexit","r")) {
		fscanf(fp,"%s",temp);
		fclose(fp);
		if (!strcmp(temp,"SHUTDOWN")) {
			unlink("bootexit");
			exit(0);
			}
		if (!strcmp(temp,"REBOOT")) {
			unlink("bootexit");
			is_active = 0;
			}
		}

	system("ps x>ps_x_file");
	fp=fopen("ps_x_file","r");

	while(!feof(fp)) {
		fscanf(fp,"%s",temp);
		if(!strcmp(temp,temp2)) is_active = 1;
		}
	fclose(fp);
	if(!is_active) {
		system(temp2);
		exit(0);
		}
	unlink("ps_x_file");
	sleep(30);
	}
}
