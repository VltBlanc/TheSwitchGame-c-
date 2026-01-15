// ============================================================================
// SURVIVOR GAME
// ============================================================================

#include "include/survivor.h"

#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/hyperspace.h"
#include "include/menu.h"
#include "include/player_input.h"
#include "include/splitscreen.h"

// ============================================================================
// CONSTANTES LOCALES
// ============================================================================

#define SCREEN_WIDTH SURVIVOR_SCREEN_WIDTH
#define SCREEN_HEIGHT SURVIVOR_SCREEN_HEIGHT
#define MAP_WIDTH SURVIVOR_MAP_WIDTH
#define MAP_HEIGHT SURVIVOR_MAP_HEIGHT

// Pour compatibilité avec l'ancien code
#define MAX_trainee MAX_TRAINEE
typedef SurvivorPlayer Player;
typedef SurvivorEnemy Enemy;
typedef SurvivorBullet Bullet;
typedef SurvivorBonus Bonus;
typedef SurvivorTrainee Trainee;

// ============================================================================
// VARIABLES GLOBALES - ENTITÉS
// ============================================================================

static Player player1;
static Player player2;
static Enemy *enemies = NULL;
static Bullet *bullets = NULL;
static Bonus *bonuses = NULL;
static Trainee *trainee = NULL;

// ============================================================================
// VARIABLES GLOBALES - SCORE & SESSION
// ============================================================================

static ScoreList *currentScores = NULL;
static char currentPlayerName[32] = "Unknown";
static bool scoreSaved = false;

// ============================================================================
// VARIABLES GLOBALES - TEXTURES
// ============================================================================

static Texture2D background;
static Texture2D spaceshipTexture;
static Texture2D chasseurTexture;
static Texture2D tankTexture;

// ============================================================================
// VARIABLES GLOBALES - CAMÉRA & ÉTAT DU JEU
// ============================================================================

static Camera2D camera1 = {0};
static Camera2D camera2 = {0};

static float gameTime = 0.0f;
static float *globalTotalTime = NULL; // Pointeur vers le temps global
static float spawnTimer = 0.0f;
static float bonusSpawnTimer = 0.0f;
static float timeToSpawn = 1.0f;
static int score = 0;
static bool gameOver = false;
static Difficulte currentDifficulty = MOYEN;
static ModeJoueur currentMode = UN_JOUEUR;
static int winner = 0; // 0 = none, 1 = P1 wins, 2 = P2 wins
static SplitScreenContext splitCtx = {0};

// ============================================================================
// FONCTIONS - INITIALISATION
// ============================================================================

// Helper to initialize a single player with given position
static void InitPlayer(Player *p, Vector2 position, Difficulte difficulty) {
  p->position = position;
  p->radius = 20.0f;
  p->speed = 450.0f;

  // Stats selon la difficulte
  if (difficulty == FACILE) {
    p->maxHealth = 10;
    p->damage = 2;
  } else if (difficulty == DIFFICILE) {
    p->maxHealth = 3;
    p->damage = 1;
  } else {
    p->maxHealth = 5;
    p->damage = 1;
  }

  p->health = p->maxHealth;
  p->isAlive = true;
  p->rotation = 0.0f;

  // Animation
  p->currentFrame = 0;
  p->frameTimer = 0.0f;
  p->frameRec = (Rectangle){0.0f, 0.0f, 331.0f, 399.0f};

  p->fireRate = 1.0f;
  p->shootTimer = 0.0f;
  p->bulletSpeed = 1200.0f;
  p->bulletSize = 10.0f;

  // Dash
  p->isDashing = false;
  p->dashTimer = 0.0f;
  p->dashCooldown = 0.1f;
  p->dashSpeed = 2000.0f;
  p->dashDuration = 0.1f;
}

void InitGame(Difficulte difficulty) {
  currentDifficulty = difficulty;

  // Initialize Player 1 (center-left in 2P, center in 1P)
  Vector2 camOffset = GetCameraOffset(currentMode);
  Vector2 p1Pos;
  if (currentMode == DEUX_JOUEURS) {
    p1Pos = (Vector2){MAP_WIDTH / 2 - 200, MAP_HEIGHT / 2};
    Vector2 p2Pos = {MAP_WIDTH / 2 + 200, MAP_HEIGHT / 2};
    InitPlayer(&player2, p2Pos, difficulty);
    camera2.target = player2.position;
    camera2.offset = camOffset;
    camera2.rotation = 0.0f;
    camera2.zoom = 1.0f;
  } else {
    p1Pos = (Vector2){MAP_WIDTH / 2, MAP_HEIGHT / 2};
  }
  InitPlayer(&player1, p1Pos, difficulty);

  // Initialize Camera 1
  camera1.target = player1.position;
  camera1.offset = camOffset;
  camera1.rotation = 0.0f;
  camera1.zoom = 1.0f;

  // Reset global variables
  gameTime = 0.0f;
  spawnTimer = 0.0f;
  timeToSpawn = 2.0f;
  score = 0;
  gameOver = false;
  scoreSaved = false;
  winner = 0;

  for (int i = 0; i < MAX_ENEMIES; i++)
    enemies[i].active = false;
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
  for (int i = 0; i < MAX_BONUSES; i++)
    bonuses[i].active = false;
  for (int i = 0; i < MAX_trainee; i++)
    trainee[i].active = false;
}

// ============================================================================
// FONCTIONS - SPAWN
// ============================================================================

