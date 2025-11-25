#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// --- CONSTANTES ---
// SCREEN_WIDTH et SCREEN_HEIGHT supprimés pour le plein écran dynamique
#define SIDEBAR_WIDTH 300
#define MAX_ENEMIES 400     // Doublé pour gérer les hordes
#define MAX_BULLETS 400
#define ENEMY_SPEED_BASE 100.0f
#define BULLET_SPEED 450.0f
#define MAP_WIDTH 4096
#define MAP_HEIGHT 4096

// --- SCALING ---
#define PLAYER_SCALE 0.08f
#define ENEMY_SCALE 0.09f
#define BULLET_SCALE 0.04f


// --- FONCTIONS MATHÉMATIQUES ---
float FloatClamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
Vector2 VecAdd(Vector2 v1, Vector2 v2) { return (Vector2){ v1.x + v2.x, v1.y + v2.y }; }
Vector2 VecSub(Vector2 v1, Vector2 v2) { return (Vector2){ v1.x - v2.x, v1.y - v2.y }; }
Vector2 VecScale(Vector2 v, float scale) { return (Vector2){ v.x * scale, v.y * scale }; }
float VecLengthSqr(Vector2 v) { return v.x*v.x + v.y*v.y; }
float VecLength(Vector2 v) { return sqrtf(VecLengthSqr(v)); }
Vector2 VecNormalize(Vector2 v) {
    float length = VecLength(v);
    if (length > 0) return VecScale(v, 1.0f/length);
    return (Vector2){ 0, 0 };
}
float VecDistSqr(Vector2 v1, Vector2 v2) {
    Vector2 v = { v1.x - v2.x, v1.y - v2.y };
    return (v.x*v.x + v.y*v.y);
}
Vector2 VecRotate(Vector2 v, float angleRad) {
    float cosres = cosf(angleRad);
    float sinres = sinf(angleRad);
    return (Vector2){ v.x * cosres - v.y * sinres, v.x * sinres + v.y * cosres };
}

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

#define UPGRADE_TYPE_COUNT 10 // Nombre total de types d'upgrades

// --- RARETÉ ---
typedef enum {
    RARITY_COMMON,
    RARITY_RARE,
    RARITY_EPIC,
    RARITY_LEGENDARY
} Rarity;

// --- GAME STATE ---
typedef enum {
    GAME_MENU,
    GAME_PLAYING
} GameState;

// --- TYPES D'ENNEMIS ---
typedef enum {
    ENEMY_NORMAL,   
    ENEMY_TANK,     
    ENEMY_RUSHER    
} EnemyType;

typedef struct UpgradeDef {
    UpgradeType type;
    const char* title;
    const char* desc;
    Rarity rarity;
    bool isUnique;
    int weight;
} UpgradeDef;

// Définition des bonus avec rareté et probabilités
UpgradeDef upgradeDefs[] = {
    { UPGRADE_FIRE_RATE, "MINIGUN", "Cadence ++", RARITY_RARE, false, 35 },
    { UPGRADE_MOVE_SPEED, "BOTTES", "Vitesse ++", RARITY_COMMON, false, 50 },
    { UPGRADE_MULTI_SHOT, "MULTI-TIR", "+1 Proj (360)", RARITY_RARE, false, 30 },
    { UPGRADE_BURST_SHOT, "RAFALE", "+1 Balle (Série)", RARITY_RARE, false, 30 },
    { UPGRADE_DAMAGE, "PUISSANCE", "Dégâts +1", RARITY_COMMON, false, 45 },
    { UPGRADE_PIERCING, "PERÇANT", "Traverse +1", RARITY_EPIC, false, 20 },
    { UPGRADE_BULLET_SIZE, "GROS CALIBRE", "Taille ++", RARITY_COMMON, false, 50 },
    { UPGRADE_HEALTH, "VITALITÉ", "Soin + HP", RARITY_COMMON, false, 50 },
    { UPGRADE_RICOCHET, "REBOND", "+1 Ricochet", RARITY_EPIC, false, 20 },
    { UPGRADE_HOMING_SHOTS, "TÉLÉGUIDÉ", "Auto-visée", RARITY_LEGENDARY, true, 8 }
};
const int UPGRADE_DEF_COUNT = sizeof(upgradeDefs) / sizeof(upgradeDefs[0]);

Color GetRarityColor(Rarity rarity) {
    switch (rarity) {
        case RARITY_COMMON: return LIGHTGRAY;
        case RARITY_RARE: return SKYBLUE;
        case RARITY_EPIC: return PURPLE;
        case RARITY_LEGENDARY: return GOLD;
        default: return WHITE;
    }
}

// --- STRUCTURES ---
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
    int projectileCount; // Nombre de balles simultanées
    int burstCount;      // NOUVEAU : Nombre de balles à la suite
    int pierceCount;    // Combien d'ennemis une balle peut traverser
    int ricochetCount;  // Combien de fois une balle peut rebondir
    float bulletSize;   // Taille des projectiles
    int damage;         // NOUVEAU : Dégâts par balle
    bool hasHomingShots; // Les projectiles suivent les cibles

    // Progression
    int level;
    float xp;
    float xpToNextLevel;
    
    // Stats Tracking
    int upgradeCounts[UPGRADE_TYPE_COUNT]; // Compteur pour chaque type d'upgrade
} Player;

typedef struct Enemy {
    Vector2 position;
    float radius;
    bool active;
    float speedMult; // Certains ennemis peuvent être plus rapides
    int health;      // NOUVEAU : Points de vie
    int maxHealth;   // NOUVEAU : Max PV (pour barre de vie éventuelle)
    EnemyType type;  // NOUVEAU : Type d'ennemi
    
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
    int damage;     // NOUVEAU : Dégâts de cette balle
    int pierceLeft; // Compteur de perçage restant pour cette balle
    int ricochetLeft; // Compteur de ricochets restants
    int targetIndex; // Index de l'ennemi ciblé (pour le homing)
} Bullet;

typedef struct UpgradeOption {
    UpgradeType type;
    const char* text; // Titre + Desc concaténés ou gérés à l'affichage
    const char* desc; // Ajout description séparée
    Color color;
    Rectangle rect;
    Rarity rarity; // Pour l'affichage
} UpgradeOption;

