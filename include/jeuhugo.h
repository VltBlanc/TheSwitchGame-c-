#ifndef JEUHUGO_H
#define JEUHUGO_H

#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// --- CONSTANTES ---
#define SIDEBAR_WIDTH 300
#define MAX_ENEMIES 400
#define MAX_BULLETS 400
#define ENEMY_SPEED_BASE 100.0f
#define BULLET_SPEED 450.0f
#define MAP_WIDTH 4096
#define MAP_HEIGHT 4096

// --- SCALING ---
#define PLAYER_SCALE 0.08f
#define ENEMY_SCALE 0.09f
#define BULLET_SCALE 0.04f

// --- ENUMS ---
typedef enum {
    UPGRADE_FIRE_RATE,
    UPGRADE_MOVE_SPEED,
    UPGRADE_MULTI_SHOT,
    UPGRADE_BURST_SHOT, 
    UPGRADE_DAMAGE,
    UPGRADE_PIERCING,
    UPGRADE_BULLET_SIZE,
    UPGRADE_HEALTH,
    UPGRADE_RICOCHET,
    UPGRADE_HOMING_SHOTS
} UpgradeType;

#define UPGRADE_TYPE_COUNT 10

typedef enum {
    RARITY_COMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY
} Rarity;

typedef enum {
    GAME_MENU,
    GAME_PLAYING
} GameState;

typedef enum {
    ENEMY_NORMAL,   
    ENEMY_TANK,     
    ENEMY_RUSHER    
} EnemyType;

// --- STRUCTURES ---
typedef struct UpgradeDef {
    UpgradeType type;
    const char* title;
    const char* desc;
    Rarity rarity;
    bool isUnique;
    int weight;
} UpgradeDef;

typedef struct Player {
    Vector2 position;
    float radius;
    int health;
    int maxHealth;
    bool isAlive;
    
    // Stats
    float moveSpeed;
    float fireRate;
    float shootTimer;
    int projectileCount;
    int burstCount;
    int pierceCount;
    int ricochetCount;
    float bulletSize;
    int damage;
    bool hasHomingShots;

    // Progression
    int level;
    float xp;
    float xpToNextLevel;
    
    // Stats Tracking
    int upgradeCounts[UPGRADE_TYPE_COUNT];
} Player;

typedef struct Enemy {
    Vector2 position;
    float radius;
    bool active;
    float speedMult;
    int health;
    int maxHealth;
    EnemyType type;
    
    // Animation
    int currentFrame;
    float frameTimer;
    int state; // 1=Move, 2=Attack, 3=Die
} Enemy;

typedef struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
    float lifeTime;
    int damage;
    int pierceLeft;
    int ricochetLeft;
    int targetIndex;
} Bullet;

typedef struct UpgradeOption {
    UpgradeType type;
    const char* text;
    const char* desc;
    Color color;
    Rectangle rect;
    Rarity rarity;
} UpgradeOption;

// --- PROTOTYPES ---
float FloatClamp(float value, float min, float max);
Vector2 VecAdd(Vector2 v1, Vector2 v2);
Vector2 VecSub(Vector2 v1, Vector2 v2);
Vector2 VecScale(Vector2 v, float scale);
float VecLengthSqr(Vector2 v);
float VecLength(Vector2 v);
Vector2 VecNormalize(Vector2 v);
float VecDistSqr(Vector2 v1, Vector2 v2);
Vector2 VecRotate(Vector2 v, float angleRad);

Color GetRarityColor(Rarity rarity);
Texture2D LoadGameTexture(const char* filename);
void InitResources();
void StartNewGame();
void SpawnEnemy();
int FindNearestEnemy();
int FindNearestEnemyFrom(Vector2 pos, int excludedIndex);
void ShootAt(Vector2 targetPos, int targetIndex);
void GenerateUpgrades();
void ApplyUpgrade(int index);
void UpdateMenu();
void DrawMenu();
void UpdateGame(float dt);
void DrawGame();

#endif // JEUHUGO_H
