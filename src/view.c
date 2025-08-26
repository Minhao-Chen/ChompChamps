#include <stdio.h>
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>


#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define MAX_WIDTH  50
#define MAX_HEIGHT 50
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
GameBoard* create_board(int width, int height) {
    GameBoard *board;
    memset(board->cells, 0, MAX_HEIGHT*MAX_WIDTH);
    board->width = width;
    board->height = height;
    
    // Matriz de celdas
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board->cells[i][j] = rand() % 100; // Recompensas aleatorias
        }
    }
    
    // Buffers para renderizado
    memset(board->prev_buffer, 0, BUFFER_SIZE);
    memset(board->render_buffer, 0, BUFFER_SIZE);
    
    return board;
}

// MÉTODO 1: Renderizado completo con buffer único
void render_full_buffer(GameBoard *board) {
    write(1, "\33[H\33[2J\33[3J", 11);
    char *buf = board->render_buffer;
    int pos = 0;
    
    // Mover cursor al inicio y limpiar pantalla
    pos += sprintf(buf + pos, "\033[H\033[2J");
    
    // Renderizar tablero
    for (int i = 0; i < board->height; i++) {
        pos += sprintf(buf + pos, "|");
        for (int j = 0; j < board->width; j++) {
            pos += sprintf(buf + pos, "%3d|", board->cells[i][j]);
        }
        pos += sprintf(buf + pos, "\n");
    }
    
    // Escribir todo de una vez
    write(STDOUT_FILENO, buf, pos);
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
int main() {
    setup_terminal();
    
    GameBoard *board = create_board(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    printf("Renderizado eficiente de tablero (Ctrl+C para salir)\n");
    usleep(1000000); // 1 segundo de pausa
    
    for (int frame = 0; frame < 30; frame++) {
        // Simular cambios
        update_board_random(board);
        
        // ELEGIR MÉTODO DE RENDERIZADO:
         render_full_buffer(board);        // Método 1: Buffer completo
        // render_differential(board);       // Método 2: Diferencial
        //render_double_buffer(board);         // Método 4: Double buffer (RECOMENDADO)
        
        usleep(200000); // 200ms
    }
    
    restore_terminal();
    printf("\n\nSimulación terminada.\n");
    return 0;
}