// --- VARIABLES GLOBALES ---
Player player;
Enemy enemies[MAX_ENEMIES] = { 0 };
Bullet bullets[MAX_BULLETS] = { 0 };
int score = 0;
float spawnTimer = 0.0f;
float timeToSpawn = 1.0f; // Délai dynamique entre les spawns
float gameTime = 0.0f;    // NOUVEAU : Temps de jeu global en secondes
bool gameOver = false;
bool isLevelingUp = false;
bool isPaused = false;

UpgradeOption upgradeOptions[3];

// --- MENU VARIABLES ---
GameState gameState = GAME_MENU;
bool enemyEnabled[3] = { true, true, true }; // 0: Normal, 1: Tank, 2: Rusher
bool upgradeEnabled[UPGRADE_TYPE_COUNT];
float menuTime = 0.0f;

// --- TEXTURES & CAMERA ---
Texture2D backgroundTexture;
Texture2D playerTexture;
Texture2D enemyBarbareTexture;
Texture2D enemyGobelinTexture;
Texture2D enemyGeantTexture;
Texture2D bulletTexture;
Texture2D upgradeTextures[UPGRADE_TYPE_COUNT]; // Tableau pour les textures d'upgrades
Camera2D camera = { 0 };

// --- LOGIQUE ---

Texture2D LoadGameTexture(const char* filename) {
    // 1. Essayer "assets/filename" (Exécution depuis la racine)
    const char* pathRoot = TextFormat("assets/%s", filename);
    if (FileExists(pathRoot)) return LoadTexture(pathRoot);

    // 2. Essayer "../assets/filename" (Exécution depuis bin/)
    const char* pathBin = TextFormat("../assets/%s", filename);
    if (FileExists(pathBin)) return LoadTexture(pathBin);

    // 3. Fallback (Exécution depuis assets/ ou autre)
    return LoadTexture(filename);
}

void InitResources() {
    // Chargement du background (seulement si pas déjà chargé)
    if (backgroundTexture.id == 0) {
        backgroundTexture = LoadGameTexture("background.png");
    }
    if (playerTexture.id == 0) {
        playerTexture = LoadGameTexture("sorcier.png");
    }
    if (enemyBarbareTexture.id == 0) {
        enemyBarbareTexture = LoadGameTexture("sprite_barbare.png");
    }
    if (enemyGobelinTexture.id == 0) {
        enemyGobelinTexture = LoadGameTexture("sprite_gobelin.png");
    }
    if (enemyGeantTexture.id == 0) {
        enemyGeantTexture = LoadGameTexture("sprite_geant.png");
    }
    if (bulletTexture.id == 0) {
        bulletTexture = LoadGameTexture("boule_de_feu.png");
    }

    // Chargement des textures d'upgrades
    if (upgradeTextures[UPGRADE_FIRE_RATE].id == 0) upgradeTextures[UPGRADE_FIRE_RATE] = LoadGameTexture("minigun.png");
    if (upgradeTextures[UPGRADE_MOVE_SPEED].id == 0) upgradeTextures[UPGRADE_MOVE_SPEED] = LoadGameTexture("bottes.png");
    if (upgradeTextures[UPGRADE_MULTI_SHOT].id == 0) upgradeTextures[UPGRADE_MULTI_SHOT] = LoadGameTexture("multi-tir.png");
    if (upgradeTextures[UPGRADE_BURST_SHOT].id == 0) upgradeTextures[UPGRADE_BURST_SHOT] = LoadGameTexture("rafale.png");
    if (upgradeTextures[UPGRADE_DAMAGE].id == 0) upgradeTextures[UPGRADE_DAMAGE] = LoadGameTexture("puissance.png");
    if (upgradeTextures[UPGRADE_PIERCING].id == 0) upgradeTextures[UPGRADE_PIERCING] = LoadGameTexture("percant.png");
    if (upgradeTextures[UPGRADE_BULLET_SIZE].id == 0) upgradeTextures[UPGRADE_BULLET_SIZE] = LoadGameTexture("gros_calibre.png");
    if (upgradeTextures[UPGRADE_HEALTH].id == 0) upgradeTextures[UPGRADE_HEALTH] = LoadGameTexture("vitalite.png");
    if (upgradeTextures[UPGRADE_RICOCHET].id == 0) upgradeTextures[UPGRADE_RICOCHET] = LoadGameTexture("rebond.png");
    if (upgradeTextures[UPGRADE_HOMING_SHOTS].id == 0) upgradeTextures[UPGRADE_HOMING_SHOTS] = LoadGameTexture("teleguide.png");

    // Init default upgrades enabled
    static bool firstInit = true;
    if (firstInit) {
        for(int i=0; i<UPGRADE_TYPE_COUNT; i++) upgradeEnabled[i] = true;
        firstInit = false;
    }
}

void StartNewGame() {
    // Init Camera
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Position de départ au milieu de la map
    player.position = (Vector2){ MAP_WIDTH / 2, MAP_HEIGHT / 2 };
    
    float playerDrawHeight = screenHeight * PLAYER_SCALE;
    float playerDrawWidth = playerDrawHeight; // square approximation
    player.radius = (playerDrawWidth + playerDrawHeight) / 4; // moyenne -> playerDrawHeight/2
    player.maxHealth = 3; // On commence avec 3 PV maintenant
    player.health = player.maxHealth;
    player.isAlive = true;
    
    // Stats de départ
    player.moveSpeed = 200.0f;
    player.fireRate = 0.7f; 
    player.projectileCount = 1;
    player.burstCount = 1; // 1 balle par défaut (pas 0 sinon ça tire rien)
    player.pierceCount = 0; // Ne traverse pas au début
    player.ricochetCount = 0; // Pas de ricochet au début
    player.bulletSize = 10.0f;
    player.damage = 1; // Dégâts de base
    player.hasHomingShots = false;
    player.shootTimer = 0.0f;
    
    player.level = 1;
    player.xp = 0;
    player.xpToNextLevel = 15.0f;
    
    for(int i=0; i<UPGRADE_TYPE_COUNT; i++) player.upgradeCounts[i] = 0;

    score = 0;
    spawnTimer = 0.0f;
    timeToSpawn = 1.0f;
    gameTime = 0.0f;
    gameOver = false;
    isLevelingUp = false;
    isPaused = false;

    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
}

