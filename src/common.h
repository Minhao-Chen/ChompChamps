#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_STATE "/game_state"
#define SHM_SYNC "/game_sync"
#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define MAX_WIDTH  50
#define MAX_HEIGHT 50

typedef struct { 
    char name[16]; 
    unsigned int score;
    unsigned int invalid_move;
    unsigned int valid_move;
    unsigned short pos_x, pos_y;
    pid_t pid;
    bool blocked;
} player;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned int player_count;
    player players[9];
    bool active_game;
    int board[];
} gameState;

typedef struct {
    sem_t sem_view_notify; // Máster → Vista: hay cambios
    sem_t sem_view_done; // Vista → Máster: imprimió
    sem_t sem_master_starvation; // Mutex para evitar inanición del máster
    sem_t sem_state_lock; // Mutex para el estado del juego
    sem_t sem_counter_lock; // Mutex para la variable F
    unsigned int reader_activated; // Cantidad de jugadores leyendo
    sem_t sem_players[9]; // Semáforos por jugador
} synchronization;

typedef struct {
    int width;
    int height;
    int delay_ms;
    int timeout_sec;
    unsigned int seed;
    char *view_path;
    char *player_names[9];
    int num_players;
 }Config;

#endif