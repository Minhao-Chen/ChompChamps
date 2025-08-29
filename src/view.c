#include <stdio.h>
#include "common.h"
#include "ipc_utils.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>     // srand, time

#define BUFFER_SIZE 4096


gameState * state;
synchronization* sems;
char render_buffer[BUFFER_SIZE]={0};
char prev_buffer[BUFFER_SIZE]={0};


// MÉTODO 1: Renderizado completo con buffer único
/*static void render_full_buffer() {
    // limpia pantalla
    (void)write(STDOUT_FILENO, "\033[H\033[2J\033[3J", 12);

    char *buf = render_buffer;
    int   pos = 0;

    // Cursor al inicio
    int n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0,
                     "\033[H");
    pos += (n > 0) ? n : 0;

    // Dibuja tablero con control de capacidad para evitar overflow
    for (int i = 0; i < state->height; i++) {
        n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0, "|");
        pos += (n > 0) ? n : 0;

        for (int j = 0; j < state->width; j++) {
            n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0,
                         "%3d|", state->board[i*state->width+j]);
            pos += (n > 0) ? n : 0;
            if (pos >= BUFFER_SIZE - 8) break; // margen de seguridad
        }
        n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0, "\n");
        pos += (n > 0) ? n : 0;
        if (pos >= BUFFER_SIZE - 8) break;
    }

    // imprime lo que entró en el buffer
    (void)write(STDOUT_FILENO, buf, (pos < BUFFER_SIZE) ? pos : BUFFER_SIZE);
}*/

void render_board() {
    for (int y = 0; y < state->height; y++) {
        for (int x = 0; x < state->width; x++) {
            int cell = state->board[y * state->width + x];
            if (cell > 0) {
                printf(" %d ", cell);   // recompensa
            } else {
                int player_id = -cell;
                printf(" P%d", player_id); // jugador
            }
        }
        printf("\n");
    }
}

void render_players() {
    printf("Jugadores:\n");
    for (int i = 0; i < state->player_count; i++) {
        player *p = &state->players[i];
        printf("P%d %-16s score=%u pos=(%hu,%hu)\n",
               i, p->name, p->score, p->pos_x, p->pos_y);
    }
}

// Simular cambios aleatorios en el tablero
void update_board_random() {
    int num_changes = rand() % 5 + 1; // 1-5 cambios
    for (int i = 0; i < num_changes; i++) {
        int row = rand() % state->height;
        int col = rand() % state->width;
        state->board[row*state->width+col] = (rand() % 9) + 1;
    }
}

int main(int argc, char *argv[]) {
     printf("VIEW: hola!\n");
    fflush(stdout); // para q se vea con el flush



    printf("VISTA");
    srand((unsigned)time(NULL));
    int fd = shm_open(SHM_STATE, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); exit(1); }

    state = mmap(NULL, sizeof(gameState) + (atoi(argv[0]) * atoi(argv[1]) * sizeof(int)),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (state == MAP_FAILED) { perror("mmap"); exit(1); }

    fd = shm_open(SHM_SYNC, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); exit(1); }

    sems = mmap(NULL, sizeof(synchronization),
                             PROT_READ,
                             MAP_SHARED, fd, 0);
    if (sems == MAP_FAILED) { perror("mmap"); exit(1); }


    //while (state->active_game) {
        printf("ANTES DEL WAIT VISTA");
            fflush(stdout);  // para q se vea con el flush

        sem_wait(&sems->sem_view_notify);
        printf("\033[H\033[2J"); // limpiar
        render_board(state);
        render_players(state);
        sem_post(&sems->sem_view_done);

        sem_wait(&sems->sem_view_notify);
        printf("\033[H\033[2J"); // limpiar
        render_board(state);
        render_players(state);
        sem_post(&sems->sem_view_done);
        //sem_post()
       // sem_post(&sems->sem_view_notify);
    //}

    
    



    //restore_terminal();
    printf("\n\nSimulacion terminada.\n");
    return 0;
}