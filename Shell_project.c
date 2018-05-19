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

job * jobList; /* declaration of job list */

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

#define ANSI_COLOR_RED "\x1b[31m"

#define ANSI_COLOR_GREEN "\x1b[32m"

#define ANSI_COLOR_YELLOW "\x1b[33m"

#define ANSI_COLOR_BLUE "\x1b[34m"

#define ANSI_COLOR_MAGENTA "\x1b[35m"

#define ANSI_COLOR_CYAN "\x1b[36m"

#define ANSI_COLOR_RESET "\x1b[0m"



// -----------------------------------------------------------------------
//                            RESPAWNABLE
// -----------------------------------------------------------------------

void respawn(char *args[]){

	int pid_fork = fork();

	if (pid_fork<0){
		printf("ERROR, en la creacion del hijo\n");
		fflush(stdout);
		exit(EXIT_FAILURE);
	}
	else if (pid_fork == 0){
		restore_terminal_signals();
		//setpgid(getpid(), getpid());
		execvp(args[0],args);
		printf("Error, command not found: %s\n",args[0]);
		fflush(stdout);
		exit(EXIT_FAILURE);
	}
	else{ //Respawnable
		printf("Respawnable job running... pid: %d,command: %s\n", pid_fork, args[0]);
		new_process_group(pid_fork);
		/* Adding job to the list */
		block_SIGCHLD();
		job * newJob = new_job(pid_fork, args[0], RESPAWNABLE, args);
		add_job(jobList, newJob);
		unblock_SIGCHLD();
	}
}

// -----------------------------------------------------------------------
//                            HANDLER
// -----------------------------------------------------------------------

void handler(int signum){
	int pid_aux;
	int info_aux;
	enum status status_res_aux;
	int status_aux;
	job * current = jobList->next;

	/* Iterate entire list until find job */

	block_SIGCHLD();
	while(current!=NULL){

		job * nextJob = current->next;

		pid_aux = waitpid(current->pgid,&status_aux, WUNTRACED | WNOHANG | WCONTINUED);

		if(pid_aux==current->pgid){ //child found
			status_res_aux = analyze_status(status_aux, &info_aux);
			if(status_res_aux==CONTINUED){
				printf("Resuming job with pid: %d, command: %s\n",pid_aux, current->command);
				current->state = BACKGROUND;
			}
			else if(status_res_aux==SUSPENDED){
				printf("Suspending job with pid: %d, command: %s\n",pid_aux, current->command);
				current->state = STOPPED;
			}
			else {//EXITED OR SIGNALED
				printf("Removing job with pid:  %d, command: %s\n",pid_aux, current->command);
				if(current->state == RESPAWNABLE){
					respawn(current->args); //respawn the job
				}
			}
		}


		current = nextJob;
	}
	unblock_SIGCHLD();

}




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

	jobList = new_list("jobList");

	ignore_terminal_signals();
	signal(SIGCHLD, handler);
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{

		printf(ANSI_COLOR_CYAN"COMMAND->~"ANSI_COLOR_RESET);
		printf(ANSI_COLOR_YELLOW "%s " ANSI_COLOR_RESET,getcwd(directory, sizeof(directory)));


		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		if(args[0]==NULL) continue;   // if empty command

		/* CD COMMAND */
		if(!strcmp(args[0], "cd")){
			if (args[1] != NULL) {
				if (chdir(args[1])) {
					printf("Path is not valid\n"); //chdir(args[1]) != 0
					fflush(stdout);
				}
			} else {
				chdir(getenv("HOME"));//Return home route
			}
			continue;
		}

		/* JOBS COMMAND : show job list */
		if(!strcmp(args[0], "jobs")){
			block_SIGCHLD();
			if(empty_list(jobList)){
				printf("ERROR: EMPTY LIST\n");
			}
			else{
				print_job_list(jobList);
			}
			unblock_SIGCHLD();
			continue;
		}

		/* FG COMMAND: change a job from suspended or stopped to foreground */
		if(!strcmp(args[0], "fg")){
			block_SIGCHLD();
			job * fgJob;
			int fg_pid;
			int fg_status;
			int fg_info;
			enum status fg_status_res;

			if(args[1]!=NULL){ //command has an argument
				fgJob = get_item_bypos(jobList, atoi(args[1]));
			}
			else{ //first job from the list chosen
				fgJob = get_item_bypos(jobList, 1);
			}
			fg_pid = fgJob->pgid;
			fgJob->state = FOREGROUND;
			killpg(fg_pid, SIGCONT); //send signal to group to continue
			set_terminal(fg_pid); //give terminal control to the job
			waitpid(fg_pid, &fg_status, WUNTRACED);
			set_terminal(getpid());
			fg_status_res = analyze_status(fg_status, &fg_info);

			printf("Foreground pid: %d,command: %s, status: %s,info: %d\n",fg_pid,fgJob->command,status_strings[fg_status_res],fg_info);

			if(fg_status_res==SIGNALED || fg_status_res==EXITED){
				delete_job(jobList, fgJob);
					printf("Removing job with pid:  %d\n",fg_pid);
				}


			if(fg_status_res==SUSPENDED){ //change state if job is suspended
				fgJob->state = STOPPED;
			}

			unblock_SIGCHLD();
			continue;
		}

		/* BG COMMAND: change job from supended to background */

		if(!strcmp(args[0],"bg")){
			job * bg_job;
			block_SIGCHLD();
			if(args[1]!=NULL){
				bg_job = get_item_bypos(jobList, atoi(args[1]));
				killpg(bg_job->pgid, SIGCONT); //send signal to group to continue in bg
			}
			else{
				printf("ERROR: POSITION IN THE JOB LIST IS REQUIRED\n");
			}
			unblock_SIGCHLD();
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
						set_terminal(getpid());	//Como está en foreground asignamos terminal al proceso
				}
			execvp(args[0],args);
			printf("Error, command not found: %s\n",args[0]);
			fflush(stdout);
			exit(EXIT_FAILURE);
			continue;
		}
		else if(background==2){ // respawnable

			printf("Respawnable job running... pid: %d,command: %s\n", pid_fork, args[0]);

			/* Adding job to the list */
			block_SIGCHLD();
			job * newJob = new_job(pid_fork, args[0], RESPAWNABLE, args);
			add_job(jobList, newJob);
			unblock_SIGCHLD();

			/*
			if(info!=1){
				printf("Foreground pid: %d,command: %s, status: %s,info: %d\n",pid_fork,args[0],status_strings[status_res],info);
				fflush(stdout);
			}
			*/

		}else if (background==1){ //bg
			printf("Background job running... pid: %d,command: %s\n", pid_fork, args[0]);
			fflush(stdout);

			/* Adding job to the list */
			block_SIGCHLD();
			job * newJob = new_job(pid_fork, args[0], BACKGROUND, NULL);
			add_job(jobList, newJob);
			unblock_SIGCHLD();

		}
		else{ //fg

			set_terminal(pid_fork);
			pid_wait = waitpid(pid_fork,&status, WUNTRACED | WCONTINUED);
			set_terminal(getpid());
			status_res = analyze_status(status, &info);


			/* Adding suspended job to jobList */
			if(status_res==SUSPENDED){
				block_SIGCHLD();
				job * newJob = new_job(pid_fork, args[0], STOPPED, NULL);
				add_job(jobList, newJob);
				unblock_SIGCHLD();
			}

			if(info!=1){

			printf("Foreground pid: %d,command: %s, status: %s,info: %d\n",pid_fork,args[0],status_strings[status_res],info);
			fflush(stdout);

		}
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
