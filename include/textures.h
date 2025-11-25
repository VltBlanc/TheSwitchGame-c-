#ifndef TEXTURES_H
#define TEXTURES_H

#include "jeuhugo.h" // For UPGRADE_TYPE_COUNT
#include "raylib.h"


// --- GLOBAL TEXTURES ---
extern Texture2D backgroundTexture;
extern Texture2D playerTexture;
extern Texture2D enemyBarbareTexture;
extern Texture2D enemyGobelinTexture;
extern Texture2D enemyGeantTexture;
extern Texture2D bulletTexture;
extern Texture2D upgradeTextures[UPGRADE_TYPE_COUNT];

// --- FUNCTIONS ---
void InitResources();
void UnloadResources();

#endif // TEXTURES_H
