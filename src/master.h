#ifndef MASTER_H
#define MASTER_H

#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10

static const int POLYGON_OFFSETS[][9][2] = {
    {},  // 0 jugadores (no usado)
    {{0, 0}},  // 1 jugador: centro
    {{-800, 0}, {800, 0}},  // 2 jugadores: línea horizontal
    // 3 jugadores: triángulo (vértice superior)
    {{0, -1000}, {866, 500}, {-866, 500}},
    // 4 jugadores: cuadrado
    {{0, -1000}, {1000, 0}, {0, 1000}, {-1000, 0}},
    // 5 jugadores: pentágono
    {{0, -1000}, {951, -309}, {588, 809}, {-588, 809}, {-951, -309}},
    // 6 jugadores: hexágono
    {{0, -1000}, {866, -500}, {866, 500}, {0, 1000}, {-866, 500}, {-866, -500}},
    // 7 jugadores: heptágono
    {{0, -1000}, {782, -623}, {975, -222}, {434, 901}, {-434, 901}, {-975, -222}, {-782, -623}},
    // 8 jugadores: octágono
    {{0, -1000}, {707, -707}, {1000, 0}, {707, 707}, {0, 1000}, {-707, 707}, {-1000, 0}, {-707, -707}},
    // 9 jugadores: eneágono
    {{0, -1000}, {643, -766}, {985, -174}, {866, 500}, {342, 940}, {-342, 940}, {-866, 500}, {-985, -174}, {-643, -766}}
};

#endif