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
typedef SurvivorMissile Missile;

// ============================================================================
// VARIABLES GLOBALES - ENTITÉS
// ============================================================================

static Player player;
static Enemy *enemies = NULL;
static Bullet *bullets = NULL;
static Bonus *bonuses = NULL;
static Trainee *trainee = NULL;
static Missile *missiles = NULL;

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

static Camera2D camera = {0};

static float gameTime = 0.0f;
static float spawnTimer = 0.0f;
static float bonusSpawnTimer = 0.0f;
static float timeToSpawn = 1.0f;
static int score = 0;
static bool gameOver = false;
static Difficulte currentDifficulty = MOYEN;

// ============================================================================
// FONCTIONS - INITIALISATION
// ============================================================================

void InitGame(Difficulte difficulty) {
  currentDifficulty = difficulty;

  // Initialisation du joueur
  player.position = (Vector2){
      MAP_WIDTH / 2, MAP_HEIGHT / 2}; // le joueur est au centre de la map
  player.radius = 20.0f;
  player.speed = 450.0f;

  // Définir les stats selon la difficulté
  if (difficulty == FACILE) {
    player.maxHealth = 10;
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
  player.rotation = 0.0f;

  // Animation
  player.currentFrame = 0;
  player.frameTimer = 0.0f;

  // Le sprite fait 331x1197, 3 frames verticales -> 331x399 par frame
  player.frameRec = (Rectangle){0.0f, 0.0f, 331.0f, 399.0f};

  player.fireRate = 1.0f; // Cadence de tir rapide
  player.shootTimer = 0.0f;

  player.bulletSpeed = 1200.0f;
  player.bulletSize = 10.0f;

  // Initialisation du Dash
  player.isDashing = false;
  player.dashTimer = 0.0f;
  player.dashCooldown = 0.1f;
  player.dashSpeed = 2000.0f; // Dash rapide
  player.dashDuration = 0.1f; // Courte durée

  // Initialisation du missile
  player.missileMaxCooldown = 15.0f;
  player.missileCooldownTimer = 0.0f;

  // Initialisation de la caméra
  camera.target = player.position;
  camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // Réinitialisation des variables globales
  gameTime = 0.0f;
  spawnTimer = 0.0f;
  timeToSpawn = 2.0f;
  spawnTimer = 0.0f;
  timeToSpawn = 2.0f;
  score = 0;
  gameOver = false;
  scoreSaved = false;

  for (int i = 0; i < MAX_ENEMIES; i++)
    enemies[i].active = false;
  for (int i = 0; i < MAX_BULLETS; i++)
    bullets[i].active = false;
  for (int i = 0; i < MAX_BONUSES; i++)
    bonuses[i].active = false;
  for (int i = 0; i < MAX_trainee; i++)
    trainee[i].active = false;
  for (int i = 0; i < MAX_MISSILES; i++)
    missiles[i].active = false;
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
  } else if (typeRoll < 80) { //
    bonuses[slot].type = BONUS_TIR_RAPIDE;
    bonuses[slot].color = YELLOW;
  } else {
    bonuses[slot].type = BONUS_MISSILE_CD;
    bonuses[slot].color = PURPLE;
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

  // Position de spawn (hors du champ de la caméra)
  float angle = GetRandomValue(0, 360) * DEG2RAD;
  float dist = 800.0f; // Hors de la portée typique de l'écran
  Vector2 spawnPos = {player.position.x + cosf(angle) * dist,
                      player.position.y + sinf(angle) * dist};

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

int FindNearestEnemy() {
  int nearest = -1;
  float minDistSq = 99999999.0f;
  // on regarde chaque enemi actif
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

// ============================================================================
// FONCTIONS - LOGIQUE DE JEU
// ============================================================================

void UpdateGame(float dt) {
  if (gameOver) {
    if (!scoreSaved && currentScores != NULL) {
      add_score(currentScores, currentPlayerName, score);
      save_scores(currentScores);
      scoreSaved = true;
    }
    if (IsKeyPressed(KEY_R))
      InitGame(currentDifficulty);
    return;
  }

  gameTime += dt;

  // --- MOUVEMENT DU JOUEUR ---
  Vector2 move = {0};
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    move.y -= 1;
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    move.y += 1;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    move.x -= 1;
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    move.x += 1;
  // on calcule la longueur du vecteur move
  float moveLen = sqrtf(move.x * move.x + move.y * move.y);
  // on normalise le vecteur move
  if (moveLen > 0) {
    move.x /= moveLen;
    move.y /= moveLen;
    // on calcule la rotation du joueur
    player.rotation = atan2f(move.y, move.x) * RAD2DEG + 90.0f;
  }

  // Logique du Dash
  // on décrémente le cooldown du dash
  if (player.dashCooldown > 0)
    player.dashCooldown -= dt;

  // si on appuie sur espace et que le cooldown est terminé et que le joueur
  // bouge, on lance le dash
  if (IsKeyPressed(KEY_SPACE) && player.dashCooldown <= 0 && moveLen > 0) {
    player.isDashing = true;
    player.dashTimer = player.dashDuration;
    player.dashCooldown = 1.0f; // 1 seconde de cooldown
  }

  float currentSpeed = player.speed;
  // Etat de Dash
  if (player.isDashing) {
    currentSpeed = player.dashSpeed;
    player.dashTimer -= dt;
    // effet de trainée du dash
    static float traineeSpawnTimer = 0.0f;
    traineeSpawnTimer += dt;
    if (traineeSpawnTimer >= 0.02f) { // Spawn toutes les 20ms
      traineeSpawnTimer = 0.0f;
      for (int i = 0; i < MAX_trainee; i++) {
        if (!trainee[i].active) {
          trainee[i].active = true;
          trainee[i].position = player.position;
          trainee[i].rotation = player.rotation;
          trainee[i].frameRec = player.frameRec;
          trainee[i].alpha = 0.8f;
          break;
        }
      }
    }

    if (player.dashTimer <= 0) {
      player.isDashing = false;
    }
  }

  // Update trainee
  for (int i = 0; i < MAX_trainee; i++) {
    if (trainee[i].active) {
      trainee[i].alpha -= 2.0f * dt; // Fade out rapide
      if (trainee[i].alpha <= 0)
        trainee[i].active = false;
    }
  }

  // Camera Zoom Effect
  float targetZoom = player.isDashing ? 0.9f : 1.0f;
  camera.zoom += (targetZoom - camera.zoom) * 5.0f * dt;

  if (moveLen > 0 || player.isDashing) {
    // Le dash ne fonctionne que si on bouge
    player.position.x += move.x * currentSpeed * dt;
    player.position.y += move.y * currentSpeed * dt;

    // Animation du vaisseau quand on bouge
    player.frameTimer += dt;
    if (player.frameTimer >= 0.1f) {
      player.frameTimer = 0.0f;
      player.currentFrame++;
      if (player.currentFrame >= 3)
        player.currentFrame = 0;
      player.frameRec.y = (float)player.currentFrame * 399.0f;
    }
  } else {
    // Reset animation frame si on ne bouge pas
    player.currentFrame = 1; // Frame du milieu (souvent neutre)
    player.frameRec.y = 399.0f;
  }

  // Empêche le joueur de sortir de la map
  player.position.x = Clamp(player.position.x, 0, MAP_WIDTH);
  player.position.y = Clamp(player.position.y, 0, MAP_HEIGHT);

  // Suivi de la caméra
  camera.target = player.position;

  // Empêche la caméra de sortir de la map
  float minX = SCREEN_WIDTH / 2.0f;
  float minY = SCREEN_HEIGHT / 2.0f;
  float maxX = MAP_WIDTH - SCREEN_WIDTH / 2.0f;
  float maxY = MAP_HEIGHT - SCREEN_HEIGHT / 2.0f;

  if (camera.target.x < minX)
    camera.target.x = minX;
  if (camera.target.x > maxX)
    camera.target.x = maxX;
  if (camera.target.y < minY)
    camera.target.y = minY;
  if (camera.target.y > maxY)
    camera.target.y = maxY;

  // --- TIR ---
  player.shootTimer += dt;
  if (player.shootTimer >= player.fireRate) {
    int targetIdx = FindNearestEnemy();
    if (targetIdx != -1) {
      float dx = player.position.x - enemies[targetIdx].position.x;
      float dy = player.position.y - enemies[targetIdx].position.y;
      float distSq = dx * dx + dy * dy;
      // Si l'ennemi est trop éloigné, on ne tire pas
      if (distSq > 675.0f * 675.0f)
        targetIdx = -1;
    }

    if (targetIdx != -1) {
      // Trouve un slot pour la balle
      int bSlot = -1;
      for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
          bSlot = i;
          break;
        }
      }
      // test
      //  si on a une balle de disponible
      if (bSlot != -1) {
        Vector2 dir = {enemies[targetIdx].position.x - player.position.x,
                       enemies[targetIdx].position.y - player.position.y};
        float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        // Normalize direction
        if (dirLen > 0) {
          dir.x /= dirLen;
          dir.y /= dirLen;
        }

        bullets[bSlot].active = true;
        bullets[bSlot].position = player.position;
        bullets[bSlot].velocity =
            (Vector2){dir.x * player.bulletSpeed, dir.y * player.bulletSpeed};
        bullets[bSlot].radius = player.bulletSize;

        bullets[bSlot].damage = player.damage;
        bullets[bSlot].lifeTime = 2.0f; // Balle dure 2.0 secondes
        player.shootTimer = 0.0f;
      }
    }
  }

  // --- MISSILE ---
  if (player.missileCooldownTimer > 0) {
    player.missileCooldownTimer -= dt;
  }

  if (IsKeyPressed(KEY_E) && player.missileCooldownTimer <= 0) {
    // Find a slot
    int mSlot = -1;
    for (int i = 0; i < MAX_MISSILES; i++) {
      if (!missiles[i].active) {
        mSlot = i;
        break;
      }
    }

    if (mSlot != -1) {
      missiles[mSlot].active = true;
      missiles[mSlot].position = player.position;
      // Direction based on player rotation
      float rad = (player.rotation - 90.0f) * DEG2RAD;
      Vector2 dir = {cosf(rad), sinf(rad)};

      missiles[mSlot].velocity =
          (Vector2){dir.x * 600.0f, dir.y * 600.0f}; // Slower than bullets
      missiles[mSlot].radius = 15.0f;
      missiles[mSlot].damage = 10.0f; // High damage
      missiles[mSlot].explosionRadius = 150.0f;

      player.missileCooldownTimer = player.missileMaxCooldown;
    }
  }

  // Update Missiles
  for (int i = 0; i < MAX_MISSILES; i++) {
    if (missiles[i].active) {
      // Homing Logic
      int targetIdx = FindNearestEnemy();
      if (targetIdx != -1) {
        // Simple steering towards target
        Vector2 targetPos = enemies[targetIdx].position;
        Vector2 desiredDir = {targetPos.x - missiles[i].position.x,
                              targetPos.y - missiles[i].position.y};
        float dist =
            sqrtf(desiredDir.x * desiredDir.x + desiredDir.y * desiredDir.y);

        if (dist > 0) {
          desiredDir.x /= dist;
          desiredDir.y /= dist;

          float currentSpeed =
              sqrtf(missiles[i].velocity.x * missiles[i].velocity.x +
                    missiles[i].velocity.y * missiles[i].velocity.y);

          // Turn rate factor (adjust for sharper/slower turns)
          float turnRate = 4.0f * dt;

          // Lerp velocity direction
          missiles[i].velocity.x +=
              (desiredDir.x * currentSpeed - missiles[i].velocity.x) * turnRate;
          missiles[i].velocity.y +=
              (desiredDir.y * currentSpeed - missiles[i].velocity.y) * turnRate;

          // Maintain speed
          float newSpeed =
              sqrtf(missiles[i].velocity.x * missiles[i].velocity.x +
                    missiles[i].velocity.y * missiles[i].velocity.y);
          if (newSpeed > 0) {
            missiles[i].velocity.x =
                (missiles[i].velocity.x / newSpeed) * currentSpeed;
            missiles[i].velocity.y =
                (missiles[i].velocity.y / newSpeed) * currentSpeed;
          }
        }
      }
      missiles[i].position.x += missiles[i].velocity.x * dt;
      missiles[i].position.y += missiles[i].velocity.y * dt;

      bool exploded = false;

      // Check bounds
      if (missiles[i].position.x < 0 || missiles[i].position.x > MAP_WIDTH ||
          missiles[i].position.y < 0 || missiles[i].position.y > MAP_HEIGHT) {
        exploded = true; // Explode on wall hit
      }

      // Check collision with enemies (Direct hit)
      if (!exploded) {
        for (int e = 0; e < MAX_ENEMIES; e++) {
          if (enemies[e].active) {
            if (CheckCollisionCircles(missiles[i].position, missiles[i].radius,
                                      enemies[e].position, enemies[e].radius)) {
              exploded = true;
              break;
            }
          }
        }
      }

      if (exploded) {
        missiles[i].active = false;

        // AOE Damage
        for (int e = 0; e < MAX_ENEMIES; e++) {
          if (enemies[e].active) {
            float dx = missiles[i].position.x - enemies[e].position.x;
            float dy = missiles[i].position.y - enemies[e].position.y;
            float distSq = dx * dx + dy * dy;

            if (distSq <=
                missiles[i].explosionRadius * missiles[i].explosionRadius) {
              enemies[e].health -= (int)missiles[i].damage;
              if (enemies[e].health <= 0) {
                enemies[e].active = false;
                score += 15; // More points
              }
            }
          }
        }
      }
    }
  }

  // on fait partir la balle
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].position.x += bullets[i].velocity.x * dt;
      bullets[i].position.y += bullets[i].velocity.y * dt;
      bullets[i].lifeTime -= dt;

      if (bullets[i].lifeTime <= 0) {
        bullets[i].active = false;
        continue;
      }

      // Collision avec les ennemis
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (enemies[e].active) {
          if (CheckCollisionCircles(bullets[i].position, bullets[i].radius,
                                    enemies[e].position, enemies[e].radius)) {
            enemies[e].health -= bullets[i].damage;
            bullets[i].active = false; // Détruire la balle

            if (enemies[e].health <= 0) {
              enemies[e].active = false;
              score += 10;
            }
            break; // La balle touche un ennemi
          }
        }
      }
    }
  }

  // --- APPARITION ---
  // Le taux d'apparition augmente rapidement
  // Base 0.8s, diminue de 0.05s toutes les 10s, min 0.1s
  float currentSpawnRate = 0.8f - (gameTime / 10.0f) * 0.05f;
  if (currentSpawnRate < 0.1f)
    currentSpawnRate = 0.1f;

  spawnTimer += dt;
  if (spawnTimer >= currentSpawnRate) {
    SpawnEnemy();
    spawnTimer = 0.0f;
  }

  // --- BONUSES ---
  bonusSpawnTimer += dt;
  if (bonusSpawnTimer >= 5.0f) {
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
        } else if (bonuses[i].type == BONUS_SPEED) {
          player.speed += 50.0f;
          if (player.speed > 800.0f)
            player.speed = 800.0f;
        } else if (bonuses[i].type == BONUS_TIR_RAPIDE) {
          player.fireRate = player.fireRate / 1.15;
        } else if (bonuses[i].type == BONUS_MISSILE_CD) {
          player.missileMaxCooldown -= 1.0f; // Reduce max cooldown by 1s
          if (player.missileMaxCooldown < 5.0f)
            player.missileMaxCooldown = 5.0f; // Cap at 5s
        }
        bonuses[i].active = false;
      }
    }
  }

  // --- ENNEMIS ---
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      Vector2 dir = {player.position.x - enemies[i].position.x,
                     player.position.y - enemies[i].position.y};
      // on doit normaliser le vecteur direction sinon
      // l'ennemi se dirige vers le joueur
      //  à une vitesse proportionnelle à la distance
      float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
      // pour éviter une division par 0
      if (dirLen > 0.001f) {
        dir.x /= dirLen;
        dir.y /= dirLen;
      }
      enemies[i].position.x += dir.x * enemies[i].speed * dt;
      enemies[i].position.y += dir.y * enemies[i].speed * dt;

      // Rotation
      if (enemies[i].type == ENEMY_TANK) {
        // Tank: Orientation horizontale fixe (Gauche ou Droite)
        // On utilise le flip horizontal (largeur négative) au lieu de la
        // rotation
        enemies[i].rotation = 0.0f;

        if (dir.x >= 0) {
          // Vers la droite : largeur positive
          if (enemies[i].frameRec.width < 0)
            enemies[i].frameRec.width *= -1;
        } else {
          // Vers la gauche : largeur négative (flip)
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

      // Collision avec le joueur
      if (CheckCollisionCircles(player.position, player.radius,
                                enemies[i].position, enemies[i].radius)) {
        player.health--;
        enemies[i].active = false; // L'ennemi meurt à l'impact

        if (player.health <= 0) {
          player.isAlive = false;
          gameOver = true;
        }
      }
    }
  }
}

