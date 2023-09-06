#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


int main()
{
    const char sentencebreaker[] = "&";
    const char wordbreaker[] = " ";
    size_t length = 0;
    
    char *input=malloc(sizeof(char)*length);
    char *sentence = NULL;
    char *words = NULL;
    size_t read = 0;
    char *myargs[9];
    myargs[0] = input;
    myargs[8] = NULL;
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
        int j =0;
        
        char *sentence_token = strtok_r(input, sentencebreaker, &sentence);
        while (sentence_token != NULL)
        {
            i++;
            char *inner_token = strtok_r(sentence_token, wordbreaker, &words);
            if (inner_token != NULL && i == 1 && (strcmp(inner_token, "BYE") == 0 || strcmp(inner_token, "BYE\n") == 0))
            {
                printf("Goodbye, exiting shell ... \n");
                return 0;
            }
            myargs[0] = inner_token;
            //printf("%s", inner_token);
            while (inner_token != NULL)
            {   
                inner_token = strtok_r(NULL, wordbreaker, &words);
                j++;
                myargs[j] = inner_token;
            }
            sentence_token = strtok_r(NULL, sentencebreaker,
                                      &sentence);
            }
        int rc = fork();
        if (rc == 0)
        { // child (new process)
            execvp(myargs[0], myargs);
            printf(" Error: 2cwshould not be here \n");
        }
        else
        { // parent goes down this path (main)
            int wc = wait(NULL);
        }
    }
    exit(EXIT_SUCCESS);
}
