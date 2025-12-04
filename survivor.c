#include "include/hyperspace.h"
#include "include/menu.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// --- Constantes ---
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAP_WIDTH 3000
#define MAP_HEIGHT 3000

#define MAX_ENEMIES 300
#define MAX_BULLETS 1000
#define MAX_BONUSES 50
#define MAX_TRAILS 20

// --- STRUCTURES ---
typedef struct {
  // personnage
  Vector2 position;
  float radius;
  float speed;
  int health;
  int maxHealth;
  bool isAlive;
  float rotation; // Rotation en degrés

  // Animation
  Rectangle frameRec;
  int currentFrame;
  float frameTimer;

  // variables pour le tir
  float fireRate;
  float shootTimer;
  float damage;
  float bulletSpeed;
  float bulletSize;

  // dash
  bool isDashing;
  float dashTimer;
  float dashCooldown;
  float dashSpeed;
  float dashDuration;

} Player;

typedef enum { ENEMY_NORMAL, ENEMY_TANK, ENEMY_FAST } EnemyType;
// ennemis
typedef struct {
  bool active;
  Vector2 position;
  EnemyType type;
  float radius;
  float speed;
  int health;
  int maxHealth;
  Color color;
  // Animation
  Rectangle frameRec;
  int currentFrame;
  float frameTimer;
  float rotation;
} Enemy;

typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  float radius;
  float lifeTime;
  int damage;
} Bullet;

typedef enum { BONUS_DAMAGE, BONUS_HEALTH, BONUS_SPEED } BonusType;

typedef struct {
  bool active;
  Vector2 position;
  float radius;
  BonusType type;
  Color color;
} Bonus;

typedef struct {
  bool active;
  Vector2 position;
  float rotation;
  Rectangle frameRec;
  float alpha;
} Trail;

// --- GLOBALS ---
Player player;
// on stock les enemis dans un tableau
Enemy *enemies = NULL;
// de même pour les balles tirées et les bonus
Bullet *bullets = NULL;
Bonus *bonuses = NULL;
Trail trails[MAX_TRAILS] = {0};
Texture2D background;
Texture2D spaceshipTexture;
Texture2D hunterTexture;
Texture2D tankTexture;

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

  // Animation Init
  player.currentFrame = 0;
  player.frameTimer = 0.0f;
  // Le sprite fait 331x1197, 3 frames verticales -> 331x399 par frame
  player.frameRec = (Rectangle){0.0f, 0.0f, 331.0f, 399.0f};

  player.fireRate = 0.15f; // Cadence de tir rapide
  player.shootTimer = 0.0f;
  // Les dégâts de base sont définis au-dessus, mais on s'assure qu'ils sont au
  // moins de 1
  if (player.damage < 1)
    player.damage = 1;

  player.bulletSpeed = 1200.0f;
  player.bulletSize = 10.0f;

  // Initialisation du Dash
  player.isDashing = false;
  player.dashTimer = 0.0f;
  player.dashCooldown = 0.1f;
  player.dashSpeed = 2000.0f; // Dash rapide
  player.dashDuration = 0.1f; // Courte durée

  // Initialisation de la caméra
  camera.target = player.position;
  camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  // Réinitialisation des variables globales
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
  for (int i = 0; i < MAX_TRAILS; i++)
    trails[i].active = false;
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
  // on se place à l'emplacement disponible pour un slot en attendant qu'une
  // place se libère

  // Position aléatoire dans les limites de la map
  float x = (float)GetRandomValue(50, MAP_WIDTH - 50);
  float y = (float)GetRandomValue(50, MAP_HEIGHT - 50);

  bonuses[slot].active = true;
  bonuses[slot].position = (Vector2){x, y};
  bonuses[slot].radius = 15.0f;

  int typeRoll = GetRandomValue(0, 100);
  if (typeRoll < 40) {
    bonuses[slot].type = BONUS_DAMAGE;
    bonuses[slot].color = RED;
  } else if (typeRoll < 80) {
    bonuses[slot].type = BONUS_HEALTH;
    bonuses[slot].color = GREEN;
  } else {
    bonuses[slot].type = BONUS_SPEED;
    bonuses[slot].color = SKYBLUE;
  }
}

