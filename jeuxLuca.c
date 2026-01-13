#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/luca.h"

//---VARIABLE---

#define Taille_Grid 10
#define Taille_Case 60
#define Taille_Marge_Grille 200

#define Demi_Cote_Joueur_X 14
#define Demi_Cote_Joueur_Y 19
#define HITBOX_PIEDS_HAUTEUR 8.0f
#define HITBOX_PIEDS_LARGEUR 19.0f

#define Vitesse_Marche 200.0f
#define CoefCourse 2.0f
#define Distance_Case_Dash 1.0f

const int screenWidth = Taille_Marge_Grille * 2 + Taille_Grid * Taille_Case;
const int screenHeight = Taille_Marge_Grille * 2 + Taille_Grid * Taille_Case;
const int Debut_Grille_X = Taille_Marge_Grille;
const int Debut_Grille_Y = Taille_Marge_Grille;

// Dimensions de la fenêtre standard
#define LARGEUR_FENETRE 1280
#define HAUTEUR_FENETRE 720
// Centrer horizontalement, mais décaler verticalement pour montrer la zone de
// jeu (le jeu est 1000x1000, la fenêtre est 720 de haut, donc on clip 140px en
// haut et 140px en bas)
static int offsetX;
static int offsetY;
static RenderTexture2D lucaRenderTarget;

// etats possibles pour chaque case
typedef enum {
  Block_Normal = 0,
  Block_Alerte = 1,
  Block_Tomber = 2,
  Block_Prochain_Cycle = 3
} Block;

// phases possibles
typedef enum { Phase_Attente = 0, Phase_Alerte = 1, Phase_Tomber = 2 } Phase;

typedef struct Joueur {
  Vector2 position;
  bool vivant;
  Vector2 dernierMouvement;
} Joueur;

Phase phaseActuelle = Phase_Attente;
Block **grid = NULL; // initialisation du tableau a Null
float timer = 0.0f;
const float Temps_Alerte = 0.6f; // temps en secondes pour l'avertissement
const float Temps_Tomber = 0.3f;
const float Temps_Attente = 0.2f; // temps en secondes avant prochaine phase
int nbrBlockQuiTombe = 8;
static bool nbrBlockDoitAugmenter =
    true; // mis a true pour faire un premier cycle
          // sans augmenter le nombre de block
static bool jeuEnPause = false;

// Variables pour le resume (état sauvegardé entre les rotations)
static Joueur savedJoueur;
static bool hasGameState = false;

Texture2D playerTexture;
Texture2D fondTexture;
Texture2D solTexture;
Texture2D solFissureTexture;

//---PROTOTYPES---
// utile pour écrire les fonctions dans l'ordre qu'on veut
void InitGrid();
void FreeGrid();
void ResetGridState();
Joueur InitJoueur();
void UpdateJoueurMouvement(Joueur *Joueur, float deltaTime);
void DashJoueur(Joueur *Joueur);
void VerifieMort(Joueur *Joueur);
void UpdateGridPhase(Joueur *Joueur, float deltaTime);
void DessinePartie();
void DessineJoueur(Joueur Joueur);
void JeuLuca(bool *game_active, float timeout_seconds, bool resume,
             float *total_time);

//---FONCTIONS---
void InitGrid() {
  grid = (Block **)malloc(Taille_Grid * sizeof(Block *));
  if (grid == NULL) {
    printf("Erreur malloc lignes initGrid\n");
    exit(1);
  }

  for (int x = 0; x < Taille_Grid; x++) {
    grid[x] = (Block *)malloc(Taille_Grid * sizeof(Block));

    if (grid[x] == NULL) {
      printf("Erreur malloc cases initGrid\n");
      FreeGrid();
      exit(1);
    }
  }

  for (int y = 0; y < Taille_Grid; y++) {
    for (int x = 0; x < Taille_Grid; x++) {
      grid[x][y] = Block_Normal;
    }
  }
}

void FreeGrid() {
  if (grid != NULL) {
    // Libere chaque ligne
    for (int x = 0; x < Taille_Grid; x++) {
      if (grid[x] != NULL) {
        free(grid[x]);
        grid[x] = NULL;
      }
    }
    // libere le tableau de pointeurs
    free(grid);
    grid = NULL;
  }
}