// ============================================================================
// FONCTIONS - RENDU
// ============================================================================

void DrawGame() {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  // Dessiner le fond
  DrawTexture(background, 0, 0, WHITE);

  // Dessiner les trainees (Ghost effect)
  for (int i = 0; i < MAX_trainee; i++) {
    if (trainee[i].active) {
      float scale = 0.15f;
      float width = 331.0f * scale;
      float height = 399.0f * scale;
      Rectangle destRec = {trainee[i].position.x, trainee[i].position.y, width,
                           height};
      Vector2 origin = {width / 2.0f, height / 2.0f};
      Color traineeColor = WHITE;
      traineeColor.a = (unsigned char)(trainee[i].alpha * 255);
      DrawTexturePro(spaceshipTexture, trainee[i].frameRec, destRec, origin,
                     trainee[i].rotation, traineeColor);
    }
  }

  // Draw Player (Spaceship)
  float scale = 0.15f;
  float width = 331.0f * scale;
  float height = 399.0f * scale;
  Rectangle destRec = {player.position.x, player.position.y, width, height};
  Vector2 origin = {width / 2.0f, height / 2.0f}; // Rotation autour du centre

  DrawTexturePro(spaceshipTexture, player.frameRec, destRec, origin,
                 player.rotation, WHITE);

  // Dessiner les ennemis
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      Texture2D tex =
          (enemies[i].type == ENEMY_TANK) ? tankTexture : chasseurTexture;
      float baseScale = (enemies[i].type == ENEMY_TANK)
                            ? 0.4f
                            : 0.6f; // Chasseurs plus gros (0.6 vs 0.4)
      // On utilise la valeur absolue pour la taille de destination car
      // frameRec.width peut être négatif pour le flip
      float width = fabsf(enemies[i].frameRec.width) * baseScale;
      float height = enemies[i].frameRec.height * baseScale;
      Rectangle dest = {enemies[i].position.x, enemies[i].position.y, width,
                        height};
      Vector2 origin = {width / 2.0f, height / 2.0f};
      DrawTexturePro(tex, enemies[i].frameRec, dest, origin,
                     enemies[i].rotation, WHITE);
    }
  }

  // Dessiner les balles
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      DrawCircleV(bullets[i].position, bullets[i].radius, GREEN);
    }
  }

  // Dessiner les missiles
  for (int i = 0; i < MAX_MISSILES; i++) {
    if (missiles[i].active) {
      DrawCircleV(missiles[i].position, missiles[i].radius, ORANGE);
      DrawCircleLines(missiles[i].position.x, missiles[i].position.y,
                      missiles[i].radius, RED);
    }
  }

  // Dessiner les bonus et indicateurs
  for (int i = 0; i < MAX_BONUSES; i++) {
    if (bonuses[i].active) {
      // Dessin du bonus
      DrawCircleV(bonuses[i].position, bonuses[i].radius, bonuses[i].color);
      DrawCircleLines(bonuses[i].position.x, bonuses[i].position.y,
                      bonuses[i].radius, WHITE);

      // Indicateur hors écran
      Vector2 screenPos = GetWorldToScreen2D(bonuses[i].position, camera);
      if (screenPos.x < 0 || screenPos.x > GetScreenWidth() ||
          screenPos.y < 0 || screenPos.y > GetScreenHeight()) {

        // Calcul en coordonnées MONDE (World Coordinates)
        Vector2 dir = Vector2Subtract(bonuses[i].position, player.position);

        // Normaliser la direction
        float length = Vector2Length(dir);
        if (length > 0) {
          dir = Vector2Scale(dir, 1.0f / length);
        }

        // Rayon du cercle autour du joueur
        float indicatorRadius = 60.0f;

        // Calcul des sommets du triangle isocèle
        float triangleSize = 15.0f;

        // Centre de la base du triangle (sur le cercle)
        Vector2 baseCenter =
            Vector2Add(player.position, Vector2Scale(dir, indicatorRadius));

        // Pointe du triangle (vers le bonus, vers l'extérieur du
        // cercle)
        Vector2 tip = Vector2Add(
            player.position, Vector2Scale(dir, indicatorRadius + triangleSize));

        // Vecteur perpendiculaire pour la base
        Vector2 perp = {-dir.y, dir.x};

        // Coins de la base
        Vector2 baseLeft =
            Vector2Add(baseCenter, Vector2Scale(perp, -triangleSize * 0.5f));
        Vector2 baseRight =
            Vector2Add(baseCenter, Vector2Scale(perp, triangleSize * 0.5f));

        // Dessiner le triangle
        DrawTriangle(tip, baseLeft, baseRight, bonuses[i].color);
      }
    }
  }

  EndMode2D();

  // UI
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // Header Stats (Score & Time) - Top Center, Large, Space Blue
  const char *scoreText = TextFormat("SCORE: %d", score);
  const char *timeText = TextFormat("TIME: %.1fs", gameTime);
  int scoreW = MeasureText(scoreText, 40);
  int timeW = MeasureText(timeText, 40);

  // Draw with a slight shadow for "glow" effect
  DrawText(scoreText, screenW / 2 - scoreW / 2 + 2, 22, 40, DARKBLUE);
  DrawText(scoreText, screenW / 2 - scoreW / 2, 20, 40, SKYBLUE);

  // Player Name Display (Top Left)
  DrawText(TextFormat("PILOT: %s", currentPlayerName), 10, 10, 20, WHITE);

  DrawText(timeText, screenW / 2 - timeW / 2 + 2, 62, 40, DARKBLUE);
  DrawText(timeText, screenW / 2 - timeW / 2, 60, 40, SKYBLUE);

  // --- COCKPIT UI ---
  int panelHeight = 90;
  int panelY = screenH - panelHeight;

  // Background Panel
  DrawRectangle(0, panelY, screenW, panelHeight, Fade(DARKGRAY, 0.9f));
  DrawLine(0, panelY, screenW, panelY, LIGHTGRAY); // Top border border

  int sectionWidth = screenW / 4;

  // Section 1: Health (Integrity)
  float hpPct = (float)player.health / (float)player.maxHealth;
  int barWidth = sectionWidth - 40;
  int barHeight = 20;
  int barX = 20;
  int barY = panelY + 45;

  DrawText("INTEGRITY", 20, panelY + 15, 20, LIGHTGRAY);
  DrawRectangle(barX, barY, barWidth, barHeight, Fade(RED, 0.5f));
  DrawRectangle(barX, barY, (int)(barWidth * hpPct), barHeight, GREEN);
  DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);
  DrawText(TextFormat("%d/%d", player.health, player.maxHealth), barX + 5,
           barY + 2, 18, WHITE);

  DrawLine(sectionWidth, panelY + 10, sectionWidth, screenH - 10,
           GRAY); // Separator

  // Section 2: Damage
  DrawText("WEAPON", sectionWidth + 20, panelY + 15, 20, LIGHTGRAY);
  DrawText(TextFormat("DMG: %.0f", player.damage), sectionWidth + 20,
           panelY + 45, 30, RED);

  DrawLine(sectionWidth * 2, panelY + 10, sectionWidth * 2, screenH - 10,
           GRAY); // Separator

  // Section 3: Speed
  DrawText("ENGINE", sectionWidth * 2 + 20, panelY + 15, 20, LIGHTGRAY);
  DrawText(TextFormat("SPD: %.0f", player.speed), sectionWidth * 2 + 20,
           panelY + 45, 30, SKYBLUE);

  DrawLine(sectionWidth * 3, panelY + 10, sectionWidth * 3, screenH - 10,
           GRAY); // Separator

  // Section 4: Fire Rate
  DrawText("SYSTEMS", sectionWidth * 3 + 20, panelY + 15, 20, LIGHTGRAY);
  DrawText(TextFormat("Fire Rate: %.2f", player.fireRate),
           sectionWidth * 3 + 20, panelY + 45, 30, YELLOW);

  // Missile Cooldown UI
  int missileBarWidth = 200;
  int missileBarHeight = 10;
  int missileBarX = screenW / 2 - missileBarWidth / 2;
  int missileBarY = screenH - panelHeight - 30; // Above panel

  DrawText("MISSILE (E)", missileBarX, missileBarY - 20, 20, ORANGE);
  DrawRectangle(missileBarX, missileBarY, missileBarWidth, missileBarHeight,
                DARKGRAY);

  float cooldownPct = 0.0f;
  if (player.missileMaxCooldown > 0) {
    cooldownPct =
        1.0f - (player.missileCooldownTimer / player.missileMaxCooldown);
  }
  if (cooldownPct < 0)
    cooldownPct = 0;
  if (cooldownPct > 1)
    cooldownPct = 1;

  DrawRectangle(missileBarX, missileBarY, (int)(missileBarWidth * cooldownPct),
                missileBarHeight, ORANGE);
  DrawRectangleLines(missileBarX, missileBarY, missileBarWidth,
                     missileBarHeight, WHITE);

  if (player.missileCooldownTimer <= 0) {
    DrawText("READY", missileBarX + missileBarWidth + 10, missileBarY - 5, 20,
             GREEN);
  } else {
    DrawText(TextFormat("%.1fs", player.missileCooldownTimer),
             missileBarX + missileBarWidth + 10, missileBarY - 5, 20, WHITE);
  }

  // Controls Display
  DrawText("WASD/Arrows: Move", screenW - 200, 20, 20, DARKGRAY);
  DrawText("SPACE: Dash", screenW - 200, 45, 20, DARKGRAY);
  DrawText("R: Restart", screenW - 200, 70, 20, DARKGRAY);

  if (gameOver) {
    DrawText("GAME OVER", screenW / 2 - 100, screenH / 2 - 40, 40, RED);
    DrawText("Appuyez sur R pour recommencer", screenW / 2 - 100,
             screenH / 2 + 10, 20, DARKGRAY);
  }

  EndDrawing();
}

