#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct newJob{
    pid_t pid;  //add PID
    char commands[1024]; 
} newJob;

static newJob alljobs[100]; //no more than 100 suspended jobs at one time 
static int total_jobs = 0; //counting total # of jobs

// void signalHandler(int signum){
//     if(signum == SIGINT || signum == SIGQUIT || signum == SIGTSTP){
//         if(alljobs[total_jobs-1].pid != 0){
//             kill(alljobs[total_jobs-1].pid, signum);
//         }
//     }
    
// }

void signalHandler(int signum){
    if(signum == SIGINT || signum == SIGQUIT || signum == SIGTSTP){
        if(alljobs[total_jobs-1].pid != 0){
            kill(alljobs[total_jobs-1].pid, signum);
        }
    }
    
}

void signalIgnore(){
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGTSTP, signalHandler);
}

void addJob(char** commands, pid_t pid){ //"jobs" command
    //alljobs[total_jobs].commands[0] = '\0';
    alljobs[total_jobs].pid = pid; 
    strncpy(alljobs[total_jobs].commands, *commands, 1000);
   // strcpy(alljobs[total_jobs].pid, pid);
    //printf("Passed in char* commands: %s\n", *commands);
    //printf("Command: %s Total jobs: %d\n", alljobs[total_jobs].commands, total_jobs);
    total_jobs++;
}

void removeJob(int index){ //"fg" command
    //grab pid from index 
    if(index < 1 || index > total_jobs){
        //printf("index: %d, total jobs: %d\n", index, total_jobs);
        fprintf(stderr, "Error: invalid job\n");
        return; 
    }
    pid_t pid = alljobs[index-1].pid;
    for(int i = index; i < total_jobs; i++){
        // alljobs[i-1].pid = alljobs[i].pid; //updating PIDs
        // alljobs[i-1]=alljobs[i];
        alljobs[i-1].pid = alljobs[i].pid;
        strcpy(alljobs[i-1].commands, alljobs[i].commands);
    }
    //alljobs[total_jobs-1].pid = 0; 
    //alljobs[total_jobs - 1].commands[0] = '\0';
    total_jobs--;
    kill(pid, SIGCONT);
    
    
}

void printJobs(){
    for(int i = 0; i < total_jobs; i++){
        printf("[%d] %s\n", i+1, alljobs[i].commands); 
    }
}

