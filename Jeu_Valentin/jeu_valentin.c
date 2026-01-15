#include "../include/valentin.h"
#include "../include/player_input.h"
#include "etoile.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// --- CONFIGURATION ET ÉQUILIBRAGE ---
#define LARGEUR_JEU 800
#define HAUTEUR_JEU 600
#define LARGEUR_FENETRE 1280
#define HAUTEUR_FENETRE 720
#define OFFSET_X ((LARGEUR_FENETRE - LARGEUR_JEU) / 2)
#define OFFSET_Y ((HAUTEUR_FENETRE - HAUTEUR_JEU) / 2)
#define NB_VOIES_X 3
#define NB_VOIES_Y 2
#define MAX_OBSTACLES 30
#define VITESSE_ASTEROIDE 0.50f
#define Z_INIT_ASTEROIDE -50.0f // Point de départ des astéroïdes au loin
#define RAYON_ASTEROIDE 1.0f

#define TAUX_APPARITION_BASE 0.35f
#define TEMPS_PALIER_DIFFICULTE 5.0f
#define MULTIPLIER_DIFFICULTE 0.85f

// --- STRUCTURES DE DONNÉES ---
typedef struct Obstacle {
  float z_distance; // Position sur l'axe de profondeur
  int Voie_X;       // Index du couloir  horizontale
  int Voie_Y;       // Index du couloir  verticale
  bool actif;
  float rayon;
  Color couleur;
} Obstacle;

typedef struct {
  Rectangle frameRec;
  int currentFrame;
  float frameTimer;
} PlayerVisuals;

// Prototypes
static int LimiteEspace(int valeur, int min, int max);
static void InitGame(void);
static void ActualiseJoueur(void);
static void ApparitionAsteroide(void);
static void ActualiseAsteroides(void);
static void VerifieCollisions(void);
static void ActualiseJeu(void);
static void DessinerJeu(float scale, Vector2 shake_offset);

// Variables globales - Joueur 1
Vector3 positionJoueur1 = {0.0f, 0.0f, 0.0f};
int voieJoueur1X = 1;
int voieJoueur1Y = 0;
static bool jeuFini1 = false;

// Variables globales - Joueur 2
Vector3 positionJoueur2 = {0.0f, 0.0f, 0.0f};
int voieJoueur2X = 1;
int voieJoueur2Y = 1;
static bool jeuFini2 = false;

// Mode de jeu
static ModeJoueur modeActuel = UN_JOUEUR;
static int winner = 0;

float positionsXVoies[NB_VOIES_X] = {-4.0f, 0.0f, 4.0f};
float positionsYVoies[NB_VOIES_Y] = {-2.0f, 2.0f};

Obstacle tableauObstacles[MAX_OBSTACLES];
Camera3D cameraJeu = {0};
static bool jeuEnPause = false;
static bool hasGameState = false;
static float *valentinGlobalTime = NULL;

float tempsSurvie = 0.0f;
int niveauDifficulte = 0;

Model vaisseauModel;
bool modelCharge = false;
Model asteroidModel;
bool asteroidModelCharge = false;
static RenderTexture2D renderTarget;

// Initialisation ou remise a zero du jeu
static void InitGame(void) {
  srand((unsigned int)time(NULL));

  // Configuration de la perspective
  cameraJeu.position = (Vector3){0.0f, 6.0f, 12.0f};
  cameraJeu.target = (Vector3){0.0f, 0.5f, 0.0f};
  cameraJeu.up = (Vector3){0.0f, 1.0f, 0.0f};
  cameraJeu.fovy = 80.0f;
  cameraJeu.projection = CAMERA_PERSPECTIVE;

  // Joueur 1 - voie gauche/bas par defaut
  voieJoueur1X = 0;
  voieJoueur1Y = 0;
  positionJoueur1.x = positionsXVoies[voieJoueur1X];
  positionJoueur1.y = positionsYVoies[voieJoueur1Y];
  positionJoueur1.z = 0.0f;
  jeuFini1 = false;

  // Joueur 2 - voie droite/haut par defaut (mode 2 joueurs)
  voieJoueur2X = 2;
  voieJoueur2Y = 1;
  positionJoueur2.x = positionsXVoies[voieJoueur2X];
  positionJoueur2.y = positionsYVoies[voieJoueur2Y];
  positionJoueur2.z = 0.0f;
  jeuFini2 = false;

  winner = 0;

  // On desactive tous les obstacles au demarrage
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    tableauObstacles[i].actif = false;
  }

  jeuEnPause = false;
  tempsSurvie = 0.0f;
  niveauDifficulte = 0;

  Initialiseretoile(LARGEUR_JEU, HAUTEUR_JEU);
}

