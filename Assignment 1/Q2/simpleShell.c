#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

/*
Author: Shah Tausif Iqbal - 260515282
McGill University
*/

//Initialize methods to be used
void initialize(char *args[]);
void jobs();
void redirect_output(char *args[]);
static void signalHandler(int sig);
static void childSignalHandler(int sig);
void addToJobList(char *args[], int process_pid);

int commandNumber=1;
struct node *head_job = NULL;
struct node *current_job = NULL;


//Gets the line from stdin and parses it into args. Also gives us whether it is a background or an output redirection call
int getcmd(char *line, char *args[], int *background, int *redirection)
{
	int i = 0;
	char *token, *loc;

	//Copy the line to a new char array because after the tokenization a big part of the line gets deleted since the null pointer is moved
	char *copyCmd = malloc(sizeof(char) * sizeof(line) * strlen(line));
	sprintf(copyCmd, "%s", line);

	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *redirection = 0;
        *loc=' ';
    //Check if redirection is specified
	} else if((loc = index(line, '>')) != NULL){
        *redirection = 1;
        *background = 0;
    } else {
        *redirection = 0;
		*background = 0;
    }
        

	//Create a new line pointer to solve the problem of memory leaking created by strsep() and getline() when making line = NULL
    char *lineCopy = line;
    
	while ((token = strsep(&lineCopy, " \t\n")) != NULL) {
		for (int j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
        if (strlen(token) > 0){
            args[i++] = token;
        }
            
            
	}

	return i;
}

//initializes all the arguments to be null
void initialize(char *args[]) {
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}
	return;
}

//Signal Handler for the parent process
static void signalHandler(int sig){
    //do nothing when this method is called
    //this method is implemented to ignore the signal(int sig) being passed
}

//Signal Handler for the child process
static void childSignalHandler(int sig){
    //Checks if Ctrl-C is pressed or not. If pressed, exits out of the child process
    if(sig == SIGINT) exit(0);
}

struct node {
	int number; // the job number
	int pid; // the process id of the a specific process
	struct node *next; // when another process is called you add to the end of the linked list
};

/* Add a job to the list of jobs
 */
void addToJobList(char *args[], int process_pid) {

    struct node *job = malloc(sizeof(struct node));
    
    

	//If the job list is empty, create a new head
	if (head_job == NULL) {
		job->number = 1;
		job->pid = process_pid;

		//the new head is also the current node
		job->next = NULL;
		head_job = job;
		current_job = head_job;
	}

	//Otherwise create a new job node and link the current node to it
	else {

		job->number = current_job->number + 1;
		job->pid = process_pid;

		current_job->next = job;
		current_job = job;
		job->next = NULL;
	}
}




//Function for output redirection
void redirect_output(char *args[]) {
    int i = 0;
    char *filename = "";
    //Loop till the end of arguments
    while (args[i] != '\0') {
      // Checks where the > sign is
      if (strcmp(args[i++], ">") == 0) {
        if (args[i] != NULL) {
        //Stores the argument where the filename is specified after the > sign
        
        filename = args[i];
        args[i] = NULL;
        args[i - 1] = NULL;
        //Opens filename with the name stored. Creates one if no file exists
        int filedescriptor = open(filename, O_CREAT | O_WRONLY | O_CLOEXEC, 0777 );
  
        close(1);
        //Check dup on man
        dup(filedescriptor);
        close(filedescriptor);
            
        //Command is executed here and instead of outputting on screen it prints the output inside the file descriptor, because of the dup call
        execvp(args[0], args);
    
        } else {
          perror("No filename provided");
          exit(1);
        }
      }
    }
}

void jobs(){
    
    if(head_job != NULL) {
        
        struct node *someNode = head_job;
        

        //loop through all the nodes till we hit NULL 
        while(someNode!=NULL){
            int status;
            char *stat = malloc(sizeof(char) + 20);
            pid_t process_pid;
            //Checks process status through waitpid
            process_pid=waitpid(someNode->pid, &status, WNOHANG);
            if(process_pid == someNode->pid) stat="Changed";
            else if(process_pid == 0) stat="No change";
            else stat = "Done";

            //Print the node for the jobs to be displayed
            printf("Number: %d ; Pid: %d; Status: %s\n", someNode->number, someNode->pid, stat);
            someNode = someNode->next;
        }

        

    } else {
        printf("There are no jobs");
    } 
    
}


