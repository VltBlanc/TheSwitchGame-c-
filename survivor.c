#include "include/menu.h"
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// --- Constantes ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAP_WIDTH 2000
#define MAP_HEIGHT 2000

#define MAX_ENEMIES 100
#define MAX_BULLETS 200
#define MAX_BONUSES 10

// --- STRUCTS ---
typedef struct {
  // personnage
  Vector2 position;
  float radius;
  float speed;
  int health;
  int maxHealth;
  bool isAlive;

  // variables pour le tir
  float fireRate;
  float shootTimer;
  float damage;
  float bulletSpeed;
  float bulletSize;
} Player;

typedef enum { ENEMY_NORMAL, ENEMY_TANK, ENEMY_FAST } EnemyType;
// enemies
typedef struct {
  bool active;
  Vector2 position;
  EnemyType type;
  float radius;
  float speed;
  int health;
  int maxHealth;
  Color color;
} Enemy;

typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  float radius;
  float lifeTime;
  int damage;
} Bullet;

typedef enum { BONUS_DAMAGE, BONUS_HEALTH } BonusType;

typedef struct {
  bool active;
  Vector2 position;
  float radius;
  BonusType type;
  Color color;
} Bonus;

// --- GLOBALS ---
Player player;
Enemy enemies[MAX_ENEMIES] = {0};
Bullet bullets[MAX_BULLETS] = {0};
Bonus bonuses[MAX_BONUSES] = {0};

Camera2D camera = {0};

float gameTime = 0.0f;
float spawnTimer = 0.0f;
float bonusSpawnTimer = 0.0f;
float timeToSpawn = 1.0f; // le spawn rate est de 1 par frame
int score = 0;
bool gameOver = false;
Difficulte currentDifficulty = MOYEN;

// --- GAME LOGIC ---

void InitGame(Difficulte difficulty) {
  currentDifficulty = difficulty;

  // Player Init
  player.position = (Vector2){
      MAP_WIDTH / 2, MAP_HEIGHT / 2}; // le joueur est au centre de la map
  player.radius = 20.0f;
  player.speed = 200.0f;

  // Set stats based on difficulty
  if (difficulty == FACILE) {
    player.maxHealth = 8;
    player.damage = 2;
  } else if (difficulty == DIFFICILE) {
    player.maxHealth = 3;
    player.damage = 1;
  } else {
    player.maxHealth = 5;
    player.damage = 1;
  }

  player.health = player.maxHealth;
  player.isAlive = true;

  player.fireRate = 0.5f;
  player.shootTimer = 0.0f;
  // Base damage is set above, but we ensure it's at least 1
  if (player.damage < 1)
    player.damage = 1;

  player.bulletSpeed = 600.0f;
  player.bulletSize = 8.0f;

  // Camera Init
  camera.target = player.position;
  camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // Reset Globals
  gameTime = 0.0f;
  spawnTimer = 0.0f;
  timeToSpawn = 2.0f;
  score = 0;
  gameOver = false;

  for (int i = 0; i < MAX_ENEMIES; i++)
    enemies[i].active = false;
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
  for (int i = 0; i < MAX_BONUSES; i++)
    bonuses[i].active = false;
}

void SpawnBonus() {
  int slot = -1;
  for (int i = 0; i < MAX_BONUSES; i++) {
    if (!bonuses[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1)
    return;

  // Random position within map bounds
  float x = (float)GetRandomValue(50, MAP_WIDTH - 50);
  float y = (float)GetRandomValue(50, MAP_HEIGHT - 50);

  bonuses[slot].active = true;
  bonuses[slot].position = (Vector2){x, y};
  bonuses[slot].radius = 15.0f;

  if (GetRandomValue(0, 1) == 0) {
    bonuses[slot].type = BONUS_DAMAGE;
    bonuses[slot].color = RED;
  } else {
    bonuses[slot].type = BONUS_HEALTH;
    bonuses[slot].color = GREEN;
  }
}

void SpawnEnemy() {
  int slot = -1;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1)
    return;

  // Spawn logic based on time
  // Difficulty factor increases slowly
  float difficultyFactor = gameTime / 60.0f; // Increase every minute

  // Adjust difficulty factor based on selected difficulty
  if (currentDifficulty == FACILE) {
    difficultyFactor = gameTime / 90.0f; // Slower increase
  } else if (currentDifficulty == DIFFICILE) {
    difficultyFactor = gameTime / 45.0f; // Faster increase
  }

  // Spawn Position (outside camera view)
  float angle = GetRandomValue(0, 360) * DEG2RAD;
  float dist = 800.0f; // Outside typical screen range
  Vector2 spawnPos = {player.position.x + cosf(angle) * dist,
                      player.position.y + sinf(angle) * dist};

  // Clamp to map
  if (spawnPos.x < 0)
    spawnPos.x = 0;
  else if (spawnPos.x > MAP_WIDTH)
    spawnPos.x = MAP_WIDTH;

  if (spawnPos.y < 0)
    spawnPos.y = 0;
  else if (spawnPos.y > MAP_HEIGHT)
    spawnPos.y = MAP_HEIGHT;

  enemies[slot].position = spawnPos;
  enemies[slot].active = true;

  // Determine Type based on difficulty/random
  int roll = GetRandomValue(0, 100);

  if (difficultyFactor > 2.0f && roll < 20) {
    enemies[slot].type = ENEMY_TANK;
    enemies[slot].radius = 30.0f;
    enemies[slot].speed = 100.0f;
    enemies[slot].maxHealth = 5 + (int)difficultyFactor * 2;
    enemies[slot].color = DARKPURPLE;
  } else if (difficultyFactor > 1.0f && roll > 80) {
    enemies[slot].type = ENEMY_FAST;
    enemies[slot].radius = 15.0f;
    enemies[slot].speed = 280.0f;
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth = 1 + (int)difficultyFactor;
    enemies[slot].color = ORANGE;
  } else {
    enemies[slot].type = ENEMY_NORMAL;
    enemies[slot].radius = 20.0f;
    enemies[slot].speed = 180.0f + (difficultyFactor * 10.0f);
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth = 1.5 + (int)difficultyFactor;
    enemies[slot].color = RED;
  }

  enemies[slot].health = enemies[slot].maxHealth;
}

int FindNearestEnemy() {
  int nearest = -1;
  float minDistSq = 99999999.0f;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      float dx = player.position.x - enemies[i].position.x;
      float dy = player.position.y - enemies[i].position.y;
      float d = dx * dx + dy * dy;
      if (d < minDistSq) {
        minDistSq = d;
        nearest = i;
      }
    }
  }
  return nearest;
}