// Définition des bordures, permet de limiter les mouvements du joueur
static int LimiteEspace(int valeur, int min, int max) {
  if (valeur < min)
    return min;
  if (valeur > max)
    return max;
  return valeur;
}

// Mise a jour pour un joueur specifique
static void ActualiseJoueurIndividuel(int player_id) {
  Vector3 *position;
  int *voieX, *voieY;
  bool *jeuFini_joueur;

  if (player_id == 1) {
    position = &positionJoueur1;
    voieX = &voieJoueur1X;
    voieY = &voieJoueur1Y;
    jeuFini_joueur = &jeuFini1;
  } else {
    position = &positionJoueur2;
    voieX = &voieJoueur2X;
    voieY = &voieJoueur2Y;
    jeuFini_joueur = &jeuFini2;
  }

  if (*jeuFini_joueur) return;

  bool isMoving = false;

  if (player_id == 1) {
    // Joueur 1: ZQSD
    if (IS_P1_LEFT_PRESSED()) { (*voieX)--; isMoving = true; }
    if (IS_P1_RIGHT_PRESSED()) { (*voieX)++; isMoving = true; }
    if (IS_P1_DOWN_PRESSED()) { (*voieY)--; isMoving = true; }
    if (IS_P1_UP_PRESSED()) { (*voieY)++; isMoving = true; }
  } else {
    // Joueur 2: Fleches
    if (IS_P2_LEFT_PRESSED()) { (*voieX)--; isMoving = true; }
    if (IS_P2_RIGHT_PRESSED()) { (*voieX)++; isMoving = true; }
    if (IS_P2_DOWN_PRESSED()) { (*voieY)--; isMoving = true; }
    if (IS_P2_UP_PRESSED()) { (*voieY)++; isMoving = true; }
  }

  *voieX = LimiteEspace(*voieX, 0, NB_VOIES_X - 1);
  *voieY = LimiteEspace(*voieY, 0, NB_VOIES_Y - 1);

  float cibleX = positionsXVoies[*voieX];
  float cibleY = positionsYVoies[*voieY];

  const float facteurLissage = 0.2f;
  position->x = Lerp(position->x, cibleX, facteurLissage);
  position->y = Lerp(position->y, cibleY, facteurLissage);

  (void)isMoving; // Animation supprimee pour simplifier
}

static void ActualiseJoueur(void) {
  ActualiseJoueurIndividuel(1);
  if (modeActuel == DEUX_JOUEURS) {
    ActualiseJoueurIndividuel(2);
  }
}

