#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#define pass (void)0
int main()
{
    const char sentencebreaker[] = "&&";
    const char wordbreaker[] = " ";
    size_t length = 0;
    char *input=malloc(sizeof(char)*length);
    char *sentence = NULL;
    char *words = NULL;
    size_t read = 0;
    char *myargs[10];
    myargs[0] = input;
    char *programpath = "/bin/";
    char *redirection[20];
    redirection[0] = input;
    myargs[10] = NULL;
    char combined[100];
    while (1)
    {
        printf("xxr_dash$: ");
        read = getline(&input, &length, stdin);
        if (read == -1)
        {
            printf("Error reading input.\n");
            return 1;
        }
        int i = 0;
        
	    int x;
        for (x = 0; input[x] != '\0'; x++) {
        if (input[x] == '\n') {
            // Shift characters to the left to overwrite the '\r' character
            memmove(&input[x], &input[x + 1], strlen(input) - x);
            }
        }
        char *sentence_token = strtok_r(input, sentencebreaker, &sentence);
        while (sentence_token != NULL)
        {
            
        //printf("%s \n", sentence_token);
        //printf(" ---------separate here---------- sentence token above \n");    
            i++;
            char *inner_token = strtok_r(sentence_token, wordbreaker, &words);
            if (inner_token != NULL && i == 1 && (strcmp(inner_token, "BYE") == 0 || strcmp(inner_token, "BYE\n") == 0))
            {
                printf("Goodbye, exiting shell ... \n");
                exit(EXIT_SUCCESS);
            }
            int j = 0;
            snprintf(combined, sizeof(combined), "%s%s", programpath, inner_token);
            myargs[0] = strdup(combined);
            // myargs[0] = programpath;
            // strcat(myargs[j], inner_token);
            while (inner_token != NULL)
            {   
                inner_token = strtok_r(NULL, wordbreaker, &words);
                j++;
                myargs[j] = inner_token;
            }
            printf("%s  %s  %s  %s  %s\n", myargs[0], myargs[1], myargs[2], myargs[3], myargs[4]);
            printf(" ---------separate here---------- arg above\n");
            int rc = fork();
            if (rc == 0){
                int logic_imp;
                int redirect_counter = 0;
                int doublecheck =0;
                for(logic_imp = 0; logic_imp <j;logic_imp++)
                {
                    if(strcmp(myargs[logic_imp], ">")==0){
                        for(x = logic_imp; x <= j; x ++){
                            redirection[redirect_counter] = myargs[x+1];
                            myargs[logic_imp] = NULL;
                            redirect_counter++;
                        }
                        doublecheck ++;
                    }
                    if(doublecheck >1){
                        printf("Invalid Input, returning shell . . . \n");
                        break;
                    }
                    else{
                        pass;
                    }
                }
                int fd;
                if(doublecheck ==1){
                    fd = open(*redirection, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(fd == -1 && doublecheck ==1){
                        printf("File Open Failed . . . \n");
                        printf("returning to shell . . .\n");
                    }
                    dup2(fd, 1);
                }
                printf("%s \n", *redirection);
                printf(" ---------separate here---------- redir above\n");
                printf("%d \n", doublecheck);
                printf(" ---------separate here---------- doublecheck above\n");
                execv(myargs[0],myargs);
                if(execv(myargs[0], myargs)==-1){
                    printf("Error: Commond Execution Failed . . . \n");
                    printf("returning to shell . . .\n");
                }

            }
            
            else{
                wait(NULL);
            }
            memset(myargs, '\0', sizeof(myargs));
            sentence_token = strtok_r(NULL, sentencebreaker,
                                      &sentence);

        }

        //for (x = 0; x <i; i++)




        // int rc = fork();
        // if (rc == 0)
        // { // child (new process)
        //     printf("sentence count is %d, word count is %d: \n", i, j);
        //     execvp(myargs[0], myargs);
        //     printf(" Error: 2cwshould not be here \n");
        // }
        // else
        // { // parent goes down this path (main)
            
        //     int wc = wait(NULL);
        //     printf("%d \n", wc);
        //      printf(" ---------separate here---------- paraent PID \n");
        // }
    }
    exit(EXIT_SUCCESS);
}
