#include "include/raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// --- VECTOR MATH HELPERS ---
Vector2 MyVector2Add(Vector2 v1, Vector2 v2) {
  return (Vector2){v1.x + v2.x, v1.y + v2.y};
}

Vector2 MyVector2Subtract(Vector2 v1, Vector2 v2) {
  return (Vector2){v1.x - v2.x, v1.y - v2.y};
}

Vector2 MyVector2Scale(Vector2 v, float scale) {
  return (Vector2){v.x * scale, v.y * scale};
}

float MyVector2Length(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

Vector2 MyVector2Normalize(Vector2 v) {
  float length = MyVector2Length(v);
  if (length > 0)
    return MyVector2Scale(v, 1.0f / length);
  return (Vector2){0, 0};
}

float MyVector2DistanceSqr(Vector2 v1, Vector2 v2) {
  float dx = v1.x - v2.x;
  float dy = v1.y - v2.y;
  return dx * dx + dy * dy;
}

// --- CONSTANTS ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAP_WIDTH 2000
#define MAP_HEIGHT 2000

#define MAX_ENEMIES 100
#define MAX_BULLETS 200
#define MAX_BONUSES 10

// --- STRUCTS ---
typedef struct {
  Vector2 position;
  float radius;
  float speed;
  int health;
  int maxHealth;
  bool isAlive;

  // Shooting
  float fireRate;
  float shootTimer;
  float damage;
  float bulletSpeed;
  float bulletSize;
} Player;

typedef enum { ENEMY_NORMAL, ENEMY_TANK, ENEMY_FAST } EnemyType;

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

typedef enum { BONUS_DAMAGE } BonusType;

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

// --- MATH HELPERS ---
float FloatClamp(float value, float min, float max) {
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

// --- GAME LOGIC ---

void InitGame() {
  // Player Init
  player.position = (Vector2){
      MAP_WIDTH / 2, MAP_HEIGHT / 2}; // le joueur est au centre de la map
  player.radius = 20.0f;
  player.speed = 200.0f;
  player.maxHealth = 5;
  player.health = player.maxHealth;
  player.isAlive = true;

  player.fireRate = 0.5f;
  player.shootTimer = 0.0f;
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
  bonuses[slot].type = BONUS_DAMAGE;
  bonuses[slot].color = GREEN;
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
  float difficulty = gameTime / 60.0f; // Increase every minute

  // Spawn Position (outside camera view)
  float angle = GetRandomValue(0, 360) * DEG2RAD;
  float dist = 800.0f; // Outside typical screen range
  Vector2 spawnPos = {player.position.x + cosf(angle) * dist,
                      player.position.y + sinf(angle) * dist};

  // Clamp to map
  spawnPos.x = FloatClamp(spawnPos.x, 0, MAP_WIDTH);
  spawnPos.y = FloatClamp(spawnPos.y, 0, MAP_HEIGHT);

  enemies[slot].position = spawnPos;
  enemies[slot].active = true;

  // Determine Type based on difficulty/random
  int roll = GetRandomValue(0, 100);

  if (difficulty > 2.0f && roll < 20) {
    enemies[slot].type = ENEMY_TANK;
    enemies[slot].radius = 30.0f;
    enemies[slot].speed = 100.0f;
    enemies[slot].maxHealth = 5 + (int)difficulty * 2;
    enemies[slot].color = DARKPURPLE;
  } else if (difficulty > 1.0f && roll > 80) {
    enemies[slot].type = ENEMY_FAST;
    enemies[slot].radius = 15.0f;
    enemies[slot].speed = 280.0f;
    enemies[slot].maxHealth = 1 + (int)difficulty;
    enemies[slot].color = ORANGE;
  } else {
    enemies[slot].type = ENEMY_NORMAL;
    enemies[slot].radius = 20.0f;
    enemies[slot].speed = 180.0f + (difficulty * 10.0f);
    enemies[slot].maxHealth = 1.5 + (int)difficulty;
    enemies[slot].color = RED;
  }

  enemies[slot].health = enemies[slot].maxHealth;
}

int FindNearestEnemy() {
  int nearest = -1;
  float minDistSq = 99999999.0f;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      float d = MyVector2DistanceSqr(player.position, enemies[i].position);
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
      InitGame();
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

  if (MyVector2Length(move) > 0) {
    move = MyVector2Normalize(move);
    player.position =
        MyVector2Add(player.position, MyVector2Scale(move, player.speed * dt));
  }

  // Clamp Player
  player.position.x = FloatClamp(player.position.x, 0, MAP_WIDTH);
  player.position.y = FloatClamp(player.position.y, 0, MAP_HEIGHT);

  // Camera Follow
  camera.target = player.position;

  // --- SHOOTING ---
  player.shootTimer += dt;
  if (player.shootTimer >= player.fireRate) {
    int targetIdx = FindNearestEnemy();
    if (targetIdx != -1) {
      float distSq =
          MyVector2DistanceSqr(player.position, enemies[targetIdx].position);
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
        Vector2 dir =
            MyVector2Subtract(enemies[targetIdx].position, player.position);
        dir = MyVector2Normalize(dir);

        bullets[bSlot].active = true;
        bullets[bSlot].position = player.position;
        bullets[bSlot].velocity = MyVector2Scale(dir, player.bulletSpeed);
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
      bullets[i].position = MyVector2Add(
          bullets[i].position, MyVector2Scale(bullets[i].velocity, dt));
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
        }
        bonuses[i].active = false;
      }
    }
  }

  // --- ENEMIES ---
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      Vector2 dir = MyVector2Subtract(player.position, enemies[i].position);
      dir = MyVector2Normalize(dir);
      enemies[i].position = MyVector2Add(
          enemies[i].position, MyVector2Scale(dir, enemies[i].speed * dt));

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

  // Draw Player
  DrawCircleV(player.position, player.radius, BLUE);
  // Health Bar above player
  float hpPct = (float)player.health / (float)player.maxHealth;
  DrawRectangle(player.position.x - 20, player.position.y - 35, 40, 5, RED);
  DrawRectangle(player.position.x - 20, player.position.y - 35, 40 * hpPct, 5,
                GREEN);

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
  DrawText(TextFormat("DAMAGE: %.0f", player.damage), 20, 80, 20, DARKGREEN);

  if (gameOver) {
    DrawText("GAME OVER", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, 40,
             RED);
    DrawText("Press R to Restart", SCREEN_WIDTH / 2 - 100,
             SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
  }

  EndDrawing();
}

#include "include/survivor.h"

void RunSurvivorGame(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Survivor Simple");
  SetTargetFPS(60);

  InitGame();

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateGame(dt);
    DrawGame();
  }

  CloseWindow();
}
