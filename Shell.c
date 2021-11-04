#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define BUFSIZE 100
#define CWD_SIZE 100
#define PATH_MAX 100



void changeDirectory(char **tokens) {

    if(chdir(tokens[1])!=0){
	    printf("Shell: Incorrect command\n");
    }

}

char** parseInput(char *input){
    int position = 0;
    int buf_size = BUFSIZE;

    char **tokens = malloc(buf_size*sizeof(char*));
    char *token;
    while((token = strsep(&input," ")) != NULL) {
        if(strlen(token) == 0) {
            continue;
        }
        tokens[position++] = token;
    }

    tokens[position] = NULL;

    // for(int i=0;tokens[i];i++) {
    //     printf("%d : len = %lu :%s\n",i,strlen(tokens[i]),tokens[i]);
    // }

    return tokens;

}
    

void executeCommand(char **tokens)
{
	// This function will fork a new process to execute a command
    if(strcmp(tokens[0],"cd") == 0) {
        changeDirectory(tokens);
    }
    else {
        int rc = fork();

        if(rc < 0) {
            //printf("Forking failed\n");
            exit(0);
        }
        else if(rc == 0) {
            // child process

            // restoring the signals
            signal(SIGINT, SIG_DFL);        // for ctrl + c    
		    signal(SIGTSTP, SIG_DFL);       // for ctrl + z

            if(execvp(tokens[0], tokens) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }   
        }
        else {
            // parent process
            int rc_wait = wait(NULL);
        } 
    }

    
    
}


void executeSequentialCommands(char **tokens)
{	
	// This function will run multiple commands in sequence
    int start_of_cmd = 0;
    int i=0;
    while(tokens[i]){
        
        while(tokens[i] && strcmp(tokens[i],"##")!=0) {
            i++;
        }
        tokens[i] = NULL;
        executeCommand(&tokens[start_of_cmd]);
        i++;
        start_of_cmd = i;
        
    }
}


void executeParallelCommands(char **tokens)
{
	// This function will run multiple commands in parallel
    int rc;
    int i = 0;
    int start_of_cmd = 0;
    while(tokens[i]){

        while(tokens[i] && strcmp(tokens[i],"&&")!=0) {
            i++;
        }
        tokens[i] = NULL;
        if(strcmp(tokens[start_of_cmd],"cd") == 0) {
            changeDirectory(&tokens[start_of_cmd]);
        }
        else {
            // fork 
            rc = fork();
            if(rc < 0) {
                // forking failed
                exit(0);
            }
            else if(rc == 0) {
                // forked new child process

                // restoring the signals
                signal(SIGINT, SIG_DFL);
		        signal(SIGTSTP, SIG_DFL);

                if(execvp(tokens[start_of_cmd],&tokens[start_of_cmd]) == -1) {
                    printf("Shell: Incorrect command\n");
                    exit(1);
                }
            }
            else {
               int rc_wait = wait(NULL);
            }
        }
        i++;
        start_of_cmd = i;
    }

     
   
}


void executeCommandRedirection(char **tokens)
{
	// This function will run a single command with output redirected to an output file specificed by user
    int total_tokens = 0;   
    
    for(int i=0;tokens[i]!=NULL;i++) {
        total_tokens++;
    }

    int rc = fork();

    if(rc < 0) {
        // forking failed
        exit(0);
    }
    else if(rc == 0) {
        // child process

        // restoring the signals
        signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);

        // ------- Redirecting STDOUT --------

        close(STDOUT_FILENO);

        int f = open(tokens[total_tokens-1],O_CREAT | O_WRONLY | O_APPEND);         // tokens[total_tokens-1] represents filename (to redirect output)

       

        char **execCommand;
        execCommand = (char**)malloc((total_tokens-1)*sizeof(char*));

        for(int i=0;i<total_tokens-2;i++) {
            execCommand[i] = tokens[i];
        }
        execCommand[total_tokens-2] = NULL;


        if(execvp(execCommand[0],execCommand) == -1) {
            printf("Shell: Incorrect command\n");
            exit(1);
        }

        // closing file descriptor
        fflush(stdout); 
        close(f);
    }
    else {
        int rc_wait = wait(NULL);
    }

}
char cwd[PATH_MAX];     // stored the location of current directory


void signalHandler(int sig) {
    printf("\n");
	printf("%s$", cwd);
	fflush(stdout);
    return;

}

int main()
{

    // Initial declarations
    signal(SIGTSTP,&signalHandler);              // for ctrl + z
    signal(SIGINT,&signalHandler);              // for ctrl + c
	

	
	while(1)	// This loop will keep your shell running until user exits.
	{
		// Print the prompt in format - currentWorkingDirectory$
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
	   	{
	       printf("%s$", cwd);
	   	}

		
		// accept input with 'getline()'
        char *input = NULL;
        size_t size = 0;
        
        int byte_read  = getline(&input,&size,stdin);
        int len = strlen(input);
        input[len-1] = '\0';



		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		char **args = parseInput(input); 		
		
		if(strcmp(args[0],"exit") == 0 || strcmp(args[0], "EXIT") == 0)	
		{
            // When user uses exit command.
			printf("Exiting shell...\n");
			break;
		}


        // checking the type of command 
        // 0 represents simple command
        // 1 represents parallel execution
        // 2 represents sequential execution
        // 3 represents redirection of output 


        int type = 0;
        for(int i=0; args[i]!= NULL ; i++) {

            if(strcmp(args[i],"&&") == 0) {
                // parallel execution
                type = 1;
                break;
            }
            else if(strcmp(args[i],"##") == 0) {
                // sequential execution
                type = 2;
                
                break;
            }
            else if(strcmp(args[i],">") == 0) {
                // redirection execution
                type=3;
                break;
            }
        }

        if(type == 1) {
            // This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
            executeParallelCommands(args);
        }
        else if(type == 2) {
            // 	executeSequentialCommands();	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
            executeSequentialCommands(args);
        }
        else if(type == 3) {
            // This function is invoked when user wants redirect output of a single command to and output file specificed by user
            executeCommandRedirection(args);
        }
        else {
            // simple command
            executeCommand(args);
        }
				
	}
	
	return 0;
}