#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "common.h"
#include "ipc_utils.h"

gameState * state_ptr;
synchronization * sync_ptr;

int adjacent_cells[NMOVS];

static int movement(int width, int height){

    int max_adjacent_score = 0;
    char move = NMOVS;
    for(int direction=0; direction < NMOVS; direction++){ 
        if(adjacent_cells[direction] > max_adjacent_score){
            max_adjacent_score = adjacent_cells[direction];
            move = direction;
        }
    }
    
    if(move<NMOVS){
        write(STDOUT_FILENO, &move,  1);
        return 0;
    }

    return -1;
}


static bool is_valid_move(int width, int height, unsigned short player_x, unsigned short player_y, int diff_x, int diff_y){
  return player_y+diff_y >= 0 && player_y+diff_y < height && player_x+diff_x >= 0 && player_x+diff_x < width; 
}


static int get_id(){
    pid_t my_pid = getpid();
    int my_id=-1;
    
    for (int i = 0; i < state_ptr->player_count; i++){
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



int main(int argc, char *argv[]) {
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
            break;
        }
 
        copy_board_info(width, height, my_id);

        unlock_reader(sync_ptr);

        if(movement(width, height)<0){
            close(STDOUT_FILENO);
            break;
        }
        
    }

    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}
