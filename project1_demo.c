#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#define pass (void)0
int NUM_OF_PROGRAM_PATH = 0;
size_t length = 100;
const char sentencebreaker[] = "&";
const char wordbreaker[] = " ";
char *defaultpath = ("/bin/");
char **programpath = NULL;

void add_path_root(const char *path)
{
    char *newPath = strdup(path);
    programpath = (char **)realloc(programpath, (NUM_OF_PROGRAM_PATH + 1) * sizeof(char *));
    programpath[NUM_OF_PROGRAM_PATH] = newPath;
    NUM_OF_PROGRAM_PATH++;
}
void reset_path()
{
    NUM_OF_PROGRAM_PATH = 1;
    free(programpath);
    programpath = NULL;
    programpath = (char **)realloc(programpath, (NUM_OF_PROGRAM_PATH + 1) * sizeof(char *));
    programpath[0] = "/bin/";
}
void readfromfile(const char *command)
{
    int status = system(command);
    if (status == -1)
    {
        perror("Error executing command");
    }
    else
    {
        printf("Command executed with status %d\n", status);
    }
}
void printdot()
{
    printf(". ");
    printf(". ");
    printf(". \n");
}
char *fetch_program_path(char *arg1, const char *arg2)
{
    int comb_length = snprintf(NULL, 0, "%s %s", arg1, arg2);
    char *combined = (char *)malloc(comb_length + 1);
    if (combined == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(0);
    }
    snprintf(combined, comb_length + 1, "%s%s", arg1, arg2);
    return combined;
}
int main(int argc, char *argv[])
{
    char *input = malloc(sizeof(char) * length);
    char *sentence = NULL;
    char *words = NULL;
    size_t read = 0;
    char *myargs[500];
    myargs[0] = input;
    add_path_root(defaultpath);
    char *redirection[20];
    redirection[0] = input;
    myargs[500] = NULL;
    if (argc == 1)
    {
        printf("Entering Interactive Mode:\n");
        printdot();
        if (input == NULL)
        {
            perror("Memory allocation error");
            return 1;
        }
        while (1)
        {
            int i = 0;
            int j = 0;
            printf("dash>: ");
            read = getline(&input, &length, stdin);
            if (read == -1)
            {
                printf("Error reading input.\n");
                return 1;
            }
            int x;
            for (x = 0; input[x] != '\0'; x++)
            {
                if (input[x] == '\n')
                {
                    memmove(&input[x], &input[x + 1], strlen(input) - x);
                }
            }
            char *sentence_token = strtok_r(input, sentencebreaker, &sentence);
            while (sentence_token != NULL)
            {
                i++;
                char *inner_token = strtok_r(sentence_token, wordbreaker, &words);
                myargs[j] = inner_token;
                while (inner_token != NULL)
                {
                    inner_token = strtok_r(NULL, wordbreaker, &words);
                    j++;
                    myargs[j] = inner_token;
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "exit") == 0 || strcmp(myargs[0], "exit\n") == 0))
                {
                    printf("Goodbye, exiting shell ... \n");
                    exit(EXIT_SUCCESS);
                }
                else if (myargs[0] != NULL && (strcmp(myargs[0], "cd") == 0 || strcmp(myargs[0], "cd\n") == 0))
                {
                    if (j == 2)
                    {
                        printf("Change directory to: %s \n", myargs[1]);
                        chdir(myargs[1]);
                        break;
                    }
                    else
                    {
                        printf("Invalid directory path, returning to shell . . . \n");
                        break;
                    }
                }
                else if (myargs[0] != NULL && (strcmp(myargs[0], "path") == 0 || strcmp(myargs[0], "path\n") == 0))
                {
                    if (j == 1)
                    {
                        printf("Default Directory enabled \n");
                        reset_path();
                        break;
                    }
                    else
                    {
                        int path_num = 1;
                        while (1)
                        {
                            if (myargs[path_num] != NULL)
                            {
                                add_path_root(myargs[path_num]);
                                if (programpath[path_num] == NULL)
                                {
                                    printf("Path Add Failed \n");
                                    break;
                                }
                                printf("Path %s Added \n", programpath[path_num]);
                                printf("%d \n", path_num);
                                path_num++;
                            }
                            else
                                break;
                        }
                    }
                }
                else
                    pass;
                char *temp = NULL;
                temp = myargs[0];
                myargs[0] = strdup(fetch_program_path(programpath[0], myargs[0]));
                // printf("%s  %s  %s  %s  %s\n", myargs[0], myargs[1], myargs[2], myargs[3], myargs[4]);
                // printf(" ---------separate here---------- arg above\n");
                // for (x = 0; i < NUM_OF_PROGRAM_PATH; i++)
                // {
                //     printf("Program %d path: %s\n", i + 1, programpath[i]);
                // }
                pid_t childpid = fork();
                if (childpid == 0)
                {
                    int logic_imp;
                    int redirect_counter = 0;
                    int doublecheck = 0;
                    for (logic_imp = 0; logic_imp < j; logic_imp++)
                    {
                        if (strcmp(myargs[logic_imp], ">") == 0)
                        {
                            for (x = logic_imp; x <= j; x++)
                            {
                                redirection[redirect_counter] = myargs[x + 1];
                                myargs[logic_imp] = NULL;
                                redirect_counter++;
                            }
                            doublecheck++;
                        }
                        if (doublecheck > 1)
                        {
                            printf("Invalid Input, returning shell . . . \n");
                            break;
                        }
                        else
                        {
                            pass;
                        }
                    }
                    int fd;
                    if (doublecheck == 1)
                    {
                        fd = open(*redirection, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                        if (fd == -1 && doublecheck == 1)
                        {
                            printf("File Open Failed . . . \n");
                            printf("returning to shell . . .\n");
                        }
                        dup2(fd, 1);
                    }
                    execv(myargs[0], myargs);
                    int path_counter = 1; // start from 1 because default is 0
                    // printf("original path /bin/ not aviliable \n");
                    while (1)
                    {
                        // printf("Path Directory '%s' not accessable, trying alternative path %s . . . \n", programpath[path_counter - 1], programpath[path_counter]);
                        myargs[0] = strdup(fetch_program_path(programpath[path_counter], temp));
                        execv(myargs[0], myargs);
                        //    /bin/clear
                        path_counter++;
                        if (programpath[path_counter] == NULL)
                        {
                            printf("Invalid path, returning to shell \n");
                            exit(0);
                        }
                    }
                }
                else
                {
                    int status;
                    waitpid(childpid, &status, 0);
                }
                memset(myargs, '\0', sizeof(myargs));
                sentence_token = strtok_r(NULL, sentencebreaker,
                                          &sentence);
            }
        }
    }

    else if (argc == 2)
    {
        printf("Entering Batch Mode \n");
        printdot();
        FILE *batchFile = fopen(argv[1], "r");
        if (batchFile == NULL)
        {
            perror("Error opening batch file");
            return 1;
        }
        while (getline(&input, &length, batchFile) != -1)
        {
            int i = 0;
            int j = 0;
            int x;
            for (x = 0; input[x] != '\0'; x++)
            {
                if (input[x] == '\n')
                {
                    memmove(&input[x], &input[x + 1], strlen(input) - x);
                }
            }
            char *sentence_token = strtok_r(input, sentencebreaker, &sentence);
            while (sentence_token != NULL)
            {
                i++;
                char *inner_token = strtok_r(sentence_token, wordbreaker, &words);
                myargs[j] = inner_token;
                while (inner_token != NULL)
                {
                    inner_token = strtok_r(NULL, wordbreaker, &words);
                    j++;
                    myargs[j] = inner_token;
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "exit") == 0 || strcmp(myargs[0], "exit\n") == 0))
                {
                    printf("Goodbye, exiting shell ... \n");
                    exit(EXIT_SUCCESS);
                }
                else if (myargs[0] != NULL && (strcmp(myargs[0], "cd") == 0 || strcmp(myargs[0], "cd\n") == 0))
                {
                    if (j == 2)
                    {
                        printf("Change directory to: %s \n", myargs[1]);
                        chdir(myargs[1]);
                        break;
                    }
                    else
                    {
                        printf("Invalid directory path, returning to shell . . . \n");
                        break;
                    }
                }
                else if (myargs[0] != NULL && (strcmp(myargs[0], "path") == 0 || strcmp(myargs[0], "path\n") == 0))
                {
                    if (j == 1)
                    {
                        printf("Default Directory enabled \n");
                        reset_path();
                        break;
                    }
                    else
                    {
                        int path_num = 1;
                        while (1)
                        {
                            if (myargs[path_num] != NULL)
                            {
                                add_path_root(myargs[path_num]);
                                if (programpath[path_num] == NULL)
                                {
                                    printf("Path Add Failed \n");
                                    break;
                                }
                                printf("Path %s Added \n", programpath[path_num]);
                                printf("%d \n", path_num);
                                path_num++;
                            }
                            else
                                break;
                        }
                    }
                }
                else
                    pass;
                char *temp = NULL;
                temp = myargs[0];
                myargs[0] = strdup(fetch_program_path(programpath[0], myargs[0]));
                // printf("%s  %s  %s  %s  %s\n", myargs[0], myargs[1], myargs[2], myargs[3], myargs[4]);
                // printf(" ---------separate here---------- arg above\n");
                // for (x = 0; i < NUM_OF_PROGRAM_PATH; i++)
                // {
                //     printf("Program %d path: %s\n", i + 1, programpath[i]);
                // }
                pid_t childpid = fork();
                if (childpid == 0)
                {
                    int logic_imp;
                    int redirect_counter = 0;
                    int doublecheck = 0;
                    for (logic_imp = 0; logic_imp < j; logic_imp++)
                    {
                        if (strcmp(myargs[logic_imp], ">") == 0)
                        {
                            for (x = logic_imp; x <= j; x++)
                            {
                                redirection[redirect_counter] = myargs[x + 1];
                                myargs[logic_imp] = NULL;
                                redirect_counter++;
                            }
                            doublecheck++;
                        }
                        if (doublecheck > 1)
                        {
                            printf("Invalid Input, returning shell . . . \n");
                            break;
                        }
                        else
                        {
                            pass;
                        }
                    }
                    int fd;
                    if (doublecheck == 1)
                    {
                        fd = open(*redirection, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                        if (fd == -1 && doublecheck == 1)
                        {
                            printf("File Open Failed . . . \n");
                            printf("returning to shell . . .\n");
                        }
                        dup2(fd, 1);
                    }
                    execv(myargs[0], myargs);
                    int path_counter = 1; // start from 1 because default is 0
                    //printf("original path /bin/ not aviliable \n");
                    while (1)
                    {
                        //printf("Path Directory '%s' not accessable, trying alternative path %s . . . \n", programpath[path_counter - 1], programpath[path_counter]);
                        myargs[0] = strdup(fetch_program_path(programpath[path_counter], temp));
                        execv(myargs[0], myargs);
                        //    /bin/clear
                        path_counter++;
                        if (programpath[path_counter] == NULL)
                        {
                            printf("All PATH Unaviliable, returning to shell \n");
                            exit(0);
                        }
                    }
                }
                else
                {
                    int status;
                    waitpid(childpid, &status, 0);
                }
                memset(myargs, '\0', sizeof(myargs));
                sentence_token = strtok_r(NULL, sentencebreaker,
                                          &sentence);
            }
        }
    }
    else
    {
        printf("initilization Failed . . .\n");
        printf("Terminating\n");
        exit(0);
    }
    exit(EXIT_SUCCESS);
}
