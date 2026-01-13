#include "../include/valentin.h"
#include "etoile.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>
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
static void DessinerJeu(void);

// Variables globales
Vector3 positionJoueur = {0.0f, 0.0f, 0.0f};
int voieJoueurX = 1; // Milieu par défaut
int voieJoueurY = 0;

float positionsXVoies[NB_VOIES_X] = {-4.0f, 0.0f, 4.0f};
float positionsYVoies[NB_VOIES_Y] = {-2.0f, 2.0f};

Obstacle tableauObstacles[MAX_OBSTACLES];
Camera3D cameraJeu = {0};
static bool jeuFini = false;
static bool jeuEnPause = false;
static bool hasGameState = false;        // Pour le resume
static float *valentinGlobalTime = NULL; // Pour affichage

float tempsSurvie = 0.0f;
int niveauDifficulte = 0;

Model vaisseauModel;
bool modelCharge = false;
Model asteroidModel;
bool asteroidModelCharge = false;
static RenderTexture2D renderTarget;

// Initialisation ou remise à zéro du jeu
static void InitGame(void) {
  srand((unsigned int)time(NULL));

  // Configuration de la perspective
  cameraJeu.position = (Vector3){0.0f, 6.0f, 12.0f};
  cameraJeu.target = (Vector3){0.0f, 0.5f, 0.0f};
  cameraJeu.up = (Vector3){0.0f, 1.0f, 0.0f};
  cameraJeu.fovy = 80.0f;
  cameraJeu.projection = CAMERA_PERSPECTIVE;

  voieJoueurX = 1;
  voieJoueurY = 0;
  positionJoueur.x = positionsXVoies[voieJoueurX];
  positionJoueur.y = positionsYVoies[voieJoueurY];
  positionJoueur.z = 0.0f;

  // On désactive tous les obstacles au démarrage
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    tableauObstacles[i].actif = false;
  }

  jeuFini = false;
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

static void ActualiseJoueur(void) {
  bool isMoving = false;
  // Gestion des entrées clavier pour changer de voie
  if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
    voieJoueurX--;
    isMoving = true;
  }
  if (IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D)) {
    voieJoueurX++;
    isMoving = true;
  }
  if (IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_S)) {
    voieJoueurY--;
    isMoving = true;
  }
  if (IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_W)) {
    voieJoueurY++;
    isMoving = true;
  }

  voieJoueurX = LimiteEspace(voieJoueurX, 0, NB_VOIES_X - 1);
  voieJoueurY = LimiteEspace(voieJoueurY, 0, NB_VOIES_Y - 1);

  float cibleX = positionsXVoies[voieJoueurX];
  float cibleY = positionsYVoies[voieJoueurY];

  // Crée un mouvement fluide vers la cible !
  const float facteurLissage = 0.2f;
  positionJoueur.x = Lerp(positionJoueur.x, cibleX, facteurLissage);
  positionJoueur.y = Lerp(positionJoueur.y, cibleY, facteurLissage);

  float dt = GetFrameTime();

  // Logique d'animation
  if (isMoving || fabsf(positionJoueur.x - cibleX) > 0.1f ||
      fabsf(positionJoueur.y - cibleY) > 0.1f) {
    static float frameTimer = 0.0f;
    frameTimer += dt;
    if (frameTimer >= 0.1f) {
      frameTimer = 0.0f;
      static int currentFrame = 1;
      currentFrame++;
      if (currentFrame >= 3)
        currentFrame = 0;
    }
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
      // Si l'astéroïde dépasse le joueur, on le désactive pour le réutiliser
      if (tableauObstacles[i].z_distance >
          positionJoueur.z + tableauObstacles[i].rayon) {
        tableauObstacles[i].actif = false;
      }
    }
  }
}

static void VerifieCollisions(void) {
  float rayonJoueur = 0.5f;

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (tableauObstacles[i].actif) {
      // On ne vérifie la distance que s'ils sont sur la même voie (petite
      // optmisation )
      if (tableauObstacles[i].Voie_X == voieJoueurX &&
          tableauObstacles[i].Voie_Y == voieJoueurY) {
        Vector3 positionAsteroide = {
            positionsXVoies[tableauObstacles[i].Voie_X],
            positionsYVoies[tableauObstacles[i].Voie_Y],
            tableauObstacles[i].z_distance};

        // Collision sphérique : distance < somme des rayons
        float distance = Vector3Distance(positionJoueur, positionAsteroide);
        float distanceMinimale = rayonJoueur + tableauObstacles[i].rayon;

        if (distance < distanceMinimale) {
          jeuFini = true;
        }
      }
    }
  }
}

// Gère le cycle du jeu (Pause, MiseÀjour, Game Over)
static void ActualiseJeu(void) {
  if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
    if (!jeuFini)
      jeuEnPause = !jeuEnPause;
  }

  if (!jeuFini && !jeuEnPause) {
    tempsSurvie += GetFrameTime();

    // Progression de la difficulté par paliers de temps ! (trouver un
    // équilibre)
    int nouveauNiveauDifficulte = (int)(tempsSurvie / TEMPS_PALIER_DIFFICULTE);
    if (nouveauNiveauDifficulte > niveauDifficulte) {
      niveauDifficulte = nouveauNiveauDifficulte;
    }

    ActualiseJoueur();
    ApparitionAsteroide();
    ActualiseAsteroides();
    VerifieCollisions();
    Actualiseretoile(tempsSurvie, niveauDifficulte);
  } else if (jeuFini) {
    if (IsKeyPressed(KEY_ENTER))
      InitGame();
  }
}