// Spawn adapté au niveau (Plus le niveau est haut, plus ça spawn par paquet)
// Spawn adapté au temps de jeu
void SpawnEnemy() {
    // Difficulté basée sur le temps :
    // - Augmente le nombre d'ennemis par vague toutes les 30 secondes
    int batchSize = 1 + ((int)gameTime / 30); 

    // Check enabled enemies
    EnemyType allowedTypes[3];
    int allowedCount = 0;
    if (enemyEnabled[0]) allowedTypes[allowedCount++] = ENEMY_NORMAL;
    if (enemyEnabled[1]) allowedTypes[allowedCount++] = ENEMY_TANK;
    if (enemyEnabled[2]) allowedTypes[allowedCount++] = ENEMY_RUSHER;

    if (allowedCount == 0) return; // No enemies enabled!

    for(int k=0; k<batchSize; k++) {
        int slot = -1;
        for(int i=0; i<MAX_ENEMIES; i++) { if(!enemies[i].active) { slot = i; break; }}
        if (slot == -1) return;

        // Spawn autour du joueur (hors champ caméra)
        float spawnRadius = 1000.0f; // Assez loin pour être hors écran
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        Vector2 pos = {
            player.position.x + cosf(angle) * spawnRadius,
            player.position.y + sinf(angle) * spawnRadius
        };
        
        // Wrap spawn position (Infinite Map)
        if (pos.x < 0) pos.x += MAP_WIDTH;
        else if (pos.x >= MAP_WIDTH) pos.x -= MAP_WIDTH;
        if (pos.y < 0) pos.y += MAP_HEIGHT;
        else if (pos.y >= MAP_HEIGHT) pos.y -= MAP_HEIGHT;

        enemies[slot].position = pos;
        
        // Choix du type d'ennemi selon le temps
        EnemyType selectedType = ENEMY_NORMAL;
        
        // Original logic for desired type
        EnemyType desiredType = ENEMY_NORMAL;
        int roll = GetRandomValue(0, 100);
        if (gameTime > 120.0f) {
            if (roll < 60) desiredType = ENEMY_NORMAL;
            else if (roll < 80) desiredType = ENEMY_TANK;
            else desiredType = ENEMY_RUSHER;
        } else if (gameTime > 60.0f) {
            if (roll < 80) desiredType = ENEMY_NORMAL;
            else if (roll < 90) desiredType = ENEMY_TANK;
            else desiredType = ENEMY_RUSHER;
        } else {
            desiredType = ENEMY_NORMAL;
        }

        // Check if desiredType is enabled
        bool isEnabled = false;
        if (desiredType == ENEMY_NORMAL && enemyEnabled[0]) isEnabled = true;
        else if (desiredType == ENEMY_TANK && enemyEnabled[1]) isEnabled = true;
        else if (desiredType == ENEMY_RUSHER && enemyEnabled[2]) isEnabled = true;

        if (isEnabled) {
            selectedType = desiredType;
        } else {
            // Pick random allowed
            selectedType = allowedTypes[GetRandomValue(0, allowedCount - 1)];
        }
        
        enemies[slot].type = selectedType;

        float baseSize = GetScreenHeight() * ENEMY_SCALE;
        float speedBase = ENEMY_SPEED_BASE + (gameTime * 0.5f); // Vitesse augmente doucement avec le temps
        int hpBase = 1 + ((int)gameTime / 45); // +1 PV toutes les 45s

        switch (enemies[slot].type) {
            case ENEMY_NORMAL:
                enemies[slot].radius = baseSize / 2;
                enemies[slot].speedMult = 1.0f;
                enemies[slot].maxHealth = hpBase;
                break;
            case ENEMY_TANK: // Gros, lent, fort
                enemies[slot].radius = baseSize * 0.8f; // Plus gros
                enemies[slot].speedMult = 0.6f; // Plus lent
                enemies[slot].maxHealth = hpBase * 3; // Beaucoup plus de PV
                break;
            case ENEMY_RUSHER: // Petit, rapide, faible
                enemies[slot].radius = baseSize * 0.35f; // Plus petit
                enemies[slot].speedMult = 1.6f; // Très rapide
                enemies[slot].maxHealth = (hpBase / 2) < 1 ? 1 : (hpBase / 2); // Moins de PV
                break;
        }

        enemies[slot].active = true;
        // Variation aléatoire de vitesse
        enemies[slot].speedMult *= (1.0f + (GetRandomValue(-10, 10) / 100.0f));
        enemies[slot].health = enemies[slot].maxHealth;
        
        // Init Animation
        enemies[slot].currentFrame = 0;
        enemies[slot].frameTimer = 0.0f;
        enemies[slot].state = 1; // 1 = Move (2nd ligne)
    }
}

int FindNearestEnemy() {
    int nearestIndex = -1;
    float minDistanceSq = 9999999.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            float distSq = VecDistSqr(player.position, enemies[i].position);
            if (distSq < minDistanceSq) {
                minDistanceSq = distSq;
                nearestIndex = i;
            }
        }
    }
    return nearestIndex;
}

int FindNearestEnemyFrom(Vector2 pos, int excludedIndex) {
    int nearestIndex = -1;
    float minDistanceSq = 9999999.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && i != excludedIndex) {
            float distSq = VecDistSqr(pos, enemies[i].position);
            if (distSq < minDistanceSq) {
                minDistanceSq = distSq;
                nearestIndex = i;
            }
        }
    }
    return nearestIndex;
}