void UpdateGame(float dt) {
  if (gameOver) {
    if (IsKeyPressed(KEY_R))
      InitGame(currentDifficulty);
    return;
  }

  gameTime += dt;

  // --- PLAYER MOVEMENT ---
  Vector2 move = {0};
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    move.y -= 1;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    move.y += 1;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    move.x -= 1;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    move.x += 1;

  float moveLen = sqrtf(move.x * move.x + move.y * move.y);
  if (moveLen > 0) {
    move.x /= moveLen;
    move.y /= moveLen;
    player.position.x += move.x * player.speed * dt;
    player.position.y += move.y * player.speed * dt;
  }

  // Clamp Player
  player.position.x =
      (player.position.x < 0)
          ? 0
          : ((player.position.x > MAP_WIDTH) ? MAP_WIDTH : player.position.x);
  player.position.y =
      (player.position.y < 0)
          ? 0
          : ((player.position.y > MAP_HEIGHT) ? MAP_HEIGHT : player.position.y);

  // Camera Follow
  camera.target = player.position;

  // --- SHOOTING ---
  player.shootTimer += dt;
  if (player.shootTimer >= player.fireRate) {
    int targetIdx = FindNearestEnemy();
    if (targetIdx != -1) {
      float dx = player.position.x - enemies[targetIdx].position.x;
      float dy = player.position.y - enemies[targetIdx].position.y;
      float distSq = dx * dx + dy * dy;
      // Only shoot if visible (approx half screen diagonal + buffer)
      if (distSq > 675.0f * 675.0f)
        targetIdx = -1;
    }

    if (targetIdx != -1) {
      // Find bullet slot
      int bSlot = -1;
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
          bSlot = i;
          break;
        }
      }

      if (bSlot != -1) {
        Vector2 dir = {enemies[targetIdx].position.x - player.position.x,
                       enemies[targetIdx].position.y - player.position.y};
        float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dirLen > 0) {
          dir.x /= dirLen;
          dir.y /= dirLen;
        }

        bullets[bSlot].active = true;
        bullets[bSlot].position = player.position;
        bullets[bSlot].velocity =
            (Vector2){dir.x * player.bulletSpeed, dir.y * player.bulletSpeed};
        bullets[bSlot].radius = player.bulletSize;
        bullets[bSlot].lifeTime = 2.0f;
        bullets[bSlot].damage = player.damage;

        player.shootTimer = 0.0f;
      }
    }
  }

  // --- BULLETS ---
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].position.x += bullets[i].velocity.x * dt;
      bullets[i].position.y += bullets[i].velocity.y * dt;
      bullets[i].lifeTime -= dt;

      if (bullets[i].lifeTime <= 0) {
        bullets[i].active = false;
        continue;
      }

      // Collision with Enemies
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (enemies[e].active) {
          if (CheckCollisionCircles(bullets[i].position, bullets[i].radius,
                                    enemies[e].position, enemies[e].radius)) {
            enemies[e].health -= bullets[i].damage;
            bullets[i].active = false; // Destroy bullet

            if (enemies[e].health <= 0) {
              enemies[e].active = false;
              score += 10;
            }
            break; // Bullet hit one enemy
          }
        }
      }
    }
  }

  // --- SPAWNING ---
  // Spawn rate increases slowly over time
  // Base 2.0s, decreases by 0.1s every 30s, min 0.2s
  float currentSpawnRate = 1.0f - (gameTime / 30.0f) * 0.1f;
  if (currentSpawnRate < 0.2f)
    currentSpawnRate = 0.2f;

  spawnTimer += dt;
  if (spawnTimer >= currentSpawnRate) {
    SpawnEnemy();
    spawnTimer = 0.0f;
  }

  // --- BONUSES ---
  bonusSpawnTimer += dt;
  if (bonusSpawnTimer >= 15.0f) {
    SpawnBonus();
    bonusSpawnTimer = 0.0f;
  }

  for (int i = 0; i < MAX_BONUSES; i++) {
    if (bonuses[i].active) {
      if (CheckCollisionCircles(player.position, player.radius,
                                bonuses[i].position, bonuses[i].radius)) {
        if (bonuses[i].type == BONUS_DAMAGE) {
          player.damage += 1;
        } else if (bonuses[i].type == BONUS_HEALTH) {
          player.maxHealth += 1;
          player.health = player.maxHealth;
        }
        bonuses[i].active = false;
      }
    }
  }

  // --- ENEMIES ---
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      Vector2 dir = {player.position.x - enemies[i].position.x,
                     player.position.y - enemies[i].position.y};
      float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
      if (dirLen > 0) {
        dir.x /= dirLen;
        dir.y /= dirLen;
      }
      enemies[i].position.x += dir.x * enemies[i].speed * dt;
      enemies[i].position.y += dir.y * enemies[i].speed * dt;

      // Collision with Player
      if (CheckCollisionCircles(player.position, player.radius,
                                enemies[i].position, enemies[i].radius)) {
        player.health--;
        enemies[i].active =
            false; // Enemy dies on impact (or bounces? simple: dies)

        if (player.health <= 0) {
          player.isAlive = false;
          gameOver = true;
        }
      }
    }
  }
}

