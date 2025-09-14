#include <stdio.h>
#include "common.h"
#include "ipc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>


#define BUFFER_SIZE 4096


gameState * state_ptr;
synchronization* sync_ptr;
char render_buffer[BUFFER_SIZE]={0};
char prev_buffer[BUFFER_SIZE]={0};

static void render_board(int width, int height){
    int positions_of_players[MAX_PLAYERS];
    for (int i = 0; i < state_ptr->player_count; i++) {
        positions_of_players[i]=state_ptr->players[i].pos_y*width + state_ptr->players[i].pos_x;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cell_pos = y * width + x;
            int cell_content = state_ptr->board[cell_pos];
            int printed = 0;

            for (int i = 0; i < state_ptr->player_count; i++) {
                if (cell_pos == positions_of_players[i]) {
                    printf("  P%d  ", i);
                    printed = 1;
                    break;
                }
            }

            if (!printed) {
                printf("  %2d  ", cell_content);
            
            }
        }
        printf("\n");
    }
}

static void render_players() {
    printf("Jugadores:\n");
    for (int i = 0; i < state_ptr->player_count; i++) {
        player *p = &state_ptr->players[i];
        printf("P%d %-16s score=%u pos=(%hu,%hu)\n",
               i, p->name, p->score, p->pos_x, p->pos_y);
    }
}

int main(int argc, char *argv[]) {
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    srand((unsigned)time(NULL));

    state_ptr = connect_shm_state(width, height);
    if (state_ptr == NULL) { 
        perror("connect_shm_state"); 
        return 1; 
    }

    sync_ptr = connect_shm_sync();
    if (sync_ptr == NULL) { 
        perror("connect_shm_sync"); 
        close_shm_state(state_ptr);
        return 1; 
    }


    while (1){
        view_wait_changes(sync_ptr);
        if (state_ptr == NULL || sync_ptr == NULL) {
            break;
        }

        (void)write(STDOUT_FILENO, "\033[H\033[2J\033[3J", 12);
        
        render_board(width, height);
        render_players();
        if (state_ptr->game_ended){
            break;
        }
        view_notify_print(sync_ptr);
    }
    
    view_notify_print(sync_ptr);
    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}