void ShootAt(Vector2 targetPos, int targetIndex) {
    Vector2 baseDir = VecSub(targetPos, player.position);
    baseDir = VecNormalize(baseDir);

    // Tir réparti sur 360 degrés
    // Si 1 projectile : angle 0 (vers la cible)
    // Si 2 projectiles : 0 et 180
    // Si 3 projectiles : 0, 120, 240
    float angleStep = (2.0f * PI) / player.projectileCount;

    // Boucle sur les directions (360)
    for (int p = 0; p < player.projectileCount; p++) {
        
        // Rotation relative à la cible
        float currentAngle = p * angleStep;
        Vector2 dir = VecRotate(baseDir, currentAngle);

        // Boucle sur la rafale (Burst) - Balles à la suite
        for (int b = 0; b < player.burstCount; b++) {
            int slot = -1;
            for(int i=0; i<MAX_BULLETS; i++) { if(!bullets[i].active) { slot = i; break; }}
            if (slot == -1) break;

            // Décalage spatial pour simuler le "à la suite"
            // On recule le point de départ de la balle selon son rang dans la rafale
            float burstOffset = b * 25.0f; // 25 pixels d'écart
            Vector2 spawnPos = VecSub(player.position, VecScale(dir, burstOffset));

            bullets[slot].position = spawnPos;
            bullets[slot].velocity = VecScale(dir, BULLET_SPEED);
            bullets[slot].radius = player.bulletSize;
            bullets[slot].active = true;
            bullets[slot].lifeTime = 2.5f;
            bullets[slot].damage = player.damage; // Assigne les dégâts du joueur
            bullets[slot].pierceLeft = player.pierceCount; 
            bullets[slot].ricochetLeft = player.ricochetCount; 
            bullets[slot].targetIndex = targetIndex; 
        }
    }
}

void GenerateUpgrades() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int boxWidth = 220;
    int gap = 20;
    int startX = (screenWidth - (3 * boxWidth + 2 * gap)) / 2;
    int boxHeight = 300;
    int y = (screenHeight - boxHeight) / 2;

    // Filter enabled upgrades
    int validIndices[UPGRADE_DEF_COUNT];
    int validCount = 0;
    for (int i=0; i<UPGRADE_DEF_COUNT; i++) {
        if (upgradeEnabled[upgradeDefs[i].type]) {
            validIndices[validCount++] = i;
        }
    }

    if (validCount == 0) return; // Should not happen if UI prevents unchecking all

    for (int i = 0; i < 3; i++) {
        // 1. Calcul du poids total des valides
        int totalWeight = 0;
        for (int j = 0; j < validCount; j++) {
             totalWeight += upgradeDefs[validIndices[j]].weight;
        }

        // 2. Tirage au sort pondéré
        int r = GetRandomValue(0, totalWeight - 1);
        int currentWeight = 0;
        int selectedIndex = validIndices[0];
        
        for (int j = 0; j < validCount; j++) {
            currentWeight += upgradeDefs[validIndices[j]].weight;
            if (r < currentWeight) {
                selectedIndex = validIndices[j];
                break;
            }
        }

        // 3. Remplir l'option
        upgradeOptions[i].type = upgradeDefs[selectedIndex].type;
        upgradeOptions[i].text = upgradeDefs[selectedIndex].title;
        upgradeOptions[i].desc = upgradeDefs[selectedIndex].desc;
        upgradeOptions[i].rarity = upgradeDefs[selectedIndex].rarity;
        upgradeOptions[i].color = GetRarityColor(upgradeDefs[selectedIndex].rarity);
        upgradeOptions[i].rect = (Rectangle){ startX + i * (boxWidth + gap), y, boxWidth, boxHeight };
    }
}

void ApplyUpgrade(int index) {
    UpgradeType type = upgradeOptions[index].type;
    player.upgradeCounts[type]++; // Incrémente le compteur

    switch (type) {
        case UPGRADE_FIRE_RATE:
            player.fireRate *= 0.85f;
            if (player.fireRate < 0.05f) player.fireRate = 0.05f; // Cap limite
            break;
        case UPGRADE_MOVE_SPEED:
            player.moveSpeed *= 1.15f;
            break;
        case UPGRADE_MULTI_SHOT: // MULTI-TIR
            player.projectileCount++;
            break;
        case UPGRADE_BURST_SHOT: // RAFALE
            player.burstCount++;
            break;
        case UPGRADE_DAMAGE:
            player.damage++;
            break;
        case UPGRADE_PIERCING:
            player.pierceCount++;
            break;
        case UPGRADE_BULLET_SIZE:
            player.bulletSize *= 1.3f;
            break;
        case UPGRADE_HEALTH:
            player.maxHealth += 1;
            player.health = player.maxHealth; // Soin complet
            break;
        case UPGRADE_RICOCHET:
            player.ricochetCount++;
            break;
        case UPGRADE_HOMING_SHOTS:
            player.hasHomingShots = true;
            break;
    }

    // Progression
    player.level++;
    player.xp = 0;
    player.xpToNextLevel *= 1.4f;
    
    // Augmentation Difficulté : Spawn plus rapide
    timeToSpawn *= 0.95f; 
    if (timeToSpawn < 0.2f) timeToSpawn = 0.2f;

    isLevelingUp = false;
}

void UpdateMenu() {
    menuTime += GetFrameTime();
    
    Vector2 mousePoint = GetMousePosition();
    bool mousePressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Start Button
    Rectangle startBtn = { screenWidth/2 - 100, screenHeight - 100, 200, 50 };
    if (mousePressed && CheckCollisionPointRec(mousePoint, startBtn)) {
        StartNewGame();
        gameState = GAME_PLAYING;
    }

    // Enemy Toggles
    int startY = 150;
    for(int i=0; i<3; i++) {
        Rectangle toggle = { 100, startY + i * 120, 30, 30 };
        if (mousePressed && CheckCollisionPointRec(mousePoint, toggle)) {
            enemyEnabled[i] = !enemyEnabled[i];
        }
    }

    // Upgrade Toggles
    int upgradeStartX = screenWidth - 550;
    int upgradeStartY = 150;
    for(int i=0; i<UPGRADE_TYPE_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        Rectangle toggle = { upgradeStartX + col * 260, upgradeStartY + row * 110, 60, 60 };
        if (mousePressed && CheckCollisionPointRec(mousePoint, toggle)) {
            upgradeEnabled[i] = !upgradeEnabled[i];
        }
    }
}

