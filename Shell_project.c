/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

#define ANSI_COLOR_RED "\x1b[31m"

#define ANSI_COLOR_GREEN "\x1b[32m"

#define ANSI_COLOR_YELLOW "\x1b[33m"

#define ANSI_COLOR_BLUE "\x1b[34m"

#define ANSI_COLOR_MAGENTA "\x1b[35m"

#define ANSI_COLOR_CYAN "\x1b[36m"

#define ANSI_COLOR_RESET "\x1b[0m"

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	char directory[1024]; /* current directory */

	void intro();
	intro(); /* show characters intro image */

ignore_terminal_signals();
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{

		printf(ANSI_COLOR_CYAN"COMMAND->~"ANSI_COLOR_RESET);
		printf(ANSI_COLOR_YELLOW "%s " ANSI_COLOR_RESET,getcwd(directory, sizeof(directory)));


		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		if(args[0]==NULL) continue;   // if empty command

		if(!strcmp(args[0], "cd")){
			if (args[1] != NULL) {
				if (chdir(args[1])) {
					printf("Path is not valid\n"); //chdir(args[1]) != 0
				}
			} else {
				chdir(getenv("HOME"));//Return home route
			}
			continue;
		}



		pid_fork = fork();
		if (pid_fork<0){
			printf("ERROR, en la creacion del hijo\n");
			fflush(stdout);
		  exit(EXIT_FAILURE);
		}
		else if (pid_fork == 0){
			restore_terminal_signals();
			setpgid(getpid(), getpid());
			if(!background){
						set_terminal(getpid());	//Como está en foreground asignamos terminal al grupo
				}
			execvp(args[0],args);
			printf("Error, command not found: %s\n",args[0]);
			continue;
			fflush(stdout);
			exit(EXIT_FAILURE);
		}
		else if(background==0){
			set_terminal(pid_fork);
			waitpid(pid_fork,&status, WUNTRACED | WCONTINUED);
			set_terminal(getpid());
			status_res = analyze_status(status, &info);
			printf("Foreground pid: %d,command: %s, status: %s,info: %d\n",pid_fork,args[0],status_strings[status_res],info);
			fflush(stdout);
		}else{
			printf("Background job running... pid: %d,command: %s\n", pid_fork, args[0]);
			fflush(stdout);
		}
	} // end while
}

void intro(){
	printf(ANSI_COLOR_CYAN"CREATED BY:\n\n"
"FFFFFFFFFFFFFFFFFFFFFF\n"
"F::::::::::::::::::::F\n"
"F::::::::::::::::::::Frrrrr   rrrrrrrrr   aaaaaaaaaaaaa  nnnn  nnnnnnnn\n"
"FF::::::FFFFFFFFF::::Fr::::rrr:::::::::r  a::::::::::::a n:::nn::::::::nn\n"
"  F::::::FFFFFFFFFF   r:::::::::::::::::r aaaaaaaaa:::::an::::::::::::::nn\n"
"  F:::::::::::::::F   rr::::::rrrrr::::::r         a::::ann:::::::::::::::n\n"
"  F:::::::::::::::F    r:::::r     r:::::r  aaaaaaa:::::a  n:::::nnnn:::::n\n"
"  F::::::FFFFFFFFFF    r:::::r     rrrrrrraa::::::::::::a  n::::n    n::::n\n"
"  F:::::F              r:::::r           a::::aaaa::::::a  n::::n    n::::n\n"
"  F:::::F              r:::::r          a::::a    a:::::a  n::::n    n::::n\n"
"FF:::::::FF            r:::::r          a::::a    a:::::a  n::::n    n::::n\n"
"F::::::::FF            r:::::r          a:::::aaaa::::::a  n::::n    n::::n\n"
"F::::::::FF            r:::::r           a::::::::::aa:::a n::::n    n::::n\n"
"FFFFFFFFFFF            rrrrrrr            aaaaaaaaaa  aaaa nnnnnn    nnnnnn\n\n"ANSI_COLOR_RESET);
printf("\n");
}