void SpawnBonus() {
  int slot = -1;
  int i = 0;
  // on s'arrete quand on trouve un bonus inactif
  while (i < MAX_BONUSES && bonuses[i].active) {
    i++;
  }
  if (i < MAX_BONUSES && !bonuses[i].active) {
    slot = i;
  }
  // si aucun slot n'est disponible, on quitte la fonction (pas de bonus à
  // apparaitre)
  if (slot == -1)
    return;

  // Position aléatoire dans les limites de la map
  float x = (float)GetRandomValue(50, MAP_WIDTH - 50);
  float y = (float)GetRandomValue(50, MAP_HEIGHT - 50);

  bonuses[slot].active = true;
  bonuses[slot].position = (Vector2){x, y};
  bonuses[slot].radius = 15.0f;

  int typeRoll = GetRandomValue(0, 100);
  if (typeRoll < 20) {
    bonuses[slot].type = BONUS_DAMAGE;
    bonuses[slot].color = RED;
  } else if (typeRoll < 40) {
    bonuses[slot].type = BONUS_HEALTH;
    bonuses[slot].color = GREEN;
  } else if (typeRoll < 60) {
    bonuses[slot].type = BONUS_SPEED;
    bonuses[slot].color = SKYBLUE;
    bonuses[slot].type = BONUS_TIR_RAPIDE;
    bonuses[slot].color = YELLOW;
  }
}

void SpawnEnemy() {
  int slot = -1;
  int i = 0;
  // on s'arrete quand on trouve un ennemi inactif
  while (i < MAX_ENEMIES && enemies[i].active) {
    i++;
  }
  if (i < MAX_ENEMIES && !enemies[i].active) {
    slot = i;
  }
  // si aucun slot n'est disponible, on quitte la fonction (pas d'ennemi à
  // apparaitre)
  if (slot == -1)
    return;

  // Logique de spawn basée sur le temps
  // Le facteur de difficulté augmente rapidement
  // dificulté normale
  float difficultyFactor = gameTime / 10.0f; // Augmente toutes les 10f

  // Ajuster le facteur de difficulté selon la difficulté sélectionnée
  if (currentDifficulty == FACILE) {
    difficultyFactor = gameTime / 20.0f; // Augmentation plus lente
  } else if (currentDifficulty == DIFFICILE) {
    difficultyFactor = gameTime / 5.0f; // Augmentation plus rapide
  }

  // Position de spawn (hors du champ de la camera)
  // En mode 2 joueurs, spawn autour d'un joueur aleatoire
  Vector2 spawnCenter = player1.position;
  if (currentMode == DEUX_JOUEURS && player2.isAlive) {
    if (GetRandomValue(0, 1) == 1) {
      spawnCenter = player2.position;
    }
  }
  float angle = GetRandomValue(0, 360) * DEG2RAD;
  float dist = 800.0f; // Hors de la portee typique de l'ecran
  Vector2 spawnPos = {spawnCenter.x + cosf(angle) * dist,
                      spawnCenter.y + sinf(angle) * dist};

  // Restreindre à la map
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

  // Déterminer le type d'ennemi basé sur la difficulté/aléatoire
  int roll = GetRandomValue(0, 100);
  // génération des enemies en fonction du temps, et de la difficulté
  // difficultyFactor correspond au temps/ (n frames) ou n dépend de la
  // difficulté si difficultyFactor est plus grand que 2, on a un tank si
  // difficultyFactor est plus grand que 1, on a un fast si difficultyFactor est
  // plus petit que 1, on a un normal
  // roll est un random entre 0 et 100, pour ajouter de la variabilité si roll
  // est inférieur à 15, on a un tank si roll est supérieur à 70, on a un fast
  // sinon, on a un normal
  if (difficultyFactor > 2.0f && roll < 15) {
    enemies[slot].type = ENEMY_TANK;
    enemies[slot].radius = 35.0f;
    enemies[slot].speed = 250.0f;
    enemies[slot].maxHealth = 5 + (int)difficultyFactor; // Scaling réduit
    enemies[slot].color = DARKPURPLE;
  } else if (difficultyFactor > 1.0f && roll > 70) {
    enemies[slot].type = ENEMY_FAST;
    enemies[slot].radius = 12.0f;
    enemies[slot].speed = 450.0f; // Vitesse réduite (était 550)
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth = 1 + (int)(difficultyFactor * 0.2f);
    enemies[slot].color = ORANGE;
  } else {
    enemies[slot].type = ENEMY_NORMAL;
    enemies[slot].radius = 30.0f; // Rayon augmenté (était 20)
    enemies[slot].speed =
        280.0f + (difficultyFactor * 5.0f); // Vitesse réduite (était 380)
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth = 1 + (int)(difficultyFactor * 0.3f);
    enemies[slot].color = RED;
  }

  // Init Animation
  if (enemies[slot].type == ENEMY_TANK) {
    enemies[slot].frameRec = (Rectangle){0.0f, 0.0f, (float)tankTexture.width,
                                         (float)tankTexture.height / 2.0f};
  } else {
    enemies[slot].frameRec =
        (Rectangle){0.0f, 0.0f, (float)chasseurTexture.width,
                    (float)chasseurTexture.height / 3.0f};
  }
  enemies[slot].currentFrame = 0;
  enemies[slot].frameTimer = 0.0f;
  enemies[slot].rotation = 0.0f;

  enemies[slot].health = enemies[slot].maxHealth;
}

// ============================================================================
// FONCTIONS - UTILITAIRES
// ============================================================================