void DrawMenu() {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Draw Background Tiling (same as game)
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    for (int x = 0; x <= screenWidth; x += backgroundTexture.width) {
        for (int y = 0; y <= screenHeight; y += backgroundTexture.height) {
            DrawTexture(backgroundTexture, x, y, WHITE);
        }
    }
    
    // Overlay to darken background
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(WHITE, 0.8f));

    DrawText("GAME MENU", screenWidth/2 - MeasureText("GAME MENU", 50)/2, 50, 50, DARKGRAY);

    // --- ENEMIES ---
    DrawText("SELECT ENEMIES", 100, 100, 30, BLACK);
    const char* enemyNames[] = { "Normal", "Tank (Giant)", "Rusher (Goblin)" };
    Texture2D enemyTextures[] = { enemyBarbareTexture, enemyGeantTexture, enemyGobelinTexture };
    
    int startY = 150;
    for(int i=0; i<3; i++) {
        Rectangle toggle = { 100, startY + i * 120, 30, 30 };
        DrawRectangleRec(toggle, enemyEnabled[i] ? GREEN : RED);
        DrawRectangleLinesEx(toggle, 2, BLACK);
        DrawText(enemyNames[i], 140, startY + i * 120 + 5, 20, BLACK);

        // Draw Sprites (Walk & Attack)
        Texture2D sprite = enemyTextures[i];
        
        // Animation Logic
        int currentFrame = (int)(menuTime * 8.0f) % 4; // 4 frames loop
        int currentFrameAttack = (int)(menuTime * 8.0f) % 6;

        // Walk (Row 1 -> index 1)
        float frameWidthWalk = (float)sprite.width / 8.0f; // Using 8 columns logic
        float frameHeight = (float)sprite.height / 4.0f;
        Rectangle sourceWalk = { currentFrame * frameWidthWalk, 1 * frameHeight, frameWidthWalk, frameHeight };
        
        // Attack (Row 2 -> index 2)
        float frameWidthAttack = (float)sprite.width / 6.7f; // Using 6 columns logic
        Rectangle sourceAttack = { currentFrameAttack * frameWidthAttack, 2 * frameHeight, frameWidthAttack, frameHeight };

        // Fix bleeding and shift
        sourceWalk.x += 4.0f;
        sourceWalk.width -= 8.0f;
        sourceWalk.height -= 1.0f;

        sourceAttack.x += 4.0f;
        sourceAttack.width -= 8.0f;
        sourceAttack.height -= 1.0f;

        // Calculate draw dimensions to preserve aspect ratio
        // We fix height to 80.0f
        float scale = 80.0f / frameHeight;
        float drawWidthWalk = frameWidthWalk * scale;
        float drawWidthAttack = frameWidthAttack * scale;

        // Draw Walk
        Rectangle destWalk = { 300, startY + i * 120 - 20, drawWidthWalk, 80 };
        DrawTexturePro(sprite, sourceWalk, destWalk, (Vector2){0,0}, 0.0f, WHITE);
        DrawText("Walk", 300 + drawWidthWalk/2 - 10, startY + i * 120 + 60, 10, DARKGRAY);

        // Draw Attack
        Rectangle destAttack = { 300 + drawWidthWalk + 40, startY + i * 120 - 20, drawWidthAttack, 80 };
        DrawTexturePro(sprite, sourceAttack, destAttack, (Vector2){0,0}, 0.0f, WHITE);
        DrawText("Attack", 300 + drawWidthWalk + 40 + drawWidthAttack/2 - 15, startY + i * 120 + 60, 10, DARKGRAY);
    }

    // --- UPGRADES ---
    int upgradeStartX = screenWidth - 550;
    int upgradeStartY = 150;
    DrawText("SELECT UPGRADES", upgradeStartX, 100, 30, BLACK);
    
    for(int i=0; i<UPGRADE_TYPE_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = upgradeStartX + col * 260;
        int y = upgradeStartY + row * 110;
        
        Rectangle toggle = { x, y, 60, 60 };
        DrawRectangleRec(toggle, upgradeEnabled[i] ? GREEN : RED);
        DrawRectangleLinesEx(toggle, 3, BLACK);
        
        // Icon
        Texture2D icon = upgradeTextures[i];
        if (icon.id != 0) {
            DrawTexturePro(icon, (Rectangle){0,0,icon.width,icon.height}, (Rectangle){x + 70, y - 15, 90, 90}, (Vector2){0,0}, 0.0f, WHITE);
        }
        
        // Name (Shortened if needed)
        // Find name from upgradeDefs
        const char* name = "Unknown";
        for(int j=0; j<UPGRADE_DEF_COUNT; j++) {
            if(upgradeDefs[j].type == i) {
                name = upgradeDefs[j].title;
                break;
            }
        }
        DrawText(name, x + 170, y + 20, 20, BLACK);
    }

    // Start Button
    Rectangle startBtn = { screenWidth/2 - 100, screenHeight - 100, 200, 50 };
    DrawRectangleRec(startBtn, SKYBLUE);
    DrawRectangleLinesEx(startBtn, 3, BLUE);
    DrawText("START GAME", startBtn.x + 40, startBtn.y + 15, 20, WHITE);

    EndDrawing();
}

