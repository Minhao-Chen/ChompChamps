#include <stdio.h>
#include <stdlib.h>   // atoi
#include <stdbool.h>
#include <unistd.h>
#include "common.h"
#include "ipc_utils.h"


gameState * state_ptr;
synchronization * sync_ptr;

bool adentro(int w, int h, player * p, int x, int y);
void movement(int w, int h, player * p);


void movement(int w, int h, player * p){ // basicamente agarra el puntaje mas alto nada mas
    int max = 0;
    char m = NMOVS;
    int next_pos;
    for(int i=0; i < NMOVS; i++){ // sacar el 8 y ponerlo con define o macro
        if(adentro(w,h,p, movs[i][0], movs[i][1])){
            next_pos = (p->pos_y + movs[i][1]) * w + p->pos_x + movs[i][0];
            if(state_ptr->board[next_pos] > max){
                max = state_ptr->board[next_pos];
                m = i;
            }
        }
    }
    
    if(m<NMOVS){
        write(STDOUT_FILENO, &m,  1);
    }else{
        close(STDOUT_FILENO);
    }
    

}

bool adentro(int w, int h, player * p, int x, int y){
  return p->pos_y+y >= 0 && p->pos_y+y < h && p->pos_x+x >= 0 && p->pos_x+x < w ? true : false; 
}


int main(int argc, char *argv[]) {
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    state_ptr = connect_shm_state(width, height);
    if (state_ptr == NULL) { perror("connect_shm_state"); exit(1); }

    sync_ptr = connect_shm_sync();
    if (sync_ptr == NULL) { perror("connect_shm_sync"); exit(1); }

    /*int shm_fd = shm_open(SHM_SYNC, O_RDWR, 0);

    if (shm_fd == -1){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "shm_open connect %s", SHM_SYNC);
        perror(error_msg);
        return 1;
    }

    synchronization  * shm = mmap(NULL, sizeof(synchronization), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "mmap connect %s", SHM_SYNC);
        perror(error_msg);
        close(shm_fd);
        return 1;
    }

    close(shm_fd);*/

    
    //sync_ptr = shm;

    //return shm;

    
    while (1) { 
        //sem_wait(&sync_ptr->sem_players[0]);
        
        player_wait_turn(sync_ptr, 0); 
        if (!state_ptr->active_game) {
            break;
        }
        sem_wait(&sync_ptr->sem_master_starvation);
        //lock_writer(sync_ptr);     // turnstile (paso breve)
        lock_reader(sync_ptr);
        sem_post(&sync_ptr->sem_master_starvation);
        //unlock_writer(sync_ptr);     // liberar turnstile rápido

        if (!state_ptr->active_game) {
            unlock_reader(sync_ptr);
            break;
        }
 
        // ... lee el tablero ...
        //  copiarlo
        unlock_reader(sync_ptr);

        char m=2;

        write(STDOUT_FILENO, &m,  1);

        
        //movement(state_ptr->width, state_ptr->height, &state_ptr->players[player_id]);
        //sem_wait(&sync_ptr->sem_state_lock);
        // ... lee el tablero ...
        //sem_post(&sync_ptr->sem_state_lock);
    }

    close_shm_sync(sync_ptr);
    close_shm_state(state_ptr);
    return 0;
}
