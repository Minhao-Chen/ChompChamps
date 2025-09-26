/*
* Con este player se busca optimizar el tiempo que tarda el jugador en hacer el movimento. La lógica es parecida a greedy,
* pero difiere en que no prioriza el mejor puntaje, sino la primer casilla que encuentre cuyo puntaje sea mayor o igual a 5.
* Primero obtiene de manera random una dirección para el movimiento. Si esa dirección lo lleva a un puntaje mayor igual a 5,
* lo hace. Sino, va a comenzar a moverse de manera horaria o anti horaria hasta que encuentre una casilla con el puntaje re-
* querido. En su defecto, se va a mover a la casilla con el mejor puntaje posible (que va a ser menor a 5).  
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "ipc_utils.h"

gameState * state_ptr;
synchronization * sync_ptr;

int adjacent_cells[NMOVS];

static int get_id() {
    pid_t my_pid = getpid();
    int my_id = -1;
    
    for (int i = 0; i < state_ptr->player_count; i++) {
        if (state_ptr->players[i].pid == my_pid) {
            my_id = i;
            break;
        }
    }
    
    if (my_id < 0) {
        fprintf(stderr, "Error: no encontré mi PID en game_state\n");
        return -1;
    }

    return my_id;
}

static bool is_valid_move(int width, int height, unsigned short player_x, unsigned short player_y, int diff_x, int diff_y) {
    return player_y + diff_y >= 0 && player_y + diff_y < height && 
           player_x + diff_x >= 0 && player_x + diff_x < width;
}

static void copy_board_info(int width, int height, int player_id){
    unsigned short player_x = state_ptr->players[player_id].pos_x, player_y = state_ptr->players[player_id].pos_y;

    for(int i=0; i < NMOVS; i++){
        if(is_valid_move(width,height,player_x, player_y , movs[i][0], movs[i][1])){
            adjacent_cells[i] = state_ptr->board[(player_y + movs[i][1] ) * width + player_x + movs[i][0]];
        }else{
            adjacent_cells[i] = 0 ;
        }
    }

    return;
}

static int find_best_move(int start_dir, bool clockwise) {
    int best_score = -9;
    int best_move = start_dir;
    
    int init_score = adjacent_cells[start_dir];
    
    if (init_score >= 5) {
        return start_dir;
    }
    
    int search_order[NMOVS];
    if (clockwise) {
        for (int i = 0; i < NMOVS; i++) {
            search_order[i] = (start_dir + i) % NMOVS;
        }
    } else {
        for (int i = 0; i < NMOVS; i++) {
            search_order[i] = (start_dir - i + NMOVS) % NMOVS;
        }
    }
    
    for (int i = 0; i < NMOVS; i++) {
        int dir = search_order[i];
        int score = adjacent_cells[dir];
        
        if (score >= 5) {
            return dir;
        }
        
        if (score >= 1 && score > best_score) {
            best_score = score;
            best_move = dir;
        }
        
    }
    
  
    if (best_score < 0) {
        return -1;
    }
    
    return best_move;
}

static int movement(int width, int height, int player_id) {
    int start_dir = rand() % NMOVS;
    
    bool clockwise = (rand() % 2 == 0);
    
    int move = find_best_move(start_dir, clockwise);
    
    if (move < 0) {
        return -1;
    }
    
    unsigned char move_char = (unsigned char)move;
    write(STDOUT_FILENO, &move_char, 1);
    
    return 0;
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

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

    lock_reader(sync_ptr);
    int my_id = get_id(); 
    unlock_reader(sync_ptr);

    if(my_id  < 0){
        close_shm_sync(sync_ptr);
        close_shm_state(state_ptr);
        return 1;
    }
    
    while (1) {
        player_wait_turn(sync_ptr, my_id); 

        lock_reader(sync_ptr);
        
        if (state_ptr->game_ended) {
            unlock_reader(sync_ptr);
            close(STDOUT_FILENO);
            break;
        }
        
        copy_board_info(width, height, my_id);
        
        unlock_reader(sync_ptr);

        if (movement(width, height, my_id) < 0) {
            close(STDOUT_FILENO);
            break;
        }
    }

    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}