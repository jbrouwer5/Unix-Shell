#include <stdlib.h>

#include <unistd.h>

#include <stdio.h>

#include <string.h>

#include <sys/wait.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

void myPrint(char *msg)
{

    write(STDOUT_FILENO, msg, strlen(msg));
}

// checks if cmd is build in = { exit, cd, or pwd }

// returns 1 if build in 0 otherwize

int is_buildin(char *cmd)

{
    if (cmd == NULL)
    {

        return 0;
    }
    else if (strncmp(cmd, "exit", 4) == 0)
    {

        return 1;
    }
    else if (strncmp(cmd, "cd", 2) == 0)
    {
        return 1;
    }
    else if (strncmp(cmd, "pwd", 3) == 0)
    {
        return 1;
    }

    return 0;
}

int toknize(char *cmd_buff, char *argvec[], char *token)

{

    int cnt = 0;

    char *buf = NULL;

    buf = strtok(cmd_buff, token);

    while (buf != NULL)
    {

        argvec[cnt] = buf;

        buf = strtok(NULL, token);

        cnt++;
    }

    return cnt;
}

int process_buildin_cmd(char *cmd[], int cnt)
{

    char s[2048], buf[2048], *homedir;

    if (strncmp(cmd[0], "exit", 4) == 0)
    {
        if (cnt > 1 || strlen(cmd[0]) > 4)
        {
            char errMsg[2048];

            sprintf(errMsg, "An error has occurred\n");

            myPrint(errMsg);
        }
        else
        {
            exit(0);
        }
    }
    else if (strncmp(cmd[0], "pwd", 3) == 0)
    {
        if (cnt < 2 && strlen(cmd[0]) == 3)
        {
            sprintf(buf, "%s\n", getcwd(s, 2048));

            myPrint(buf);
        }
        else
        {
            char errMsg[2048];

            sprintf(errMsg, "An error has occurred\n");

            myPrint(errMsg);
        }
    }
    else if (strstr(cmd[0], "cd") != NULL)
    {

        if (strlen(cmd[0]) > 2 || cnt > 2)
        {
            char errMsg[2048];

            sprintf(errMsg, "An error has occurred\n");

            myPrint(errMsg);
        }
        else if (cnt < 2)
        {
            homedir = getenv("HOME");

            if (chdir(homedir) != 0)
            {
                return -1;
            }
        }
        else
        {
            if (chdir(cmd[1]) < 0)
            {
                char errMsg[2048];

                sprintf(errMsg, "An error has occurred\n");

                myPrint(errMsg);
            }
        }
    }
    else
    {

        return -1;
    }

    return 0;
}

int process_other_cmd(char *cmd[], int cnt)

