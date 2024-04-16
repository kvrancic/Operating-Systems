#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

pid_t shell_pid;

void handle_allocation_error()
{
    fprintf(stderr, "fsh: nije moguće alocirati memoriju\n");
    exit(1);
}

char **readLine()
{
    char *linija = NULL;
    size_t buffer = 0;
    getline(&linija, &buffer, stdin);
    linija[strcspn(linija, "\n")] = '\0';

    char **argumenti = malloc(buffer * sizeof(char *));
    if (!argumenti)
        handle_allocation_error();

    char *delimitatori = " \t\r\n";
    char *argument = strtok(linija, delimitatori);
    int pozicija = 0;

    while (argument)
    {
        argumenti[pozicija] = argument;
        pozicija++;

        if (pozicija >= buffer)
        {
            buffer *= 2;
            argumenti = realloc(argumenti, buffer * sizeof(char *));
            if (!argumenti)
                handle_allocation_error();
        }

        argument = strtok(NULL, delimitatori);
    }

    argumenti[pozicija] = NULL;

    return argumenti;
}

/*
 * Funkcija 'fsh_launch' pokreće program sa zadanim argumentima.
 * @param args: lista argumenata u obliku niza znakova
 * @return: 1 ako je izvršavanje uspješno, inače se vraća greška
 */
int fsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    // Stvori novi proces
    pid = fork();
    
    //printf("\n prvi argument je %s\n", args[0]);
    // Ako se izvodi u djetetu
    if (pid == 0)
    {
        if(execv(args[0], args) != -1){
            return 0;  
        }
        else{
            //printf("\nUšli smo u ovaj dio\n");
            // Dohvati putanju direktorija iz PATH varijable okoline
            char *path = getenv("PATH");
            //printf("%s", path); 

            // Podijeli putanju direktorija na pojedine direktorije
            char *dir = strtok(path, ":");

            // Prolazi kroz sve direktorije
            while (dir != NULL)
            {
                // Stvori buffer za spremanje putanje i argumenta
                char cmd[strlen(dir) + strlen(args[0]) + 2]; 

                // Sastavi putanju i argumente
                sprintf(cmd, "%s/%s", dir, args[0]);
                strcat(cmd, "\0");
                //printf("%s\n", cmd);

                // Ako postoji datoteka s tim imenom u direktoriju i dopuštena je izvedba
                if (access(cmd, X_OK) == 0)
                {
                    // Zamijeni argument s punom putanjom i izvrši program
                    printf("Ušao");
                    args[0] = cmd;
                    execv(cmd, args);

                    // Ispisuje grešku ako izvršavanje nije uspjelo
                    perror("fsh");
                    exit(EXIT_FAILURE);
                }

                // Dohvati sljedeći direktorij
                dir = strtok(NULL, ":");
            }

            // Ako nema programa s tim imenom
            printf("fsh: nema takve naredbe: %s\n", args[0]);
            exit(EXIT_FAILURE);

            // Ako se izvodi u roditelju
        }
    }
        
    else if (pid < 0)
    {
        // Ispiši grešku ako nije moguće stvoriti proces
        perror("fsh");

        // Čekaj na izvršavanje djeteta
    }
    else
    {
        // cekanje, ovu sintaksu nasao na internetu u pokusajima debugiranja
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    // Ako je izvršavanje uspješno
    return 1;
}

// Trebamo posebnu funkciju za izvrsavanje built-in naredbi cd, exit itd. jer te naredbe se moraju izvrsiti u maticnom procesu shella
int fsh_execute(char **argumenti)
{
    if (argumenti[0] == NULL)
    {
        return 1;
    }
    else if (!strcmp(argumenti[0], "cd"))
    {
        if (argumenti[1] == NULL)
        {
            fprintf(stderr, "fsh: cd: folder ne postoji\n");
        }
        else if (chdir(argumenti[1]) != 0)
        {
            perror("fsh: cd");
        }
        return 0;
    }
    else if(!strcmp(argumenti[0], "exit") || !strcmp(argumenti[0], "")){
        exit(0);
        return 0;
    }

  return fsh_launch(argumenti);
}

void sigint_handler(int signal)
{
    // Ako se signal SIGINT (CTRL-C) primi, provjeri je li izvršava se proces ljuske
    if (getpid() == shell_pid)
    {
        printf("\n");
        return;
    }

    // Ako nije, proslijedi signal trenutnom procesu
    kill(0, SIGINT);
}


int main()
{
    // Postavi signal handler za SIGINT signal
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    //sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    // Spremi PID procesa ljuske
    shell_pid = getpid();

    while (true)
    {
        printf("fsh> ");
        char **argumenti = readLine(); // dvostruki jer pokazuje na nizove koji su zapravo pointeri

        if (argumenti[0] != NULL)
        {
            /* int i = 0;
            while (argumenti[i] != NULL) {
                printf("%s\n", argumenti[i]);
                i++;
            } */
            fsh_execute(argumenti);
        }
        else{
            exit(0);
        }

        free(argumenti);
    }

    return 0;
}