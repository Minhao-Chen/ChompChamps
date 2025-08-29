#include <stdio.h>
#include <stdlib.h>   // atoi
#include <stdbool.h>
#include <unistd.h>
#include "common.h"
#include "ipc_utils.h"

#define NMOVS 8

gameState * state;
synchronization * sync;

bool adentro(int w, int h, player * p, int x, int y);
void movement(int w, int h, player * p, int movs[NMOVS][2], int board[10][10]);

int main(int argc, char *argv[]){
     if (argc < 4) {
        //fprintf(stderr, "Uso: %s <i> <pid>\n", argv[0]);
        return 1;
    }

    int width   = atoi(argv[1]);   // convierto argv[1] → int
    int height = atoi(argv[2]);   // convierto argv[2] → int
    int num_jugador = atoi(argv[3]);   // convierto argv[3] → int

    int movs[NMOVS][2] = {
    {-1,  0}, // Norte
    {-1,  1}, // Noreste
    { 0,  1}, // Este
    { 1,  1}, // Sudeste
    { 1,  0}, // Sur
    { 1, -1}, // Suroeste
    { 0, -1}, // Oeste
    {-1, -1}  // Noroeste
    };

    state = connect_shm_state();
    sync = connect_shm_sync(); 

    int board[10][10]; // es como ejemplo esto, despues usar el board posta

    while(1){
        //ver bien lo de los semaforos
        movement(width, height, &state->players[num_jugador], movs, board/*seria el board pero es una matriz.....*/);
    }

    //no hace falta hacer open con los semaforos pq estan init
}

void movement(int w, int h, player * p, int movs[NMOVS][2], int board[10][10]){ // basicamente agarra el puntaje mas alto nada mas
    int max = -10;
    int m = -1;
    for(int i=0; i < NMOVS; i++){ // sacar el 8 y ponerlo con define o macro
        if(adentro(w,h,p, movs[i][0], movs[i][1])){
            if(board[ p->pos_y + movs[i][0]][p->pos_x + movs[i][1]] > max){
                max = board[ p->pos_y + movs[i][0]][p->pos_x + movs[i][1]];
                m = i;
            }
        }
    }

    if(max > -10){
        p->pos_y += movs[m][0];
        p->pos_x += movs[m][1];
    }

}

bool adentro(int w, int h, player * p, int x, int y){
  return p->pos_y+y >= 0 && p->pos_y+y < h && p->pos_x+x >= 0 && p->pos_x+x < w ? true : false; 
}