void UpdateGame(float dt) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        isPaused = !isPaused;
    }

    if (isPaused) return;

    if (gameOver) {
        if (IsKeyPressed(KEY_R)) StartNewGame();
        if (IsKeyPressed(KEY_M)) gameState = GAME_MENU;
        return;
    }

    if (isLevelingUp) {
        Vector2 mousePoint = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for (int i = 0; i < 3; i++) {
                if (CheckCollisionPointRec(mousePoint, upgradeOptions[i].rect)) {
                    ApplyUpgrade(i);
                    break;
                }
            }
        }
        return;
    }

    // 1. Joueur
    Vector2 moveDir = { 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveDir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveDir.x += 1.0f;

    if (VecLengthSqr(moveDir) > 0) {
        moveDir = VecNormalize(moveDir);
        player.position = VecAdd(player.position, VecScale(moveDir, player.moveSpeed * dt));
    }
    
    // Wrap Player to Map (Infinite Loop)
    if (player.position.x < 0) player.position.x += MAP_WIDTH;
    else if (player.position.x >= MAP_WIDTH) player.position.x -= MAP_WIDTH;
    
    if (player.position.y < 0) player.position.y += MAP_HEIGHT;
    else if (player.position.y >= MAP_HEIGHT) player.position.y -= MAP_HEIGHT;

    // Update Camera
    camera.target = player.position;

    // 2. Ennemis
    gameTime += dt; // Incrémente le temps de jeu
    spawnTimer += dt;
    if (spawnTimer >= timeToSpawn) {
        SpawnEnemy();
        spawnTimer = 0.0f;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            Vector2 dirToPlayer = VecSub(player.position, enemies[i].position);
            dirToPlayer = VecNormalize(dirToPlayer);
            
            // Animation Update
            enemies[i].frameTimer += dt;
            if (enemies[i].frameTimer >= 0.15f) { // Vitesse d'animation
                enemies[i].frameTimer = 0.0f;
                enemies[i].currentFrame++;
                
                int maxFramesMove = 4;
                int maxFramesDie = 6;
                
                // Le Géant n'a que 4 frames (4 colonnes)
                if (enemies[i].type == ENEMY_TANK) {
                    maxFramesDie = 6; // Le géant a aussi 6 frames pour la mort maintenant
                }

                if (enemies[i].state == 1) { // Move
                    if (enemies[i].currentFrame >= maxFramesMove) enemies[i].currentFrame = 0;
                } else { // Attack (2) or Die (3)
                    if (enemies[i].currentFrame >= maxFramesDie) {
                        enemies[i].active = false; // Fin de l'animation -> Disparition
                    }
                }
            }

            if (enemies[i].state == 3 || enemies[i].state == 2) continue; // Ne bouge plus si mort ou attaque

            // Vitesse varie selon le temps de jeu + multiplicateur individuel
            float baseSpeed = ENEMY_SPEED_BASE + (gameTime * 0.5f);
            float currentEnemySpeed = baseSpeed * enemies[i].speedMult;
            enemies[i].position = VecAdd(enemies[i].position, VecScale(dirToPlayer, currentEnemySpeed * dt));

            if (CheckCollisionCircles(player.position, player.radius, enemies[i].position, enemies[i].radius)) {
                // Collision Ennemi -> Joueur
                player.health--;
                // Passage en mode Attaque (Ligne 3 -> index 2)
                enemies[i].state = 2;
                enemies[i].currentFrame = 0;
                enemies[i].frameTimer = 0.0f;
                
                if(player.health <= 0) {
                    player.isAlive = false;
                    gameOver = true;
                }
            }
        }
    }

    // 3. Tir
    player.shootTimer += dt;
    if (player.shootTimer >= player.fireRate) {
        int nearestEnemy = FindNearestEnemy();
        if (nearestEnemy != -1) {
            ShootAt(enemies[nearestEnemy].position, nearestEnemy);
            player.shootTimer = 0.0f;
        }
    }

    // 4. Balles
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (bullets[b].active) {
            // --- LOGIQUE DE GUIDAGE ---
            if (player.hasHomingShots && bullets[b].targetIndex != -1 && enemies[bullets[b].targetIndex].active) {
                Vector2 targetPos = enemies[bullets[b].targetIndex].position;
                Vector2 desiredDir = VecNormalize(VecSub(targetPos, bullets[b].position));
                
                // Interpolation pour un guidage plus doux
                float turnFactor = 0.08f; 
                Vector2 currentDir = VecNormalize(bullets[b].velocity);
                Vector2 newDir = VecNormalize(VecAdd(VecScale(currentDir, 1.0f - turnFactor), VecScale(desiredDir, turnFactor)));

                bullets[b].velocity = VecScale(newDir, BULLET_SPEED);
            }

            bullets[b].position = VecAdd(bullets[b].position, VecScale(bullets[b].velocity, dt));
            bullets[b].lifeTime -= dt;

            if (bullets[b].lifeTime <= 0) { bullets[b].active = false; continue; }

            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemies[e].active && enemies[e].state == 1) { // Seulement si vivant (Move)
                    if (CheckCollisionCircles(bullets[b].position, bullets[b].radius, enemies[e].position, enemies[e].radius)) {
                        // Appliquer les dégâts
                        enemies[e].health -= bullets[b].damage;
                        
                        // Si l'ennemi meurt
                        if (enemies[e].health <= 0) {
                            // Passage en mode Mort (Ligne 4 -> index 3)
                            enemies[e].state = 3;
                            enemies[e].currentFrame = 0;
                            enemies[e].frameTimer = 0.0f;
                            
                            score += 10;
                            player.xp += 5.0f;

                            if (player.xp >= player.xpToNextLevel) {
                                isLevelingUp = true;
                                GenerateUpgrades();
                            }
                        }
                        
                        // Si la balle touche sa cible, elle ne la suit plus
                        if(bullets[b].targetIndex == e) bullets[b].targetIndex = -1;

                        if (bullets[b].pierceLeft > 0) {
                            bullets[b].pierceLeft--; 
                        } else if (bullets[b].ricochetLeft > 0) {
                            bullets[b].ricochetLeft--;
                            int nextTarget = FindNearestEnemyFrom(bullets[b].position, e);
                            if (nextTarget != -1) {
                                Vector2 newDir = VecNormalize(VecSub(enemies[nextTarget].position, bullets[b].position));
                                bullets[b].velocity = VecScale(newDir, BULLET_SPEED);
                                bullets[b].targetIndex = nextTarget; // Nouvelle cible pour le guidage
                            } else {
                                bullets[b].active = false;
                            }
                        } else {
                            bullets[b].active = false; 
                            break; 
                        }
                    }
                }
            }
        }
    }
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    BeginMode2D(camera);
    
    // Calcul des bornes visibles pour le tiling infini du background
    Vector2 camMin = GetScreenToWorld2D((Vector2){0, 0}, camera);
    Vector2 camMax = GetScreenToWorld2D((Vector2){(float)screenWidth, (float)screenHeight}, camera);
    
    // On étend un peu la zone de rendu pour être sûr
    int startX = (int)floorf(camMin.x / backgroundTexture.width) * backgroundTexture.width;
    int startY = (int)floorf(camMin.y / backgroundTexture.height) * backgroundTexture.height;
    int endX = (int)ceilf(camMax.x / backgroundTexture.width) * backgroundTexture.width;
    int endY = (int)ceilf(camMax.y / backgroundTexture.height) * backgroundTexture.height;

    // Background
    for (int x = startX; x <= endX; x += backgroundTexture.width) {
        for (int y = startY; y <= endY; y += backgroundTexture.height) {
            DrawTexture(backgroundTexture, x, y, WHITE);
        }
    }

    if (!gameOver) {
        // Dessin
        float playerDrawHeight = screenHeight * PLAYER_SCALE;
        // Draw player sprite
        Rectangle source = { 0.0f, 0.0f, (float)playerTexture.width, (float)playerTexture.height };
        Rectangle dest = { player.position.x, player.position.y, player.radius * 2.5f, player.radius * 2.5f }; 
        Vector2 origin = { dest.width / 2, dest.height / 2 };
        DrawTexturePro(playerTexture, source, dest, origin, 0.0f, WHITE);
        
        // DrawCircleLines((int)player.position.x, (int)player.position.y, player.radius, Fade(WHITE, 0.3f)); // Debug Hitbox

        // Barre de vie sous le joueur
        float hpRatio = (float)player.health / (float)player.maxHealth;
        DrawRectangle(player.position.x - 20, player.position.y + 20, 40, 5, RED);
        DrawRectangle(player.position.x - 20, player.position.y + 20, 40 * hpRatio, 5, GREEN);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                Texture2D sprite;
                Rectangle source;

                if (enemies[i].type == ENEMY_TANK) {
                    sprite = enemyGeantTexture;
                    float frameWidth;
                    if (enemies[i].state == 1) frameWidth = (float)sprite.width / 8.0f; // Marche : 8 colonnes
                    else frameWidth = (float)sprite.width / 6.0f; // Attaque/Mort : 6 colonnes
                    
                    float frameHeight = (float)sprite.height / 4.0f;
                    int row = enemies[i].state; // 1=Move, 2=Attack, 3=Die
                    source = (Rectangle){ enemies[i].currentFrame * frameWidth, row * frameHeight, frameWidth, frameHeight };
                }
                else if (enemies[i].type == ENEMY_RUSHER) {
                    sprite = enemyGobelinTexture;
                    float frameWidth;
                    if (enemies[i].state == 1) frameWidth = (float)sprite.width / 8.0f; // Marche : 8 colonnes (utilise 4)
                    else frameWidth = (float)sprite.width / 6.0f; // Attaque/Mort : 6 colonnes (utilise 6)
                    
                    float frameHeight = (float)sprite.height / 4.0f;
                    int row = enemies[i].state;
                    source = (Rectangle){ enemies[i].currentFrame * frameWidth, row * frameHeight, frameWidth, frameHeight };
                }
                else {
                    sprite = enemyBarbareTexture;
                    float frameWidth;
                    if (enemies[i].state == 1) frameWidth = (float)sprite.width / 8.0f; // Marche : 8 colonnes (utilise 4)
                    else frameWidth = (float)sprite.width / 6.0f; // Attaque/Mort : 6 colonnes (utilise 6)

                    float frameHeight = (float)sprite.height / 4.0f;
                    int row = enemies[i].state;
                    source = (Rectangle){ enemies[i].currentFrame * frameWidth, row * frameHeight, frameWidth, frameHeight };
                }

                // On ajuste la taille visuelle (un peu plus grand que la hitbox pour le style)
                float visualSize = enemies[i].radius * 2.5f; 
                Rectangle dest = { enemies[i].position.x, enemies[i].position.y, visualSize, visualSize };
                Vector2 origin = { visualSize / 2, visualSize / 2 };
                
                // Petit ajustement pour éviter le saignement de pixels (bleeding)
                source.width -= 1.0f;
                source.height -= 1.0f;

                // Orientation vers le joueur (Flip horizontal si le joueur est à gauche)
                if (player.position.x < enemies[i].position.x) {
                    source.width = -source.width;
                }
                
                DrawTexturePro(sprite, source, dest, origin, 0.0f, WHITE);
                
                // Debug Hitbox (optionnel, commenté)
                // DrawCircleLines((int)enemies[i].position.x, (int)enemies[i].position.y, enemies[i].radius, Fade(RED, 0.5f));

                // Barre de vie ennemie (seulement si vivant)
                if (enemies[i].state == 1) {
                    float enemyHpRatio = (float)enemies[i].health / (float)enemies[i].maxHealth;
                    float barWidth = 30.0f;
                    float barHeight = 4.0f;
                    DrawRectangle(enemies[i].position.x - barWidth/2, enemies[i].position.y + enemies[i].radius + 5, barWidth, barHeight, RED);
                    DrawRectangle(enemies[i].position.x - barWidth/2, enemies[i].position.y + enemies[i].radius + 5, barWidth * enemyHpRatio, barHeight, GREEN);
                }
            }
        }
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Draw bullet sprite
                Rectangle source = { 0.0f, 0.0f, (float)bulletTexture.width, (float)bulletTexture.height };
                float visualSize = bullets[i].radius * 4.0f; // Un peu plus grand pour l'effet visuel
                Rectangle dest = { bullets[i].position.x, bullets[i].position.y, visualSize, visualSize };
                Vector2 origin = { visualSize / 2, visualSize / 2 };
                
                // Rotation selon la vélocité
                float rotation = atan2f(bullets[i].velocity.y, bullets[i].velocity.x) * RAD2DEG;
                
                DrawTexturePro(bulletTexture, source, dest, origin, rotation, WHITE);
            }
        }
    }
    
    EndMode2D();

    if (!gameOver) {
        // UI
        DrawText(TextFormat("Lvl: %i", player.level), 10, 10, 20, BLACK);
        DrawText(TextFormat("Score: %i", score), 10, 35, 20, DARKGRAY);
        DrawText(TextFormat("HP: %i/%i", player.health, player.maxHealth), 10, 60, 20, MAROON);
        
        // Barre XP
        DrawRectangle(0, 0, screenWidth * (player.xp / player.xpToNextLevel), 8, LIME);

        if (isLevelingUp) {
            // Fond sombre transparent
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.85f));
            DrawText("LEVEL UP!", screenWidth/2 - MeasureText("LEVEL UP!", 50)/2, 100, 50, GOLD);
            
            Vector2 mousePoint = GetMousePosition();
            for (int i = 0; i < 3; i++) {
                bool hover = CheckCollisionPointRec(mousePoint, upgradeOptions[i].rect);
                Color rarityColor = upgradeOptions[i].color;
                
                // Image
                Texture2D icon = upgradeTextures[upgradeOptions[i].type];
                if (icon.id != 0) {
                    float maxSize = 180.0f;
                    if (hover) maxSize = 200.0f; // Zoom au survol

                    float scale = 1.0f;
                    if (icon.width > 0 && icon.height > 0) {
                        float scaleX = maxSize / icon.width;
                        float scaleY = maxSize / icon.height;
                        scale = (scaleX < scaleY) ? scaleX : scaleY;
                    }
                    
                    Rectangle source = { 0, 0, (float)icon.width, (float)icon.height };
                    Rectangle dest = { 
                        upgradeOptions[i].rect.x + (upgradeOptions[i].rect.width - icon.width * scale) / 2, 
                        upgradeOptions[i].rect.y + (upgradeOptions[i].rect.height - icon.height * scale) / 2 - 20, 
                        icon.width * scale, 
                        icon.height * scale 
                    };
                    DrawTexturePro(icon, source, dest, (Vector2){0,0}, 0.0f, WHITE);
                }

                // Titre
                int textW = MeasureText(upgradeOptions[i].text, 20);
                DrawText(upgradeOptions[i].text, 
                         (int)upgradeOptions[i].rect.x + (upgradeOptions[i].rect.width - textW) / 2, 
                         (int)upgradeOptions[i].rect.y + 230, 
                         20, rarityColor);

                // Description
                int descW = MeasureText(upgradeOptions[i].desc, 18);
                DrawText(upgradeOptions[i].desc, 
                         (int)upgradeOptions[i].rect.x + (upgradeOptions[i].rect.width - descW) / 2, 
                         (int)upgradeOptions[i].rect.y + 260, 
                         18, WHITE);
            }
        }

    } else {
        DrawText("GAME OVER", screenWidth/2 - MeasureText("GAME OVER", 50)/2, screenHeight/2 - 50, 50, MAROON);
        DrawText(TextFormat("Level Reached: %i", player.level), screenWidth/2 - MeasureText(TextFormat("Level Reached: %i", player.level), 20)/2, screenHeight/2 + 10, 20, BLACK);
        DrawText("Press 'R' to restart", screenWidth/2 - MeasureText("Press 'R' to restart", 20)/2, screenHeight/2 + 40, 20, DARKGRAY);
        DrawText("Press 'M' for Menu", screenWidth/2 - MeasureText("Press 'M' for Menu", 20)/2, screenHeight/2 + 70, 20, DARKGRAY);
    }
    // --- SIDEBAR & STATS ---
    int gameWidth = screenWidth - SIDEBAR_WIDTH;

    // Sidebar Background (Transparent)
    DrawRectangle(gameWidth, 0, SIDEBAR_WIDTH, screenHeight, Fade(BLACK, 0.2f));
    // DrawLine(gameWidth, 0, gameWidth, screenHeight, DARKGRAY); // Removed line

    // Stats
    int statX = gameWidth + 20;
    int statY = 20;
    int lineH = 35; // Plus d'espace
    int fontSizeTitle = 25;
    int fontSizeText = 20;
    Color textColor = WHITE;
    Color titleColor = GOLD;
    
    DrawText("--- PLAYER STATS ---", statX, statY, fontSizeTitle, titleColor); statY += lineH + 10;
    DrawText(TextFormat("Damage: %d", player.damage), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Fire Rate: %.2fs", player.fireRate), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Speed: %.0f", player.moveSpeed), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Projectiles: %d", player.projectileCount), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Burst: %d", player.burstCount), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Pierce: %d", player.pierceCount), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Ricochet: %d", player.ricochetCount), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Size: %.1f", player.bulletSize), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Homing: %s", player.hasHomingShots ? "YES" : "NO"), statX, statY, fontSizeText, textColor); statY += lineH;
    
    statY += 20;
    DrawText("--- ENEMY STATS ---", statX, statY, fontSizeTitle, titleColor); statY += lineH + 10;
    DrawText(TextFormat("Spawn Rate: %.2fs", timeToSpawn), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Base HP: %d", 1 + (player.level / 5)), statX, statY, fontSizeText, textColor); statY += lineH;
    DrawText(TextFormat("Base Speed: %.0f", ENEMY_SPEED_BASE + (player.level * 2.0f)), statX, statY, fontSizeText, textColor); statY += lineH;

    statY += 30;
    DrawText("--- COLLECTED ---", statX, statY, fontSizeTitle, textColor); statY += lineH + 10;
    for(int i=0; i<UPGRADE_DEF_COUNT; i++) {
        if (player.upgradeCounts[upgradeDefs[i].type] > 0) {
            DrawText(TextFormat("%s: %d", upgradeDefs[i].title, player.upgradeCounts[upgradeDefs[i].type]), statX, statY, fontSizeText, textColor);
            statY += lineH;
        }
    }

    if (isPaused) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
        DrawText("PAUSED", screenWidth/2 - MeasureText("PAUSED", 40)/2, screenHeight/2 - 20, 40, WHITE);
    }

    EndDrawing();
}

int main() {
    // Fenêtre réduite (pas plein écran)
    InitWindow(1280, 800, "Raylib Survivor - Horde Mode");
    SetExitKey(KEY_NULL); // Désactive la fermeture avec Echap
    SetTargetFPS(60);
    InitResources();
    
    while (!WindowShouldClose()) {
        if (gameState == GAME_MENU) {
            UpdateMenu();
            DrawMenu();
        } else {
            float dt = GetFrameTime();
            UpdateGame(dt);
            DrawGame();
        }
    }
    UnloadTexture(backgroundTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(enemyBarbareTexture);
    UnloadTexture(enemyGobelinTexture);
    UnloadTexture(enemyGeantTexture);
    UnloadTexture(bulletTexture);
    for(int i=0; i<UPGRADE_TYPE_COUNT; i++) {
        UnloadTexture(upgradeTextures[i]);
    }
    CloseWindow();
    return 0;
}