void SpawnEnemy() {
  // on se place à l'emplacement disponible pour un slot en attendant qu'une
  // place se libère
  int slot = -1;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) {
      slot = i;
      break;
    }
  }
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
  // plus petit que 1, on a un normal roll est un random entre 0 et 100, pour
  // ajouter de la variabilité si roll est inférieur à 15, on a un tank si roll
  // est supérieur à 70, on a un fast sinon, on a un normal
  if (difficultyFactor > 2.0f && roll < 15) {
    enemies[slot].type = ENEMY_TANK;
    enemies[slot].radius = 35.0f;
    enemies[slot].speed = 250.0f;
    enemies[slot].maxHealth = 10 + (int)difficultyFactor; // Scaling réduit
    enemies[slot].color = DARKPURPLE;
  } else if (difficultyFactor > 1.0f && roll > 70) {
    enemies[slot].type = ENEMY_FAST;
    enemies[slot].radius = 12.0f;
    enemies[slot].speed = 450.0f; // Vitesse réduite (était 550)
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth =
        1 + (int)(difficultyFactor * 0.2f); // Scaling très faible
    enemies[slot].color = ORANGE;
  } else {
    enemies[slot].type = ENEMY_NORMAL;
    enemies[slot].radius = 30.0f; // Rayon augmenté (était 20)
    enemies[slot].speed =
        280.0f + (difficultyFactor * 5.0f); // Vitesse réduite (était 380)
    if (currentDifficulty == DIFFICILE)
      enemies[slot].speed *= 1.2f;
    enemies[slot].maxHealth =
        2 + (int)(difficultyFactor * 0.3f); // Scaling faible
    enemies[slot].color = RED;
  }

  // Init Animation
  if (enemies[slot].type == ENEMY_TANK) {
    enemies[slot].frameRec = (Rectangle){0.0f, 0.0f, (float)tankTexture.width,
                                         (float)tankTexture.height / 2.0f};
  } else {
    enemies[slot].frameRec = (Rectangle){0.0f, 0.0f, (float)hunterTexture.width,
                                         (float)hunterTexture.height / 3.0f};
  }
  enemies[slot].currentFrame = 0;
  enemies[slot].frameTimer = 0.0f;
  enemies[slot].rotation = 0.0f;

  enemies[slot].health = enemies[slot].maxHealth;
}

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

