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
    int player_id = atoi(argv[3]);

    int fd = shm_open(SHM_STATE, O_RDWR, 0666);

    if (fd == -1) { perror("shm_open"); exit(1); }

    state_ptr = mmap(NULL, sizeof(gameState) + (width * height* sizeof(int)), //no es con argv[1] y [2]??
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (state_ptr == MAP_FAILED) { perror("mmap"); exit(1); }

    fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); exit(1); }

    sync_ptr = mmap(NULL, sizeof(synchronization),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (sync_ptr == MAP_FAILED) { perror("mmap"); exit(1); }

    

    while (1) {
        sem_wait(&sync_ptr->sem_players[player_id]);

          sem_wait(&sync_ptr->sem_master_starvation);     // turnstile (paso breve)
        sem_wait(&sync_ptr->sem_counter_lock);
        sync_ptr->reader_activated++;
        if (sync_ptr->reader_activated == 1) {
            // Primer lector → bloquea al máster
            //sem_wait(&sync_ptr->sem_master_starvation); 
             sem_wait(&sync_ptr->sem_state_lock);
        }
        sem_post(&sync_ptr->sem_counter_lock);
        sem_post(&sync_ptr->sem_master_starvation);     // liberar turnstile rápido

        movement(state_ptr->width, state_ptr->height, &state_ptr->players[player_id]);
        //sem_wait(&sync_ptr->sem_state_lock);
        // ... lee el tablero ...
        //sem_post(&sync_ptr->sem_state_lock);


        sem_wait(&sync_ptr->sem_counter_lock);
        sync_ptr->reader_activated--;
        if (sync_ptr->reader_activated == 0) {
            // Último lector libera al máster
            //sem_post(&sync_ptr->sem_master_starvation);
            sem_post(&sync_ptr->sem_state_lock);
        }

        sem_post(&sync_ptr->sem_counter_lock);
    }
}
