#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <semaphore.h>

#define SHM_GAME_STATE_NAME "/game_state"


typedef struct { 
    char name[16]; 
    unsigned int score;
    unsigned int invalid_move;
    unsigned int valid_move;
    unsigned short pos_x, pos_y;
    pid_t pid;
    bool blocked;
}player;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned int player_count;
    player players[9];
    bool active_game;
    int board[];
}gameParams;

typedef struct {
    sem_t A; // Máster → Vista: hay cambios
    sem_t B; // Vista → Máster: imprimió
    sem_t C; // Mutex para evitar inanición del máster
    sem_t D; // Mutex para el estado del juego
    sem_t E; // Mutex para la variable F
    unsigned int F; // Cantidad de jugadores leyendo
    sem_t G[9]; // Semáforos por jugador
}SyncStructure;

#endif