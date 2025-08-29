#ifndef IPC_UTILS_H
#define IPC_UTILS_H

#include "common.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>


// Funciones que manejan las memorias compartidas
gameState* create_shm_state(int width, int height);
synchronization* creat_shm_sync(int num_players);
int destroy_shm_state(gameState* state);
int destroy_shm_sync(synchronization* sync, int num_players);
gameState* connect_shm_state();
synchronization* connect_shm_sync();
int close_shm_state(gameState* state);
int close_shm_sync(synchronization* sync);

// Funciones de sincronización
void lock_writer(synchronization* sync);
void unlock_writer(synchronization* sync);
void lock_reader(synchronization* sync);
void unlock_reader(synchronization* sync);

// Funciones para procesos específicos
void player_wait_turn(synchronization *sync, int player_id);
void master_release_player(synchronization *sync, int player_id);
void view_wait_changes(synchronization *sync);
void view_notify_print(synchronization *sync);
void master_notify_view(synchronization *sync);
void master_wait_view(synchronization *sync);

int get_state_size(int width, int height);

#endif