void shell_prompt(){
    char curr[PATH_MAX]; 
    char* base; //base of directory 

    char* buffer = (char*)malloc(1500);  //pointer to the user input string 
    size_t buffer_size = 0; //initialize buffer 
    size_t inputs;  
    
    signalIgnore();
    

    while(1){
        if(getcwd(curr, sizeof(curr)) != NULL){

            if(strcmp(curr, "/") == 0){ //root directory 
                printf("[nyush /]$ ");
            }
            else{
                        // printf("Current directory: %s\n", curr);
                base = strrchr(curr, '/'); //find last occurence of '/'
                if(base != NULL){
                        // if(base == curr){
                        //     printf("[nyush /]$ ");
                        // }
                        
                        printf("[nyush %s]$ ",base+1); //print characters after the '/'
                        
                        fflush(stdout);
                }
            }
           
            
        }    

        inputs = getline(&buffer, &buffer_size, stdin);  
        //in case of failure  
        if(inputs == -1){
            break; 
        }

        if(strlen(buffer) > 0 && buffer[strlen(buffer)-1] == '\n'){
            buffer[strlen(buffer)-1] = '\0'; //taking out newline character from user command 
        }  


        //parsing the user input string 
        //printf("Buffer before strtok_r: %s\n", buffer);
        char* tempBuffer = (char*)malloc(1500);
        char bufferCopy[1024];
        strncpy(bufferCopy, buffer, 1024);
        char* token = strtok_r(bufferCopy," ", &tempBuffer); //grabbing command name
        char* cmd_args[1024];
        int i = 0;
        while(token != NULL){
            cmd_args[i++] = token;
            token = strtok_r(NULL, " ", &tempBuffer); //pointing to the next 'token'
            //printf("%s\n",token);
        }
        //printf("Buffer bafter strtok_r: %s\n", buffer);
        cmd_args[i] = NULL; //null terminating argument list 
        //printf("%s", cmd_args[0]);
        // printf("going into this if statement after strcmp()\n");
        
        if(strcmp(cmd_args[0],"|") == 0 || strcmp(cmd_args[0],"<") == 0|| strcmp(cmd_args[0],">") == 0 || strcmp(cmd_args[0],"<<") == 0 || strcmp(cmd_args[0],">>") == 0){
            fprintf(stderr, "Error: invalid command\n");
        }

        //implementing 'cd' command
        if(strcmp(cmd_args[0], "cd") == 0){ //checking if user typed in 'cd' 
            if(cmd_args[1] == NULL){  //if there are 0 arguments
                fprintf(stderr, "Error: invalid command\n");
            }
            else if(i > 2){ //if there are 2+ arguments
                fprintf(stderr, "Error: invalid command\n");
            }
            else if(chdir(cmd_args[1]) == -1){ //directory doesn't exist 
                fprintf(stderr, "Error: invalid directory\n");
            }
            else{
                getcwd(curr, sizeof(curr)); //update cwd
            }
        }
        /* Implementing 'exit' */
        else if(strcmp(cmd_args[0], "exit") == 0){ //checking if user typed in 'exit'
            if (i > 1){ //if there is 1+ arguments
                fprintf(stderr, "Error: invalid command\n");
            }
            /* HANDLE SUSPENDED JOBS CASE WITH ELSE IF*/
            else if(total_jobs > 0){
                fprintf(stderr, "Error: there are suspended jobs\n");
            }
            else{
                break; //terminate the shell 
            }
        } 

        /*Implementing 'jobs'*/
        else if(strcmp(cmd_args[0],"jobs") == 0){ //checking if user typed in 'jobs'
            if(i > 1){ //if there is any arguments 
                fprintf(stderr, "Error: invalid command\n");
            }
            
            printJobs();
        }
        
        /*implementing 'fg'*/
        else if(strcmp(cmd_args[0],"fg") == 0){
            if(cmd_args[1] == NULL){  //if there are 0 arguments
                fprintf(stderr, "Error: invalid command\n");
            }
            else if(i > 2){ //if there are 2+ arguments
                fprintf(stderr, "Error: invalid command\n");
            }
            else if(cmd_args[1] != NULL){
                int index;
                index = atoi(cmd_args[1]);
                pid_t pid = alljobs[index-1].pid;
                char cmdBuffer[1024];
                strncpy(cmdBuffer, alljobs[index-1].commands, 1024);
                char* cmdPointer = cmdBuffer; 
                removeJob(index);
                int status; 
                waitpid(pid, &status, WUNTRACED);
                if(WIFSTOPPED(status)){
                    addJob(&cmdPointer, pid);
                }
            }
        }
        
            
        
        else{ /* Commented out Pipe code from scratch file was here right after else */

           
            //executing user commands 
            pid_t pid = fork();
            if (pid < 0) {
                // fork failed (this shouldn't happen)
                exit(1); //exit failure /**FIX THIS (maybe change to break;) 
            } else if (pid == 0) {
                // child (new process)
                char file_path[5000];
                if(cmd_args[0][0] == '/'){ //absolute path 
                    strcpy(file_path, cmd_args[0]);
                }

                else if(cmd_args[0][0] == '.' || strchr(cmd_args[0], '/') && cmd_args[0][0] != '/'){ //relative path
                    sprintf(file_path, "%s/%s", curr, cmd_args[0]);
                }

                else{ //only base name 
                    sprintf(file_path, "/usr/bin/%s", cmd_args[0]); //formatting file path
                }

                //OUTPUT REDIRECTION
                char* file_out = NULL;
                char* out_mode = NULL; //"> or >>"

                for(int i = 1; cmd_args[i]!=NULL; i++){
                    if(strcmp(cmd_args[i], ">") == 0 || strcmp(cmd_args[i], ">>") == 0 || strcmp(cmd_args[i],"<") == 0){
                        file_out = cmd_args[i+1];
                        out_mode = cmd_args[i]; 
                        cmd_args[i] = NULL; //so "ls"  DOESN'T INTERPRET AS DIRECTORY 
                        // break;
                        if(file_out == NULL){
                                fprintf(stderr, "Error: invalid command\n");
                        }

                        int file_desc; 
                        if(strcmp(out_mode, ">") == 0){ //> output redirection
                            
                            file_desc = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                            if (file_desc == -1){
                                fprintf(stderr, "Error: can't open file\n");
                                exit(1);
                            }
                            if (dup2(file_desc, 1) == -1){ 
                                fprintf(stderr, "Error: failed to redirect output\n");
                                exit(1);
                            }
                            close(file_desc); 
                        }

                        else if(strcmp(out_mode, "<") == 0){ //input redirection <
                            file_desc = open(file_out, O_RDONLY); 
                            if(file_desc == -1){
                                fprintf(stderr, "Error: invalid file\n");
                                exit(1);
                            }
                            if(dup2(file_desc, 0) == -1 ){//stdin 
                                fprintf(stderr, "Error: failed to redirect input\n");
                                exit(1);
                            } 
                            close(file_desc);
                        }

                        else{ //append >>
                            file_desc = open(file_out, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                            if (file_desc == -1){
                                fprintf(stderr, "Error: can't open file\n"); 
                                exit(1);
                            }
                            if (dup2(file_desc, 1) == -1){ 
                                fprintf(stderr, "Error: failed to redirect output\n");
                                exit(1);
                            }
                            close(file_desc); 
                        }
                        

                    }
                }
                execv(file_path, cmd_args);
                fprintf(stderr, "Error: invalid program\n");
                exit(1);
            } else {
               // int status; 
                // parent
                //waitpid(-1, &status, 0);

                int status; 
                pid_t pid = waitpid(-1, &status, WUNTRACED);
                // printf("Status: %d\n", status);
                if(WIFSTOPPED(status)){ //if the child process was stopped by delivery of a signal
                    //printf("Buffer: %s\n", buffer);
                    
                    addJob(&buffer, pid);
                } 
                
                // else if (WIFEXITED(status)){
                //     if (total_jobs > 0){
                //         continue;
                //     }
                    
                // }
                
                
            } 
        }    
        //free(buffer); //free the user command
        //buffer = NULL; 
        //buffer_size = 0; 
    
    }   
}

int main(){
    //signalIgnore();
    shell_prompt();
    return 0;
}


//SOURCES
/*


https://www.educative.io/answers/how-to-convert-a-string-to-an-integer-in-c 
https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program 

https://www.ibm.com/docs/it/ssw_aix_72/osmanagement/c_jobcontrol.html



https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/


https://c-for-dummies.com/blog/?p=1112 

https://c-for-dummies.com/blog/?p=1216

https://www.tutorialspoint.com/c_standard_library/c_function_strrchr.htm

https://www.programiz.com/c-programming/library-function/string.h/strcmp


https://www.includehelp.com/c/process-identification-pid_t-data-type.aspx#:~:text=pid_t%20data%20type%20stands%20for,or%20we%20can%20say%20int).

https://www.geeksforgeeks.org/exec-family-of-functions-in-c/


https://www.tutorialspoint.com/unix_system_calls/waitpid.htm

https://www.scaler.com/topics/c/string-comparison-in-c/

https://www.digitalocean.com/community/tutorials/execvp-function-c-plus-plus


https://www.ibm.com/docs/en/i/7.3?topic=functions-snprintf-print-formatted-data-buffer

https://www.tutorialspoint.com/c_standard_library/c_function_sprintf.htm

https://stackoverflow.com/questions/41884685/implicit-declaration-of-function-wait

https://www.delftstack.com/howto/c/c-print-to-stderr/#use-the-fprintf-function-to-print-to-stderr-in-c

https://cboard.cprogramming.com/c-programming/164689-how-get-users-home-directory.html

https://www.geeksforgeeks.org/different-ways-to-copy-a-string-in-c-c/



https://www.geeksforgeeks.org/dup-dup2-linux-system-call/

https://www.geeksforgeeks.org/piping-in-unix-or-linux/



https://www.tutorialspoint.com/c_standard_library/c_function_memcpy.htm

https://people.cs.rutgers.edu/~pxk/416/notes/c-tutorials/pipe.html

https://stackoverflow.com/questions/33508997/waitpid-wnohang-wuntraced-how-do-i-use-these

https://stackoverflow.com/questions/1258550/why-should-you-use-strncpy-instead-of-strcpy



*/