// Trouve l'ennemi le plus proche d'une position donnee
int FindNearestEnemyToPos(Vector2 pos) {
  int nearest = -1;
  float minDistSq = 99999999.0f;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      float dx = pos.x - enemies[i].position.x;
      float dy = pos.y - enemies[i].position.y;
      float d = dx * dx + dy * dy;
      if (d < minDistSq) {
        minDistSq = d;
        nearest = i;
      }
    }
  }
  return nearest;
}

// Trouve l'ennemi le plus proche (utilise par les missiles pour le homing)
int FindNearestEnemy() {
  // En mode 2 joueurs, trouve l'ennemi le plus proche des deux joueurs
  if (currentMode == DEUX_JOUEURS) {
    int nearest1 = FindNearestEnemyToPos(player1.position);
    int nearest2 =
        player2.isAlive ? FindNearestEnemyToPos(player2.position) : -1;

    if (nearest1 == -1)
      return nearest2;
    if (nearest2 == -1)
      return nearest1;

    // Retourne celui qui est globalement le plus proche
    float d1 = Vector2DistanceSqr(player1.position, enemies[nearest1].position);
    float d2 = Vector2DistanceSqr(player2.position, enemies[nearest2].position);
    return (d1 < d2) ? nearest1 : nearest2;
  }
  return FindNearestEnemyToPos(player1.position);
}

// ============================================================================
// FONCTIONS - LOGIQUE DE JEU
// ============================================================================

// Mise a jour du mouvement d'un joueur
static void UpdatePlayerMovement(Player *p, Vector2 move, bool dashPressed,
                                 float dt) {
  if (!p->isAlive)
    return;

  float moveLen = sqrtf(move.x * move.x + move.y * move.y);
  if (moveLen > 0) {
    move.x /= moveLen;
    move.y /= moveLen;
    p->rotation = atan2f(move.y, move.x) * RAD2DEG + 90.0f;
  }

  // Logique du Dash
  if (p->dashCooldown > 0)
    p->dashCooldown -= dt;

  if (dashPressed && p->dashCooldown <= 0 && moveLen > 0) {
    p->isDashing = true;
    p->dashTimer = p->dashDuration;
    p->dashCooldown = 1.0f;
  }

  float currentSpeed = p->speed;
  if (p->isDashing) {
    currentSpeed = p->dashSpeed;
    p->dashTimer -= dt;

    // Effet de trainee
    static float traineeSpawnTimer1 = 0.0f;
    static float traineeSpawnTimer2 = 0.0f;
    float *traineeTimer =
        (p == &player1) ? &traineeSpawnTimer1 : &traineeSpawnTimer2;
    *traineeTimer += dt;
    if (*traineeTimer >= 0.02f) {
      *traineeTimer = 0.0f;
      for (int i = 0; i < MAX_trainee; i++) {
        if (!trainee[i].active) {
          trainee[i].active = true;
          trainee[i].position = p->position;
          trainee[i].rotation = p->rotation;
          trainee[i].frameRec = p->frameRec;
          trainee[i].alpha = 0.8f;
          break;
        }
      }
    }

    if (p->dashTimer <= 0)
      p->isDashing = false;
  }

  if (moveLen > 0 || p->isDashing) {
    p->position.x += move.x * currentSpeed * dt;
    p->position.y += move.y * currentSpeed * dt;

    // Animation
    p->frameTimer += dt;
    if (p->frameTimer >= 0.1f) {
      p->frameTimer = 0.0f;
      p->currentFrame++;
      if (p->currentFrame >= 3)
        p->currentFrame = 0;
      p->frameRec.y = (float)p->currentFrame * 399.0f;
    }
  } else {
    p->currentFrame = 1;
    p->frameRec.y = 399.0f;
  }

  // Limites de la map
  p->position.x = Clamp(p->position.x, 0, MAP_WIDTH);
  p->position.y = Clamp(p->position.y, 0, MAP_HEIGHT);
}

// Mise a jour du tir automatique d'un joueur
static void UpdatePlayerShooting(Player *p, float dt) {
  if (!p->isAlive)
    return;

  p->shootTimer += dt;
  if (p->shootTimer >= p->fireRate) {
    int targetIdx = FindNearestEnemyToPos(p->position);
    if (targetIdx != -1) {
      float dx = p->position.x - enemies[targetIdx].position.x;
      float dy = p->position.y - enemies[targetIdx].position.y;
      float distSq = dx * dx + dy * dy;
      if (distSq > 675.0f * 675.0f)
        targetIdx = -1;
    }

    if (targetIdx != -1) {
      int bSlot = -1;
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
          bSlot = i;
          break;
        }
      }

      if (bSlot != -1) {
        Vector2 dir = {enemies[targetIdx].position.x - p->position.x,
                       enemies[targetIdx].position.y - p->position.y};
        float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (dirLen > 0) {
          dir.x /= dirLen;
          dir.y /= dirLen;
        }

        bullets[bSlot].active = true;
        bullets[bSlot].position = p->position;
        bullets[bSlot].velocity =
            (Vector2){dir.x * p->bulletSpeed, dir.y * p->bulletSpeed};
        bullets[bSlot].radius = p->bulletSize;
        bullets[bSlot].damage = p->damage;
        bullets[bSlot].lifeTime = 2.0f;
        p->shootTimer = 0.0f;
      }
    }
  }
}

