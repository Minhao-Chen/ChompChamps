#include <stdio.h>
#include <stdlib.h>   // atoi
#include <stdbool.h>
#include <unistd.h>
#include "common.h"
#include "ipc_utils.h"


gameState * state_ptr;
synchronization * sync_ptr;

bool adentro(int w, int h, player * p, int x, int y);
int movement(int w, int h, int player_id);


int movement(int w, int h, int player_id){ // basicamente agarra el puntaje mas alto nada mas
    player * p = &state_ptr->players[player_id];

    int max = 0;
    char m = NMOVS;
    int next_pos;
    for(int i=0; i < NMOVS; i++){ 
        if(adentro(w,h,p, movs[i][0], movs[i][1])){
            next_pos = (p->pos_y + movs[i][1]) * w + p->pos_x + movs[i][0];
            if(state_ptr->board[next_pos] > max){
                max = state_ptr->board[next_pos];
                m = i;
            }
        }
    }
    m=2;
    if(m<NMOVS){
        write(STDOUT_FILENO, &m,  1);
        return 0;
    }

    return -1;
}

bool adentro(int w, int h, player * p, int x, int y){
  return p->pos_y+y >= 0 && p->pos_y+y < h && p->pos_x+x >= 0 && p->pos_x+x < w ? true : false; 
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
    
    if (my_id == -1) {
        fprintf(stderr, "Error: no encontré mi PID en game_state\n");
        exit(1);
    }

    return my_id;

}



int main(int argc, char *argv[]) {
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    state_ptr = connect_shm_state(width, height);
    if (state_ptr == NULL) { perror("connect_shm_state"); exit(1); }

    sync_ptr = connect_shm_sync();
    if (sync_ptr == NULL) { perror("connect_shm_sync"); exit(1); }

    lock_reader(sync_ptr);
    int my_id = get_id();
    unlock_reader(sync_ptr);

    
    while (1) { 
        player_wait_turn(sync_ptr, my_id); 
        if (state_ptr->game_ended) {
            break;
        }
        sem_wait(&sync_ptr->master_inanition_mutex);

        lock_reader(sync_ptr);
        sem_post(&sync_ptr->master_inanition_mutex);


        if (state_ptr->game_ended) {
            unlock_reader(sync_ptr);
            break;
        }
 
        // ... lee el tablero ...
        if(movement(width, height, my_id)<0){
            close(STDOUT_FILENO);
            unlock_reader(sync_ptr);
            break;
        }

        //  copiarlo
        unlock_reader(sync_ptr);
        
    }

    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}
