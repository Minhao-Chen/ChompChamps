#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include "common.h"
#include "ipc_utils.h"
#include <time.h>
#include <getopt.h>
#include <sys/select.h>

#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

//gameState state;
gameState * state_ptr = NULL;
synchronization * sync_ptr = NULL;

char * view = NULL;
unsigned int delay = DEFAULT_DELAY;
unsigned int timeout = DEFAULT_TIMEOUT;
unsigned int seed=0;


static bool is_position_unique(int current_index) {
    for (int i = 0; i < current_index; i++) {
        if (state_ptr->players[i].pos_x == state_ptr->players[current_index].pos_x &&
            state_ptr->players[i].pos_y == state_ptr->players[current_index].pos_y) {
            return false;
        }
    }
    return true;
}

static void start_players() {
    for (int i = 0; i < state_ptr->player_count; i++) {
        state_ptr->players[i].score = 0;
        state_ptr->players[i].invalid_move = 0;
        state_ptr->players[i].valid_move = 0;
        
        // Posiciones iniciales únicas
        do {
            state_ptr->players[i].pos_x = rand() % state_ptr->width;
            state_ptr->players[i].pos_y = rand() % state_ptr->height;
        } while (!is_position_unique(i));
        
        state_ptr->players[i].pid = 0;
        state_ptr->players[i].blocked = false;
    }
}


// Función para parsear parámetros
gameState parse_arguments(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    gameState config_args = {
        .width = DEFAULT_WIDTH,
        .height = DEFAULT_HEIGHT,
        .player_count=0,
        .game_ended=false,
    };

    for (int i = 0; i < 9; i++) {
        memset(config_args.players[i].name, 0, MAX_LENGHT_NAME);
    }

    int opt;

    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w': {
                int temp = atoi(optarg);
                if (temp > 10) config_args.width = temp;
                break;
            }
            case 'h': {
                int temp = atoi(optarg);
                if (temp > 10) config_args.height = temp;
                break;
            }
            case 'd': {
                delay = atoi(optarg);
                break;
            }
            case 't': {
                timeout = atoi(optarg);
                break;
            }
            case 's': {
                seed = (unsigned int)atoi(optarg);
                break;
            }
            case 'v': {
                view = optarg;
                break;
            }
            case 'p': {
                    if (config_args.player_count < 9) {
                        memset(config_args.players[config_args.player_count].name, 0, MAX_LENGHT_NAME);
                        for (int i = 0; i < MAX_LENGHT_NAME-1 && optarg[i]!=0; i++){
                            config_args.players[config_args.player_count].name[i]=optarg[i];
                        }
                        

                        //strncpy(config_args.players[config_args.player_count].name, optarg, MAX_LENGHT_NAME-1);
                        config_args.players[config_args.player_count].name[MAX_LENGHT_NAME-1] = '\0';
                        config_args.player_count++;
                    }

                // Jugadores siguientes
                while (optind < argc && argv[optind][0] != '-' && config_args.player_count < 9) {
                    memset(config_args.players[config_args.player_count].name, 0, MAX_LENGHT_NAME);
                        for (int i = 0; i < MAX_LENGHT_NAME-1 && optarg[i]!=0; i++){
                            config_args.players[config_args.player_count].name[i]=optarg[i];
                        }
                    //strncpy(config_args.players[config_args.player_count].name, argv[optind], MAX_LENGHT_NAME-1);
                    config_args.players[config_args.player_count].name[MAX_LENGHT_NAME-1] = '\0';
                    config_args.player_count++;
                    optind++;
                }
                break;
            }
            default:
                fprintf(stderr, "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view_path] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config_args.player_count == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    return config_args;
}