// Mise a jour de la camera pour un joueur
static void UpdateCameraForPlayer(Camera2D *cam, Player *p) {
  cam->target = p->position;

  // Calcul des limites selon le mode (split = 640x720, full = 1280x720)
  float viewW = (currentMode == DEUX_JOUEURS) ? SPLIT_WIDTH : SCREEN_WIDTH;
  float viewH = (currentMode == DEUX_JOUEURS) ? SPLIT_HEIGHT : SCREEN_HEIGHT;

  float minX = viewW / 2.0f;
  float minY = viewH / 2.0f;
  float maxX = MAP_WIDTH - viewW / 2.0f;
  float maxY = MAP_HEIGHT - viewH / 2.0f;

  if (cam->target.x < minX)
    cam->target.x = minX;
  if (cam->target.x > maxX)
    cam->target.x = maxX;
  if (cam->target.y < minY)
    cam->target.y = minY;
  if (cam->target.y > maxY)
    cam->target.y = maxY;
}

// Application d'un bonus a un joueur
static void ApplyBonusToPlayer(Player *p, Bonus *b) {
  if (b->type == BONUS_DAMAGE) {
    p->damage += 1;
  } else if (b->type == BONUS_HEALTH) {
    p->maxHealth += 1;
    p->health = p->maxHealth;
  } else if (b->type == BONUS_SPEED) {
    p->speed += 50.0f;
    if (p->speed > 800.0f)
      p->speed = 800.0f;
  } else if (b->type == BONUS_TIR_RAPIDE) {
    p->fireRate = p->fireRate / 1.15f;
  }
  b->active = false;
}

void UpdateGame(float dt) {
  if (gameOver)
    return;

  gameTime += dt;
  if (globalTotalTime != NULL)
    *globalTotalTime += dt;

  // --- MOUVEMENT JOUEUR 1 (ZQSD + SPACE) ---
  Vector2 move1 = {0};
  if (IS_P1_UP())
    move1.y -= 1;
  if (IS_P1_DOWN())
    move1.y += 1;
  if (IS_P1_LEFT())
    move1.x -= 1;
  if (IS_P1_RIGHT())
    move1.x += 1;
  UpdatePlayerMovement(&player1, move1, IS_P1_ACTION(), dt);

  // --- MOUVEMENT JOUEUR 2 (Fleches + CTRL) ---
  if (currentMode == DEUX_JOUEURS && player2.isAlive) {
    Vector2 move2 = {0};
    if (IS_P2_UP())
      move2.y -= 1;
    if (IS_P2_DOWN())
      move2.y += 1;
    if (IS_P2_LEFT())
      move2.x -= 1;
    if (IS_P2_RIGHT())
      move2.x += 1;
    UpdatePlayerMovement(&player2, move2, IS_P2_ACTION(), dt);
  }

  // --- UPDATE TRAINEES ---
  for (int i = 0; i < MAX_trainee; i++) {
    if (trainee[i].active) {
      trainee[i].alpha -= 2.0f * dt;
      if (trainee[i].alpha <= 0)
        trainee[i].active = false;
    }
  }

  // --- CAMERA ZOOM ---
  float targetZoom1 = player1.isDashing ? 0.9f : 1.0f;
  camera1.zoom += (targetZoom1 - camera1.zoom) * 5.0f * dt;
  UpdateCameraForPlayer(&camera1, &player1);

  if (currentMode == DEUX_JOUEURS && player2.isAlive) {
    float targetZoom2 = player2.isDashing ? 0.9f : 1.0f;
    camera2.zoom += (targetZoom2 - camera2.zoom) * 5.0f * dt;
    UpdateCameraForPlayer(&camera2, &player2);
  }

  // --- TIR AUTOMATIQUE ---
  UpdatePlayerShooting(&player1, dt);
  if (currentMode == DEUX_JOUEURS && player2.isAlive) {
    UpdatePlayerShooting(&player2, dt);
  }

  // --- UPDATE BALLES ---
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].position.x += bullets[i].velocity.x * dt;
      bullets[i].position.y += bullets[i].velocity.y * dt;
      bullets[i].lifeTime -= dt;

      if (bullets[i].lifeTime <= 0) {
        bullets[i].active = false;
        continue;
      }

      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (enemies[e].active &&
            CheckCollisionCircles(bullets[i].position, bullets[i].radius,
                                  enemies[e].position, enemies[e].radius)) {
          enemies[e].health -= bullets[i].damage;
          bullets[i].active = false;
          if (enemies[e].health <= 0) {
            enemies[e].active = false;
            score += 10;
          }
          break;
        }
      }
    }
  }

  // --- SPAWN ENNEMIS ---
  float difficultyTime = gameTime;
  float currentSpawnRate = 0.8f - (difficultyTime / 10.0f) * 0.05f;
  if (currentSpawnRate < 0.1f)
    currentSpawnRate = 0.1f;

  spawnTimer += dt;
  if (spawnTimer >= currentSpawnRate) {
    SpawnEnemy();
    spawnTimer = 0.0f;
  }

  // --- SPAWN BONUSES ---
  bonusSpawnTimer += dt;
  if (bonusSpawnTimer >= 5.0f) {
    SpawnBonus();
    bonusSpawnTimer = 0.0f;
  }

  // --- COLLECTE BONUSES ---
  for (int i = 0; i < MAX_BONUSES; i++) {
    if (bonuses[i].active) {
      // P1 collecte
      if (player1.isAlive &&
          CheckCollisionCircles(player1.position, player1.radius,
                                bonuses[i].position, bonuses[i].radius)) {
        ApplyBonusToPlayer(&player1, &bonuses[i]);
        continue;
      }
      // P2 collecte
      if (currentMode == DEUX_JOUEURS && player2.isAlive &&
          CheckCollisionCircles(player2.position, player2.radius,
                                bonuses[i].position, bonuses[i].radius)) {
        ApplyBonusToPlayer(&player2, &bonuses[i]);
      }
    }
  }

  // --- UPDATE ENNEMIS ---
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      // En 2P, l'ennemi chasse le joueur le plus proche
      Vector2 targetPos = player1.position;
      if (currentMode == DEUX_JOUEURS) {
        float d1 = player1.isAlive ? Vector2DistanceSqr(enemies[i].position,
                                                        player1.position)
                                   : 999999999.0f;
        float d2 = player2.isAlive ? Vector2DistanceSqr(enemies[i].position,
                                                        player2.position)
                                   : 999999999.0f;
        targetPos = (d1 <= d2) ? player1.position : player2.position;
      }

      Vector2 dir = {targetPos.x - enemies[i].position.x,
                     targetPos.y - enemies[i].position.y};
      float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
      if (dirLen > 0.001f) {
        dir.x /= dirLen;
        dir.y /= dirLen;
      }

      enemies[i].position.x += dir.x * enemies[i].speed * dt;
      enemies[i].position.y += dir.y * enemies[i].speed * dt;

      // Rotation
      if (enemies[i].type == ENEMY_TANK) {
        enemies[i].rotation = 0.0f;
        if (dir.x >= 0) {
          if (enemies[i].frameRec.width < 0)
            enemies[i].frameRec.width *= -1;
        } else {
          if (enemies[i].frameRec.width > 0)
            enemies[i].frameRec.width *= -1;
        }
      } else {
        enemies[i].rotation = atan2f(dir.y, dir.x) * RAD2DEG + 90.0f;
      }

      // Animation
      enemies[i].frameTimer += dt;
      if (enemies[i].frameTimer >= 0.1f) {
        enemies[i].frameTimer = 0.0f;
        enemies[i].currentFrame++;
        int maxFrames = (enemies[i].type == ENEMY_TANK) ? 2 : 3;
        if (enemies[i].currentFrame >= maxFrames)
          enemies[i].currentFrame = 0;
        enemies[i].frameRec.y =
            (float)enemies[i].currentFrame * enemies[i].frameRec.height;
      }

      // Collision avec P1
      if (player1.isAlive &&
          CheckCollisionCircles(player1.position, player1.radius,
                                enemies[i].position, enemies[i].radius)) {
        player1.health--;
        enemies[i].active = false;
        if (player1.health <= 0) {
          player1.isAlive = false;
        }
      }

      // Collision avec P2
      if (currentMode == DEUX_JOUEURS && player2.isAlive && enemies[i].active &&
          CheckCollisionCircles(player2.position, player2.radius,
                                enemies[i].position, enemies[i].radius)) {
        player2.health--;
        enemies[i].active = false;
        if (player2.health <= 0) {
          player2.isAlive = false;
        }
      }
    }
  }

  // --- CONDITION DE VICTOIRE (2P) ---
  if (currentMode == DEUX_JOUEURS) {
    if (!player1.isAlive && player2.isAlive) {
      winner = 2;
      gameOver = true;
    } else if (!player2.isAlive && player1.isAlive) {
      winner = 1;
      gameOver = true;
    } else if (!player1.isAlive && !player2.isAlive) {
      winner = -1; // Egalite
      gameOver = true;
    }
  } else {
    // Mode 1P classique
    if (!player1.isAlive) {
      gameOver = true;
    }
  }
}