static void ApparitionAsteroide(void) {
  static float minuterie = 0.0f;

  // Calcul de la difficulté
  float tauxApparition = TAUX_APPARITION_BASE;
  for (int i = 0; i < niveauDifficulte; i++) {
    tauxApparition *= MULTIPLIER_DIFFICULTE;
  }
  if (tauxApparition < 0.05f)
    tauxApparition = 0.05f;

  minuterie += GetFrameTime();

  if (minuterie >= tauxApparition) {
    minuterie = 0.0f;

    int obstaclesSurMemeLigne = 0;
    float margeZ = 5.0f;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (tableauObstacles[i].actif) {
        if (fabsf(tableauObstacles[i].z_distance - Z_INIT_ASTEROIDE) < margeZ) {
          obstaclesSurMemeLigne++;
        }
      }
    }

    // Si 5 positions sur 6 sont occupées, on bloque l'apparition du 6ème
    if (obstaclesSurMemeLigne >= (NB_VOIES_X * NB_VOIES_Y) - 1)
      return;

    // Apparition de l'obstacle
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (!tableauObstacles[i].actif) {
        tableauObstacles[i].z_distance = Z_INIT_ASTEROIDE;
        tableauObstacles[i].Voie_X = rand() % NB_VOIES_X;
        tableauObstacles[i].Voie_Y = rand() % NB_VOIES_Y;
        tableauObstacles[i].actif = true;
        tableauObstacles[i].rayon = RAYON_ASTEROIDE;

        // couleur des astéroides
        int choixCouleur = rand() % 4;
        switch (choixCouleur) {
        case 0:
          tableauObstacles[i].couleur = WHITE;
          break;
        case 1:
          tableauObstacles[i].couleur = LIGHTGRAY;
          break;
        case 2:
          tableauObstacles[i].couleur = RAYWHITE;
          break;
        case 3:
          tableauObstacles[i].couleur = SKYBLUE;
          break;
        }

        return;
      }
    }
  }
}

static void ActualiseAsteroides(void) {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (tableauObstacles[i].actif) {
      // Les astéroïdes avancent vers le joueur (z augmente)
      tableauObstacles[i].z_distance += VITESSE_ASTEROIDE;
      // Si l'asteroide depasse le joueur, on le desactive pour le reutiliser
      if (tableauObstacles[i].z_distance >
          positionJoueur1.z + tableauObstacles[i].rayon) {
        tableauObstacles[i].actif = false;
      }
    }
  }
}

static void VerifieCollisions(void) {
  float rayonJoueur = 0.5f;

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (tableauObstacles[i].actif) {
      Vector3 positionAsteroide = {
          positionsXVoies[tableauObstacles[i].Voie_X],
          positionsYVoies[tableauObstacles[i].Voie_Y],
          tableauObstacles[i].z_distance};

      // Collision Joueur 1
      if (!jeuFini1 && tableauObstacles[i].Voie_X == voieJoueur1X &&
          tableauObstacles[i].Voie_Y == voieJoueur1Y) {
        float distance = Vector3Distance(positionJoueur1, positionAsteroide);
        float distanceMinimale = rayonJoueur + tableauObstacles[i].rayon;
        if (distance < distanceMinimale) {
          jeuFini1 = true;
          if (modeActuel == DEUX_JOUEURS && !jeuFini2) {
            winner = 2;
          }
        }
      }

      // Collision Joueur 2 (mode 2 joueurs)
      if (modeActuel == DEUX_JOUEURS && !jeuFini2 &&
          tableauObstacles[i].Voie_X == voieJoueur2X &&
          tableauObstacles[i].Voie_Y == voieJoueur2Y) {
        float distance = Vector3Distance(positionJoueur2, positionAsteroide);
        float distanceMinimale = rayonJoueur + tableauObstacles[i].rayon;
        if (distance < distanceMinimale) {
          jeuFini2 = true;
          if (!jeuFini1) {
            winner = 1;
          }
        }
      }
    }
  }

  // Verifier egalite (les deux morts en meme temps)
  if (modeActuel == DEUX_JOUEURS && jeuFini1 && jeuFini2 && winner == 0) {
    winner = -1; // Egalite
  }
}

