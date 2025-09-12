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
#define MAX_LENGHT_NAME 16
#define MAX_PLAYERS 9
#define NMOVS 8

typedef struct { 
    char name[MAX_LENGHT_NAME]; 
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
    player players[MAX_PLAYERS];
    bool game_ended;
    int board[];
} gameState;

typedef struct {
    sem_t master_notify_view_mutex; // A: El máster le indica a la vista que hay cambios por imprimir
    sem_t view_is_done_mutex; // B: La vista le indica al máster que terminó de imprimir
    sem_t master_inanition_mutex; // C: Mutex para evitar inanición del máster al acceder al estado
    sem_t state_lock_mutex; // D: Mutex para el estado del juego
    sem_t reader_count_lock_mutex; // E: Mutex para la variable reader_activated
    unsigned int activated_reader_counter; // F: Cantidad de jugadores leyendo el estado
    sem_t players_can_move_mutex[9]; // G: Le indican a cada jugador que puede enviar 1 movimiento
} synchronization;

static const int movs[NMOVS][2] = {
    { 0,  -1}, // Norte
    { 1,  -1}, // Noreste
    { 1,  0}, // Este
    { 1,  1}, // Sudeste
    { 0,  1}, // Sur
    { -1, 1}, // Suroeste
    { -1, 0}, // Oeste
    { -1, -1}  // Noroeste
};

#endif