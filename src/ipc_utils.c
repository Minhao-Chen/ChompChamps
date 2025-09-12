#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

static void * create_shm(const char * mem_name, size_t mem_size){
    int shm_fd = shm_open(mem_name, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "shm_open %s", mem_name);
        perror(error_msg);
        return NULL;
    }

    if(ftruncate(shm_fd, mem_size) == -1){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ftruncate %s", mem_name);
        perror(error_msg);
        close(shm_fd);
        return NULL;
    }

    void  * shm = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "mmap %s", mem_name);
        perror(error_msg);
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);

    return shm;    
}

gameState* create_shm_state(int width, int height){
    size_t gameState_size = get_state_size(width, height);

    return (gameState*) create_shm(SHM_STATE, gameState_size);
}

synchronization* create_shm_sync(int num_players){
    synchronization *sync = (synchronization*) create_shm(SHM_SYNC, sizeof(synchronization));

    if(sem_init(&sync->master_notify_view_mutex, 1, 0) == -1){
        perror("sem_init view_notify");
        return NULL;
    }
    if(sem_init(&sync->view_is_done_mutex, 1, 0) == -1) {
        perror("sem_init view_done");
        return NULL;
    }
    if(sem_init(&sync->master_inanition_mutex, 1, 1) == -1) {
        perror("sem_init master_starvation");
        return NULL;
    }
    if(sem_init(&sync->state_lock_mutex, 1, 1) == -1) {
        perror("sem_init state_lock");
        return NULL;
    }
    if(sem_init(&sync->reader_count_lock_mutex, 1, 1) == -1) {
        perror("sem_init count_lock");
        return NULL;
    }
    for(int i = 0; i < num_players; i++){
        if(sem_init(&sync->players_can_move_mutex[i], 1, 1) == -1){
            perror("sem_init player");
            for(int j = 0; j <i; j++){
                sem_destroy(&sync->players_can_move_mutex[j]);
            }
            return NULL;
        }
    }

    sync->activated_reader_counter = 0;

    return sync;
}

static void * connect_shm(const char * mem_name, int oflag, int prot, size_t mem_size){
    int shm_fd = shm_open(mem_name, oflag, 0);

    if (shm_fd == -1){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "shm_open connect %s", mem_name);
        perror(error_msg);
        return NULL;
    }

    void  * shm = mmap(NULL, mem_size, prot, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED){
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "mmap connect %s", mem_name);
        perror(error_msg);
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);

    return shm;    
}

// Conectar a memoria compartida del estado (para vista y jugadores)
gameState* connect_shm_state(int width, int height){
    size_t gameState_size = get_state_size(width, height);

    return (gameState*) connect_shm(SHM_STATE, O_RDONLY, PROT_READ, gameState_size);
}

// Conectar a memoria compartida de sincronización (para vista y jugadores)
synchronization* connect_shm_sync() {
    return (synchronization*) connect_shm(SHM_SYNC, O_RDWR, PROT_READ | PROT_WRITE, sizeof(synchronization));
}

static int close_shm(void * shm, const char * mem_name, size_t mem_size){
    int result = 0;

    if (shm != MAP_FAILED && shm != NULL) {
        if (munmap(shm, mem_size) == -1) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "munmap close %s", mem_name);
            perror(error_msg);
            result = -1;
        }
    }

    return result;

}

// Cerrar conexión a memoria de estado (sin destruir)
int close_shm_state(gameState* state) {    
    if (state == NULL || state == MAP_FAILED) {
        return 0;
    }
    
    size_t size = get_state_size(state->width, state->height);
    return close_shm(state, SHM_STATE, size);
}

// Cerrar conexión a memoria de sincronización (sin destruir)
int close_shm_sync(synchronization* sync) {
    return close_shm(sync, SHM_SYNC,sizeof(synchronization));
}


static int destroy_shm(const char * mem_name){
    int result = 0;

    if (shm_unlink(mem_name) == -1) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "shm_unlink %s", mem_name);
        perror(error_msg);
        result = -1;
    }

    return result;
}

int destroy_shm_state(gameState* state){
    int result = 0;

    if (state == NULL) {
        return 0; // Ya es NULL, nada que hacer
    }

    if(close_shm_state(state) == -1){
        result = -1;
    }

    if(destroy_shm(SHM_STATE) == -1){
        result = -1;
    }

    return result;
}

int destroy_shm_sync(synchronization* sync, int num_players){
    int result = 0;

    if(sync == NULL){
        return 0;
    }

    if (sync != MAP_FAILED){
        for (int i = 0; i < num_players; i++) {
            if (sem_destroy(&sync->players_can_move_mutex[i]) == -1) {
                perror("sem_destroy player");
                result = -1;
            }
        }
        if(sem_destroy(&sync->master_notify_view_mutex) == -1){
            perror("sem_destroy view_notify");
            result = -1;
        }    
        if(sem_destroy(&sync->view_is_done_mutex) == -1){
            perror("sem_destroy view_done");
            result = -1;
        }
        if(sem_destroy(&sync->master_inanition_mutex) == -1){
            perror("sem_destroy master_starvation");
            result = -1;
        }
        if(sem_destroy(&sync->state_lock_mutex) == -1){
            perror("sem_destroy state_lock");
            result = -1;
        }
        if(sem_destroy(&sync->reader_count_lock_mutex) == -1){
            perror("sem_destroy counter_lock");
            result = -1;
        }

        if(close_shm_sync(sync) == -1){
            result = -1;
        }

        if(destroy_shm(SHM_SYNC) == -1){
            result = -1;
        }
    }
    
    return result;
}

void lock_writer(synchronization* sync){
    // Protocolo escritor (evita inanición)
    sem_wait(&sync->master_inanition_mutex);  // C: Evitar inanición
    sem_wait(&sync->state_lock_mutex);        // D: Lock de escritura
}

void unlock_writer(synchronization* sync){
    sem_post(&sync->state_lock_mutex);        // D: Liberar lock
    sem_post(&sync->master_inanition_mutex);  // C: Permitir siguiente escritor
}

void lock_reader(synchronization* sync){
    // Protocolo lector
    sem_wait(&sync->reader_count_lock_mutex);      // E: Lock contador
    if (sync->activated_reader_counter++ == 0) {
        sem_wait(&sync->state_lock_mutex);    // D: Primer lector bloquea escritores
    }
    sem_post(&sync->reader_count_lock_mutex);      // E: Liberar contador
}

void unlock_reader(synchronization* sync){
    sem_wait(&sync->reader_count_lock_mutex);      // E: Lock contador
    if (sync->activated_reader_counter-- == 1) {
        sem_post(&sync->state_lock_mutex);    // D: Último lector libera escritores
    }
    sem_post(&sync->reader_count_lock_mutex);      // E: Liberar contador
}

void player_wait_turn(synchronization *sync, int player_id){
    if(player_id >= 0 && player_id < 9){
        sem_wait(&sync->players_can_move_mutex[player_id]);
    }
}

void master_release_player(synchronization *sync, int player_id){
    if(player_id >= 0 && player_id < 9){
        sem_post(&sync->players_can_move_mutex[player_id]);
    }
}

void view_wait_changes(synchronization *sync){
    sem_wait(&sync->master_notify_view_mutex);
}

void view_notify_print(synchronization *sync){
    sem_post(&sync->view_is_done_mutex);
}

void master_notify_view(synchronization *sync){
    sem_post(&sync->master_notify_view_mutex);
}

void master_wait_view(synchronization *sync){
    sem_wait(&sync->view_is_done_mutex);
}

int get_state_size(int width, int height){
    return sizeof(gameState) + (width * height * sizeof(int));
}