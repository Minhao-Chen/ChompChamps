#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include "common.h"
#include "ipc_utils.h"
#include <time.h>
#include <bits/getopt_core.h>
#include <getopt.h>

#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

gameState * state;

char * view = NULL;
unsigned int delay=DEFAULT_DELAY;
unsigned int timeout=DEFAULT_TIMEOUT;
unsigned int seed;

static void initial_player(player* p, int num/*, char* name*/){
    /*p->blocked = false;
    p->invalid_move = 0;
    p->valid_move = 0;
    p->score = 0;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->pos_x = rand() % state->width;
    p->pos_y = rand() % state->height;*/

    memset(p->name, 0, sizeof(p->name));
    p->score = 0;
    p->invalid_move = 0;
    p->valid_move = 0;
    //hay q chequear q posiciones iniciales sean distintas...
    p->pos_x = rand() % state->width; 
    p->pos_y = rand() % state->height;
    for(int i = 0; i <= num; i++){
        if(i != num){
            while(p->pos_x == state->players[i].pos_x && p->pos_y == state->players[i].pos_y){
                p->pos_x = rand() % state->width; 
                p->pos_y = rand() % state->height;
            }
        }
    }
    p->pid = 0;                         ///????????????????
    p->blocked = false;
}


void start_players(){
    for (int i = 0; i < state->player_count; i++) { 
        initial_player(&state->players[i], i);
    }

}

// Función para parsear parámetros
void parse_arguments(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w':
                state->width = atoi(optarg);
                if (state->width < 10) state->width = 10;
                break;
            case 'h':
                state->height = atoi(optarg);
                if (state->height < 10) state->height = 10;
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 's':
                seed = (unsigned int)atoi(optarg);
                break;
            case 'v':
                view = optarg;
                break;
            case 'p':
                // Procesar múltiples jugadores
                while (optind < argc && argv[optind][0] != '-') {
                    if (state->player_count < 9) {
                        strncpy(state->players[state->player_count++].name, argv[optind], sizeof(argv[optind++]) - 1);
                    } else {
                        optind++;
                    }
                }
                break;
            default:
                fprintf(stderr, "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view_path] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (state->player_count == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
}

/*int passstate(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            state->width = atoi(argv[++i]);
            if (state->width < 10) state->width = 10;
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            state->height = atoi(argv[++i]);
            if (state->height < 10) state->height = 10;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            seed = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            view = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            while (i + 1 < argc && argv[i + 1][0] != '-' && state->player_count < 9) {
                strncpy(state->players[state->player_count++].name, argv[++i], sizeof(argv[i]) - 1);
                //initial_player(&state->players[state->player_count++],argv[++i]);
            }
        }
    }

    if (state->player_count < 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}*/

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

void createGameState (/*int fd*/){

    // Mapear
    /*gameState *state = mmap(NULL, sizeof(gameState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(fd);    
    */

    //gameState * state = create_shm_state();

    state = create_shm_state(MAX_WIDTH,MAX_HEIGHT);

    // Inicializar estructura
    state->width = DEFAULT_WIDTH;
    state->height = DEFAULT_HEIGHT;
    state->player_count = 0;
    state->active_game = true;

    /*
    for (int i = 0; i < 9; i++) { 
        /*memset(state->players[i].name, 0, sizeof(state->players[i].name));
        state->players[i].score = 0;
        state->players[i].invalid_move = 0;
        state->players[i].valid_move = 0;
        state->players[i].pos_x = 0;
        state->players[i].pos_y = 0;
        state->players[i].pid = 0;
        state->players[i].blocked = false;
        initial_player(&state->players[i], i);
    }
    */
    

    //printf("Memoria compartida '%s' creada (%zu bytes).\n", SHM_GAME_STATE_NAME, SIZE);

    //return 0;
}







int main(int argc, char *argv[]) {

    // Si seed sigue siendo 1, usamos time(NULL) como valor por defecto
    seed = (seed == 1) ? (unsigned int)time(NULL) : seed;

    //int fd = createShm(SHM_STATE, sizeof(gameState));
    createGameState(/*fd*/);
    parse_arguments(argc, argv);
    start_players();
    //createGameState();

    // Imprimir en el formato solicitado
    printf("width: %u\n", state->width);
    printf("height: %u\n", state->height);
    printf("delay: %u\n", delay);
    printf("timeout: %u\n", timeout);
    printf("seed: %u\n", seed);
    printf("view: %s\n", view ? view : "-");
    printf("num_players: %d\n", state->player_count);
    for (int i = 0; i < state->player_count; i++){
        printf("  %s\n", state->players[i].name);
    }




    int status;
    char arg_w[16], arg_h[16], num_player[16]; // ver bien esto..., se pasan como strings...
    sprintf(arg_w, "%d", state->width);
    sprintf(arg_h, "%d", state->height);

    //Crear la vista
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        // vista
        execl("./view", arg_w, arg_h, NULL);
        perror("execl view");
        exit(1);
    }
    int pipes[state->player_count][2]; // te abre pipe para cada jugador
    for (int i = 0; i < state->player_count; i++) {
        pipe(pipes[i]);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
               // jugador
            close(pipes[i][0]);
            dup2(pipes[i][1], 1); // redirijo stdout → pipe
            close(pipes[i][1]); // cierro el original, ya no lo necesito
            
            sprintf(num_player, "%d", i);


            execl("./player", "jugador", arg_w, arg_h, num_player, NULL); //le pondria la i ahi, para saber q numero de jugador es
            perror("execl jugador");
            exit(1);
        } else {
            // master
            close(pipes[i][1]);
            state->players[i].pid = pid;
            printf("Jugador %d pid=%d name=%s\n", i, pid, state->players[i].name);
            pid_t wpid = waitpid(pid, &status, 0);
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