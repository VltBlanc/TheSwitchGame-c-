#ifndef SURVIVOR_H
#define SURVIVOR_H

#include "menu.h"
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CONSTANTES
// ============================================================================

#define SURVIVOR_SCREEN_WIDTH 1280
#define SURVIVOR_SCREEN_HEIGHT 720
#define SURVIVOR_MAP_WIDTH 3000
#define SURVIVOR_MAP_HEIGHT 3000

#define MAX_ENEMIES 300
#define MAX_BULLETS 1000
#define MAX_BONUSES 50
#define MAX_TRAINEE 20

// ============================================================================
// TYPES - PLAYER
// ============================================================================

typedef struct {
  // Personnage
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

  // Variables pour le tir
  float fireRate;
  float shootTimer;
  float damage;
  float bulletSpeed;
  float bulletSize;

  // Dash
  bool isDashing;
  float dashTimer;
  float dashCooldown;
  float dashSpeed;
  float dashDuration;
} SurvivorPlayer;

// ============================================================================
// TYPES - ENEMIES
// ============================================================================

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

  // Animation
  Rectangle frameRec;
  int currentFrame;
  float frameTimer;
  float rotation;
} SurvivorEnemy;

// ============================================================================
// TYPES - PROJECTILES
// ============================================================================

typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  float radius;
  float lifeTime;
  int damage;
} SurvivorBullet;

// ============================================================================
// TYPES - BONUS
// ============================================================================

typedef enum {
  BONUS_DAMAGE,
  BONUS_HEALTH,
  BONUS_SPEED,
  BONUS_TIR_RAPIDE
} BonusType;

typedef struct {
  bool active;
  Vector2 position;
  float radius;
  BonusType type;
  Color color;
} SurvivorBonus;

// ============================================================================
// TYPES - EFFETS VISUELS
// ============================================================================

typedef struct {
  bool active;
  Vector2 position;
  float rotation;
  Rectangle frameRec;
  float alpha;
} SurvivorTrainee;

// ============================================================================
// FONCTIONS INTERNES (utilisées dans survivor.c)
// ============================================================================

// Initialisation
void InitGame(Difficulte difficulty);

// Spawn
void SpawnEnemy(void);
void SpawnBonus(void);

// Logique
int FindNearestEnemy(void);
void UpdateGame(float dt);

// Rendu
void DrawGame(void);

// ============================================================================
// FONCTION PUBLIQUE - Point d'entrée du jeu
// ============================================================================

// timeout_seconds: 0 = pas de limite, >0 = durée max en secondes
// total_time: pointeur vers le temps total écoulé (tous jeux confondus)
// start_delay: temps d'attente avant le début du jeu (animation zoom in)
void RunSurvivorGame(Difficulte difficulty, ModeJoueur mode, bool resume,
                     bool *game_active, const char *playerName,
                     float timeout_seconds, float *total_time,
                     float start_delay);

#endif // SURVIVOR_H