void ResetGridState() {
  for (int y = 0; y < Taille_Grid; y++) {
    for (int x = 0; x < Taille_Grid; x++) {
      grid[x][y] = Block_Normal;
    }
  }
}

Joueur InitJoueur() {
  Joueur Joueur;
  Joueur.position.x = screenWidth / 2;
  Joueur.position.y = screenHeight / 2;
  Joueur.vivant = true;
  Joueur.dernierMouvement = (Vector2){0.0f, 0.0f}; // utile pour le dash
  return Joueur;
}

void DashJoueur(Joueur *Joueur) {
  if (!Joueur->vivant) {
    return;
  }

  float DashX = Joueur->dernierMouvement.x * Distance_Case_Dash * Taille_Case;
  float DashY = Joueur->dernierMouvement.y * Distance_Case_Dash * Taille_Case;

  Joueur->position.x += DashX;
  Joueur->position.y += DashY;
}

void UpdateJoueurMouvement(Joueur *Joueur, float deltaTime) {
  if (!Joueur->vivant) {
    return;
  }

  Vector2 mouvement = {0.0f, 0.0f};

  if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
    mouvement.x += 1.0f;
  if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
    mouvement.x -= 1.0f;
  if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
    mouvement.y += 1.0f;
  if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
    mouvement.y -= 1.0f;

  if (mouvement.x != 0.0f || mouvement.y != 0.0f) {
    Joueur->dernierMouvement = Vector2Normalize(mouvement);
    mouvement = Joueur->dernierMouvement;
  }

  float vitesseActuel = Vitesse_Marche;
  if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
    vitesseActuel *= CoefCourse;
  }

  Joueur->position.x += mouvement.x * vitesseActuel * deltaTime;
  Joueur->position.y += mouvement.y * vitesseActuel * deltaTime;

  if (IsKeyPressed(KEY_SPACE)) {
    DashJoueur(Joueur);
  }
}

void VerifieMort(Joueur *Joueur) {
  if (!Joueur->vivant) {
    return;
  }

  // Hitbox autour des pieds
  float HitboxBas = Joueur->position.y + Demi_Cote_Joueur_Y;
  float HitboxHaut = HitboxBas - HITBOX_PIEDS_HAUTEUR;
  float HitboxGauche = Joueur->position.x - (HITBOX_PIEDS_LARGEUR / 2.0f);

  // rectangle = (poisition x haut gauche,poisition y haut
  // gauche,longueur,largeur)
  Rectangle JoueurRec = {HitboxGauche, HitboxHaut, HITBOX_PIEDS_LARGEUR,
                         HITBOX_PIEDS_HAUTEUR};

  // Vérification chute de la grille
  bool horsGrilleX = (JoueurRec.x < Debut_Grille_X) ||
                     (JoueurRec.x + JoueurRec.width >
                      (Debut_Grille_X + Taille_Grid * Taille_Case));
  bool horsGrilleY = (JoueurRec.y < Debut_Grille_Y) ||
                     (JoueurRec.y + JoueurRec.height >
                      (Debut_Grille_Y + Taille_Grid * Taille_Case));

  if (horsGrilleX || horsGrilleY) {
    Joueur->vivant = false;
    return;
  }

  // converit la hitbox joueur en l'ensemble des cases qu'il touche
  //  et on vérifie qu'on ne sort pas du tableau

  int minGridX =
      (int)floor((JoueurRec.x - Debut_Grille_X) / (float)Taille_Case);
  if (minGridX < 0) {
    minGridX = 0;
  }

  int maxGridX = (int)floor(((JoueurRec.x + JoueurRec.width) - Debut_Grille_X) /
                            (float)Taille_Case);
  if (maxGridX >= Taille_Grid) {
    maxGridX = Taille_Grid - 1;
  }

  int minGridY =
      (int)floor((JoueurRec.y - Debut_Grille_Y) / (float)Taille_Case);
  if (minGridY < 0) {
    minGridY = 0;
  }

  int maxGridY = (int)floor(
      ((JoueurRec.y + JoueurRec.height) - Debut_Grille_Y) / (float)Taille_Case);
  if (maxGridY >= Taille_Grid) {
    maxGridY = Taille_Grid - 1;
  }

  // vérifie collision hitbox-Rectangle
  for (int y = minGridY; y <= maxGridY; y++) {
    for (int x = minGridX; x <= maxGridX; x++) {

      if (grid[x][y] == Block_Tomber) {

        Rectangle Case = {Debut_Grille_X + x * Taille_Case,
                          Debut_Grille_Y + y * Taille_Case, Taille_Case,
                          Taille_Case};

        //(centre cercle,rayon,rectangle)
        if (CheckCollisionRecs(JoueurRec, Case)) {
          Joueur->vivant = false;
          return;
        }
      }
    }
  }
}