void createGameState (gameState state){

    state_ptr = create_shm_state(state.width, state.height);

    if (state_ptr == NULL) {
        fprintf(stderr, "Error creando shared memory del estado\n");
        exit(EXIT_FAILURE);
    }

    // Inicializar estructura
    state_ptr->width = state.width;
    state_ptr->height = state.height;
    state_ptr->player_count = state.player_count;
    state_ptr->game_ended = false;
    for (int i = 0; i < state.player_count; i++) {
        memset(state_ptr->players[i].name, 0, MAX_LENGHT_NAME);
        for (int j = 0; state.players[i].name[j]!=0; j++){
            state_ptr->players[i].name[j]=state.players[i].name[j];
        }
    }

    // Llena con aleatorios entre 1 y 9
    for (int i = 0; i < state_ptr->height; i++) {
        for (int j = 0; j < state_ptr->width; j++) {
            state_ptr->board[j+i*state_ptr->width] = (rand() % 9) + 1;
        }
    }
    start_players();
}

void createSync (int player_count){
    sync_ptr = create_shm_sync(player_count);
    if(sync_ptr == NULL){
        fprintf(stderr, "Error creando shared memeory del semaforo");
        exit(EXIT_FAILURE);
    }
}

bool is_valid_movement(unsigned char move, int id){
    if (move > 7){
        return false;
    }

    int next_x = state_ptr->players[id].pos_x + movs[move][0]; 
    int next_y = state_ptr->players[id].pos_y + movs[move][1]; 

    if (next_x < 0 || next_y < 0 || next_x >= state_ptr->width || next_y >= state_ptr->height){
        return false;
    }

    int next_pos = state_ptr->board[next_y * state_ptr->width + next_x];

    if (next_pos<=0){
        return false;
    }

    return true;
    
}

void apply_movement(unsigned char move, int id){
    if (move > 7){
        return;
    }

    int next_x = state_ptr->players[id].pos_x + movs[move][0]; 
    int next_y = state_ptr->players[id].pos_y + movs[move][1]; 

    state_ptr->players[id].pos_x = next_x;
    state_ptr->players[id].pos_y = next_y;

    state_ptr->players[id].score+=state_ptr->board[next_y * state_ptr->width + next_x];

    state_ptr->board[next_y * state_ptr->width + next_x] = -id;
    
}

void print_arguments(){
    printf("width: %u\n", state_ptr->width);
    printf("height: %u\n", state_ptr->height);
    printf("delay: %u\n", delay);
    printf("timeout: %u\n", timeout);
    printf("seed: %u\n", seed);
    printf("view: %s\n", view ? view : "-");
    printf("num_players: %d\n", state_ptr->player_count);

    for (int i = 0; i < state_ptr->player_count; i++){
        printf("  %s\n", state_ptr->players[i].name);
    }

}

static pid_t fork_view(const char* width, const char* height){
    pid_t pid = fork();
    if (pid < 0) {
        // Error al hacer fork: conviene no matar todo acá,
        // sino devolver un error al main.
        perror("fork");
        return -1; 
    }
    if (pid == 0) {
        // vista
        execl(view, "vista", width, height, NULL);
        perror("execl view");
        _exit(1);
    }

    return pid;
}
static void fork_players(int player_count, int pipes[][2], int fds[], const char *width, const char *height){
    char num_player[16];
    for (int i = 0; i < player_count; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); exit(1); }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
               // jugador
            for (int j = 0; j <= i; j++) {
                close(pipes[j][0]);
                if (j != i) close(pipes[j][1]);
            }
            dup2(pipes[i][1], STDOUT_FILENO); // redirijo stdout → pipe
            close(pipes[i][1]); // cierro el original, ya no lo necesito

            
            sprintf(num_player, "%d", i);


            execl(state_ptr->players[i].name, "jugador", width, height, num_player, NULL);
            perror("execl jugador");
            _exit(1);
        } else {
            // master
            close(pipes[i][1]);
            fds[i] = pipes[i][0];            // guardo el read-end para select
            state_ptr->players[i].pid = pid;
            printf("Jugador %d pid=%d name=%s\n", i, pid, state_ptr->players[i].name);
        }
    }
}


