#include "raylib.h"
#include "core/game.h"

int main() {
    InitWindow(1280, 720, "Asteroid Rush");
    SetTargetFPS(60);

    Game game;
    game.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
