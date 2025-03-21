#include "raylib.h"


int main() {
    InitWindow(800, 600, "Hello from QBS");

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RED);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