// ============================================================================
// FONCTIONS - RENDU
// ============================================================================

// Dessine le monde (background, entites, effets) - appele pour chaque viewport
static void DrawWorld(Camera2D *cam, Player *viewPlayer) {
  BeginMode2D(*cam);

  // Fond
  DrawTexture(background, 0, 0, WHITE);

  // Trainees (effet de dash)
  for (int i = 0; i < MAX_trainee; i++) {
    if (trainee[i].active) {
      float scale = 0.15f;
      float w = 331.0f * scale;
      float h = 399.0f * scale;
      Rectangle destRec = {trainee[i].position.x, trainee[i].position.y, w, h};
      Vector2 origin = {w / 2.0f, h / 2.0f};
      Color col = WHITE;
      col.a = (unsigned char)(trainee[i].alpha * 255);
      DrawTexturePro(spaceshipTexture, trainee[i].frameRec, destRec, origin,
                     trainee[i].rotation, col);
    }
  }

  // Dessiner P1 si vivant
  if (player1.isAlive) {
    float scale = 0.15f;
    float w = 331.0f * scale;
    float h = 399.0f * scale;
    Rectangle destRec = {player1.position.x, player1.position.y, w, h};
    Vector2 origin = {w / 2.0f, h / 2.0f};
    DrawTexturePro(spaceshipTexture, player1.frameRec, destRec, origin,
                   player1.rotation, WHITE);
  }

  // Dessiner P2 si vivant (mode 2P)
  if (currentMode == DEUX_JOUEURS && player2.isAlive) {
    float scale = 0.15f;
    float w = 331.0f * scale;
    float h = 399.0f * scale;
    Rectangle destRec = {player2.position.x, player2.position.y, w, h};
    Vector2 origin = {w / 2.0f, h / 2.0f};
    // P2 avec une teinte differente pour le distinguer
    DrawTexturePro(spaceshipTexture, player2.frameRec, destRec, origin,
                   player2.rotation, (Color){180, 180, 255, 255});
  }

  // Ennemis
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      Texture2D tex =
          (enemies[i].type == ENEMY_TANK) ? tankTexture : chasseurTexture;
      float baseScale = (enemies[i].type == ENEMY_TANK) ? 0.4f : 0.6f;
      float w = fabsf(enemies[i].frameRec.width) * baseScale;
      float h = enemies[i].frameRec.height * baseScale;
      Rectangle dest = {enemies[i].position.x, enemies[i].position.y, w, h};
      Vector2 origin = {w / 2.0f, h / 2.0f};
      DrawTexturePro(tex, enemies[i].frameRec, dest, origin,
                     enemies[i].rotation, WHITE);
    }
  }

  // Balles
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      DrawCircleV(bullets[i].position, bullets[i].radius, GREEN);
    }
  }

  // Bonus
  for (int i = 0; i < MAX_BONUSES; i++) {
    if (bonuses[i].active) {
      DrawCircleV(bonuses[i].position, bonuses[i].radius, bonuses[i].color);
      DrawCircleLines(bonuses[i].position.x, bonuses[i].position.y,
                      bonuses[i].radius, WHITE);

      // Indicateur hors ecran (fleche vers bonus)
      Vector2 screenPos = GetWorldToScreen2D(bonuses[i].position, *cam);
      int viewW = (currentMode == DEUX_JOUEURS) ? SPLIT_WIDTH : SCREEN_WIDTH;
      int viewH = (currentMode == DEUX_JOUEURS) ? SPLIT_HEIGHT : SCREEN_HEIGHT;

      if (screenPos.x < 0 || screenPos.x > viewW || screenPos.y < 0 ||
          screenPos.y > viewH) {
        Vector2 dir =
            Vector2Subtract(bonuses[i].position, viewPlayer->position);
        float length = Vector2Length(dir);
        if (length > 0)
          dir = Vector2Scale(dir, 1.0f / length);

        float indicatorRadius = 60.0f;
        float triangleSize = 15.0f;

        Vector2 baseCenter = Vector2Add(viewPlayer->position,
                                        Vector2Scale(dir, indicatorRadius));
        Vector2 tip =
            Vector2Add(viewPlayer->position,
                       Vector2Scale(dir, indicatorRadius + triangleSize));
        Vector2 perp = {-dir.y, dir.x};
        Vector2 baseLeft =
            Vector2Add(baseCenter, Vector2Scale(perp, -triangleSize * 0.5f));
        Vector2 baseRight =
            Vector2Add(baseCenter, Vector2Scale(perp, triangleSize * 0.5f));

        DrawTriangle(tip, baseLeft, baseRight, bonuses[i].color);
      }
    }
  }

  EndMode2D();
}

