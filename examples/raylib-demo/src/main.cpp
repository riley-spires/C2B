#include "raylib.h"


int main() {
    InitWindow(800, 600, "Hello from c2b");

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RED);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
