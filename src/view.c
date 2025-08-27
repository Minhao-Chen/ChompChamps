#include <stdio.h>
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>     // srand, time

#define BUFFER_SIZE 4096

// Estructura para el tablero
typedef struct {
    int cells[MAX_HEIGHT][MAX_WIDTH];
    int width;
    int height;
    char render_buffer[BUFFER_SIZE];
    char prev_buffer[BUFFER_SIZE];
} GameBoard;

// Inicializar tablero
static void init_board(GameBoard *board, int width, int height) {
    if (width < DEFAULT_WIDTH || height < DEFAULT_HEIGHT || width > MAX_WIDTH || height > MAX_HEIGHT) {
        fprintf(stderr, "Dimensiones invalidas: %dx%d (max %dx%d)\n",
                width, height, MAX_WIDTH, MAX_HEIGHT);
        exit(EXIT_FAILURE);
    }
    board->width  = width;
    board->height = height;

    // Limpia toda la matriz correctamente
    memset(board->cells, 0, sizeof(board->cells));
    memset(board->render_buffer, 0, sizeof(board->render_buffer));
    memset(board->prev_buffer,   0, sizeof(board->prev_buffer));

    // Llena con aleatorios
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board->cells[i][j] = rand() % 100;
        }
    }
}

// MÉTODO 1: Renderizado completo con buffer único
static void render_full_buffer(GameBoard *board) {
    // limpia pantalla
    (void)write(STDOUT_FILENO, "\033[H\033[2J\033[3J", 12);

    char *buf = board->render_buffer;
    int   pos = 0;

    // Cursor al inicio
    int n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0,
                     "\033[H");
    pos += (n > 0) ? n : 0;

    // Dibuja tablero con control de capacidad para evitar overflow
    for (int i = 0; i < board->height; i++) {
        n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0, "|");
        pos += (n > 0) ? n : 0;

        for (int j = 0; j < board->width; j++) {
            n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0,
                         "%3d|", board->cells[i][j]);
            pos += (n > 0) ? n : 0;
            if (pos >= BUFFER_SIZE - 8) break; // margen de seguridad
        }
        n = snprintf(buf + pos, (pos < BUFFER_SIZE) ? BUFFER_SIZE - pos : 0, "\n");
        pos += (n > 0) ? n : 0;
        if (pos >= BUFFER_SIZE - 8) break;
    }

    // imprime lo que entró en el buffer
    (void)write(STDOUT_FILENO, buf, (pos < BUFFER_SIZE) ? pos : BUFFER_SIZE);
}

// Configurar terminal para modo raw (sin buffering)
void setup_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void restore_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Simular cambios aleatorios en el tablero
void update_board_random(GameBoard *board) {
    int num_changes = rand() % 5 + 1; // 1-5 cambios
    for (int i = 0; i < num_changes; i++) {
        int row = rand() % board->height;
        int col = rand() % board->width;
        board->cells[row][col] = rand() % 100;
    }
}

// Ejemplo de uso
int main(void) {
    srand((unsigned)time(NULL));

    //setup_terminal();

    GameBoard board;                           // <-- memoria automática
    init_board(&board, MAX_WIDTH, MAX_HEIGHT);

    printf("Renderizado eficiente de tablero (Ctrl+C para salir)\n");
    usleep(300000);

    for (int frame = 0; frame < 30; frame++) {
        update_board_random(&board);
        render_full_buffer(&board);
        usleep(200000);
    }


    //restore_terminal();
    printf("\n\nSimulacion terminada.\n");
    return 0;
}