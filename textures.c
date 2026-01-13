#include "include/textures.h"
#include <stdio.h>

// --- GLOBAL TEXTURES DEFINITION ---
Texture2D backgroundTexture;
Texture2D playerTexture;
Texture2D enemyBarbareTexture;
Texture2D enemyGobelinTexture;
Texture2D enemyGeantTexture;
Texture2D bulletTexture;
Texture2D upgradeTextures[UPGRADE_TYPE_COUNT];

// --- INTERNAL HELPER ---
Texture2D LoadGameTexture(const char *filename) {
  // 1. Essayer "assets/filename" (Exécution depuis la racine)
  const char *pathRoot = TextFormat("assets/%s", filename);
  if (FileExists(pathRoot))
    return LoadTexture(pathRoot);

  // 2. Essayer "../assets/filename" (Exécution depuis bin/)
  const char *pathBin = TextFormat("../assets/%s", filename);
  if (FileExists(pathBin))
    return LoadTexture(pathBin);

  // 3. Fallback (Exécution depuis assets/ ou autre)
  return LoadTexture(filename);
}

// --- PUBLIC FUNCTIONS ---
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
  if (upgradeTextures[UPGRADE_FIRE_RATE].id == 0)
    upgradeTextures[UPGRADE_FIRE_RATE] = LoadGameTexture("minigun.png");
  if (upgradeTextures[UPGRADE_MOVE_SPEED].id == 0)
    upgradeTextures[UPGRADE_MOVE_SPEED] = LoadGameTexture("bottes.png");
  if (upgradeTextures[UPGRADE_MULTI_SHOT].id == 0)
    upgradeTextures[UPGRADE_MULTI_SHOT] = LoadGameTexture("multi-tir.png");
  if (upgradeTextures[UPGRADE_BURST_SHOT].id == 0)
    upgradeTextures[UPGRADE_BURST_SHOT] = LoadGameTexture("rafale.png");
  if (upgradeTextures[UPGRADE_DAMAGE].id == 0)
    upgradeTextures[UPGRADE_DAMAGE] = LoadGameTexture("puissance.png");
  if (upgradeTextures[UPGRADE_PIERCING].id == 0)
    upgradeTextures[UPGRADE_PIERCING] = LoadGameTexture("percant.png");
  if (upgradeTextures[UPGRADE_BULLET_SIZE].id == 0)
    upgradeTextures[UPGRADE_BULLET_SIZE] = LoadGameTexture("gros_calibre.png");
  if (upgradeTextures[UPGRADE_HEALTH].id == 0)
    upgradeTextures[UPGRADE_HEALTH] = LoadGameTexture("vitalite.png");
  if (upgradeTextures[UPGRADE_RICOCHET].id == 0)
    upgradeTextures[UPGRADE_RICOCHET] = LoadGameTexture("rebond.png");
  if (upgradeTextures[UPGRADE_HOMING_SHOTS].id == 0)
    upgradeTextures[UPGRADE_HOMING_SHOTS] = LoadGameTexture("teleguide.png");
}

void UnloadResources() {
  UnloadTexture(backgroundTexture);
  UnloadTexture(playerTexture);
  UnloadTexture(enemyBarbareTexture);
  UnloadTexture(enemyGobelinTexture);
  UnloadTexture(enemyGeantTexture);
  UnloadTexture(bulletTexture);
  for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
    UnloadTexture(upgradeTextures[i]);
  }
}