void UpdateGridPhase(Joueur *Joueur, float deltaTime) {
  if (!Joueur->vivant) {
    return;
  }

  timer += deltaTime;

  // début phase alerte
  if (phaseActuelle == Phase_Attente && timer >= Temps_Attente) {
    phaseActuelle = Phase_Alerte;
    timer = 0.0f;

    if (!nbrBlockDoitAugmenter) {
      if (nbrBlockQuiTombe <=
          (Taille_Grid * Taille_Grid) /
              2) // le nombre de block ne peut pas dépasser la moitié des cases
      {
        nbrBlockQuiTombe += 1;
      }
      nbrBlockDoitAugmenter = true;
    }

    for (int y = 0; y < Taille_Grid; y++) {
      for (int x = 0; x < Taille_Grid; x++) {
        if (grid[x][y] == Block_Tomber) {
          grid[x][y] = Block_Normal;
        }
        if (grid[x][y] == Block_Prochain_Cycle) {
          grid[x][y] = Block_Alerte;
        }
      }
    }
  }

  // début chute block
  else if (phaseActuelle == Phase_Alerte && timer >= Temps_Alerte) {

    phaseActuelle = Phase_Tomber;
    timer = 0.0f;

    for (int y = 0; y < Taille_Grid; y++) {
      for (int x = 0; x < Taille_Grid; x++) {
        if (grid[x][y] == Block_Alerte) {
          grid[x][y] = Block_Tomber;
        }
      }
    }
  }

  // début nouveau cycle
  else if (phaseActuelle == Phase_Tomber && timer >= Temps_Tomber) {
    nbrBlockDoitAugmenter = false;

    for (int y = 0; y < Taille_Grid; y++) {
      for (int x = 0; x < Taille_Grid; x++) {
        if (grid[x][y] == Block_Prochain_Cycle || grid[x][y] == Block_Alerte ||
            grid[x][y] == Block_Tomber) {
          grid[x][y] = Block_Normal;
        }
      }
    }

    // Sélection aléatoire des blocs pour le prochain cycle
    int cpt = 0;
    while (cpt < nbrBlockQuiTombe) {
      int randX = GetRandomValue(0, Taille_Grid - 1);
      int randY = GetRandomValue(0, Taille_Grid - 1);

      if (grid[randX][randY] == Block_Normal) {
        grid[randX][randY] = Block_Prochain_Cycle;
        cpt++;
      }
    }
    phaseActuelle = Phase_Attente;
    timer = 0.0f; // on lance le timer en décalé donc il ne correspond pas
                  // exactement au temps voulu mais on est sur que tous les
                  // blocks sont choisi
  }
}

