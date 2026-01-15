#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/luca.h"
#include "include/player_input.h"

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

// Variables pour le resume (etat sauvegarde entre les rotations)
static Joueur savedJoueur1;
static Joueur savedJoueur2;
static bool hasGameState = false;

// Mode de jeu actuel
static ModeJoueur modeActuel = UN_JOUEUR;
static int winner = 0; // 0 = aucun, 1 = J1 gagne, 2 = J2 gagne

Texture2D playerTexture;
Texture2D fondTexture;
Texture2D solTexture;
Texture2D solFissureTexture;

//---PROTOTYPES---
// utile pour ecrire les fonctions dans l'ordre qu'on veut
void InitGrid();
void FreeGrid();
void ResetGridState();
Joueur InitJoueur(int player_id);
void UpdateJoueurMouvement(Joueur *Joueur, float deltaTime, int player_id);
void DashJoueur(Joueur *Joueur, int player_id);
void VerifieMort(Joueur *Joueur);
void UpdateGridPhase(Joueur *Joueur1, Joueur *Joueur2, float deltaTime);
void DessinePartie();
void DessineJoueur(Joueur Joueur, int player_id);
void JeuLuca(bool *game_active, float timeout_seconds, bool resume,
             float *total_time, float start_delay);

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

Joueur InitJoueur(int player_id) {
  Joueur j;
  if (modeActuel == DEUX_JOUEURS) {
    // En mode 2 joueurs, placer les joueurs de chaque cote
    if (player_id == 1) {
      j.position.x = screenWidth / 3;
    } else {
      j.position.x = screenWidth * 2 / 3;
    }
  } else {
    j.position.x = screenWidth / 2;
  }
  j.position.y = screenHeight / 2;
  j.vivant = true;
  j.dernierMouvement = (Vector2){0.0f, 0.0f};
  return j;
}

void DashJoueur(Joueur *Joueur, int player_id) {
  (void)player_id; // Utilise pour verifier la touche action
  if (!Joueur->vivant) {
    return;
  }

  float DashX = Joueur->dernierMouvement.x * Distance_Case_Dash * Taille_Case;
  float DashY = Joueur->dernierMouvement.y * Distance_Case_Dash * Taille_Case;

  Joueur->position.x += DashX;
  Joueur->position.y += DashY;
}