static void DessinerJeu(void) {
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

  // Rendu du vaisseau
  if (modelCharge) {
    float echelleVaisseau = 0.004f;
    DrawModelEx(
        vaisseauModel, positionJoueur, (Vector3){0.0f, 1.0f, 0.0f}, 180.0f,
        (Vector3){echelleVaisseau, echelleVaisseau, echelleVaisseau}, WHITE);
  } else {
    // Si le modèle png ne fonctionne pas remplacement par le cube (éphémère à
    // enlever à terme )
    DrawCube(positionJoueur, 1.5f, 1.5f, 2.0f, BLUE);
    DrawCubeWires(positionJoueur, 1.5f, 1.5f, 2.0f, DARKBLUE);
  }

  EndMode3D();

  // INTERFACE utilisateur

  char texteTemps[64];
  sprintf(texteTemps, "Temps: %.1f s | Difficulte: %d", tempsSurvie,
          niveauDifficulte);
  DrawText(texteTemps, 10, 10, 20, WHITE);

  if (jeuEnPause && !jeuFini) {
    DrawRectangle(0, 0, LARGEUR_JEU, HAUTEUR_JEU, (Color){0, 0, 0, 150});
    DrawText("PAUSE", LARGEUR_JEU / 2 - MeasureText("PAUSE", 60) / 2,
             HAUTEUR_JEU / 2 - 80, 60, YELLOW);
  }

  if (jeuFini) {
    DrawRectangle(0, 0, LARGEUR_JEU, HAUTEUR_JEU, (Color){0, 0, 0, 200});
    DrawText("GAME OVER", LARGEUR_JEU / 2 - MeasureText("GAME OVER", 40) / 2,
             HAUTEUR_JEU / 2 - 50, 40, RED);
    DrawText("Appuyez sur ENTREE pour recommencer",
             LARGEUR_JEU / 2 -
                 MeasureText("Appuyez sur ENTREE pour recommencer", 20) / 2,
             HAUTEUR_JEU / 2 + 40, 20, WHITE);
  }

  EndTextureMode();

  // Dessiner la texture centrée sur l'écran
  BeginDrawing();
  ClearBackground(BLACK);
  DrawTextureRec(renderTarget.texture,
                 (Rectangle){0, 0, (float)LARGEUR_JEU, (float)-HAUTEUR_JEU},
                 (Vector2){OFFSET_X, OFFSET_Y}, WHITE);

  // Afficher le temps global sur le côté (dans la zone noire)
  if (valentinGlobalTime != NULL) {
    DrawText("TOTAL TIME", 20, HAUTEUR_FENETRE / 2 - 20, 20, GRAY);
    DrawText(TextFormat("%.1fs", *valentinGlobalTime), 20,
             HAUTEUR_FENETRE / 2 + 5, 30, WHITE);
  }

  EndDrawing();
}

void RunValentinGame(Difficulte difficulty, bool resume, bool *game_active,
                     const char *playerName, ScoreList *scores,
                     float timeout_seconds, float *total_time) {
  valentinGlobalTime = total_time;

  (void)difficulty; // Non utilisé pour l'instant
  (void)playerName;
  (void)scores;

  // Utiliser la taille de fenêtre standard
  SetWindowSize(LARGEUR_FENETRE, HAUTEUR_FENETRE);
  SetWindowTitle("Vaisseau 3D - Esquive d'Astéroïdes");

  // Créer la render texture pour le rendu centré
  renderTarget = LoadRenderTexture(LARGEUR_JEU, HAUTEUR_JEU);

  // Note: Modèles OBJ désactivés car problèmes de compatibilité avec le loader
  // Le jeu utilise DrawCube() et DrawSphere() comme fallbacks
  modelCharge = false;
  asteroidModelCharge = false;

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

    // Accumuler le temps dans le temps global (si disponible)
    if (total_time != NULL) {
      *total_time += dt;
    }

    // Vérifier timeout (si > 0)
    if (timeout_seconds > 0 && elapsed_time >= timeout_seconds) {
      break; // Sortir automatiquement après le timeout
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

    // Si le joueur est mort, permettre de quitter
    if (jeuFini) {
      int key = GetKeyPressed();
      if (key != 0) {
        *game_active = false;
        break;
      }
    }

    ActualiseJeu();
    DessinerJeu();
  }

  // Déchargement de la mémoire !
  UnloadRenderTexture(renderTarget);
  if (modelCharge)
    UnloadModel(vaisseauModel);
  if (asteroidModelCharge)
    UnloadModel(asteroidModel);

  // Si le joueur est mort, réinitialiser l'état pour la prochaine partie
  if (jeuFini) {
    hasGameState = false;
  }

  // Note: game_active n'est pas mis à false ici pour permettre la rotation
  // Il est set à false seulement si ESC est pressé ou si le joueur meurt
}