void DessinePartie() {
  // le fond
  Rectangle fondSource = {0.0f, 0.0f, (float)fondTexture.width,
                          (float)fondTexture.height};
  Rectangle fondDest = {0.0f, 0.0f, (float)screenWidth, (float)screenHeight};
  DrawTexturePro(fondTexture,
                 fondSource, // on peut rogner l'image si on veut
                 fondDest,
                 (Vector2){0, 0}, // point autour duquel la rotation se fait
                 0.0f, WHITE);

  for (int y = 0; y < Taille_Grid; y++) {
    for (int x = 0; x < Taille_Grid; x++) {

      Rectangle Case = {Debut_Grille_X + x * Taille_Case,
                        Debut_Grille_Y + y * Taille_Case, Taille_Case,
                        Taille_Case};

      switch (grid[x][y]) {
      case Block_Normal:
      case Block_Prochain_Cycle:
        DrawTexturePro(solTexture,
                       (Rectangle){0.0f, 0.0f, (float)solTexture.width,
                                   (float)solTexture.height},
                       Case, (Vector2){0, 0}, 0.0f, WHITE);

        break;
      case Block_Tomber:
        break;
        ;
      case Block_Alerte:
        DrawTexturePro(solFissureTexture,
                       (Rectangle){0.0f, 0.0f, (float)solTexture.width,
                                   (float)solTexture.height},
                       Case, (Vector2){0, 0}, 0.0f, WHITE);

      default: // normalement sert a rien
        break;
      }

      DrawRectangleLines(
          Case.x, Case.y, Taille_Case, Taille_Case,
          (Color){0, 0, 0, 0}); // contour case utile pour voir bug
    }
  }
}

void DessineJoueur(Joueur Joueur) {
  if (Joueur.vivant) {
    Rectangle sourceRec = {0.0f, 0.0f, (float)playerTexture.width,
                           (float)playerTexture.height};

    // on centre l'image sur joueur.position
    Rectangle destRec = {Joueur.position.x - Demi_Cote_Joueur_X,
                         Joueur.position.y - Demi_Cote_Joueur_Y,
                         Demi_Cote_Joueur_X * 2.0f, Demi_Cote_Joueur_Y * 2.0f};

    DrawTexturePro(playerTexture, sourceRec, destRec, (Vector2){0, 0}, 0.0f,
                   WHITE);

    /*//---SERT JUSTE A VERIF SI BESOIN BUG HITBOX---
    //Hitbox autour des pieds
    float HitboxBas = Joueur.position.y + Demi_Cote_Joueur_Y;
    float HitboxHaut = HitboxBas - HITBOX_PIEDS_HAUTEUR;
    float HitboxGauche = Joueur.position.x - (HITBOX_PIEDS_LARGEUR / 2.0f);
    // Dessinez le rectangle de collision en magenta
    DrawRectangleLinesEx((Rectangle){HitboxGauche, HitboxHaut,
    HITBOX_PIEDS_LARGEUR, HITBOX_PIEDS_HAUTEUR}, 1, MAGENTA);
    */
  }
}