void UpdateGame(float dt) {
  if (gameOver) {
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
  // bouge
  if (IsKeyPressed(KEY_SPACE) && player.dashCooldown <= 0 && moveLen > 0) {
    player.isDashing = true;
    player.dashTimer = player.dashDuration;
    player.dashCooldown = 1.0f; // 1 seconde de cooldown
  }

  float currentSpeed = player.speed;
  if (player.isDashing) {
    currentSpeed = player.dashSpeed;
    player.dashTimer -= dt;

    // Spawn trail
    static float trailSpawnTimer = 0.0f;
    trailSpawnTimer += dt;
    if (trailSpawnTimer >= 0.02f) { // Spawn toutes les 20ms
      trailSpawnTimer = 0.0f;
      for (int i = 0; i < MAX_TRAILS; i++) {
        if (!trails[i].active) {
          trails[i].active = true;
          trails[i].position = player.position;
          trails[i].rotation = player.rotation;
          trails[i].frameRec = player.frameRec;
          trails[i].alpha = 0.8f;
          break;
        }
      }
    }

    if (player.dashTimer <= 0) {
      player.isDashing = false;
    }
  }

  // Update trails
  for (int i = 0; i < MAX_TRAILS; i++) {
    if (trails[i].active) {
      trails[i].alpha -= 2.0f * dt; // Fade out rapide
      if (trails[i].alpha <= 0)
        trails[i].active = false;
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
      // si on a une balle de disponible
      if (bSlot != -1) {
        Vector2 dir = {enemies[targetIdx].position.x - player.position.x,
                       enemies[targetIdx].position.y - player.position.y};
        float dirLen = sqrtf(dir.x * dir.x + dir.y * dir.y);
        // pour éviter une division par 0
        if (dirLen > 0.001f) {
          dir.x /= dirLen;
          dir.y /= dirLen;
        }
        // on prépare le tir
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
  if (bonusSpawnTimer >= 7.5f) {
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

void DrawGame() {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  BeginMode2D(camera);

  // Dessiner le fond
  DrawTexture(background, 0, 0, WHITE);

  // Dessiner les trails (Ghost effect)
  for (int i = 0; i < MAX_TRAILS; i++) {
    if (trails[i].active) {
      float scale = 0.15f;
      float width = 331.0f * scale;
      float height = 399.0f * scale;
      Rectangle destRec = {trails[i].position.x, trails[i].position.y, width,
                           height};
      Vector2 origin = {width / 2.0f, height / 2.0f};
      Color trailColor = WHITE;
      trailColor.a = (unsigned char)(trails[i].alpha * 255);
      DrawTexturePro(spaceshipTexture, trails[i].frameRec, destRec, origin,
                     trails[i].rotation, trailColor);
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
          (enemies[i].type == ENEMY_TANK) ? tankTexture : hunterTexture;
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

  DrawText(TextFormat("SCORE: %d", score), 20, 20, 20, DARKGRAY);
  DrawText(TextFormat("TIME: %.1fs", gameTime), 20, 50, 20, DARKGRAY);

  // Barre de vie en bas
  float hpPct = (float)player.health / (float)player.maxHealth;
  int barWidth = 300;
  int barHeight = 20;
  int barX = screenW / 2 - barWidth / 2;
  int barY = screenH - 40;

  DrawRectangle(barX, barY, barWidth, barHeight, RED);
  DrawRectangle(barX, barY, (int)(barWidth * hpPct), barHeight, GREEN);
  DrawRectangleLines(barX, barY, barWidth, barHeight, BLACK);

  DrawText(TextFormat("%d/%d", player.health, player.maxHealth),
           barX + barWidth / 2 - 20, barY + 2, 20, WHITE);

  // Indicateur de dégâts à côté de la barre de vie
  DrawText(TextFormat("DMG: %.0f", player.damage), barX + barWidth + 20, barY,
           20, RED);
  DrawText(TextFormat("SPD: %.0f", player.speed), barX + barWidth + 20,
           barY + 20, 20, SKYBLUE);

  // Controls Display
  DrawText("WASD/Arrows: Move", screenW - 200, 20, 20, DARKGRAY);
  DrawText("SPACE: Dash", screenW - 200, 45, 20, DARKGRAY);
  DrawText("R: Restart", screenW - 200, 70, 20, DARKGRAY);

  if (gameOver) {
    DrawText("GAME OVER", screenW / 2 - 100, screenH / 2 - 40, 40, RED);
    DrawText("Appuyez sur R pour recommencer", screenW / 2 - 100,
             screenH / 2 + 10, 20, DARKGRAY);
  }

  DrawFPS(10, 10);
  EndDrawing();
}

#include "include/survivor.h"

void RunSurvivorGame(Difficulte difficulty, bool resume, bool *game_active) {
  // On ajuste la fenêtre au lieu de la recréer
  SetWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
  SetWindowTitle("survivor");
  SetTargetFPS(60);

  // Allocation dynamique seulement si on ne reprend pas une partie
  if (!resume) {
    // Sécurité : on libère si jamais il restait quelque chose
    if (enemies)
      free(enemies);
    if (bullets)
      free(bullets);
    if (bonuses)
      free(bonuses);

    enemies = (Enemy *)calloc(MAX_ENEMIES, sizeof(Enemy));
    bullets = (Bullet *)calloc(MAX_BULLETS, sizeof(Bullet));
    bonuses = (Bonus *)calloc(MAX_BONUSES, sizeof(Bonus));

    if (enemies == NULL || bullets == NULL || bonuses == NULL) {
      printf("Erreur d'allocation mémoire !\n");
      if (enemies)
        free(enemies);
      if (bullets)
        free(bullets);
      if (bonuses)
        free(bonuses);
      // On ne ferme pas la fenêtre ici car elle appartient au main
      *game_active = false;
      return;
    }

    InitGame(difficulty);
  }

  background = LoadTexture("assets/Space-Background.png");
  spaceshipTexture = LoadTexture("assets/spaceship.png");
  hunterTexture = LoadTexture("assets/sprite-chasseur.png");
  tankTexture = LoadTexture("assets/sprite-tank.png");

  // Jouer l'effet d'hyperespace avant de commencer (seulement si nouvelle
  // partie)
  if (!resume) {
    PlayHyperspaceEffect(1.0f, spaceshipTexture);
  }

  *game_active = true;

  while (!WindowShouldClose()) {
    // Si on appuie sur Echap, on met en pause ou on quitte
    if (IsKeyPressed(KEY_ESCAPE)) {
      if (gameOver) {
        *game_active = false;
      } else {
        *game_active = true; // Pause
      }

      // Attendre que la touche soit relâchée pour éviter de fermer le menu
      // aussi
      while (IsKeyDown(KEY_ESCAPE)) {
        BeginDrawing();
        EndDrawing();
      }
      break;
    }

    float dt = GetFrameTime();
    UpdateGame(dt);
    DrawGame();
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
    enemies = NULL;
    bullets = NULL;
    bonuses = NULL;
  }

  // Ne pas fermer la fenêtre ici !
}
