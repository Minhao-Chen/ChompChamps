#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include "common.h"
#include "ipc_utils.h"
#include <time.h>
#include <getopt.h>

#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

//gameState state;
gameState * state_ptr = NULL;
synchronization * sync_sem = NULL;

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
/*Config parse_arguments(int argc, char *argv[]) {
    Config config = {
        .width = DEFAULT_WIDTH,
        .height = DEFAULT_HEIGHT,
        .delay_ms = DEFAULT_DELAY,
        .timeout_sec = DEFAULT_TIMEOUT,
        .seed = 0,
        .view_path = NULL,
        .num_players = 0
    };

    for (int i = 0; i < 9; i++) {
        config.player_names[i] = NULL;
    }

    int opt;

    while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
        switch (opt) {
            case 'w': {
                int temp = atoi(optarg);
                if (temp >= 10) config.width = temp;
                break;
            }
            case 'h': {
                int temp = atoi(optarg);
                if (temp >= 10) config.height = temp;
                break;
            }
            case 'd': {
                config.delay_ms = atoi(optarg);
                break;
            }
            case 't': {
                config.timeout_sec = atoi(optarg);
                break;
            }
            case 's': {
                config.seed = (unsigned int)atoi(optarg);
                break;
            }
            case 'v': {
                config.view_path = optarg;
                break;
            }
            case 'p': {
                while (optind-1 < argc && argv[optind-1][0] != '-') {
                    if (config.num_players < 9) {
                        config.player_names[config.num_players++] = argv[optind-1];
                    }
                    optind++;
                }
                break;
            }
            default:
                fprintf(stderr, "Uso: %s [-w width] [-h height] [-d delay] [-t timeout] [-s seed] [-v view_path] -p player1 [player2 ...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config.num_players == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    return config;
}*/

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
        .active_game=false,
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
                    strncpy(config_args.players[config_args.player_count].name, optarg, MAX_LENGHT_NAME-1);
                    config_args.players[config_args.player_count].name[MAX_LENGHT_NAME-1] = '\0';
                    config_args.player_count++;
                }

                // Jugadores siguientes
                while (optind < argc && argv[optind][0] != '-' && config_args.player_count < 9) {
                    strncpy(config_args.players[config_args.player_count].name, argv[optind], MAX_LENGHT_NAME-1);
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

/* gameState parse_arguments(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    gameState config_args = {
        .width = DEFAULT_WIDTH,
        .height = DEFAULT_HEIGHT,
        .player_count=0,
        .active_game=false,
    };

    for (int i = 0; i < 9; i++) {
        memset(config_args.players[i].name, 0, MAX_LENGHT_NAME);
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            int width_aux = atoi(argv[++i]);
            if (width_aux > 10) config_args.width = width_aux;
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            int height_aux = atoi(argv[++i]);
            if (height_aux > 10) config_args.height = height_aux;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            timeout = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            seed = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            view = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0) {
            while (i + 1 < argc && argv[i + 1][0] != '-' && config_args.player_count < 9) {
                i++;
                for (int j = 0; j < MAX_LENGHT_NAME-1 && argv[i][j]!=0; j++){
                    config_args.players[config_args.player_count].name[j] = argv[i][j];
                }
                config_args.player_count++;
            }
        }
    }

    if (config_args.player_count < 1) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }

    return config_args;

}*/


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
    state_ptr->active_game = true;
    for (int i = 0; i < state.player_count; i++) {
        for (int j = 0; state.players[i].name[j]!=0; j++){
            state_ptr->players[i].name[j]=state.players[i].name[j];
        }
        //strncpy(state->players[i].name, config.player_names[i], sizeof(state->players[i].name) - 1);
        //state->players[i].name[sizeof(state->players[i].name) - 1] = '\0';
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
    sync_sem = creat_shm_sync(player_count);
    if(sync_sem == NULL){
        fprintf(stderr, "Error creando shared memeory del semaforo");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    //Config config = parse_arguments(argc, argv);

    gameState state = parse_arguments(argc, argv);

    //seed = (config.seed == 0) ? (unsigned int)time(NULL) : config.seed;
    if (seed == 0){
        seed = (unsigned int)time(NULL);
    }
    
    srand(seed);
    
    createGameState(state);
    createSync(state_ptr->player_count);

    // Imprimir en el formato solicitado
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


    int status;
    char arg_w[16], arg_h[16], num_player[16]; // ver bien esto..., se pasan como strings...
    sprintf(arg_w, "%d", state_ptr->width);
    sprintf(arg_h, "%d", state_ptr->height);

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
    printf("ANTES DEL POST");
    sem_post(&sync_sem->sem_view_notify);
    printf("DESPUES DEL POST");
    sem_wait(&sync_sem->sem_view_done);
    printf("DESPPUES DEL WAIT");
    usleep(2000);
    ///
    state_ptr->board[10]=-2;
    sem_post(&sync_sem->sem_view_done);
    sem_post(&sync_sem->sem_view_notify);
    usleep(delay);

    /*
    // Esperar a que todos los hijos (jugadores más vista) terminen
    int children = state->player_count + (view!=NULL ? 1 : 0);
    for (int i = 0; i < children; i++) {
        pid_t wpid = waitpid(-1, &status, 0);
    }*/

    //printf("Padre: todos los hijos terminaron, fin del programa.\n");
    return 0;
}