// Gere le cycle du jeu (Pause, MiseAjour, Game Over)
static void ActualiseJeu(void) {
  // Determiner si le jeu est termine
  bool game_over;
  if (modeActuel == DEUX_JOUEURS) {
    game_over = (winner != 0);
  } else {
    game_over = jeuFini1;
  }

  if (!game_over) {
    tempsSurvie += GetFrameTime();

    int nouveauNiveauDifficulte = (int)(tempsSurvie / TEMPS_PALIER_DIFFICULTE);
    if (nouveauNiveauDifficulte > niveauDifficulte) {
      niveauDifficulte = nouveauNiveauDifficulte;
    }

    ActualiseJoueur();
    ApparitionAsteroide();
    ActualiseAsteroides();
    VerifieCollisions();
    Actualiseretoile(tempsSurvie, niveauDifficulte);
  }
}

static void DessinerJeu(float scale, Vector2 shake_offset) {
  // Rendu dans la texture à la taille native du jeu
  BeginTextureMode(renderTarget);
  ClearBackground(BLACK);
  Dessineretoile(); // Fond étoilé 2D ( géré par etoile.h)

  BeginMode3D(cameraJeu);

  // Rendu des astéroïdes
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (tableauObstacles[i].actif) {
      Vector3 positionDessin = {positionsXVoies[tableauObstacles[i].Voie_X],
                                positionsYVoies[tableauObstacles[i].Voie_Y],
                                tableauObstacles[i].z_distance};

      if (asteroidModelCharge) {
        float echelleAsteroide = 2.0f;
        float angle = (float)(GetTime() * 50.0); // Rotation continue
        DrawModelEx(
            asteroidModel, positionDessin, (Vector3){0.0f, 1.0f, 0.0f}, angle,
            (Vector3){echelleAsteroide, echelleAsteroide, echelleAsteroide},
            tableauObstacles[i].couleur);
      } else {
        DrawSphere(positionDessin, tableauObstacles[i].rayon,
                   tableauObstacles[i].couleur);
      }
    }
  }

  // Rendu du vaisseau Joueur 1 (si vivant)
  if (!jeuFini1) {
    Color couleur1 = (modeActuel == DEUX_JOUEURS) ? (Color){100, 180, 255, 255} : WHITE;
    if (modelCharge) {
      float echelleVaisseau = 0.005f;
      DrawModelEx(
          vaisseauModel, positionJoueur1, (Vector3){0.0f, 1.0f, 0.0f}, 180.0f,
          (Vector3){echelleVaisseau, echelleVaisseau, echelleVaisseau}, couleur1);
    } else {
      DrawCube(positionJoueur1, 1.5f, 1.5f, 2.0f, BLUE);
      DrawCubeWires(positionJoueur1, 1.5f, 1.5f, 2.0f, DARKBLUE);
    }
  }

  // Rendu du vaisseau Joueur 2 (mode 2 joueurs, si vivant)
  if (modeActuel == DEUX_JOUEURS && !jeuFini2) {
    Color couleur2 = (Color){255, 120, 120, 255};
    if (modelCharge) {
      float echelleVaisseau = 0.005f;
      DrawModelEx(
          vaisseauModel, positionJoueur2, (Vector3){0.0f, 1.0f, 0.0f}, 180.0f,
          (Vector3){echelleVaisseau, echelleVaisseau, echelleVaisseau}, couleur2);
    } else {
      DrawCube(positionJoueur2, 1.5f, 1.5f, 2.0f, RED);
      DrawCubeWires(positionJoueur2, 1.5f, 1.5f, 2.0f, MAROON);
    }
  }

  EndMode3D();

  // INTERFACE utilisateur
  char texteTemps[64];
  sprintf(texteTemps, "Temps: %.1f s | Difficulte: %d", tempsSurvie,
          niveauDifficulte);
  DrawText(texteTemps, 10, 10, 20, WHITE);

  // Labels joueurs en mode 2 joueurs
  if (modeActuel == DEUX_JOUEURS) {
    DrawText("P1: ZQSD", 10, 35, 16, (Color){100, 180, 255, 255});
    DrawText("P2: Fleches", 10, 55, 16, (Color){255, 120, 120, 255});
  }

  // Determiner si game over
  bool game_over = (modeActuel == DEUX_JOUEURS) ? (winner != 0) : jeuFini1;

  if (jeuEnPause && !game_over) {
    DrawRectangle(0, 0, LARGEUR_JEU, HAUTEUR_JEU, (Color){0, 0, 0, 150});
    DrawText("PAUSE", LARGEUR_JEU / 2 - MeasureText("PAUSE", 60) / 2,
             HAUTEUR_JEU / 2 - 80, 60, YELLOW);
  }

  if (game_over) {
    DrawRectangle(0, 0, LARGEUR_JEU, HAUTEUR_JEU, (Color){0, 0, 0, 200});

    if (modeActuel == DEUX_JOUEURS && winner == -1) {
      // Egalite
      const char *tie_text = "EGALITE!";
      DrawText(tie_text, LARGEUR_JEU / 2 - MeasureText(tie_text, 40) / 2,
               HAUTEUR_JEU / 2 - 50, 40, YELLOW);
    } else if (modeActuel == DEUX_JOUEURS && (winner == 1 || winner == 2)) {
      const char *winner_text = (winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
      Color winner_color = (winner == 1) ? (Color){100, 180, 255, 255} : (Color){255, 120, 120, 255};
      DrawText(winner_text, LARGEUR_JEU / 2 - MeasureText(winner_text, 40) / 2,
               HAUTEUR_JEU / 2 - 50, 40, winner_color);
    } else {
      DrawText("GAME OVER", LARGEUR_JEU / 2 - MeasureText("GAME OVER", 40) / 2,
               HAUTEUR_JEU / 2 - 50, 40, RED);
    }
    DrawText("ESCAPE: Menu",
             LARGEUR_JEU / 2 - MeasureText("ESCAPE: Menu", 20) / 2,
             HAUTEUR_JEU / 2 + 40, 20, WHITE);
  }

  EndTextureMode();

  // Dessiner la texture centrée sur l'écran AVEC ZOOM ET SHAKE
  BeginDrawing();
  ClearBackground(BLACK);

  // Calcul du rectangle de destination centré et mis à l'échelle
  float destWidth = LARGEUR_JEU * scale;
  float destHeight = HAUTEUR_JEU * scale;
  float destX = (LARGEUR_FENETRE - destWidth) / 2.0f + shake_offset.x;
  float destY = (HAUTEUR_FENETRE - destHeight) / 2.0f + shake_offset.y;

  // Source (taille réelle du jeu, y négatif pour Render Texture Raylib)
  Rectangle sourceRec = {0, 0, (float)LARGEUR_JEU, (float)-HAUTEUR_JEU};
  Rectangle destRec = {destX, destY, destWidth, destHeight};
  Vector2 origin = {0, 0};

  DrawTexturePro(renderTarget.texture, sourceRec, destRec, origin, 0.0f, WHITE);

  // Afficher le temps global sur le côté (dans la zone noire)
  if (valentinGlobalTime != NULL && scale > 0.8f) {
    DrawText("TOTAL TIME", 20, HAUTEUR_FENETRE / 2 - 20, 20, GRAY);
    DrawText(TextFormat("%.1fs", *valentinGlobalTime), 20,
             HAUTEUR_FENETRE / 2 + 5, 30, WHITE);
  }

  EndDrawing();
}

