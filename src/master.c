#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include "common.h"
#include "ipc_utils.h"

gameState params = {
    .width=10,
    .height=10,
    .player_count=0,
    .active_game=true
};

char * view = NULL;
unsigned int delay=200;
unsigned int timeout=10;
unsigned int seed = 1;

static void initial_player(player* p, char* name){
    p->blocked = false;
    p->invalid_move = 0;
    p->valid_move = 0;
    p->score = 0;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->pos_x = rand() % params.width;
    p->pos_y = rand() % params.height;
}

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
            delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            seed = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            view = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            while (i + 1 < argc && argv[i + 1][0] != '-' && params.player_count < 9) {
                initial_player(&params.players[params.player_count++],argv[++i]);
            }
        }
    }

    if (params.player_count < 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int createShm (const char * shm_name, size_t size){
        // Crear memoria compartida
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Ajustar tamaño
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    return fd;
}

void closeShm(const char * shm_name, size_t size, void * ptr){
    // Liberar
    munmap(ptr, size);
    shm_unlink(SHM_STATE);
}

void createGameState (int fd){

    // Mapear
    gameState *state = mmap(NULL, sizeof(gameState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    close(fd);

    // Inicializar estructura
    state->width = 10;
    state->height = 10;
    state->player_count = 0;
    state->active_game = true;

    for (int i = 0; i < 9; i++) {
        memset(state->players[i].name, 0, sizeof(state->players[i].name));
        state->players[i].score = 0;
        state->players[i].invalid_move = 0;
        state->players[i].valid_move = 0;
        state->players[i].pos_x = 0;
        state->players[i].pos_y = 0;
        state->players[i].pid = 0;
        state->players[i].blocked = false;
    }

    //printf("Memoria compartida '%s' creada (%zu bytes).\n", SHM_GAME_STATE_NAME, SIZE);

    //return 0;
}





int main(int argc, char *argv[]) {
    int fd = createShm(SHM_STATE, sizeof(gameState));
    createGameState(fd);
    passParams(argc, argv);
    //createGameState();

    // Si seed sigue siendo 1, usamos time(NULL) como valor por defecto
    seed = (seed == 1) ? (unsigned int)time(NULL) : seed;

    // Imprimir en el formato solicitado
    printf("width: %u\n", params.width);
    printf("height: %u\n", params.height);
    printf("delay: %u\n", delay);
    printf("timeout: %u\n", timeout);
    printf("seed: %u\n", seed);
    printf("view: %s\n", view ? view : "-");
    printf("num_players: %d\n", params.player_count);
    for (int i = 0; i < params.player_count; i++){
        printf("  %s\n", params.players[i].name);
    }

    int pipes[params.player_count][2];
    int status;
    
    for (int i = 0; i < params.player_count; i++) {
        pipe(pipes[i]);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            // jugador
            close(pipes[i][0]);
            dup2(pipes[i][1], 1);
            execl("./player", "jugador", NULL);
            perror("execl jugador");
            exit(1);
        } else {
            // master
            close(pipes[i][1]);
            params.players[i].pid = pid;
            printf("Jugador %d pid=%d name=%s\n", i, pid, params.players[i].name);
            pid_t wpid = waitpid(pid, &status, 0);
            // if (WEXITSTATUS(status)==0){
            //     printf("Padre: hijo %d terminó de forma exitosa\n", wpid);
            //     return 0;
            // }
            if (wpid > 0) {
                if (WIFEXITED(status)) {
                    printf("Padre: hijo %d terminó con exit code %d\n", wpid, WEXITSTATUS(status));
                } else {
                    printf("Padre: hijo %d terminó de forma anormal\n", wpid);
                }
            }
        }
    }

    printf("Padre: todos los hijos terminaron, fin del programa.\n");
    return 0;
}