// ============================================================================
// FONCTION PUBLIQUE - POINT D'ENTRÉE DU JEU
// ============================================================================

void RunSurvivorGame(Difficulte difficulty, bool resume, bool *game_active,
                     const char *playerName, ScoreList *scores) {
  // On ajuste la fenêtre au lieu de la recréer
  SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
  SetWindowTitle("survivor");
  SetTargetFPS(60);

  // Allocation dynamique seulement si on ne reprend pas une partie (nouvelle
  // partie)
  if (!resume) {
    // Sécurité : on libère si jamais il restait quelque chose
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
    missiles = (Missile *)calloc(MAX_MISSILES, sizeof(Missile));

    if (enemies == NULL || bullets == NULL || bonuses == NULL ||
        trainee == NULL || missiles == NULL) {
      printf("Erreur d'allocation mémoire !\n");
      if (enemies)
        free(enemies);
      if (bullets)
        free(bullets);
      if (bonuses)
        free(bonuses);
      if (trainee)
        free(trainee);
      if (missiles)
        free(missiles);
      // On ne ferme pas la fenêtre ici car elle appartient au main
      *game_active = false;
      return;
    }

    InitGame(difficulty);
  // Jouer l'effet d'hyperespace avant de commencer (seulement si nouvelle
  // partie)
    PlayHyperspaceEffect(1.0f, spaceshipTexture);
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

  background = LoadTexture("assets/Space-Background.png");
  spaceshipTexture = LoadTexture("assets/spaceship.png");
  chasseurTexture = LoadTexture("assets/sprite-chasseur.png");
  tankTexture = LoadTexture("assets/sprite-tank.png");

  while (!WindowShouldClose()) {
    // Si on appuie sur Echap, on sort de la boucle (retour menu)
    if (IsKeyPressed(KEY_ESCAPE)) {
      if (gameOver) {
        *game_active = false;
      }
      // Attendre que la touche soit relâchée pour éviter de fermer le menu
      while (IsKeyDown(KEY_ESCAPE)) {
        BeginDrawing();
        EndDrawing();
      }
      break;
    }

    float dt = GetFrameTime();

    // Si le jeu est actif, on met à jour la logique
    // Si game_active est false, le jeu est en pause (frozen)
    if (*game_active) {
      UpdateGame(dt);
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

  // Libération de la mémoire seulement si la partie est finie
  if (!*game_active) {
    if (enemies)
      free(enemies);
    if (bullets)
      free(bullets);
    if (bonuses)
      free(bonuses);
    if (trainee)
      free(trainee);
    if (missiles)
      free(missiles);
    enemies = NULL;
    bullets = NULL;
    bonuses = NULL;
    trainee = NULL;
    missiles = NULL;
  }
}
