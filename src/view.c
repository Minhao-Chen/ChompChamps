#include <stdio.h>
#include "common.h"
#include "ipc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>     // srand, time

#define BUFFER_SIZE 4096


gameState * state_ptr;
synchronization* sync_ptr;
char render_buffer[BUFFER_SIZE]={0};
char prev_buffer[BUFFER_SIZE]={0};

void render_board(gameState * state) {
    //player_poistion();

    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            int cell = state->board[y * state->width + x];
            int printed = 0;  // flag para saber si imprimimos algo

            // ¿Hay un jugador en esta celda?
            for (int i = 0; i < state->player_count; i++) {
                if (y == state->players[i].pos_y && x == state->players[i].pos_x) {
                    printf("P ");
                    printed = 1;
                    break; // ya encontramos un jugador, no hace falta seguir
                }
            }

            // Si no había jugador, imprimimos la celda
            if (!printed) {
                printf(" %d ", cell);   // recompensa
            
            }
        }
        printf("\n");
    }
}

void render_players() {
    printf("Jugadores:\n");
    for (int i = 0; i < state_ptr->player_count; i++) {
        player *p = &state_ptr->players[i];
        printf("P%d %-16s score=%u pos=(%hu,%hu)\n",
               i, p->name, p->score, p->pos_x, p->pos_y);
    }
}

// Simular cambios aleatorios en el tablero
void update_board_random() {
    int num_changes = rand() % 5 + 1; // 1-5 cambios
    for (int i = 0; i < num_changes; i++) {
        int row = rand() % state_ptr->height;
        int col = rand() % state_ptr->width;
        state_ptr->board[row*state_ptr->width+col] = (rand() % 9) + 1;
    }
}

int main(int argc, char *argv[]) {
    printf("VIEW: hola!\n");
   // fflush(stdout); // para q se vea con el flush

    srand((unsigned)time(NULL));

    state_ptr = connect_shm_state();
    if (state_ptr == NULL) { perror("connect_shm_state"); exit(1); }

    sync_ptr = connect_shm_sync();
    if (sync_ptr == NULL) { perror("connect_shm_sync"); exit(1); }


    while (1){
        view_wait_changes(sync_ptr);
        //printf("\033[H\033[2J");
        if (state_ptr == NULL || sync_ptr == NULL) {
            break;
        }
        (void)write(STDOUT_FILENO, "\033[H\033[2J\033[3J", 12);
        if (!state_ptr->active_game){
            break;
        }
        
        render_board(state_ptr);
        render_players(state_ptr);
        view_notify_print(sync_ptr);
    }

    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);

    printf("\n\nSimulacion terminada.\n");
    return 0;
}