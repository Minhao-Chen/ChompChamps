#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "common.h"
#include "ipc_utils.h"


typedef struct { 
    int board[8];
    unsigned short player_x, player_y;
} movement_info;

gameState * state_ptr;
synchronization * sync_ptr;

static movement_info mip;                 
movement_info *movement_info_ptr = &mip;

bool is_valid_move(int width, int height, unsigned short player_x, unsigned short player_y, int diff_x, int diff_y);
int movement(int width, int height, int player_id);

int movement(int width, int height, int player_id){

    int max = 0;
    char m = NMOVS;
    int next_pos;
    unsigned short player_x = movement_info_ptr->player_x, player_y=movement_info_ptr->player_y;
    for(int i=0; i < NMOVS; i++){ 
        if(is_valid_move(width,height,player_x, player_y , movs[i][0], movs[i][1])){
           // next_pos = (player_y + movs[i][1]) * width + player_x + movs[i][0];
            if(movement_info_ptr->board[i] > max){
                max = movement_info_ptr->board[i];
                m = i;
            }
        }
    }
    
    if(m<NMOVS){
        write(STDOUT_FILENO, &m,  1);
        return 0;
    }

    return -1;
}

bool is_valid_move(int width, int height, unsigned short player_x, unsigned short player_y, int diff_x, int diff_y){
  return player_y+diff_y >= 0 && player_y+diff_y < height && player_x+diff_x >= 0 && player_x+diff_x < width; 
}


int get_id (){
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

/*
int inic_local_game_info(int width, int height){
    movement_info_ptr = malloc(sizeof(movement_info));
    if (movement_info_ptr == NULL) { 
        fprintf(stderr, "Error: malloc\n");
        return -1; 
    }

    movement_info_ptr->board = malloc(width * height * sizeof(int));
    if (movement_info_ptr->board == NULL) {
        fprintf(stderr, "Error: malloc\n");
        free(movement_info_ptr);
        return -1;
    }
    return 0;
}
*/

void free_local_game_info(){
    free(movement_info_ptr->board);
    free(movement_info_ptr);
    return;
}

/*
void copy_state_info(int width, int height, int player_id){
    movement_info_ptr->player_x = state_ptr->players[player_id].pos_x;
    movement_info_ptr->player_y = state_ptr->players[player_id].pos_y;

    int row_to_copy, pos_aux;

    //Copiar la fila en la que está el jugador y, de ser posible, la de arriba y la de abajo
    if ((row_to_copy = movement_info_ptr->player_y-1) >= 0){
        for (int i = 0; i < width; i++){
            pos_aux = row_to_copy*width + i;
            movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
        }
    }

    row_to_copy=movement_info_ptr->player_y;
    for (int i = 0; i < width; i++){
        pos_aux = row_to_copy*width + i;
        movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
    }    

    if ((row_to_copy = movement_info_ptr->player_y+1) < height){
        for (int i = 0; i < width; i++){
            pos_aux = row_to_copy*width + i;
            movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
        }
    }

    return;
    
}
*/

void copy_state_info(int width, int height, int player_id){
    movement_info_ptr->player_x = state_ptr->players[player_id].pos_x;
    movement_info_ptr->player_y = state_ptr->players[player_id].pos_y;

    //int row_to_copy, pos_aux;

    for(int i=0; i < NMOVS; i++){
        if(is_valid_move(width,height,movement_info_ptr->player_x, movement_info_ptr->player_y , movs[i][0], movs[i][1])){
            movement_info_ptr->board[i] = state_ptr->board[(movement_info_ptr->player_y + movs[i][1] ) * width + movement_info_ptr->player_x + movs[i][0]];
        }else{
            movement_info_ptr->board[i] = 0 ;
        }
    }
/*
    //Copiar la fila en la que está el jugador y, de ser posible, la de arriba y la de abajo
    if ((row_to_copy = movement_info_ptr->player_y-1) >= 0){
        for (int i = 0; i < width; i++){
            pos_aux = row_to_copy*width + i;
            movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
        }
    }

    row_to_copy=movement_info_ptr->player_y;
    for (int i = 0; i < width; i++){
        pos_aux = row_to_copy*width + i;
        movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
    }    

    if ((row_to_copy = movement_info_ptr->player_y+1) < height){
        for (int i = 0; i < width; i++){
            pos_aux = row_to_copy*width + i;
            movement_info_ptr->board[pos_aux] = state_ptr->board[pos_aux];
        }
    }
*/
    return;
    
}



int main(int argc, char *argv[]) {
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    state_ptr = connect_shm_state(width, height);
    if (state_ptr == NULL) { 
        perror("connect_shm_state"); 
        _exit(EXIT_FAILURE); 
    }

    sync_ptr = connect_shm_sync();
    if (sync_ptr == NULL) { 
        perror("connect_shm_sync"); 
        close_shm_state(state_ptr);
        _exit(EXIT_FAILURE); 
    }

    lock_reader(sync_ptr);
    int my_id = get_id(); 
    unlock_reader(sync_ptr);

    if(my_id  < 0){
        close_shm_sync(sync_ptr);
        close_shm_state(state_ptr);
        return 1;
    }

/*
    if (inic_local_game_info(width, height)<0){
        close_shm_sync(sync_ptr);
        close_shm_state(state_ptr);
        return 1;
    }
    */
    
    while (1) { 
        player_wait_turn(sync_ptr, my_id); 
        sem_wait(&sync_ptr->master_inanition_mutex);

        lock_reader(sync_ptr);
        sem_post(&sync_ptr->master_inanition_mutex);

        if (state_ptr->game_ended) {
            unlock_reader(sync_ptr);
            break;
        }
 
        copy_state_info(width, height, my_id);

        unlock_reader(sync_ptr);

        if(movement(width, height, my_id)<0){
            close(STDOUT_FILENO);
            break;
        }
        
    }


    //free_local_game_info();
    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}