{

    char *clean[] = {cmd[0], NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    int i;

    for (i = 0; i < cnt; i++)
    {

        clean[i] = cmd[i];
    }

    return execvp(cmd[0], clean);
}

int process_cmd(char *cmd)
{

    int cnt = 0, rc = 0;

    char *cmdVec[2048] = {NULL};

    int status;

    if (cmd == NULL)
        return 0;

    cnt = toknize(cmd, cmdVec, " "); // remove WSP
    // cmdVec is the formatted command - no space, one command without ;

    // example: ls -l or ls -l\n

    //  ;    ; case still not working need to handle

    // if there is no command like WSP only, do nothing

    if (cmdVec[0] == NULL)
        return 0;

    // remove line fold @ last

    if (strstr(cmdVec[cnt - 1], "\n") != NULL)
    {

        int len = strlen(cmdVec[cnt - 1]);

        cmdVec[cnt - 1][len - 1] = 0;
    }

    // remove WSP attached to the end, remove from command line option

    if (cmdVec[cnt - 1][0] == 0)
    {

        cnt--;
    }

    if (cmdVec[0][0] == 0)
    {
        return 0;
    }

    if (is_buildin(cmdVec[0]) == 1)
    {
        // char *cntstr = cnt + " ";
        //  myPrint(cntstr);

        rc = process_buildin_cmd(cmdVec, cnt);
    }
    else
    {

        int ret;

        if ((ret = fork()) == 0)
        {

            if ((cnt > 2) && (strncmp(cmdVec[cnt - 2], ">", 1) == 0) && (strlen(cmdVec[cnt - 2]) == 1))
            {

                char *output_name = cmdVec[cnt - 1];

                // existence check opening
                //#define LOCKFILE output_name
                // Integer for file descriptor returned by open() call.

                int pfd;
                if ((pfd = open(output_name, O_WRONLY | O_CREAT | O_EXCL,
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
                {
                    fprintf(stderr, "Cannot open /etc/ptmp. Try again later.\n");
                    exit(1);
                }

                dup2(pfd, 1);

                rc = process_other_cmd(cmdVec, cnt - 2);
            }
            else
            {

                rc = process_other_cmd(cmdVec, cnt);
            }

            if (rc != 0)
            {

                char errMsg[2048];

                sprintf(errMsg, "An error has occurred\n");

                myPrint(errMsg);
            }

            exit(rc);
        }

        wait(&status);

        rc = status;
    }

    return rc;
}

int main(int argc, char *argv[])
{

    char cmd_buff[2048];

    int i, cnt, nonempty;

    char *cmdVec[2048];

    FILE *fp;

    int maxlen = 513;

    if (argc > 2)
    {
        char errMsg[2048];

        sprintf(errMsg, "An error has occurred\n");

        myPrint(errMsg);

        exit(0);
    }

    if (argc == 2)
    {

        fp = fopen(argv[1], "r");

        if (fp == NULL)
        {

            // printf("file %s not found\n", argv[1]);

            char errMsg[2048];

            sprintf(errMsg, "An error has occurred\n");

            myPrint(errMsg);

            exit(0);
        }

        // printf("batch file is %s\n", argv[1]);

        while (fgets(cmd_buff, sizeof(cmd_buff), fp) != NULL)
        {

            nonempty = 0;
            for (int i = 0; i < strlen(cmd_buff); i++)
            {
                if (cmd_buff[i] != ' ' && cmd_buff[i] != '\n' && cmd_buff[i] != '\t')
                {
                    nonempty = 1;
                }
            }

            if (nonempty)
                myPrint(cmd_buff);

            char *tab = "\t";
            char *space = " ";
            for (int i = 0; i < strlen(cmd_buff); i++)
            {
                if (cmd_buff[i] == tab[0])
                {
                    cmd_buff[i] = space[0];
                }
            }

            if (strlen(cmd_buff) > maxlen)
            {

                char errMsg[2048];

                sprintf(errMsg, "An error has occurred\n");

                myPrint(errMsg);

                continue;
            }

            cnt = toknize(cmd_buff, cmdVec, ";"); // separate commands

            for (i = 0; i < cnt; i++)
            {

                if (process_cmd(cmdVec[i]) == -1)
                {

                    char errMsg[2048];

                    sprintf(errMsg, "An error has occurred\n");

                    myPrint(errMsg);
                }
            }
        }

        fclose(fp);

        return 0;
    }

    // shell mode

    while (1)
    {

        myPrint("myshell> ");

        memset(cmd_buff, 0, sizeof(cmd_buff));

        fgets(cmd_buff, 4096, stdin);

        myPrint(cmd_buff);

        if (strlen(cmd_buff) < maxlen)
        {
            char *tab = "\t";
            char *space = " ";
            for (int i = 0; i < strlen(cmd_buff); i++)
            {
                if (cmd_buff[i] == tab[0])
                {
                    cmd_buff[i] = space[0];
                }
            }

            cnt = toknize(cmd_buff, cmdVec, ";"); // separate commands

            for (i = 0; i < cnt; i++)
            {
                if (process_cmd(cmdVec[i]) == -1)
                {

                    char errMsg[2048];

                    sprintf(errMsg, "error in executing %s\n", cmdVec[i]);

                    myPrint(errMsg);
                }
            }
        }
    }
}