#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>


typedef struct masterParam{
    unsigned int width;
    unsigned int height;
    unsigned int delay;
    unsigned int timeout;
    unsigned int seed;
    char *view;
    char *players[9];
    int player_count;
}masterParam;

masterParam params = {
    .width=10,
    .height=10,
    .delay=200,
    .timeout=10,
    .seed=1,
    .view=NULL,
    .player_count=0
};


int passParams(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            params.width = atoi(argv[++i]);
            if (params.width < 10) params.width = 10;
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            params.height = atoi(argv[++i]);
            if (params.height < 10) params.height = 10;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            params.delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            params.timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            params.seed = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            params.view = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            while (i + 1 < argc && argv[i + 1][0] != '-' && params.player_count < 9) {
                params.players[params.player_count++] = argv[++i];
            }
        }
    }

    if (params.player_count < 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}


int main(int argc, char *argv[]) {
    passParams(argc, argv);

    // Si seed sigue siendo 1, usamos time(NULL) como valor por defecto
    params.seed = (params.seed == 1) ? (unsigned int)time(NULL) : params.seed;

    // Imprimir en el formato solicitado
    printf("width: %u\n", params.width);
    printf("height: %u\n", params.height);
    printf("delay: %u\n", params.delay);
    printf("timeout: %u\n", params.timeout);
    printf("seed: %u\n", params.seed);
    printf("view: %s\n", params.view ? params.view : "-");
    printf("num_players: %d\n", params.player_count);
    for (int i = 0; i < params.player_count; i++)
        printf("  %s\n", params.players[i]);

    return 0;


    /*pid_t pid;
    int status;
    //int pipefd[2];
    //pipe(pipefd);
        //sleep(5);
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
                // Código del hijo: reemplazar proceso por el ejecutable hijo
                char *args[] = {"../producer-consumer/false", NULL};
                execve(args[0], args, NULL);
            // Si execve falla:
            perror("execve");
            exit(1);
        }

        // El padre sigue aquí: crea otro hijo en la siguiente iteración

    // Esperar a que todos los hijos terminen
        pid_t wpid = waitpid(pid, &status, 0);
        if (WEXITSTATUS(status)==0){
            printf("Padre: hijo %d terminó de forma exitosa\n", wpid);
            return 0;
        }
        if (wpid > 0) {
            if (WIFEXITED(status)) {
                printf("Padre: hijo %d terminó con exit code %d\n", wpid, WEXITSTATUS(status));
            } else {
                printf("Padre: hijo %d terminó de forma anormal\n", wpid);
            }
        }

    pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
                // Código del hijo: reemplazar proceso por el ejecutable hijo
                char *args[] = {"../producer-consumer/true", NULL};
                execve(args[0], args, NULL);
            // Si execve falla:
            perror("execve");
            exit(1);
        }

        // El padre sigue aquí: crea otro hijo en la siguiente iteración

    // Esperar a que todos los hijos terminen
        wpid = waitpid(pid, &status, 0);
        if (wpid > 0) {
            if (WIFEXITED(status)) {
                printf("Padre: hijo %d terminó con exit code %d\n", wpid, WEXITSTATUS(status));
            } else {
                printf("Padre: hijo %d terminó de forma anormal\n", wpid);
            }
        }

    printf("Padre: todos los hijos terminaron, fin del programa.\n");
    return 0;*/
}