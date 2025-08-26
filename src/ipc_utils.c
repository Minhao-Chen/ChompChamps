#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


// Variables globales para file descriptors
static int shm_state_fd = -1;
static int shm_sync_fd = -1;

gameState* create_shm_state(int width, int height, int num_players){
    size_t gameState_size = get_state_size(width, height);

    shm_state_fd = shm_open(SHM_STATE, O_CREAT | O_RDWR, 0666);
    if(shm_state_fd == -1){
        perror("shm_open state");
        return NULL;
    }

    if(ftruncate(shm_state_fd, gameState_size) == -1){
        perror("ftruncate state");
        close(shm_state_fd);
        return NULL;
    }

    gameState *state = mmap(NULL, gameState_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_state_fd, 0);
    if(state == MAP_FAILED){
        perror("mmap state");
        close(shm_state_fd);
        return NULL;
    }

    state->width = width;
    state->height = height;
    state->player_count = num_players;
    state->active_game = true;

    return state;
}

synchronization* creat_shm_sync(int num_players){
    shm_sync_fd = shm_open(SHM_SYNC, O_CREAT | O_RDWR, 0666);
    if(shm_sync_fd == -1){
        perror("shm_open sync");
        return NULL;
    }

    if(ftruncate(shm_sync_fd, sizeof(synchronization)) == -1){
        perror("ftruncate sync");
        close(shm_sync_fd);
        return NULL;
    }

    synchronization *sync = mmap(NULL, sizeof(synchronization), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync_fd, 0);
    if(sync == MAP_FAILED){
        perror("mmap sync");
        close(shm_sync_fd);
        return NULL;
    }

    if(sem_init(&sync->sem_view_notify, 1, 0) == -1){
        perror("sem_init view_notify");
        return NULL;
    }
    if(sem_init(&sync->sem_view_done, 1, 0) == -1) {
        perror("sem_init view_done");
        return NULL;
    }
    if(sem_init(&sync->sem_master_starvation, 1, 1) == -1) {
        perror("sem_init master_starvation");
        return NULL;
    }
    if(sem_init(&sync->sem_state_lock, 1, 1) == -1) {
        perror("sem_init state_lock");
        return NULL;
    }
    if(sem_init(&sync->sem_counter_lock, 1, 1) == -1) {
        perror("sem_init count_lock");
        return NULL;
    }
    for(int i = 0; i < num_players; i++){
        if(sem_init(&sync->sem_players[i], 1, 1) == -1){
            perror("sem_init player");
            for(int j = 0; j <i; j++){
                sem_destroy(&sync->sem_players[j]);
            }
            return NULL;
        }
    }

    sync->reader_activated = 0;

    return sync;
}

int destroy_shm_state(gameState* state){
    int result = 0;

    if(shm_state_fd != -1){
        if (state != MAP_FAILED) {
            if (munmap(state, get_state_size(state->width, state->height)) == -1) {
                perror("munmap state");
                result = -1;
            }
        }
            
        if (close(shm_state_fd) == -1) {
            perror("close state");
            result = -1;
        }
            
        if (shm_unlink(SHM_STATE) == -1) {
            perror("shm_unlink state");
            result = -1;
        }

        shm_state_fd = -1;
    }
    return result;
}

int destroy_shm_sync(synchronization* sync, int num_players){
    int result = 0;
    if(shm_sync_fd != -1){
        if (sync != MAP_FAILED) {
            for (int i = 0; i < num_players; i++) {
                if (sem_destroy(&sync->sem_players[i]) == -1) {
                perror("sem_destroy player");
                result = -1;
            }
            }
            sem_destroy(&sync->sem_view_notify);
            sem_destroy(&sync->sem_view_done);
            sem_destroy(&sync->sem_master_starvation);
            sem_destroy(&sync->sem_state_lock);
            sem_destroy(&sync->sem_counter_lock);
        
            if(munmap(sync, sizeof(synchronization)) == -1){
                perror("munmap sync");
                result = -1;
            }
        }
    
        if(close(shm_sync_fd) == -1){
            perror("close sync");
            result  = -1;
        }
        if(shm_unlink(SHM_SYNC) == -1){
            perror("shm_unlink sync");
            result = -1;
        }
        
        shm_sync_fd = -1;
    }
    return result;
}

void lock_writer(synchronization* sync){
    // Protocolo escritor (evita inanición)
    sem_wait(&sync->sem_master_starvation);  // C: Evitar inanición
    sem_wait(&sync->sem_state_lock);        // D: Lock de escritura
}

void unlock_writer(synchronization* sync){
    sem_post(&sync->sem_state_lock);        // D: Liberar lock
    sem_post(&sync->sem_master_starvation);  // C: Permitir siguiente escritor
}

void lock_reader(synchronization* sync){
    // Protocolo lector
    sem_wait(&sync->sem_counter_lock);      // E: Lock contador
    sync->reader_activated++;                // F: Incrementar contador
    if (sync->reader_activated == 1) {
        sem_wait(&sync->sem_state_lock);    // D: Primer lector bloquea escritores
    }
    sem_post(&sync->sem_counter_lock);      // E: Liberar contador
}

void unlock_reader(synchronization* sync){
    sem_wait(&sync->sem_counter_lock);      // E: Lock contador
    sync->reader_activated--;                // F: Decrementar contador
    if (sync->reader_activated == 0) {
        sem_post(&sync->sem_state_lock);    // D: Último lector libera escritores
    }
    sem_post(&sync->sem_counter_lock);      // E: Liberar contador
}

void player_wait_turn(synchronization *sync, int player_id){
    if(player_id >= 0 && player_id < 9){
        sem_wait(&sync->sem_players[player_id]);
    }
}

void master_release_player(synchronization *sync, int player_id){
    if(player_id >= 0 && player_id < 9){
        sem_post(&sync->sem_players[player_id]);
    }
}

void view_wait_changes(synchronization *sync){
    sem_wait(&sync->sem_view_notify);
}

void view_notify_print(synchronization *sync){
    sem_post(&sync->sem_view_done);
}

void master_notify_view(synchronization *sync){
    sem_post(&sync->sem_view_notify);
}

void master_wait_view(synchronization *sync){
    sem_wait(&sync->sem_view_done);
}

int get_state_size(int width, int height){
    return sizeof(gameState) + (width * height * sizeof(int));
}