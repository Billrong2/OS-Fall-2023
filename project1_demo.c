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
const char redirectionbreaker[] = ">";
char *defaultpath = ("/bin/");
char *defaultpath2 = ("./");
char **programpath = NULL;
void perrormsg()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));

}
void add_path_root(const char *path)
{
    char *newPath = strdup(path);
    programpath = (char **)realloc(programpath, (NUM_OF_PROGRAM_PATH + 1) * sizeof(char *));
    programpath[NUM_OF_PROGRAM_PATH] = newPath;
    NUM_OF_PROGRAM_PATH++;
}
void reset_path()
{
    NUM_OF_PROGRAM_PATH = 2;
    free(programpath);
    programpath = NULL;
    programpath = (char **)realloc(programpath, (NUM_OF_PROGRAM_PATH + 1) * sizeof(char *));
    programpath[0] = "/bin/";
    programpath[1] = "./";
}
void readfromfile(const char *command)
{
    int status = system(command);
    if (status == -1)
    {
        perrormsg();
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
    char *re_dir = NULL;
    char *words = NULL;
    size_t read = 0;
    char *myargs[500];
    myargs[0] = input;
    add_path_root(defaultpath);
    add_path_root(defaultpath2);
    char *redirection[20];
    redirection[0] = input;
    myargs[500] = NULL;
    if (argc == 1)
    {
        printf("Entering Interactive Mode:\n");
        printdot();
        if (input == NULL)
        {
            perrormsg();
            return 1;
        }
        while (1)
        {
            int sentence_count = 0;
            int i = 0;

            printf("dash>: ");
            read = getline(&input, &length, stdin);
            if (read == -1)
            {
                perrormsg();

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
            int redirect_count = 0;
            for (x = 0; input[x] != '\0'; x++)
            {
                if (input[x] == '&')
                {
                    sentence_count++;
                }
                else
                    pass;
            }
            
            char *sentence_token = strtok_r(input, sentencebreaker, &sentence);

            if (i == 0 && sentence_token == NULL && sentence_count >= 1)
            {
                perrormsg();
            }


            while (sentence_token != NULL)
            {

            for (x = 0; input[x] != '\0'; x++)
            {
                if (sentence_token[x] == '>')
                {
                    redirect_count++;
                }
                else
                    pass;
            }
                i++;
                char *redirection_token = strtok_r(sentence_token, redirectionbreaker, &re_dir);

                int redir_counter = 0;
                int j = 0;
                int k = 0;
                while (redirection_token != NULL)
                {
                    char *inner_token = strtok_r(redirection_token, wordbreaker, &words);
                    if (redir_counter == 0)
                    {
                        myargs[j] = inner_token;
                        while (inner_token != NULL)
                        {
                            inner_token = strtok_r(NULL, wordbreaker, &words);
                            j++;
                            myargs[j] = inner_token;
                        }
                    }
                    redir_counter++;

                    if (redir_counter > 0)
                    {
                        redirection[k] = inner_token;
                        while (inner_token != NULL)
                        {
                            inner_token = strtok_r(NULL, wordbreaker, &words);
                            k++;
                            redirection[k] = inner_token;
                        }
                    }

                    redirection_token = strtok_r(NULL, redirectionbreaker, &re_dir);
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "exit") == 0 || strcmp(myargs[0], "exit\n") == 0))
                {
                    if (j == 1)
                    {
                        printf("Goodbye, exiting shell ... \n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        perrormsg();
                        break;
                    }
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "cd") == 0 || strcmp(myargs[0], "cd\n") == 0))
                {
                    char cwd[256];
                    if (myargs[1] != NULL && myargs[2] == NULL)
                    {
                        if (getcwd(cwd, sizeof(cwd)) == NULL)
                        {

                            perrormsg();
                            
                        }
                        strcat(cwd, "/");
                        myargs[1] = strdup(fetch_program_path(cwd, myargs[1]));
                        chdir(myargs[1]);
                        if (chdir(myargs[1]) != 0)
                        {
                            perrormsg();
                            
                        }
                    }
                    else
                    {

                        perrormsg();
                        
                    }
                }
                pid_t childpid = fork();
                if (childpid == 0)
                {

                    if (myargs[0] != NULL && (strcmp(myargs[0], "path") == 0 || strcmp(myargs[0], "path\n") == 0))
                    {
                        if (j == 1)
                        {
                            reset_path();
                            exit(0);
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
                                        perrormsg();
                                        exit(0);
                                    }
                                    path_num++;
                                }
                                else
                                    exit(0);
                            }
                        }
                    }
                    else
                        pass;
                    char *temp = NULL;

                    temp = myargs[0];
                    myargs[0] = strdup(fetch_program_path(programpath[0], myargs[0]));

                    // printf("woshishabi2 %s %d \n", redirection_token,redir_counter);
                    if (redir_counter > 2)
                    {
                        perrormsg();
                        exit(0);
                    }

                    int fd;
                    if (sentence_count != 0 && myargs[0] == NULL)
                    {
                        perrormsg();
                        exit(0);
                    }
                    if (myargs[0] == NULL)
                    {
                        exit(0);
                    }
                    if (redirect_count > 0 && redirection[0] == NULL )
                    {
                        perrormsg();
                        exit(0);
                    }
                    if (redir_counter == 2)
                    {
                        if (redirection[1] != NULL)
                        {
                            perrormsg();
                            exit(0);
                        }
                        if (redirection[0] == NULL)
                        {
                            perrormsg();
                            exit(0);
                        }
                        fd = open(*redirection, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
                        if (fd == -1)
                        {
                            perrormsg();
                            exit(0);
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perrormsg();
                            exit(0);
                        }
                    }
                    execv(myargs[0], myargs);
                    int path_counter = 1; // start from 1 because default is 0
                    while (1)
                    {
                        myargs[0] = strdup(fetch_program_path(programpath[path_counter], temp));
                        execv(myargs[0], myargs);
                        path_counter++;
                        if (programpath[path_counter] == NULL)
                        {
                            perrormsg();
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
        FILE *batchFile;
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL)
        {
            perrormsg();
            return 1;
        }
        fseek(batchFile, 0, SEEK_END);
        long file_size = ftell(batchFile);
        if (file_size == 0)
        {
            perrormsg();
            exit(0);
        }
        else
        {
            fseek(batchFile, 0, SEEK_SET);
        }
        int i = 0;
        int sentence_count = 0;
        while (getline(&input, &length, batchFile) != -1)
        {
            int x;
            for (x = 0; input[x] != '\0'; x++)
            {
                if (input[x] == '\n')
                {
                    memmove(&input[x], &input[x + 1], strlen(input) - x);
                }
            }
            int redirect_count = 0;
            for (x = 0; input[x] != '\0'; x++)
            {
                if (input[x] == '&')
                {
                    sentence_count++;
                }
                else
                    break;
            }

            char *sentence_token = strtok_r(input, sentencebreaker, &sentence);
            if (i == 0 && sentence_token == NULL && sentence_count >= 1)
            {
                perrormsg();
            }

            while (sentence_token != NULL)
            {
            for (x = 0; sentence_token[x] != '\0'; x++)
            {
                if (sentence_token[x] == '>')
                {
                    redirect_count++;
                }
                else
                    pass;
            }
                
                i++;
                // printf("woshishabi1 \n");
                char *redirection_token = strtok_r(sentence_token, redirectionbreaker, &re_dir);
                int redir_counter = 0;
                int j = 0;
                int k = 0;
                while (redirection_token != NULL)
                {

                    char *inner_token = strtok_r(redirection_token, wordbreaker, &words);

                    if (redir_counter == 0)
                    {
                        myargs[j] = inner_token;
                        while (inner_token != NULL)
                        {
                            inner_token = strtok_r(NULL, wordbreaker, &words);
                            j++;
                            myargs[j] = inner_token;
                        }
                    }

                    redir_counter++;
                    if (redir_counter > 0)
                    {
                        redirection[k] = inner_token;
                        while (inner_token != NULL)
                        {
                            inner_token = strtok_r(NULL, wordbreaker, &words);
                            k++;
                            redirection[k] = inner_token;
                        }
                    }
                    redirection_token = strtok_r(NULL, redirectionbreaker, &re_dir);
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "exit") == 0 || strcmp(myargs[0], "exit\n") == 0))
                {
                    if (j == 1)
                    {
                        printf("Goodbye, exiting shell ... \n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        perrormsg();
                        break;
                    }
                }
                if (myargs[0] != NULL && (strcmp(myargs[0], "cd") == 0 || strcmp(myargs[0], "cd\n") == 0))
                {
                    char cwd[256];
                    if (myargs[1] != NULL && myargs[2] == NULL)
                    {
                        if (getcwd(cwd, sizeof(cwd)) == NULL)
                        {
                            perrormsg();
                        }
                        strcat(cwd, "/");

                        myargs[1] = strdup(fetch_program_path(cwd, myargs[1]));
                        chdir(myargs[1]);
                        if (chdir(myargs[1]) != 0)
                        {
                            perrormsg();
                        }
                    }
                    else
                    {
                        perrormsg();
                    }
                }
                pid_t childpid = fork();
                if (childpid == 0)
                {

                    if (myargs[0] != NULL && (strcmp(myargs[0], "path") == 0 || strcmp(myargs[0], "path\n") == 0))
                    {
                        if (j == 1)
                        {
                            printf("Default Directory enabled \n");
                            reset_path();
                            exit(0);
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
                                        perrormsg();
                                        exit(0);
                                    }
                                    path_num++;
                                }
                                else
                                    exit(0);
                            }
                        }
                    }
                    else
                        pass;
                    char *temp = NULL;
                    if (myargs[0] == NULL)
                    {
                        exit(0);
                    }
                    temp = myargs[0];
                    myargs[0] = strdup(fetch_program_path(programpath[0], myargs[0]));

                    //  printf("woshishabi2 %s %d \n", redirection_token,redir_counter);
                    if (redir_counter >2)
                    {
                        perrormsg();
                        exit(0);
                    }

                    int fd;
                    if (redirect_count > 0 && redirection[0] == NULL)
                    {
                        perrormsg();
                        exit(0);
                    }
                    if (redir_counter == 2)
                    {
                        if (redirection[1] != NULL)
                        {
                            perrormsg();
                            exit(0);
                        }
                        if (redirection[0] == NULL)
                        {
                            perrormsg();
                            exit(0);
                        }
                        fd = open(*redirection, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
                        if (fd == -1)
                        {
                            perrormsg();
                            exit(0);
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perrormsg();
                            exit(0);
                        }
                    }

                    execv(myargs[0], myargs);
                    int path_counter = 1;
                    while (1)
                    {
                        myargs[0] = strdup(fetch_program_path(programpath[path_counter], temp));
                        execv(myargs[0], myargs);
                        path_counter++;
                        if (programpath[path_counter] == NULL)
                        {
                            perrormsg();
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
    exit(EXIT_SUCCESS);
}
