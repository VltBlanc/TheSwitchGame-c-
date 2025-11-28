#include "raylib.h"
#include <stdio.h>

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "test ecran");

    Vector2 ballPosition = { (float)screenWidth/2, (float)screenHeight/2 };

    float speed = 1.0f;

    float fps=120;
    SetTargetFPS(fps); 

    char buffer[32];

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {

        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyDown(KEY_RIGHT)) ballPosition.x +=GetFrameTime()*fps*speed;
        if (IsKeyDown(KEY_LEFT)) ballPosition.x -= GetFrameTime()*fps*speed;
        if (IsKeyDown(KEY_UP)) ballPosition.y -= GetFrameTime()*fps*speed;
        if (IsKeyDown(KEY_DOWN)) ballPosition.y += GetFrameTime()*fps*speed;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("move the ball with arrow keys", 10, 10, 20, DARKGRAY);
            snprintf(buffer, sizeof(buffer), "%.2f", 1.0f/GetFrameTime());
            DrawText(buffer, (4.5/5.0)*screenWidth, 10, 20, DARKGRAY);

            DrawCircleV(ballPosition, 20, MAROON);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}