int main(void) {

    //Parent signal handler for Control-C and Control-Z. The
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);

	char *args[20];
    int background, redirection;
    pid_t pid, wpid;
    int status;


    char *user = getenv("USER");
    if (user == NULL) user = "User";
    
    
    time_t now;
    srand((unsigned int) (time(&now)));

	char str[sizeof(char)*strlen(user) + 4];
    sprintf(str, "\n%s>> ", user);
    


	while (1) {
        initialize(args);
        //Setting up background and redirection to be 0 but to be changed as we parse the input from stdin
        background = 0;
        redirection = 0;
        char *loc;
		int length = 0;
		char *line = NULL;
		size_t linecap = 0; // 16 bit unsigned integer
		sprintf(str, "\n%s>> ", user);
		printf("%s", str);

		/*
		Reads an entire line from stream, storing the address of
		the buffer containing the text into *lineptr.  The buffer is null-
		terminated and includes the newline character, if one was found.
		check the linux manual for more info
		*/


        length = getline(&line, &linecap, stdin);
        
		if (length < 0) { //if argument is empty
			exit(-1);
        }

        int cnt  = getcmd(line, args, &background, &redirection);
        
        //Checker to check if fg is called or not
        int isFgCalled = 0;
        

        //Check for exit, cd, jobs and fg and execute them as it cannot be executed by execvp

        if(args[0] != NULL){
            
                        if (!strcmp("exit", args[0])) { // returns 0 if they are equal , then we negate to make the if statment true
                            
                            
                            printf("You're trying to exit \n");
                            
                            exit(0);
                            
                        }  
                        
                        if (!strcmp("cd", args[0])) {
                            
                                        int result = 0;
                                        if (args[1] == NULL) { // this will fetch home directory if you just input "cd" with no arguments
                                            char *home = getenv("HOME");
                                            if (home != NULL) {
                                                result = chdir(home);
                                            }
                                            else {
                                                printf("cd: No $HOME variable declared in the environment");
                                            }
                                        }
                                        //Otherwise go to specified directory
                                        else {
                                            result = chdir(args[1]);
                                        }
                                        if (result == -1) fprintf(stderr, "cd: %s: No such file or directory", args[1]);
                            
                        }

                        if(!strcmp("jobs", args[0])){
                            jobs();
                        }

                        if(!strcmp("fg", args[0])){
                            isFgCalled = 1;
                        }

                        

                        
                
            
       }

        
        

       
      
       
       pid = fork();
        
        

        if (pid == 0) {

            //Signal Handler for child process
            signal(SIGINT, childSignalHandler);

           
            
            if(isFgCalled == 1){
                if(args[1] == NULL){
                    printf("Usage - fg [job_number] and job number could be found by calling jobs\n");
                    exit(0);
                } else {
                    //Implement fg here
                }
            }
            
            // Child process
          if(redirection == 1){
            //Redirect output to a filename
            redirect_output(args);

          } else {

                int w;
                w = rand() % 10;
                sleep(w);


                // All the built in functions that can be executed by execvp are called here
                // This includes: pwd, ls, cat, cp, more etc

                if (execvp(args[0], args) == -1) {
                    perror("lsh");
                }
          }

          exit(EXIT_FAILURE);

        } else if (pid < 0) {
              // Error forking
              perror("lsh");
        } else {
              // Parent process
              // If background job
              if(background == 1){

                  // Add the pid to the background and not wait for the child process to complete
                  addToJobList(args, pid);

                  printf("Job with pid: %d added to background", pid);

              } else {

                // if not background, then wait for the child process to complete
                do {
                    
                    wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));

              }
              
                

              
              
        }


        
      
        
		
        free(line);

        }
        
        


}