void UpdateJoueurMouvement(Joueur *Joueur, float deltaTime, int player_id) {
  if (!Joueur->vivant) {
    return;
  }

  Vector2 mouvement = {0.0f, 0.0f};

  if (player_id == 1) {
    // Joueur 1: ZQSD
    if (IS_P1_RIGHT()) mouvement.x += 1.0f;
    if (IS_P1_LEFT()) mouvement.x -= 1.0f;
    if (IS_P1_DOWN()) mouvement.y += 1.0f;
    if (IS_P1_UP()) mouvement.y -= 1.0f;
  } else {
    // Joueur 2: Fleches
    if (IS_P2_RIGHT()) mouvement.x += 1.0f;
    if (IS_P2_LEFT()) mouvement.x -= 1.0f;
    if (IS_P2_DOWN()) mouvement.y += 1.0f;
    if (IS_P2_UP()) mouvement.y -= 1.0f;
  }

  if (mouvement.x != 0.0f || mouvement.y != 0.0f) {
    Joueur->dernierMouvement = Vector2Normalize(mouvement);
    mouvement = Joueur->dernierMouvement;
  }

  float vitesseActuel = Vitesse_Marche;
  // Sprint avec shift (J1: LEFT_SHIFT, J2: RIGHT_SHIFT)
  if (player_id == 1 && IsKeyDown(KEY_LEFT_SHIFT)) {
    vitesseActuel *= CoefCourse;
  } else if (player_id == 2 && IsKeyDown(KEY_RIGHT_SHIFT)) {
    vitesseActuel *= CoefCourse;
  }

  Joueur->position.x += mouvement.x * vitesseActuel * deltaTime;
  Joueur->position.y += mouvement.y * vitesseActuel * deltaTime;

  // Dash avec touche action
  if (player_id == 1 && IS_P1_ACTION()) {
    DashJoueur(Joueur, player_id);
  } else if (player_id == 2 && IS_P2_ACTION()) {
    DashJoueur(Joueur, player_id);
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

void UpdateGridPhase(Joueur *Joueur1, Joueur *Joueur2, float deltaTime) {
  // Verifier si au moins un joueur est vivant
  bool joueur1Vivant = Joueur1->vivant;
  bool joueur2Vivant = (modeActuel == DEUX_JOUEURS) ? Joueur2->vivant : false;

  if (!joueur1Vivant && !joueur2Vivant) {
    return;
  }
  // En mode 1 joueur, verifier seulement J1
  if (modeActuel == UN_JOUEUR && !joueur1Vivant) {
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

void DessineJoueur(Joueur Joueur, int player_id) {
  if (Joueur.vivant) {
    Rectangle sourceRec = {0.0f, 0.0f, (float)playerTexture.width,
                           (float)playerTexture.height};

    // on centre l'image sur joueur.position
    Rectangle destRec = {Joueur.position.x - Demi_Cote_Joueur_X,
                         Joueur.position.y - Demi_Cote_Joueur_Y,
                         Demi_Cote_Joueur_X * 2.0f, Demi_Cote_Joueur_Y * 2.0f};

    // Couleur selon le joueur (J1 = bleu, J2 = rouge)
    Color tint = WHITE;
    if (modeActuel == DEUX_JOUEURS) {
      if (player_id == 1) {
        tint = (Color){150, 200, 255, 255}; // Teinte bleue
      } else {
        tint = (Color){255, 150, 150, 255}; // Teinte rouge
      }
    }

    DrawTexturePro(playerTexture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, tint);

    // Etiquette joueur en mode 2 joueurs
    if (modeActuel == DEUX_JOUEURS) {
      const char *label = (player_id == 1) ? "P1" : "P2";
      Color labelColor = (player_id == 1) ? (Color){0, 150, 255, 255} : (Color){255, 100, 100, 255};
      DrawText(label, (int)Joueur.position.x - 8, (int)Joueur.position.y - 35, 16, labelColor);
    }
  }
}

void JeuLuca(bool *game_active, float timeout_seconds, bool resume,
             float *total_time, float start_delay) {
  srand((unsigned int)time(NULL));

  SetWindowSize(LARGEUR_FENETRE, HAUTEUR_FENETRE);
  SetWindowTitle("Jeux Luca");

  offsetX = (LARGEUR_FENETRE - screenWidth) / 2;
  offsetY = (HAUTEUR_FENETRE - screenHeight) / 2;

  lucaRenderTarget = LoadRenderTexture(screenWidth, screenHeight);

  playerTexture = LoadTexture("assets/playerLuca.png");
  fondTexture = LoadTexture("assets/fond.png");
  solTexture = LoadTexture("assets/sol.png");
  solFissureTexture = LoadTexture("assets/solFissure.png");

  // Initialisation des joueurs
  Joueur Joueur1, Joueur2;
  winner = 0;

  if (resume && hasGameState) {
    Joueur1 = savedJoueur1;
    Joueur2 = savedJoueur2;
  } else {
    InitGrid();
    Joueur1 = InitJoueur(1);
    Joueur2 = InitJoueur(2);
    hasGameState = true;
  }

  *game_active = true;
  float elapsed_time = 0.0f;

  while (!WindowShouldClose()) {
    float deltaTime = GetFrameTime();
    elapsed_time += deltaTime;

    // Determiner si le jeu est termine
    bool game_over;
    if (modeActuel == DEUX_JOUEURS) {
      game_over = (winner != 0);
    } else {
      game_over = !Joueur1.vivant;
    }

    // Accumuler le temps global
    if (total_time != NULL && !game_over) {
      *total_time += deltaTime;
    }

    // Verifier timeout
    if (!game_over && timeout_seconds > 0 && elapsed_time >= timeout_seconds) {
      break;
    }

    // Retour au menu avec Escape
    if (IsKeyPressed(KEY_ESCAPE)) {
      while (IsKeyDown(KEY_ESCAPE)) {
        BeginDrawing();
        EndDrawing();
      }
      *game_active = false;
      break;
    }

    if (IsKeyPressed(KEY_P)) {
      jeuEnPause = !jeuEnPause;
    }

    // Gestion des transitions
    float current_scale = 1.0f;
    Vector2 shake_transition = {0.0f, 0.0f};

    if (elapsed_time < start_delay) {
      float factor = elapsed_time / start_delay;
      current_scale = Lerp(0.1f, 1.0f, factor);
    } else if (!game_over && timeout_seconds > 0 &&
               (timeout_seconds - elapsed_time) < 0.5f) {
      float remaining = timeout_seconds - elapsed_time;
      float factor = remaining / 0.5f;
      current_scale = Lerp(0.1f, 1.0f, factor);
      float shake_amount = (1.0f - factor) * 10.0f;
      shake_transition.x = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
      shake_transition.y = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
    }

    if (!game_over && !jeuEnPause && elapsed_time >= start_delay) {
      // Mise a jour Joueur 1
      if (Joueur1.vivant) {
        UpdateJoueurMouvement(&Joueur1, deltaTime, 1);
        VerifieMort(&Joueur1);
        // Verifier si J1 meurt en mode 2 joueurs
        if (!Joueur1.vivant && modeActuel == DEUX_JOUEURS && Joueur2.vivant) {
          winner = 2;
        }
      }

      // Mise a jour Joueur 2 (mode 2 joueurs)
      if (modeActuel == DEUX_JOUEURS && Joueur2.vivant) {
        UpdateJoueurMouvement(&Joueur2, deltaTime, 2);
        VerifieMort(&Joueur2);
        if (!Joueur2.vivant && Joueur1.vivant) {
          winner = 1;
        }
      }

      // Verifier egalite (les deux morts en meme temps)
      if (modeActuel == DEUX_JOUEURS && !Joueur1.vivant && !Joueur2.vivant && winner == 0) {
        winner = -1; // Egalite
      }

      // Mise a jour de la grille si au moins un joueur vivant
      UpdateGridPhase(&Joueur1, &Joueur2, deltaTime);
    }

    // Rendu
    BeginTextureMode(lucaRenderTarget);
    ClearBackground(RAYWHITE);
    DessinePartie();

    DessineJoueur(Joueur1, 1);
    if (modeActuel == DEUX_JOUEURS) {
      DessineJoueur(Joueur2, 2);
    }

    // Pause (seulement si pas game over)
    bool anyAlive = Joueur1.vivant || (modeActuel == DEUX_JOUEURS && Joueur2.vivant);
    if (jeuEnPause && anyAlive) {
      DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
      DrawText("PAUSE", (screenWidth / 2) - MeasureText("PAUSE", 40) / 2,
               screenHeight / 2 - 20, 40, RAYWHITE);
      DrawText("Appuyez sur P pour reprendre",
               (screenWidth / 2) - MeasureText("Appuyez sur P pour reprendre", 20) / 2,
               screenHeight / 2 + 30, 20, LIGHTGRAY);
    }

    // Affichage fin de partie
    if (game_over) {
      DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));

      if (modeActuel == DEUX_JOUEURS && winner == -1) {
        // Egalite
        const char *tie_text = "EGALITE!";
        DrawText(tie_text, (screenWidth / 2) - MeasureText(tie_text, 60) / 2,
                 screenHeight / 3, 60, YELLOW);
      } else if (modeActuel == DEUX_JOUEURS && (winner == 1 || winner == 2)) {
        const char *winner_text = (winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
        Color winner_color = (winner == 1) ? (Color){0, 150, 255, 255} : (Color){255, 100, 100, 255};
        DrawText(winner_text, (screenWidth / 2) - MeasureText(winner_text, 60) / 2,
                 screenHeight / 3, 60, winner_color);
      } else {
        DrawText("GAME OVER!", (screenWidth / 2) - MeasureText("GAME OVER!", 60) / 2,
                 screenHeight / 3, 60, RED);
      }
      DrawText("ESCAPE: Menu", (screenWidth / 2) - MeasureText("ESCAPE: Menu", 20) / 2,
               screenHeight / 2, 20, RAYWHITE);
    }

    EndTextureMode();

    // Affichage final
    BeginDrawing();
    ClearBackground(BLACK);

    float destWidth = screenWidth * current_scale;
    float destHeight = screenHeight * current_scale;
    float destX = (LARGEUR_FENETRE - destWidth) / 2.0f + shake_transition.x;
    float destY = (HAUTEUR_FENETRE - destHeight) / 2.0f + shake_transition.y;

    Rectangle sourceRec = {0, 0, (float)screenWidth, (float)-screenHeight};
    Rectangle destRec = {destX, destY, destWidth, destHeight};

    DrawTexturePro(lucaRenderTarget.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);

    if (total_time != NULL && current_scale > 0.8f) {
      DrawText("TOTAL TIME", 20, HAUTEUR_FENETRE / 2 - 20, 20, GRAY);
      DrawText(TextFormat("%.1fs", *total_time), 20, HAUTEUR_FENETRE / 2 + 5, 30, WHITE);
    }

    EndDrawing();
  }

  // Sauvegarder l'etat
  savedJoueur1 = Joueur1;
  savedJoueur2 = Joueur2;

  // Nettoyage
  UnloadRenderTexture(lucaRenderTarget);
  UnloadTexture(playerTexture);
  UnloadTexture(fondTexture);
  UnloadTexture(solTexture);
  UnloadTexture(solFissureTexture);

  // Si tous les joueurs sont morts, nettoyer l'etat
  bool allDead = !Joueur1.vivant && (modeActuel != DEUX_JOUEURS || !Joueur2.vivant);
  if (allDead) {
    FreeGrid();
    hasGameState = false;
  }
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
void RunLucaGame(Difficulte difficulty, ModeJoueur mode, bool resume, bool *game_active,
                 const char *playerName, ScoreList *scores,
                 float timeout_seconds, float *total_time, float start_delay) {
  (void)playerName;
  (void)scores;

  modeActuel = mode;
  InitDifficulte(difficulty);
  JeuLuca(game_active, timeout_seconds, resume, total_time, start_delay);
}