int main(int argc, char *argv[]) {
    gameState state = parse_arguments(argc, argv);

    if (seed == 0){
        seed = (unsigned int)time(NULL);
    }
    srand(seed);
    
    createGameState(state);
    createSync(state_ptr->player_count);

    print_arguments();

    char arg_w[16], arg_h[16];
    sprintf(arg_w, "%d", state_ptr->width);
    sprintf(arg_h, "%d", state_ptr->height);

    //Crear la vista
    pid_t pid = fork_view(arg_w, arg_h);

    if (pid<0){
        perror("execl view");
        exit(1);
    }
   
    int N = state_ptr->player_count;
    int pipes[N][2]; // te abre pipe para cada jugador
    int fds[N]; // solo los read-ends para el select

    fork_players(N, pipes, fds, arg_w, arg_h);


    fd_set readfds;
    unsigned char move;
    int maxfd, id_roundrobin=0;

    time_t last_valid_request = time(NULL);

    // Bucle principal
    while (!state_ptr->game_ended) {
        // Revisar si superó timeout desde la última solicitud válida
        time_t now = time(NULL);
        if (difftime(now, last_valid_request) >= timeout) {
            printf("Timeout: ningún movimiento válido en %d segundos. Fin del juego.\n", timeout);
            state_ptr->game_ended = true;
            break;
        }

        FD_ZERO(&readfds);
        maxfd = -1;
        // Agregamos todos los pipes de los jugadores
        for (int i = 0; i < N; i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i]>maxfd){
                maxfd=fds[i];
            } 
        }

        if (maxfd < 0) {                    // no queda ningún jugador emitiendo
            state_ptr->game_ended = true;
            break;
        } 

        // Timeout opcional (1s)
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (ready < 0) {
            perror("select");
            exit(1);
        } else if (ready > 0){
            for (int i = 0; i < N; i++, id_roundrobin=(id_roundrobin+1)%N) {
                if (fds[id_roundrobin] >= 0 && FD_ISSET(fds[id_roundrobin], &readfds)) {
                    int n = read(fds[id_roundrobin], &move, 1);
                    if (n <= 0) {
                        // EOF → jugador bloqueado, close es en EOF
                        state_ptr->players[i].blocked = true;
                        close(fds[id_roundrobin]); //? creo q va asi
                        fds[id_roundrobin] = -1; 
                    } else {
                        // Bloquear estado para aplicar movimiento
                        lock_writer(sync_ptr);

                        if (is_valid_movement(move, id_roundrobin)) {
                            apply_movement(move, id_roundrobin);
                            state_ptr->players[id_roundrobin].valid_move++;
                            last_valid_request = time(NULL); // reinicia el reloj
                        } else {
                            state_ptr->players[id_roundrobin].invalid_move++;
                        }

                        unlock_writer(sync_ptr);

                    }

                    // Habilitar de nuevo al jugador
                    master_release_player(sync_ptr, id_roundrobin);
                }
            }
        }

        // Notificar a la vista
        master_notify_view(sync_ptr);
        master_wait_view(sync_ptr);


        // Delay entre actualizaciones
        usleep(delay * 1000u);
    }

    master_notify_view(sync_ptr);
    master_wait_view(sync_ptr);

    int status;
    // Esperar a que todos los hijos (jugadores más vista) terminen
    int children = state_ptr->player_count + (view!=NULL ? 1 : 0);
    for (int i = 0; i < children; i++) {
        pid_t wpid = waitpid(-1, &status, 0);
        if (wpid > 0) {
            if (WIFEXITED(status)) {
                printf("Padre: hijo %d terminó con exit code %d\n", wpid, WEXITSTATUS(status));
            } else {
                printf("Padre: hijo %d terminó de forma anormal\n", wpid);
            }
        }
    }

    destroy_shm_sync(sync_ptr, state_ptr->player_count);
    destroy_shm_state(state_ptr);

    printf("Padre: todos los hijos terminaron, fin del programa.\n");
    return 0;
}