// Dessine l'UI pour un joueur specifique
static void DrawPlayerUI(Player *p, int playerId, int screenW, int screenH) {
  // Score et temps (en haut au centre)
  const char *scoreText = TextFormat("SCORE: %d", score);
  float displayTime = (globalTotalTime != NULL) ? *globalTotalTime : gameTime;
  const char *timeText = TextFormat("TIME: %.1fs", displayTime);

  int fontSize = (currentMode == DEUX_JOUEURS) ? 24 : 40;
  int scoreW = MeasureText(scoreText, fontSize);
  int timeW = MeasureText(timeText, fontSize);

  DrawText(scoreText, screenW / 2 - scoreW / 2 + 1, 12, fontSize, DARKBLUE);
  DrawText(scoreText, screenW / 2 - scoreW / 2, 10, fontSize, SKYBLUE);
  DrawText(timeText, screenW / 2 - timeW / 2 + 1, 10 + fontSize + 2, fontSize,
           DARKBLUE);
  DrawText(timeText, screenW / 2 - timeW / 2, 10 + fontSize, fontSize, SKYBLUE);

  // Nom du joueur
  const char *label = (playerId == 1) ? "P1" : "P2";
  DrawText(label, 10, 10, 30, YELLOW);

  // Panel en bas
  int panelHeight = (currentMode == DEUX_JOUEURS) ? 60 : 90;
  int panelY = screenH - panelHeight;

  DrawRectangle(0, panelY, screenW, panelHeight, Fade(DARKGRAY, 0.9f));
  DrawLine(0, panelY, screenW, panelY, LIGHTGRAY);

  // Barre de vie
  float hpPct = (p->maxHealth > 0) ? (float)p->health / (float)p->maxHealth : 0;
  int barW = screenW / 3;
  int barH = 15;
  int barX = 10;
  int barY = panelY + 10;

  DrawText("HP", barX, panelY + 30, 16, LIGHTGRAY);
  DrawRectangle(barX + 25, barY, barW, barH, Fade(RED, 0.5f));
  DrawRectangle(barX + 25, barY, (int)(barW * hpPct), barH, GREEN);
  DrawRectangleLines(barX + 25, barY, barW, barH, WHITE);
  DrawText(TextFormat("%d/%d", p->health, p->maxHealth), barX + 30, barY + 1,
           14, WHITE);

  // Stats simples
  int statsX = barX + barW + 40;
  DrawText(TextFormat("DMG:%d", (int)p->damage), statsX, panelY + 10, 14, RED);
  DrawText(TextFormat("SPD:%.0f", p->speed), statsX, panelY + 28, 14, SKYBLUE);

  // Controles (seulement en 1P ou texte adapte)
  if (currentMode == UN_JOUEUR) {
    DrawText("ZQSD: Move  SPACE: Dash", 10, panelY - 20, 16, DARKGRAY);
  }
}

