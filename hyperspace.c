#include "include/hyperspace.h"
#include <math.h> // For powf
#include <raymath.h>


#define STAR_COUNT 420

void PlayHyperspaceEffect(float duration, Texture2D shipTexture) {
  // Initialization
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  Color bgColor = ColorLerp(DARKBLUE, BLACK, 0.69f);

  // Speed at which we fly forward
  float speed = 10.0f / 9.0f;

  // We're either drawing lines or circles
  bool drawLines = true;

  Vector3 stars[STAR_COUNT] = {0};
  Vector2 starsScreenPos[STAR_COUNT] = {0};

  // Setup the stars with a random position
  for (int i = 0; i < STAR_COUNT; i++) {
    stars[i].x = GetRandomValue(-screenWidth * 0.5f, screenWidth * 0.5f);
    stars[i].y = GetRandomValue(-screenHeight * 0.5f, screenHeight * 0.5f);
    stars[i].z = 1.0f;
  }

  float timer = 0.0f;

  // Main loop for the effect
  while (!WindowShouldClose() && timer < duration) {
    float dt = GetFrameTime();
    // Fallback si GetFrameTime retourne 0 (peut arriver avec llvmpipe/software
    // rendering)
    if (dt <= 0.0f)
      dt = 1.0f / 60.0f;
    timer += dt;

    // Update
    //----------------------------------------------------------------------------------

    // Increase speed over time for entrance effect
    if (timer < 1.0f) {
      speed = Lerp(0.1f, 2.0f, timer);
    }

    for (int i = 0; i < STAR_COUNT; i++) {
      // Update star's timer
      stars[i].z -= dt * speed;

      // Calculate the screen position
      starsScreenPos[i] = (Vector2){
          screenWidth * 0.5f + stars[i].x / stars[i].z,
          screenHeight * 0.5f + stars[i].y / stars[i].z,
      };

      // If the star is too old, or offscreen, it dies and we make a new random
      // one
      if ((stars[i].z < 0.0f) || (starsScreenPos[i].x < 0) ||
          (starsScreenPos[i].y < 0.0f) || (starsScreenPos[i].x > screenWidth) ||
          (starsScreenPos[i].y > screenHeight)) {
        stars[i].x = GetRandomValue(-screenWidth * 0.5f, screenWidth * 0.5f);
        stars[i].y = GetRandomValue(-screenHeight * 0.5f, screenHeight * 0.5f);
        stars[i].z = 1.0f;
      }
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(bgColor);

    for (int i = 0; i < STAR_COUNT; i++) {
      if (drawLines) {
        // Get the time a little while ago for this star, but clamp it
        float t = Clamp(stars[i].z + 1.0f / 32.0f, 0.0f, 1.0f);

        // If it's different enough from the current time, we proceed
        if ((t - stars[i].z) > 1e-3) {
          // Calculate the screen position of the old point
          Vector2 startPos = (Vector2){
              screenWidth * 0.5f + stars[i].x / t,
              screenHeight * 0.5f + stars[i].y / t,
          };

          // Draw a line connecting the old point to the current point
          DrawLineV(startPos, starsScreenPos[i], RAYWHITE);
        }
      } else {
        // Make the radius grow as the star ages
        float radius = Lerp(stars[i].z, 1.0f, 5.0f);

        // Draw the circle
        DrawCircleV(starsScreenPos[i], radius, RAYWHITE);
      }
    }

    // Draw Ship with Animation
    float scale = 0.3f; // Adjust scale as needed

    Rectangle shipSource = {0.0f, 399.0f, 331.0f, 399.0f};

    // Animation Logic
    // Start at bottom (visible), dash to center
    float startY = screenHeight - 100.0f;
    float targetY = screenHeight / 2.0f;

    float t = timer / duration; // 0.0 to 1.0

    // Use an ease-out curve for a "dash" feel (fast start, slow end)
    // t goes from 0 to 1.
    // We want it to reach 1.0 slightly before the end maybe? Or just use the
    // full duration. Let's use a cubic ease out.
    float easeT = 1.0f - powf(1.0f - t, 3.0f);

    float currentY = Lerp(startY, targetY, easeT);

    Rectangle destRec = {screenWidth / 2.0f, currentY, 331.0f * 0.2f,
                         399.0f * 0.2f};
    Vector2 origin = {destRec.width / 2.0f, destRec.height / 2.0f};

    DrawTexturePro(shipTexture, shipSource, destRec, origin, 0.0f, WHITE);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }
}