void JeuLuca(bool *game_active, float timeout_seconds, bool resume,
             float *total_time) {
  srand((unsigned int)time(NULL)); // graine choisi aléatoirement pour éviter
                                   // répétition du même motif

  // Utiliser la taille de fenêtre standard
  SetWindowSize(LARGEUR_FENETRE, HAUTEUR_FENETRE);
  SetWindowTitle("Jeux Luca");

  // Calculer les offsets pour centrer le jeu (avec clipping si nécessaire)
  offsetX = (LARGEUR_FENETRE - screenWidth) / 2; // Centré horizontalement
  offsetY = (HAUTEUR_FENETRE - screenHeight) /
            2; // Centré verticalement (sera négatif si clip)

  // Créer la render texture pour le rendu
  lucaRenderTarget = LoadRenderTexture(screenWidth, screenHeight);

  playerTexture = LoadTexture("assets/playerLuca.png");
  fondTexture = LoadTexture("assets/fond.png");
  solTexture = LoadTexture("assets/sol.png");
  solFissureTexture = LoadTexture("assets/solFissure.png");

  // Resume: restaurer l'état ou initialiser
  Joueur Joueur;
  if (resume && hasGameState) {
    Joueur = savedJoueur; // Restaurer l'état sauvegardé
  } else {
    InitGrid();
    Joueur = InitJoueur();
    hasGameState = true;
  }

  *game_active = true;
  float elapsed_time = 0.0f; // Timer pour le timeout

  while (!WindowShouldClose()) {
    float deltaTime = GetFrameTime();
    elapsed_time += deltaTime;

    // Accumuler le temps dans le temps global (si disponible)
    if (total_time != NULL && Joueur.vivant) {
      *total_time += deltaTime;
    }

    // Vérifier timeout (si > 0)
    if (Joueur.vivant && timeout_seconds > 0 &&
        elapsed_time >= timeout_seconds) {
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

    if (IsKeyPressed(KEY_P)) {
      jeuEnPause = !jeuEnPause; // Alterne entre vrai et faux
    }

    if (Joueur.vivant) {
      if (!jeuEnPause) {
        UpdateJoueurMouvement(&Joueur, deltaTime);
        UpdateGridPhase(&Joueur, deltaTime);
        VerifieMort(&Joueur);
      }

    } else {
      // Joueur mort - n'importe quelle touche retourne au menu
      int key = GetKeyPressed();
      if (key != 0) {
        *game_active = false;
        break;
      }
    }

    // Rendu dans la texture à la taille native du jeu
    BeginTextureMode(lucaRenderTarget);

    ClearBackground(RAYWHITE);
    DessinePartie();

    DessineJoueur(Joueur);

    if (jeuEnPause && Joueur.vivant) {
      // Overlay de pause
      DrawRectangle(0, 0, screenWidth, screenHeight,
                    Fade(BLACK, 0.5f)); // Fond sombre transparent
      DrawText("PAUSE", (screenWidth / 2) - MeasureText("PAUSE", 40) / 2,
               screenHeight / 2 - 20, 40, RAYWHITE);
      DrawText("Appuyez sur P pour reprendre",
               (screenWidth / 2) -
                   MeasureText("Appuyez sur P pour reprendre", 20) / 2,
               screenHeight / 2 + 30, 20, LIGHTGRAY);
    }

    if (!Joueur.vivant) {
      DrawText("GAME OVER!",
               (screenWidth / 2) - MeasureText("GAME OVER!", 60) / 2,
               screenHeight / 3, 60, RED);
      DrawText(
          "Appuyez sur une touche pour revenir au menu",
          (screenWidth / 2) -
              MeasureText("Appuyez sur une touche pour revenir au menu", 20) /
                  2,
          screenHeight / 2, 20, RAYWHITE);
    }

    EndTextureMode();

    // Dessiner la texture centrée sur l'écran
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTextureRec(lucaRenderTarget.texture,
                   (Rectangle){0, 0, (float)screenWidth, (float)-screenHeight},
                   (Vector2){(float)offsetX, (float)offsetY}, WHITE);

    // Afficher le temps global sur le côté (dans la zone noire)
    if (total_time != NULL) {
      DrawText("TOTAL TIME", 20, HAUTEUR_FENETRE / 2 - 20, 20, GRAY);
      DrawText(TextFormat("%.1fs", *total_time), 20, HAUTEUR_FENETRE / 2 + 5,
               30, WHITE);
    }

    EndDrawing();
  }

  // Sauvegarder l'état du joueur pour le resume
  savedJoueur = Joueur;

  // Nettoyage des textures (mais garder grid et état)
  UnloadRenderTexture(lucaRenderTarget);
  UnloadTexture(playerTexture);
  UnloadTexture(fondTexture);
  UnloadTexture(solTexture);
  UnloadTexture(solFissureTexture);

  // Si le joueur est mort, nettoyer l'état pour la prochaine partie
  if (!Joueur.vivant) {
    FreeGrid();
    hasGameState = false;
  }

  // Note: game_active n'est pas mis à false ici pour permettre la rotation
  // Il est set à false seulement si ESC est pressé ou si le joueur meurt
}

void InitDifficulte(Difficulte version) {
  if (version == FACILE) {
    nbrBlockQuiTombe = (Taille_Grid * Taille_Grid) / 12;
  }
  if (version == MOYEN) {
    nbrBlockQuiTombe = (Taille_Grid * Taille_Grid) / 6;
  }
  if (version == DIFFICILE) {
    nbrBlockQuiTombe = (Taille_Grid * Taille_Grid) / 3;
  }
}

// Fonction wrapper avec signature standard
void RunLucaGame(Difficulte difficulty, bool resume, bool *game_active,
                 const char *playerName, ScoreList *scores,
                 float timeout_seconds, float *total_time) {
  (void)difficulty; // Non utilisé pour l'instant
  (void)playerName;
  (void)scores;

  InitDifficulte(difficulty);
  JeuLuca(game_active, timeout_seconds, resume, total_time);
}