void DrawGame() {
  if (currentMode == DEUX_JOUEURS) {
    // --- MODE 2 JOUEURS (SPLIT-SCREEN) ---

    // Viewport P1 (gauche)
    BeginPlayerViewport(&splitCtx, 1);
    ClearBackground(RAYWHITE);
    DrawWorld(&camera1, &player1);
    DrawPlayerUI(&player1, 1, SPLIT_WIDTH, SPLIT_HEIGHT);
    EndPlayerViewport(&splitCtx);

    // Viewport P2 (droite)
    BeginPlayerViewport(&splitCtx, 2);
    ClearBackground(RAYWHITE);
    DrawWorld(&camera2, &player2);
    DrawPlayerUI(&player2, 2, SPLIT_WIDTH, SPLIT_HEIGHT);
    EndPlayerViewport(&splitCtx);

    // Composite les deux viewports
    DrawSplitScreen(&splitCtx);

    // Affichage du gagnant par dessus
    if (gameOver && winner != 0) {
      BeginDrawing();
      // Overlay sur P1
      const char *p1Text = (winner == 1) ? "GAGNANT!" : "PERDU";
      Color p1Col = (winner == 1) ? GREEN : RED;
      DrawRectangle(0, 0, SPLIT_WIDTH, SPLIT_HEIGHT, Fade(BLACK, 0.5f));
      int tw1 = MeasureText(p1Text, 50);
      DrawText(p1Text, SPLIT_WIDTH / 2 - tw1 / 2, SPLIT_HEIGHT / 2 - 25, 50,
               p1Col);

      // Overlay sur P2
      const char *p2Text = (winner == 2) ? "GAGNANT!" : "PERDU";
      Color p2Col = (winner == 2) ? GREEN : RED;
      DrawRectangle(SPLIT_WIDTH, 0, SPLIT_WIDTH, SPLIT_HEIGHT,
                    Fade(BLACK, 0.5f));
      int tw2 = MeasureText(p2Text, 50);
      DrawText(p2Text, SPLIT_WIDTH + SPLIT_WIDTH / 2 - tw2 / 2,
               SPLIT_HEIGHT / 2 - 25, 50, p2Col);

      DrawText("ESCAPE: Menu", SCREEN_WIDTH / 2 - 80, SPLIT_HEIGHT / 2 + 50, 20,
               WHITE);
      EndDrawing();
    } else if (gameOver && winner == -1) {
      // Egalite
      BeginDrawing();
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
      const char *txt = "EGALITE!";
      int tw = MeasureText(txt, 50);
      DrawText(txt, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT / 2 - 25, 50,
               YELLOW);
      DrawText("ESCAPE: Menu", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 50,
               20, WHITE);
      EndDrawing();
    }

  } else {
    // --- MODE 1 JOUEUR (classique) ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawWorld(&camera1, &player1);
    DrawPlayerUI(&player1, 1, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (gameOver) {
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
      const char *txt = "GAME OVER";
      int tw = MeasureText(txt, 50);
      DrawText(txt, SCREEN_WIDTH / 2 - tw / 2, SCREEN_HEIGHT / 2 - 25, 50, RED);
      DrawText("ESCAPE: Menu", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 50,
               20, WHITE);
    }

    EndDrawing();
  }
}

// ============================================================================
// FONCTION PUBLIQUE - POINT D'ENTRÉE DU JEU
// ============================================================================

void RunSurvivorGame(Difficulte difficulty, ModeJoueur mode, bool resume,
                     bool *game_active, const char *playerName,
                     ScoreList *scores, float timeout_seconds,
                     float *total_time, float start_delay) {
  // Stocker le mode et le pointeur vers le temps global
  currentMode = mode;
  globalTotalTime = total_time;

  // On ajuste la fenetre a la taille standard
  SetWindowSize(LARGEUR_ECRAN, HAUTEUR_ECRAN);
  SetWindowTitle((mode == DEUX_JOUEURS) ? "Survivor - 2 Joueurs" : "Survivor");
  SetTargetFPS(60);

  // Initialiser le contexte split-screen
  splitCtx = InitSplitScreen(mode);

  // Allocation dynamique seulement si on ne reprend pas une partie
  if (!resume) {
    // Securite : on libere si jamais il restait quelque chose
    if (enemies)
      free(enemies);
    if (bullets)
      free(bullets);
    if (bonuses)
      free(bonuses);
    if (trainee)
      free(trainee);

    enemies = (Enemy *)calloc(MAX_ENEMIES, sizeof(Enemy));
    bullets = (Bullet *)calloc(MAX_BULLETS, sizeof(Bullet));
    bonuses = (Bonus *)calloc(MAX_BONUSES, sizeof(Bonus));
    trainee = (Trainee *)calloc(MAX_trainee, sizeof(Trainee));

    if (enemies == NULL || bullets == NULL || bonuses == NULL ||
        trainee == NULL) {
      printf("Erreur d'allocation memoire !\n");
      if (enemies)
        free(enemies);
      if (bullets)
        free(bullets);
      if (bonuses)
        free(bonuses);
      if (trainee)
        free(trainee);
      UnloadSplitScreen(&splitCtx);
      *game_active = false;
      return;
    }

    InitGame(difficulty);
  }

  // Stocker les pointeurs globaux pour l'accès dans UpdateGame
  currentScores = scores;
  // Reset name content first
  memset(currentPlayerName, 0, sizeof(currentPlayerName));
  if (playerName && strlen(playerName) > 0) {
    strncpy(currentPlayerName, playerName, 31);
    currentPlayerName[31] = '\0';
  } else {
    strcpy(currentPlayerName, "JOUEUR");
  }

  printf("RunSurvivorGame started for: '%s'\n", currentPlayerName);

  // Charger les textures AVANT l'effet hyperspace
  background = LoadTexture("assets/Space-Background.png");
  spaceshipTexture = LoadTexture("assets/spaceship.png");
  chasseurTexture = LoadTexture("assets/sprite-chasseur.png");
  tankTexture = LoadTexture("assets/sprite-tank.png");

  // Jouer l'effet d'hyperespace avant de commencer (seulement si nouvelle
  // partie et pas en mode rotation)
  if (!resume && timeout_seconds <= 0) {
    PlayHyperspaceEffect(1.0f, spaceshipTexture);
  }

  // Activer le jeu après l'effet hyperspace
  *game_active = true;
  float elapsed_time = 0.0f; // Timer pour le timeout
  printf("jeu survivor lancé\n");

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    elapsed_time += dt;

    // Gestion des transitions (Zoom/Shake)
    bool in_transition = false;
    float current_zoom = 1.0f;
    Vector2 shake_offset = {0.0f, 0.0f};

    // 1. Zoom In au début (pendant start_delay)
    if (elapsed_time < start_delay) {
      in_transition = true;
      float factor = elapsed_time / start_delay;
      current_zoom = Lerp(0.1f, 1.0f, factor); // Zoom progressif de 0.1 à 1.0
    }
    // 2. Shake + Zoom Out à la fin (0.5s avant fin)
    else if (!gameOver && timeout_seconds > 0 &&
             (timeout_seconds - elapsed_time) < 0.5f) {
      float remaining = timeout_seconds - elapsed_time;
      float factor = remaining / 0.5f;         // Va de 1.0 -> 0.0
      current_zoom = Lerp(0.1f, 1.0f, factor); // Zoom out vers 0.1

      // Shake effect
      float shake_amount =
          (1.0f - factor) * 10.0f; // Augmente quand le temps diminue
      shake_offset.x = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
      shake_offset.y = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
    }

    // Appliquer le zoom et shake aux cameras
    // Note: UpdateGame met a jour camera.target sur le joueur.
    // On applique le shake APRES UpdateGame mais AVANT DrawGame.

    // Vérifier timeout (si > 0)
    if (!gameOver && timeout_seconds > 0 && elapsed_time >= timeout_seconds) {
      break; // Sortir automatiquement après le timeout
    }

    // Si on appuie sur Echap ou n'importe quelle touche en game over, on sort
    if (IsKeyPressed(KEY_ESCAPE)) {
      *game_active = false; // Toujours signaler qu'on veut quitter
      // Attendre que la touche soit relâchée pour éviter de fermer le menu
      while (IsKeyDown(KEY_ESCAPE)) {
        BeginDrawing();
        EndDrawing();
      }
      break;
    }

    if (gameOver) {
      // Only Escape allows to quit, handled at the top of the loop
    }

    // Si le jeu est actif et PAS en transition d'intro (start_delay), on met à
    // jour la logique Pendant l'intro, on pause la logique pour voir l'état
    // sans bouger
    if (*game_active && elapsed_time >= start_delay) {
      UpdateGame(dt);
    }

    // Application des effets de caméra pour le rendu
    // UpdateGame a mis à jour camera.target sur le joueur.
    // On applique ici le shake et le zoom d'animation.

    // Appliquer le Zoom d'animation (prioritaire sur le zoom du dash)
    if (elapsed_time < start_delay ||
        (timeout_seconds > 0 && (timeout_seconds - elapsed_time) < 0.5f)) {
      camera1.zoom = current_zoom;
      if (currentMode == DEUX_JOUEURS)
        camera2.zoom = current_zoom;
    }

    // Appliquer le shake
    camera1.target.x += shake_offset.x;
    camera1.target.y += shake_offset.y;
    if (currentMode == DEUX_JOUEURS) {
      camera2.target.x += shake_offset.x;
      camera2.target.y += shake_offset.y;
    }

    // Dessiner le jeu
    DrawGame();

    // Restaurer target (le shake ne doit pas deriver)
    camera1.target.x -= shake_offset.x;
    camera1.target.y -= shake_offset.y;
    if (currentMode == DEUX_JOUEURS) {
      camera2.target.x -= shake_offset.x;
      camera2.target.y -= shake_offset.y;
    }
  }

  // Si la fenêtre est fermée par la croix, on ne sauvegarde pas forcément
  if (WindowShouldClose()) {
    *game_active = false;
  }

  // Si Game Over, la partie est finie
  if (gameOver) {
    *game_active = false;
  }

  UnloadTexture(background);
  UnloadTexture(spaceshipTexture);
  UnloadTexture(chasseurTexture);
  UnloadTexture(tankTexture);
  UnloadSplitScreen(&splitCtx);

  // Liberation de la memoire seulement si la partie est finie
  if (!*game_active) {
    if (enemies)
      free(enemies);
    if (bullets)
      free(bullets);
    if (bonuses)
      free(bonuses);
    if (trainee)
      free(trainee);
    enemies = NULL;
    bullets = NULL;
    bonuses = NULL;
    trainee = NULL;
  }
}