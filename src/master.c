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

gameState state_default = {
    .width = DEFAULT_WIDTH,
    .height = DEFAULT_HEIGHT,
    .player_count = 0,
    .active_game = true
};

synchronization sems_default = {
    .reader_activated = 0
};



gameState * state = &state_default;
synchronization * sems = &sems_default;

char * view = NULL;
unsigned int delay = DEFAULT_DELAY;
unsigned int timeout = DEFAULT_TIMEOUT;
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

    //memset(p->name, 0, sizeof(p->name));
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


int parse_arguments(int argc, char *argv[]){
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
                strncpy(state->players[state->player_count++].name, argv[++i], sizeof(state->players[0].name) - 1);
                //state->players[state->player_count-1].name[sizeof(state->players[0].name) - 1] = '\0';
                //printf("%s",  state->players[state->player_count-1].name);
                //state->players[state->player_count-1].name[3]=0;
            }
        }
    }

    if (state->player_count < 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/* int createShm (const char * shm_name, size_t size){
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
} */

void createGameState (/*int fd*/){
    gameState * shm_state = create_shm_state(state->width,state->height);

    // Inicializar estructura
    shm_state->width = state->width;
    shm_state->height = state->height;
    shm_state->player_count = state->player_count;
    for (int i = 0; i < shm_state->player_count; i++){
        strncpy (shm_state->players[i].name, state->players[i].name, sizeof(state->players[0].name));      
    }
    shm_state->active_game = true;
    
    // Llena con aleatorios entre 1 y 9
    for (int i = 0; i < shm_state->height; i++) {
        for (int j = 0; j < shm_state->width; j++) {
            shm_state->board[j+i*shm_state->width] = (rand() % 9) + 1;
        }
    }
    state = shm_state;
    start_players();
}


void createSync (int player_count){
    sems = creat_shm_sync(player_count);
    return;
}


int main(int argc, char *argv[]) {

    // Si seed sigue siendo 1, usamos time(NULL) como valor por defecto
    seed = (seed == 1) ? (unsigned int)time(NULL) : seed;

    //int fd = createShm(SHM_STATE, sizeof(gameState));
    parse_arguments(argc, argv);
    //start_players();
    createGameState(/*fd*/);
    //createSync(state->player_count);
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
        
       printf("%s a\n", state->players[i].name);
  

    }
    

    printf("a");

    
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
        
        execl("./view", "vista", arg_w, arg_h, NULL);
        perror("execl view");
        exit(1);
    }

    
    /*int N = state->player_count;
    int pipes[state->player_count][2]; // te abre pipe para cada jugador
    int fds[N]; // solo los read-ends para el select
    for (int i = 0; i < state->player_count; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); exit(1); }
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
            fds[i] = pipes[i][0];            // guardo el read-end para select
            state->players[i].pid = pid;
            printf("Jugador %d pid=%d name=%s\n", i, pid, state->players[i].name);
        }
    }*/

    /// semaforos
    printf("ANTES DEL POST \n");
   // sem_post(&sems->sem_view_notify);
    printf("DESPUES DEL POST \n");
    //sem_wait(&sems->sem_view_done);
    printf("DESPPUES DEL WAIT \n");
    usleep(2000);
    ///
    state->board[10]=-2;
   // sem_post(&sems->sem_view_done);
    //sem_post(&sems->sem_view_notify);
    //usleep(delay);

    /*
    // Esperar a que todos los hijos (jugadores más vista) terminen
    int children = state->player_count + (view!=NULL ? 1 : 0);
    for (int i = 0; i < children; i++) {
        pid_t wpid = waitpid(-1, &status, 0);
    }*/

    //printf("Padre: todos los hijos terminaron, fin del programa.\n");
    return 0;
}