void DrawGame() {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  // Draw Map Borders
  DrawRectangleLines(0, 0, MAP_WIDTH, MAP_HEIGHT, GRAY);

  // Draw Grid for reference
  for (int x = 0; x <= MAP_WIDTH; x += 100)
    DrawLine(x, 0, x, MAP_HEIGHT, LIGHTGRAY);
  for (int y = 0; y <= MAP_HEIGHT; y += 100)
    DrawLine(0, y, MAP_WIDTH, y, LIGHTGRAY);

  DrawCircleV(player.position, player.radius, BLUE);

  // Draw Enemies
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      DrawCircleV(enemies[i].position, enemies[i].radius, enemies[i].color);
    }
  }

  // Draw Bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      DrawCircleV(bullets[i].position, bullets[i].radius, BLACK);
    }
  }

  // Draw Bonuses
  for (int i = 0; i < MAX_BONUSES; i++) {
    if (bonuses[i].active) {
      DrawCircleV(bonuses[i].position, bonuses[i].radius, bonuses[i].color);
      DrawCircleLines(bonuses[i].position.x, bonuses[i].position.y,
                      bonuses[i].radius, WHITE);
    }
  }

  EndMode2D();

  // UI
  DrawText(TextFormat("SCORE: %d", score), 20, 20, 20, DARKGRAY);
  DrawText(TextFormat("TIME: %.1fs", gameTime), 20, 50, 20, DARKGRAY);

  // Health Bar at bottom
  float hpPct = (float)player.health / (float)player.maxHealth;
  int barWidth = 300;
  int barHeight = 20;
  int barX = SCREEN_WIDTH / 2 - barWidth / 2;
  int barY = SCREEN_HEIGHT - 40;

  DrawRectangle(barX, barY, barWidth, barHeight, RED);
  DrawRectangle(barX, barY, (int)(barWidth * hpPct), barHeight, GREEN);
  DrawRectangleLines(barX, barY, barWidth, barHeight, BLACK);

  DrawText(TextFormat("%d/%d", player.health, player.maxHealth),
           barX + barWidth / 2 - 20, barY + 2, 20, WHITE);

  // Damage Indicator next to Health Bar
  DrawText(TextFormat("DMG: %.0f", player.damage), barX + barWidth + 20, barY,
           20, RED);

  if (gameOver) {
    DrawText("GAME OVER", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, 40,
             RED);
    DrawText("Press R to Restart", SCREEN_WIDTH / 2 - 100,
             SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
  }

  EndDrawing();
}

#include "include/survivor.h"

void RunSurvivorGame(Difficulte difficulty) {
  SetConfigFlags(0); // Reset flags to default
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Survivor Simple");
  ClearWindowState(FLAG_FULLSCREEN_MODE);  // Explicitly ensure no fullscreen
  ClearWindowState(FLAG_WINDOW_RESIZABLE); // Ensure not resizable
  ClearWindowState(FLAG_WINDOW_MAXIMIZED); // Ensure not maximized
  SetTargetFPS(60);

  InitGame(difficulty);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateGame(dt);
    DrawGame();
  }

  CloseWindow();
}