void RunValentinGame(Difficulte difficulty, ModeJoueur mode, bool resume,
                     bool *game_active, const char *playerName,
                     ScoreList *scores, float timeout_seconds,
                     float *total_time, float start_delay) {
  valentinGlobalTime = total_time;
  modeActuel = mode;

  (void)difficulty; // Non utilisé pour l'instant
  (void)playerName;
  (void)scores;

  // Utiliser la taille de fenêtre standard
  SetWindowSize(LARGEUR_FENETRE, HAUTEUR_FENETRE);
  SetWindowTitle("Vaisseau 3D - Esquive d'Astéroïdes");

  // Créer la render texture pour le rendu centré
  renderTarget = LoadRenderTexture(LARGEUR_JEU, HAUTEUR_JEU);

  // Chargement des modèles 3D
  vaisseauModel = LoadModel("Jeu_Valentin/Spaceship.obj");
  if (vaisseauModel.meshCount > 0) {
    modelCharge = true;
  } else {
    TraceLog(LOG_WARNING, "Impossible de charger le modele du vaisseau !");
    modelCharge = false;
  }

  asteroidModel = LoadModel("Jeu_Valentin/Asteroid.obj");
  if (asteroidModel.meshCount > 0) {
    asteroidModelCharge = true;
  } else {
    TraceLog(LOG_WARNING, "Impossible de charger le modele de l'asteroide !");
    asteroidModelCharge = false;
  }

  // Resume: ne pas réinitialiser si on reprend une partie
  if (!resume || !hasGameState) {
    InitGame();
    hasGameState = true;
  }

  *game_active = true;
  float elapsed_time = 0.0f; // Timer pour le timeout

  // Boucle principale du jeu
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    elapsed_time += dt;

    // Determiner si le jeu est termine (selon le mode)
    bool jeuFini = (modeActuel == DEUX_JOUEURS) ? (winner != 0) : jeuFini1;

    // Accumuler le temps dans le temps global (si disponible)
    if (total_time != NULL && !jeuFini) {
      *total_time += dt;
    }

    // Verifier timeout (si > 0)
    if (!jeuFini && timeout_seconds > 0 && elapsed_time >= timeout_seconds) {
      break; // Sortir automatiquement apres le timeout
    }

    // Retour au menu avec Escape
    if (IsKeyPressed(KEY_ESCAPE)) {
      // Attendre que la touche soit relâchée
      while (IsKeyDown(KEY_ESCAPE)) {
        BeginDrawing();
        EndDrawing();
      }
      *game_active = false; // Signaler que le joueur veut quitter
      break;
    }

    // Si le jeu est fini, gestion R (Rejouer) / ESPACE (Quitter)
    if (jeuFini) {
      // Only Escape allows to quit, handled at the top of the loop
    }

    // Gestion des transitions (Zoom/Shake)
    float current_scale = 1.0f;
    Vector2 shake_transition = {0.0f, 0.0f};

    // 1. Zoom In au début (pendant start_delay)
    if (elapsed_time < start_delay) {
      float factor = elapsed_time / start_delay;
      current_scale = Lerp(0.1f, 1.0f, factor);
    }
    // 2. Shake + Zoom Out à la fin (0.5s avant fin)
    else if (!jeuFini && timeout_seconds > 0 &&
             (timeout_seconds - elapsed_time) < 0.5f) {
      float remaining = timeout_seconds - elapsed_time;
      float factor = remaining / 0.5f;          // Va de 1.0 -> 0.0
      current_scale = Lerp(0.1f, 1.0f, factor); // Zoom out

      // Shake scale
      float shake_amount = (1.0f - factor) * 10.0f;
      shake_transition.x =
          (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
      shake_transition.y =
          (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
    }

    // Mise à jour logic SEULEMENT si pas pause intro
    if (elapsed_time >= start_delay) {
      ActualiseJeu();
    }

    DessinerJeu(current_scale, shake_transition);
  }

  // Déchargement de la mémoire !
  UnloadRenderTexture(renderTarget);
  if (modelCharge)
    UnloadModel(vaisseauModel);
  if (asteroidModelCharge)
    UnloadModel(asteroidModel);

  // Si le joueur est mort, reinitialiser l'etat pour la prochaine partie
  bool jeuTermine = (modeActuel == DEUX_JOUEURS) ? (winner != 0) : jeuFini1;
  if (jeuTermine) {
    hasGameState = false;
  }

  // Note: game_active n'est pas mis à false ici pour permettre la rotation
  // Il est set à false seulement si ESC est pressé ou si le joueur meurt
}