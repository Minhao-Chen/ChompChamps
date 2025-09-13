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
        
        do {
            state_ptr->players[i].pos_x = rand() % state_ptr->width;
            state_ptr->players[i].pos_y = rand() % state_ptr->height;
        } while (!is_position_unique(i));
        
        state_ptr->players[i].pid = 0;
        state_ptr->players[i].blocked = false;
    }
}

static void parse_player(gameState * config_args, const char *name){
    memset(config_args->players[config_args->player_count].name, 0, MAX_LENGHT_NAME);
    for (int i = 0; i < MAX_LENGHT_NAME-1 && name[i]!=0; i++){
        config_args->players[config_args->player_count].name[i]=name[i];
    }
    config_args->players[config_args->player_count].name[MAX_LENGHT_NAME-1] = '\0';
    config_args->player_count++;
}

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
                parse_player(&config_args, optarg);
                while (optind < argc && argv[optind][0] != '-') {
                    if(config_args.player_count >= 9){
                        fprintf(stderr, "Error: At most 9 players can be specified using -p.\n");
                        exit(EXIT_FAILURE);
                    }
                    parse_player(&config_args, argv[optind]);
                    optind++;
                }
                break;
            }
            default:
                fprintf(stderr, "Usage: %s [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (config_args.player_count == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        exit(EXIT_FAILURE);
    }
    return config_args;
}


void create_game_state (gameState state){
    state_ptr = create_shm_state(state.width, state.height);

    if (state_ptr == NULL) {
        fprintf(stderr, "Error: cannot create shared memory for gameState\n");
        exit(EXIT_FAILURE);
    }

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

    for (int i = 0; i < state_ptr->height; i++) {
        for (int j = 0; j < state_ptr->width; j++) {
            state_ptr->board[j+i*state_ptr->width] = (rand() % 9) + 1;
        }
    }
    start_players();
}

void create_sync (int player_count){
    sync_ptr = create_shm_sync(player_count);
    if(sync_ptr == NULL){
        fprintf(stderr, "Error: cannot create shared memory for semaphores\n");
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
        perror("fork");
        return -1; 
    }
    if (pid == 0) {
        execl(view, "vista", width, height, NULL);
        perror("execl view");
        _exit(1);
    }

    return pid;
}


static void fork_players(int player_count, int pipes[][2], int fds[],
                         const char *width, const char *height)
{
    for (int i = 0; i < player_count; i++) {
        pipes[i][0] = pipes[i][1] = -1;
        fds[i] = -1;
    }

    for (int i = 0; i < player_count; i++) {
        if (pipe(pipes[i]) == -1) { perror("pipe"); exit(1); }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }

        if (pid == 0) {
            for (int j = 0; j <= i; j++) {
                if (pipes[j][0] >= 0) { close(pipes[j][0]); pipes[j][0] = -1; }
                if (j != i && pipes[j][1] >= 0) { close(pipes[j][1]); pipes[j][1] = -1; }
            }

            if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
            if (pipes[i][1] >= 0) { close(pipes[i][1]); pipes[i][1] = -1; }

            execl(state_ptr->players[i].name, "jugador", width, height, NULL);
            perror("execl jugador");
            _exit(1);
        } else {
            if (pipes[i][1] >= 0) { close(pipes[i][1]); pipes[i][1] = -1; }

            fds[i] = pipes[i][0];
            state_ptr->players[i].pid = pid;
            printf("Jugador %d pid=%d name=%s\n", i, pid, state_ptr->players[i].name);
        }
    }
}


void game_management(int player_count, int fds[]){
    fd_set readfds;
    unsigned char move;
    int maxfd, id_roundrobin=0;

    time_t last_valid_request = time(NULL);

    while (!state_ptr->game_ended) {
        time_t now = time(NULL);
        if (difftime(now, last_valid_request) >= timeout) {
            //printf("Timeout: ningún movimiento válido en %d segundos. Fin del juego.\n", timeout);
            break;
        }

        FD_ZERO(&readfds);
        maxfd = -1;

        for (int i = 0; i < player_count; i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i]>maxfd){
                maxfd=fds[i];
            } 
        }

        if (maxfd < 0) {
            break;
        } 

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (ready < 0) {
            perror("select");
            exit(1);
        } else if (ready > 0){
            for (int i = 0; i < player_count; i++, id_roundrobin=(id_roundrobin+1)%player_count) {
                if (fds[id_roundrobin] >= 0 && FD_ISSET(fds[id_roundrobin], &readfds)) {
                    int n = read(fds[id_roundrobin], &move, 1);
                    if (n <= 0) {
                        state_ptr->players[id_roundrobin].blocked = true;
                        close(fds[id_roundrobin]);
                        fds[id_roundrobin] = -1; 
                    } else {
                        lock_writer(sync_ptr);

                        if (is_valid_movement(move, id_roundrobin)) {
                            apply_movement(move, id_roundrobin);
                            state_ptr->players[id_roundrobin].valid_move++;
                            last_valid_request = time(NULL);
                        } else {
                            state_ptr->players[id_roundrobin].invalid_move++;
                        }

                        unlock_writer(sync_ptr);
                    }
                    master_release_player(sync_ptr, id_roundrobin);
                }
            }
        }
        if (view!=NULL){
            master_notify_view(sync_ptr);
            master_wait_view(sync_ptr);
        }

        usleep(delay * 1000u);
    }
}


int main(int argc, char *argv[]) {
    (void)write(STDOUT_FILENO, "\033[H\033[2J\033[3J", 12);
    gameState state = parse_arguments(argc, argv);

    if (seed == 0){
        seed = (unsigned int)time(NULL);
    }
    srand(seed);
    
    create_game_state(state);
    create_sync(state_ptr->player_count);

    print_arguments();

    char arg_w[16], arg_h[16];
    sprintf(arg_w, "%d", state_ptr->width);
    sprintf(arg_h, "%d", state_ptr->height);

    pid_t pid_view;

    if(view!=NULL){
        pid_view = fork_view(arg_w, arg_h);
        
        if (pid_view<0){
            perror("execl view");
            exit(1);
        }
    }
   
    int N = state_ptr->player_count;
    int pipes[N][2];
    int fds[N];

    fork_players(N, pipes, fds, arg_w, arg_h);

    game_management(N, fds);

    state_ptr->game_ended = true;
    for (int i = 0; i < N; i++){
        master_release_player(sync_ptr, i);
    }

    pid_t wpid;
    int status;

    if (view!=NULL){
        master_notify_view(sync_ptr);
        master_wait_view(sync_ptr);

        
        wpid = waitpid(pid_view, &status, 0);
        if (wpid > 0) {
        printf("View exited (%d)\n", status);
    }
    }

    
    for (int i = 0; i < N; i++) {
        wpid = waitpid(state_ptr->players[i].pid, &status, 0);
        if (wpid > 0) {
            printf("Player player (%d) exited (%d) with a score of %d / %d / %d\n", i, status, 
                state_ptr->players[i].score, state_ptr->players[i].valid_move, state_ptr->players[i].invalid_move);
        }
    }

    destroy_shm_sync(sync_ptr, state_ptr->player_count);
    destroy_shm_state(state_ptr);

    return 0;
}