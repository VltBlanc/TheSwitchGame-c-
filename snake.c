#include "include/menu.h"
#include "include/scores.h"
#include "include/player_input.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <time.h>

// Taille de la grille (plus petite pour elements plus gros)
#define LARGEUR_GRILLE 15
#define HAUTEUR_GRILLE 15
#define TAILLE_CASE 32

// Dimensions de l'aire de jeu native et de la fenetre standard
#define LARGEUR_JEU (LARGEUR_GRILLE * TAILLE_CASE)
#define HAUTEUR_JEU (HAUTEUR_GRILLE * TAILLE_CASE + 50) // +50 pour l'UI
#define LARGEUR_FENETRE 1280
#define HAUTEUR_FENETRE 720
#define OFFSET_X ((LARGEUR_FENETRE - LARGEUR_JEU) / 2)
#define OFFSET_Y ((HAUTEUR_FENETRE - HAUTEUR_JEU) / 2)

// Directions possibles
#define HAUT 0
#define BAS 1
#define GAUCHE 2
#define DROITE 3

// Structure pour une position
typedef struct {
  int x;
  int y;
} Position;

// Variables globales du jeu - Joueur 1
Position serpent1[225]; // corps du serpent J1
int longueur_serpent1;
int direction1;
int direction_suivante1;
int score1;
bool jeu_termine1;

// Variables globales du jeu - Joueur 2
Position serpent2[225]; // corps du serpent J2
int longueur_serpent2;
int direction2;
int direction_suivante2;
int score2;
bool jeu_termine2;

// Variables partagees
Position pomme;         // jerrican de carburant
Position pomme2;        // deuxieme jerrican (mode 2 joueurs)
double temps_derniere_maj;
double vitesse_jeu;

// Mode de jeu actuel
static ModeJoueur modeActuel = UN_JOUEUR;
static int winner = 0; // 0 = aucun, 1 = J1 gagne, 2 = J2 gagne

// File de mouvements (input queue) - Joueur 1
#define TAILLE_FILE 3
int file_directions1[TAILLE_FILE];
int file_tete1;
int file_queue1;

// File de mouvements - Joueur 2
int file_directions2[TAILLE_FILE];
int file_tete2;
int file_queue2;

// Systeme de carburant (partage en mode 1 joueur, separe en 2 joueurs)
double carburant1;
double carburant2;
#define CARBURANT_MAX 5.0

// Obstacles (trous noirs qui apparaissent)
Position murs[15]; // Maximum pour le mode difficile
int nombre_murs;

// Difficulté du jeu
// Difficulté du jeu
Difficulte difficulte_actuelle;

// Temps global pour affichage
static float *snakeGlobalTime = NULL;

// --- NOUVEAU: Systemes visuels ---
// Etoiles pour le fond
typedef struct {
  float x, y;
  float speed;
  float brightness;
} Star;

#define MAX_STARS 100
Star stars[MAX_STARS];

// Particules
typedef struct {
  Vector2 position;
  Vector2 velocity;
  Color color;
  float life;  // 1.0 a 0.0
  float decay; // Vitesse de disparition
  float size;
  bool active;
} Particle;

#define MAX_PARTICLES 200
Particle particles[MAX_PARTICLES];

// Screen Shake
float shake_intensity = 0.0f;

// RenderTexture pour le rendu centré
static RenderTexture2D snakeRenderTarget;

// --- Fin Nouveaux Systemes ---

// Fonction pour obtenir le nombre maximum de trous noirs selon la difficulté
static int getMaxBlackHoles() {
  switch (difficulte_actuelle) {
  case FACILE:
    return 5;
  case MOYEN:
    return 10;
  case DIFFICILE:
    return 30;
  default:
    return 10;
  }
}

// Fonction pour generer une pomme a une position aleatoire
static void GenererPomme() {
  bool position_valide = false;

  while (!position_valide) {
    pomme.x = GetRandomValue(0, LARGEUR_GRILLE - 1);
    pomme.y = GetRandomValue(0, HAUTEUR_GRILLE - 1);

    position_valide = true;
    // Verifier que la pomme n'apparait pas sur le serpent 1
    for (int i = 0; i < longueur_serpent1; i++) {
      if (serpent1[i].x == pomme.x && serpent1[i].y == pomme.y) {
        position_valide = false;
        break;
      }
    }

    // Verifier que la pomme n'apparait pas sur le serpent 2 (mode 2 joueurs)
    if (position_valide && modeActuel == DEUX_JOUEURS) {
      for (int i = 0; i < longueur_serpent2; i++) {
        if (serpent2[i].x == pomme.x && serpent2[i].y == pomme.y) {
          position_valide = false;
          break;
        }
      }
    }

    // Verifier que la pomme n'apparait pas sur un mur
    if (position_valide) {
      for (int i = 0; i < nombre_murs; i++) {
        if (murs[i].x == pomme.x && murs[i].y == pomme.y) {
          position_valide = false;
          break;
        }
      }
    }
  }
}

// Fonction pour generer la deuxieme pomme (mode 2 joueurs)
static void GenererPomme2() {
  bool position_valide = false;

  while (!position_valide) {
    pomme2.x = GetRandomValue(0, LARGEUR_GRILLE - 1);
    pomme2.y = GetRandomValue(0, HAUTEUR_GRILLE - 1);

    position_valide = true;

    // Verifier que pomme2 n'est pas sur pomme1
    if (pomme2.x == pomme.x && pomme2.y == pomme.y) {
      position_valide = false;
      continue;
    }

    // Verifier que pomme2 n'apparait pas sur le serpent 1
    for (int i = 0; i < longueur_serpent1; i++) {
      if (serpent1[i].x == pomme2.x && serpent1[i].y == pomme2.y) {
        position_valide = false;
        break;
      }
    }

    // Verifier que pomme2 n'apparait pas sur le serpent 2
    if (position_valide) {
      for (int i = 0; i < longueur_serpent2; i++) {
        if (serpent2[i].x == pomme2.x && serpent2[i].y == pomme2.y) {
          position_valide = false;
          break;
        }
      }
    }

    // Verifier que pomme2 n'apparait pas sur un mur
    if (position_valide) {
      for (int i = 0; i < nombre_murs; i++) {
        if (murs[i].x == pomme2.x && murs[i].y == pomme2.y) {
          position_valide = false;
          break;
        }
      }
    }
  }
}

// Initialiser les etoiles
static void InitStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = GetRandomValue(0, LARGEUR_GRILLE * TAILLE_CASE);
    stars[i].y = GetRandomValue(0, HAUTEUR_GRILLE * TAILLE_CASE);
    stars[i].speed = (float)GetRandomValue(1, 5) / 10.0f;
    stars[i].brightness = (float)GetRandomValue(100, 255) / 255.0f;
  }
}

// Mettre a jour les etoiles
static void UpdateStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].y += stars[i].speed;
    if (stars[i].y > HAUTEUR_GRILLE * TAILLE_CASE) {
      stars[i].y = 0;
      stars[i].x = GetRandomValue(0, LARGEUR_GRILLE * TAILLE_CASE);
    }
  }
}

// Initialiser les particules
static void InitParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
  }
}

// Ajouter une particule
static void AddParticle(Vector2 pos, Vector2 vel, Color col, float size,
                        float decay) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) {
      particles[i].active = true;
      particles[i].position = pos;
      particles[i].velocity = vel;
      particles[i].color = col;
      particles[i].life = 1.0f;
      particles[i].size = size;
      particles[i].decay = decay;
      break;
    }
  }
}

// Mettre a jour les particules
static void UpdateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      particles[i].position.x += particles[i].velocity.x;
      particles[i].position.y += particles[i].velocity.y;
      particles[i].life -= particles[i].decay;

      if (particles[i].life <= 0) {
        particles[i].active = false;
      }
    }
  }
}

// Fonction pour initialiser le jeu
static void InitialiserJeu() {
  // Initialisation Joueur 1
  longueur_serpent1 = 3;
  direction1 = DROITE;
  direction_suivante1 = DROITE;
  score1 = 0;
  jeu_termine1 = false;
  carburant1 = CARBURANT_MAX;
  file_tete1 = 0;
  file_queue1 = 0;

  // Initialisation Joueur 2 (si mode 2 joueurs)
  longueur_serpent2 = 3;
  direction2 = GAUCHE;
  direction_suivante2 = GAUCHE;
  score2 = 0;
  jeu_termine2 = false;
  carburant2 = CARBURANT_MAX;
  file_tete2 = 0;
  file_queue2 = 0;

  // Variables partagees
  vitesse_jeu = 0.15;
  temps_derniere_maj = 0;
  nombre_murs = 0;
  shake_intensity = 0.0f;
  winner = 0;

  InitStars();
  InitParticles();

  // Initialiser toutes les positions serpent 1
  for (int i = 0; i < 225; i++) {
    serpent1[i].x = -1;
    serpent1[i].y = -1;
  }

  // Initialiser toutes les positions serpent 2
  for (int i = 0; i < 225; i++) {
    serpent2[i].x = -1;
    serpent2[i].y = -1;
  }

  // Initialiser les murs
  for (int i = 0; i < 15; i++) {
    murs[i].x = -1;
    murs[i].y = -1;
  }

  // Placer le serpent 1 (cote gauche, va vers la droite)
  if (modeActuel == DEUX_JOUEURS) {
    serpent1[0].x = LARGEUR_GRILLE / 4;
    serpent1[0].y = HAUTEUR_GRILLE / 2;
  } else {
    serpent1[0].x = LARGEUR_GRILLE / 2;
    serpent1[0].y = HAUTEUR_GRILLE / 2;
  }
  serpent1[1].x = serpent1[0].x - 1;
  serpent1[1].y = serpent1[0].y;
  serpent1[2].x = serpent1[1].x - 1;
  serpent1[2].y = serpent1[1].y;

  // Placer le serpent 2 (cote droit, va vers la gauche) - seulement en 2 joueurs
  if (modeActuel == DEUX_JOUEURS) {
    serpent2[0].x = LARGEUR_GRILLE * 3 / 4;
    serpent2[0].y = HAUTEUR_GRILLE / 2;
    serpent2[1].x = serpent2[0].x + 1;
    serpent2[1].y = serpent2[0].y;
    serpent2[2].x = serpent2[1].x + 1;
    serpent2[2].y = serpent2[1].y;
  }

  // Pour le mode difficile, placer 10 trous noirs des le depart
  if (difficulte_actuelle == DIFFICILE) {
    nombre_murs = 10;
    for (int i = 0; i < 10; i++) {
      bool position_valide = false;
      int tentatives = 0;

      while (!position_valide && tentatives < 200) {
        int nx = GetRandomValue(0, LARGEUR_GRILLE - 1);
        int ny = GetRandomValue(0, HAUTEUR_GRILLE - 1);
        position_valide = true;

        // Verifier serpent 1
        for (int j = 0; j < longueur_serpent1; j++) {
          if (serpent1[j].x == nx && serpent1[j].y == ny) {
            position_valide = false;
            break;
          }
        }

        // Verifier serpent 2
        if (position_valide && modeActuel == DEUX_JOUEURS) {
          for (int j = 0; j < longueur_serpent2; j++) {
            if (serpent2[j].x == nx && serpent2[j].y == ny) {
              position_valide = false;
              break;
            }
          }
        }

        // Verifier pomme
        if (pomme.x == nx && pomme.y == ny)
          position_valide = false;

        // Verifier murs existants
        for (int j = 0; j < i; j++) {
          if (murs[j].x == nx && murs[j].y == ny) {
            position_valide = false;
            break;
          }
        }

        if (position_valide) {
          murs[i].x = nx;
          murs[i].y = ny;
        }
        tentatives++;
      }
    }
  }

  GenererPomme();
  if (modeActuel == DEUX_JOUEURS) {
    GenererPomme2();
  }
}

// Fonction pour ajouter un mur aleatoire
static void AjouterMur() {
  int max_holes = getMaxBlackHoles();
  if (nombre_murs >= max_holes) {
    // FIFO: Remove oldest black hole by shifting all holes forward
    for (int i = 0; i < max_holes - 1; i++) {
      murs[i] = murs[i + 1];
    }
    nombre_murs = max_holes - 1;
  }

  bool position_valide = false;
  int tentatives = 0;

  while (!position_valide && tentatives < 200) {
    int nx = GetRandomValue(0, LARGEUR_GRILLE - 1);
    int ny = GetRandomValue(0, HAUTEUR_GRILLE - 1);
    position_valide = true;

    // Verifier serpent 1
    for (int i = 0; i < longueur_serpent1; i++) {
      if (serpent1[i].x == nx && serpent1[i].y == ny) {
        position_valide = false;
        break;
      }
    }

    // Verifier serpent 2 (mode 2 joueurs)
    if (position_valide && modeActuel == DEUX_JOUEURS) {
      for (int i = 0; i < longueur_serpent2; i++) {
        if (serpent2[i].x == nx && serpent2[i].y == ny) {
          position_valide = false;
          break;
        }
      }
    }

    // Verifier que ce n'est pas sur la pomme
    if (pomme.x == nx && pomme.y == ny)
      position_valide = false;

    // Verifier que ce n'est pas sur un mur existant
    for (int i = 0; i < nombre_murs; i++) {
      if (murs[i].x == nx && murs[i].y == ny) {
        position_valide = false;
        break;
      }
    }

    // Verifier que ce n'est pas sur les 2 blocs devant le vaisseau 1
    for (int d = 1; d <= 2; d++) {
      int fx = serpent1[0].x;
      int fy = serpent1[0].y;
      if (direction1 == DROITE) fx += d;
      else if (direction1 == GAUCHE) fx -= d;
      else if (direction1 == HAUT) fy -= d;
      else if (direction1 == BAS) fy += d;

      if (nx == fx && ny == fy) {
        position_valide = false;
        break;
      }
    }

    // Verifier que ce n'est pas sur les 2 blocs devant le vaisseau 2 (mode 2 joueurs)
    if (position_valide && modeActuel == DEUX_JOUEURS) {
      for (int d = 1; d <= 2; d++) {
        int fx = serpent2[0].x;
        int fy = serpent2[0].y;
        if (direction2 == DROITE) fx += d;
        else if (direction2 == GAUCHE) fx -= d;
        else if (direction2 == HAUT) fy -= d;
        else if (direction2 == BAS) fy += d;

        if (nx == fx && ny == fy) {
          position_valide = false;
          break;
        }
      }
    }

    if (position_valide) {
      murs[nombre_murs].x = nx;
      murs[nombre_murs].y = ny;
      nombre_murs++;
    }
    tentatives++;
  }
}

// Fonction pour verifier si le serpent se mord lui-meme
static bool VerifierCollisionCorps(Position *serpent, int longueur) {
  for (int i = 1; i < longueur; i++) {
    if (serpent[0].x == serpent[i].x && serpent[0].y == serpent[i].y) {
      return true;
    }
  }
  return false;
}

// Verifier collision entre deux serpents (tete de s1 contre corps de s2)
static bool VerifierCollisionEntreSerpents(Position *s1, Position *s2, int longueur2) {
  for (int i = 0; i < longueur2; i++) {
    if (s1[0].x == s2[i].x && s1[0].y == s2[i].y) {
      return true;
    }
  }
  return false;
}

// Fonction helper pour deplacer un serpent individuel
static void DeplacerSerpentIndividuel(Position *serpent, int *longueur, int *direction,
                                      int *file_directions, int *file_tete, int *file_queue,
                                      int *score, double *carburant, bool *jeu_termine,
                                      Color couleur_particule) {
  if (*jeu_termine) return;

  // Consommer la prochaine direction de la file si disponible
  if (*file_tete != *file_queue) {
    int nouvelle_dir = file_directions[*file_tete];
    // Verifier que ce n'est pas un demi-tour
    if (!((*direction == HAUT && nouvelle_dir == BAS) ||
          (*direction == BAS && nouvelle_dir == HAUT) ||
          (*direction == GAUCHE && nouvelle_dir == DROITE) ||
          (*direction == DROITE && nouvelle_dir == GAUCHE))) {
      *direction = nouvelle_dir;
    }
    *file_tete = (*file_tete + 1) % TAILLE_FILE;
  }

  // Particules de trainee (exhaust)
  Vector2 tailPos = {(float)serpent[0].x * TAILLE_CASE + TAILLE_CASE / 2,
                     (float)serpent[0].y * TAILLE_CASE + TAILLE_CASE / 2};
  if (*direction == HAUT) tailPos.y += 10;
  else if (*direction == BAS) tailPos.y -= 10;
  else if (*direction == GAUCHE) tailPos.x += 10;
  else if (*direction == DROITE) tailPos.x -= 10;

  AddParticle(tailPos,
              (Vector2){(float)GetRandomValue(-10, 10) / 10.0f,
                        (float)GetRandomValue(-10, 10) / 10.0f},
              couleur_particule, 4.0f, 0.05f);

  // Deplacer le corps
  for (int i = *longueur - 1; i > 0; i--) {
    serpent[i] = serpent[i - 1];
  }

  // Deplacer la tete selon la direction
  if (*direction == HAUT) serpent[0].y--;
  else if (*direction == BAS) serpent[0].y++;
  else if (*direction == GAUCHE) serpent[0].x--;
  else if (*direction == DROITE) serpent[0].x++;

  // Verifier collision avec les bords
  if (serpent[0].x < 0 || serpent[0].x >= LARGEUR_GRILLE ||
      serpent[0].y < 0 || serpent[0].y >= HAUTEUR_GRILLE) {
    *jeu_termine = true;
    shake_intensity = 10.0f;
    for (int k = 0; k < 20; k++) {
      AddParticle((Vector2){(float)serpent[0].x * TAILLE_CASE,
                            (float)serpent[0].y * TAILLE_CASE},
                  (Vector2){(float)GetRandomValue(-50, 50) / 10.0f,
                            (float)GetRandomValue(-50, 50) / 10.0f},
                  ORANGE, 8.0f, 0.02f);
    }
    return;
  }

  // Verifier collision avec son propre corps
  if (VerifierCollisionCorps(serpent, *longueur)) {
    *jeu_termine = true;
    shake_intensity = 10.0f;
    return;
  }

  // Verifier collision avec les obstacles (trous noirs)
  for (int i = 0; i < nombre_murs; i++) {
    if (serpent[0].x == murs[i].x && serpent[0].y == murs[i].y) {
      *jeu_termine = true;
      shake_intensity = 10.0f;
      return;
    }
  }

  // Verifier si le serpent prend le jerrican 1
  if (serpent[0].x == pomme.x && serpent[0].y == pomme.y) {
    (*longueur)++;
    *score += 10;
    *carburant = CARBURANT_MAX;

    for (int k = 0; k < 10; k++) {
      AddParticle((Vector2){(float)pomme.x * TAILLE_CASE + TAILLE_CASE / 2,
                            (float)pomme.y * TAILLE_CASE + TAILLE_CASE / 2},
                  (Vector2){(float)GetRandomValue(-30, 30) / 10.0f,
                            (float)GetRandomValue(-30, 30) / 10.0f},
                  YELLOW, 5.0f, 0.03f);
    }

    GenererPomme();
    AjouterMur();

    // Augmenter la vitesse (score1 pour mode 1 joueur, score total pour 2 joueurs)
    int total_score = (modeActuel == DEUX_JOUEURS) ? score1 + score2 : score1;
    if (vitesse_jeu > 0.05 && total_score < 100) {
      vitesse_jeu -= 0.005;
    }
  }

  // Verifier si le serpent prend le jerrican 2 (mode 2 joueurs)
  if (modeActuel == DEUX_JOUEURS && serpent[0].x == pomme2.x && serpent[0].y == pomme2.y) {
    (*longueur)++;
    *score += 10;
    *carburant = CARBURANT_MAX;

    for (int k = 0; k < 10; k++) {
      AddParticle((Vector2){(float)pomme2.x * TAILLE_CASE + TAILLE_CASE / 2,
                            (float)pomme2.y * TAILLE_CASE + TAILLE_CASE / 2},
                  (Vector2){(float)GetRandomValue(-30, 30) / 10.0f,
                            (float)GetRandomValue(-30, 30) / 10.0f},
                  ORANGE, 5.0f, 0.03f);
    }

    GenererPomme2();
    AjouterMur();

    int total_score = score1 + score2;
    if (vitesse_jeu > 0.05 && total_score < 100) {
      vitesse_jeu -= 0.005;
    }
  }
}

// Fonction pour deplacer les serpents
static void DeplacerSerpents() {
  // Deplacer serpent 1
  DeplacerSerpentIndividuel(serpent1, &longueur_serpent1, &direction1,
                             file_directions1, &file_tete1, &file_queue1,
                             &score1, &carburant1, &jeu_termine1,
                             (Color){0, 200, 255, 200}); // Cyan pour J1

  // Deplacer serpent 2 (mode 2 joueurs)
  if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
    DeplacerSerpentIndividuel(serpent2, &longueur_serpent2, &direction2,
                               file_directions2, &file_tete2, &file_queue2,
                               &score2, &carburant2, &jeu_termine2,
                               (Color){255, 100, 100, 200}); // Rouge pour J2
  }

  // Verifier collision entre serpents (mode 2 joueurs)
  if (modeActuel == DEUX_JOUEURS) {
    // Tete de J1 contre corps de J2
    if (!jeu_termine1 && VerifierCollisionEntreSerpents(serpent1, serpent2, longueur_serpent2)) {
      jeu_termine1 = true;
      shake_intensity = 10.0f;
    }
    // Tete de J2 contre corps de J1
    if (!jeu_termine2 && VerifierCollisionEntreSerpents(serpent2, serpent1, longueur_serpent1)) {
      jeu_termine2 = true;
      shake_intensity = 10.0f;
    }

    // Determiner le gagnant si un joueur meurt
    if (jeu_termine1 && !jeu_termine2) {
      winner = 2; // J2 gagne
    } else if (jeu_termine2 && !jeu_termine1) {
      winner = 1; // J1 gagne
    } else if (jeu_termine1 && jeu_termine2) {
      winner = 0; // Egalite (les deux morts)
    }
  }
}

// Fonction pour dessiner un vaisseau detaille (player_id: 1 = bleu, 2 = rouge)
static void DessinerVaisseau(int x, int y, int dir, int player_id) {
  int cx = x * TAILLE_CASE + TAILLE_CASE / 2;
  int cy = y * TAILLE_CASE + TAILLE_CASE / 2;

  // Couleurs du vaisseau selon le joueur
  Color corps, ailes, cockpit;
  if (player_id == 2) {
    // Joueur 2 - Rouge/Orange
    corps = (Color){220, 80, 60, 255};
    ailes = (Color){180, 60, 40, 255};
    cockpit = (Color){255, 200, 180, 230};
  } else {
    // Joueur 1 - Bleu (defaut)
    corps = (Color){60, 140, 220, 255};
    ailes = (Color){40, 100, 180, 255};
    cockpit = (Color){200, 240, 255, 230};
  }
  Color contour = WHITE;
  Color flamme1 = (Color){255, 100, 30, 255};
  Color flamme2 = (Color){255, 200, 50, 255};
  Color flamme3 = (Color){255, 255, 150, 255};

  if (dir == DROITE) {
    // Flammes reacteurs
    DrawCircle(cx - 20, cy - 5, 5, flamme1);
    DrawCircle(cx - 20, cy + 5, 5, flamme1);
    DrawCircle(cx - 18, cy - 5, 3, flamme2);
    DrawCircle(cx - 18, cy + 5, 3, flamme2);
    DrawCircle(cx - 16, cy - 5, 2, flamme3);
    DrawCircle(cx - 16, cy + 5, 2, flamme3);
    // Corps
    DrawRectangle(cx - 12, cy - 9, 24, 18, corps);
    DrawRectangleLines(cx - 12, cy - 9, 24, 18, contour);
    // Ailes
    DrawRectangle(cx - 8, cy - 13, 14, 4, ailes);
    DrawRectangle(cx - 8, cy + 9, 14, 4, ailes);
    DrawRectangleLines(cx - 8, cy - 13, 14, 4, contour);
    DrawRectangleLines(cx - 8, cy + 9, 14, 4, contour);
    // Cockpit
    DrawRectangle(cx + 6, cy - 4, 6, 8, cockpit);
  } else if (dir == GAUCHE) {
    DrawCircle(cx + 20, cy - 5, 5, flamme1);
    DrawCircle(cx + 20, cy + 5, 5, flamme1);
    DrawCircle(cx + 18, cy - 5, 3, flamme2);
    DrawCircle(cx + 18, cy + 5, 3, flamme2);
    DrawCircle(cx + 16, cy - 5, 2, flamme3);
    DrawCircle(cx + 16, cy + 5, 2, flamme3);
    DrawRectangle(cx - 12, cy - 9, 24, 18, corps);
    DrawRectangleLines(cx - 12, cy - 9, 24, 18, contour);
    DrawRectangle(cx - 6, cy - 13, 14, 4, ailes);
    DrawRectangle(cx - 6, cy + 9, 14, 4, ailes);
    DrawRectangleLines(cx - 6, cy - 13, 14, 4, contour);
    DrawRectangleLines(cx - 6, cy + 9, 14, 4, contour);
    DrawRectangle(cx - 12, cy - 4, 6, 8, cockpit);
  } else if (dir == HAUT) {
    DrawCircle(cx - 5, cy + 20, 5, flamme1);
    DrawCircle(cx + 5, cy + 20, 5, flamme1);
    DrawCircle(cx - 5, cy + 18, 3, flamme2);
    DrawCircle(cx + 5, cy + 18, 3, flamme2);
    DrawCircle(cx - 5, cy + 16, 2, flamme3);
    DrawCircle(cx + 5, cy + 16, 2, flamme3);
    DrawRectangle(cx - 9, cy - 12, 18, 24, corps);
    DrawRectangleLines(cx - 9, cy - 12, 18, 24, contour);
    DrawRectangle(cx - 13, cy - 6, 4, 14, ailes);
    DrawRectangle(cx + 9, cy - 6, 4, 14, ailes);
    DrawRectangleLines(cx - 13, cy - 6, 4, 14, contour);
    DrawRectangleLines(cx + 9, cy - 6, 4, 14, contour);
    DrawRectangle(cx - 4, cy - 12, 8, 6, cockpit);
  } else {
    DrawCircle(cx - 5, cy - 20, 5, flamme1);
    DrawCircle(cx + 5, cy - 20, 5, flamme1);
    DrawCircle(cx - 5, cy - 18, 3, flamme2);
    DrawCircle(cx + 5, cy - 18, 3, flamme2);
    DrawCircle(cx - 5, cy - 16, 2, flamme3);
    DrawCircle(cx + 5, cy - 16, 2, flamme3);
    DrawRectangle(cx - 9, cy - 12, 18, 24, corps);
    DrawRectangleLines(cx - 9, cy - 12, 18, 24, contour);
    DrawRectangle(cx - 13, cy - 8, 4, 14, ailes);
    DrawRectangle(cx + 9, cy - 8, 4, 14, ailes);
    DrawRectangleLines(cx - 13, cy - 8, 4, 14, contour);
    DrawRectangleLines(cx + 9, cy - 8, 4, 14, contour);
    DrawRectangle(cx - 4, cy + 6, 8, 6, cockpit);
  }
}

// Fonction pour ajouter une direction a la file d'un joueur
static void AjouterDirectionFile(int dir, int *file_directions, int *file_tete, int *file_queue) {
  int prochaine_queue = (*file_queue + 1) % TAILLE_FILE;
  if (prochaine_queue != *file_tete) { // File pas pleine
    file_directions[*file_queue] = dir;
    *file_queue = prochaine_queue;
  }
}

// Fonction pour gerer les entrees clavier
static void GererEntrees() {
  // --- Joueur 1: ZQSD ---
  if (!jeu_termine1) {
    int derniere_dir1 = direction1;
    if (file_tete1 != file_queue1) {
      int idx = (file_queue1 - 1 + TAILLE_FILE) % TAILLE_FILE;
      derniere_dir1 = file_directions1[idx];
    }

    if (IS_P1_UP_PRESSED() && derniere_dir1 != BAS) {
      AjouterDirectionFile(HAUT, file_directions1, &file_tete1, &file_queue1);
    }
    if (IS_P1_DOWN_PRESSED() && derniere_dir1 != HAUT) {
      AjouterDirectionFile(BAS, file_directions1, &file_tete1, &file_queue1);
    }
    if (IS_P1_LEFT_PRESSED() && derniere_dir1 != DROITE) {
      AjouterDirectionFile(GAUCHE, file_directions1, &file_tete1, &file_queue1);
    }
    if (IS_P1_RIGHT_PRESSED() && derniere_dir1 != GAUCHE) {
      AjouterDirectionFile(DROITE, file_directions1, &file_tete1, &file_queue1);
    }
  }

  // --- Joueur 2: Fleches (mode 2 joueurs uniquement) ---
  if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
    int derniere_dir2 = direction2;
    if (file_tete2 != file_queue2) {
      int idx = (file_queue2 - 1 + TAILLE_FILE) % TAILLE_FILE;
      derniere_dir2 = file_directions2[idx];
    }

    if (IS_P2_UP_PRESSED() && derniere_dir2 != BAS) {
      AjouterDirectionFile(HAUT, file_directions2, &file_tete2, &file_queue2);
    }
    if (IS_P2_DOWN_PRESSED() && derniere_dir2 != HAUT) {
      AjouterDirectionFile(BAS, file_directions2, &file_tete2, &file_queue2);
    }
    if (IS_P2_LEFT_PRESSED() && derniere_dir2 != DROITE) {
      AjouterDirectionFile(GAUCHE, file_directions2, &file_tete2, &file_queue2);
    }
    if (IS_P2_RIGHT_PRESSED() && derniere_dir2 != GAUCHE) {
      AjouterDirectionFile(DROITE, file_directions2, &file_tete2, &file_queue2);
    }
  }
}

// Fonction pour dessiner le jeu
static void DessinerJeu(float scale, Vector2 shake_offset) {
  // Rendu dans la texture à la taille native du jeu
  BeginTextureMode(snakeRenderTarget);
  ClearBackground((Color){10, 10, 30, 255});

  // Camera Shake (INTERNE AU JEU, distinct de la transition)
  Camera2D camera = {0};
  camera.zoom = 1.0f;
  if (shake_intensity > 0) {
    camera.offset.x = (float)GetRandomValue(-shake_intensity, shake_intensity);
    camera.offset.y = (float)GetRandomValue(-shake_intensity, shake_intensity);
    shake_intensity -= 0.5f;
    if (shake_intensity < 0)
      shake_intensity = 0;
  }

  BeginMode2D(camera);

  // Dessiner les etoiles (Arriere-plan)
  for (int i = 0; i < MAX_STARS; i++) {
    Color starColor = {255, 255, 255,
                       (unsigned char)(stars[i].brightness * 255)};
    DrawPixel(stars[i].x, stars[i].y, starColor);
  }

  // Dessiner la grille (plus subtile)
  for (int i = 0; i <= LARGEUR_GRILLE; i++) {
    DrawLine(i * TAILLE_CASE, 0, i * TAILLE_CASE, HAUTEUR_GRILLE * TAILLE_CASE,
             (Color){30, 30, 60, 100});
  }
  for (int i = 0; i <= HAUTEUR_GRILLE; i++) {
    DrawLine(0, i * TAILLE_CASE, LARGEUR_GRILLE * TAILLE_CASE, i * TAILLE_CASE,
             (Color){30, 30, 60, 100});
  }

  // Dessiner les particules (derriere les objets)
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].active) {
      DrawCircleV(particles[i].position, particles[i].size,
                  Fade(particles[i].color, particles[i].life));
    }
  }

  // Dessiner les trous noirs
  for (int i = 0; i < nombre_murs; i++) {
    if (murs[i].x < 0)
      continue;
    int cx = murs[i].x * TAILLE_CASE + TAILLE_CASE / 2;
    int cy = murs[i].y * TAILLE_CASE + TAILLE_CASE / 2;
    float rotation = (float)GetTime() * 3.0f;

    // Lueur exterieure violette
    DrawCircle(cx, cy, 18, Fade((Color){100, 0, 180, 100}, 0.5f));
    // Couches du trou noir
    DrawCircle(cx, cy, 14, (Color){60, 0, 100, 255});
    DrawCircle(cx, cy, 11, (Color){40, 0, 70, 255});
    DrawCircle(cx, cy, 8, (Color){20, 0, 40, 255});
    DrawCircle(cx, cy, 4, BLACK);
    // Contour brillant
    DrawCircleLines(cx, cy, 14, (Color){150, 80, 200, 255});

    // Anneau qui tourne
    for (int j = 0; j < 6; j++) {
      float angle = rotation + j * 1.047f;
      int px = cx + cosf(angle) * 13;
      int py = cy + sinf(angle) * 13;
      DrawCircle(px, py, 2, (Color){180, 100, 255, 220});
    }
  }

  // Dessiner la trainee du vaisseau 1 (effet neon cyan)
  if (!jeu_termine1) {
    for (int i = 1; i < longueur_serpent1; i++) {
      if (serpent1[i].x < 0 || serpent1[i].y < 0)
        continue;

      float opacite = 1.0f - ((float)i / longueur_serpent1) * 0.85f;
      int centre_x = serpent1[i].x * TAILLE_CASE + TAILLE_CASE / 2;
      int centre_y = serpent1[i].y * TAILLE_CASE + TAILLE_CASE / 2;

      DrawCircle(centre_x, centre_y, 12, Fade((Color){0, 200, 255, 80}, opacite));
      DrawCircle(centre_x, centre_y, 8, Fade((Color){50, 220, 255, 150}, opacite));
      DrawCircle(centre_x, centre_y, 5, Fade((Color){150, 240, 255, 220}, opacite));
      DrawCircle(centre_x, centre_y, 2, Fade(WHITE, opacite));
    }
  }

  // Dessiner la trainee du vaisseau 2 (effet neon rouge) - mode 2 joueurs
  if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
    for (int i = 1; i < longueur_serpent2; i++) {
      if (serpent2[i].x < 0 || serpent2[i].y < 0)
        continue;

      float opacite = 1.0f - ((float)i / longueur_serpent2) * 0.85f;
      int centre_x = serpent2[i].x * TAILLE_CASE + TAILLE_CASE / 2;
      int centre_y = serpent2[i].y * TAILLE_CASE + TAILLE_CASE / 2;

      DrawCircle(centre_x, centre_y, 12, Fade((Color){255, 100, 100, 80}, opacite));
      DrawCircle(centre_x, centre_y, 8, Fade((Color){255, 150, 150, 150}, opacite));
      DrawCircle(centre_x, centre_y, 5, Fade((Color){255, 200, 200, 220}, opacite));
      DrawCircle(centre_x, centre_y, 2, Fade(WHITE, opacite));
    }
  }

  // Dessiner le vaisseau 1 (bleu)
  if (!jeu_termine1) {
    DessinerVaisseau(serpent1[0].x, serpent1[0].y, direction1, 1);
  }

  // Dessiner le vaisseau 2 (rouge) - mode 2 joueurs
  if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
    DessinerVaisseau(serpent2[0].x, serpent2[0].y, direction2, 2);
  }

  // Dessiner le jerrican de carburant
  int jer_x = pomme.x * TAILLE_CASE + TAILLE_CASE / 2;
  int jer_y = pomme.y * TAILLE_CASE + TAILLE_CASE / 2;

  // Lueur verte autour
  DrawCircle(jer_x, jer_y, 16, Fade((Color){50, 255, 50, 100}, 0.4f));
  // Corps du jerrican
  DrawRectangle(jer_x - 10, jer_y - 6, 20, 16, (Color){30, 140, 30, 255});
  DrawRectangle(jer_x - 8, jer_y - 4, 16, 12, (Color){50, 200, 50, 255});
  DrawRectangleLines(jer_x - 10, jer_y - 6, 20, 16,
                     (Color){200, 255, 200, 255});
  // Goulot
  DrawRectangle(jer_x - 3, jer_y - 12, 6, 8, (Color){40, 120, 40, 255});
  DrawRectangleLines(jer_x - 3, jer_y - 12, 6, 8, (Color){200, 255, 200, 255});
  // Poignee
  DrawRectangle(jer_x + 5, jer_y - 10, 6, 4, (Color){80, 80, 80, 255});
  // Bandes de danger
  DrawRectangle(jer_x - 8, jer_y - 2, 16, 3, (Color){255, 200, 0, 255});
  DrawRectangle(jer_x - 8, jer_y + 4, 16, 3, (Color){255, 200, 0, 255});

  // Dessiner le deuxieme jerrican (mode 2 joueurs) - orange
  if (modeActuel == DEUX_JOUEURS) {
    int jer2_x = pomme2.x * TAILLE_CASE + TAILLE_CASE / 2;
    int jer2_y = pomme2.y * TAILLE_CASE + TAILLE_CASE / 2;

    // Lueur orange autour
    DrawCircle(jer2_x, jer2_y, 16, Fade((Color){255, 150, 50, 100}, 0.4f));
    // Corps du jerrican
    DrawRectangle(jer2_x - 10, jer2_y - 6, 20, 16, (Color){180, 90, 20, 255});
    DrawRectangle(jer2_x - 8, jer2_y - 4, 16, 12, (Color){255, 140, 40, 255});
    DrawRectangleLines(jer2_x - 10, jer2_y - 6, 20, 16,
                       (Color){255, 220, 180, 255});
    // Goulot
    DrawRectangle(jer2_x - 3, jer2_y - 12, 6, 8, (Color){150, 80, 20, 255});
    DrawRectangleLines(jer2_x - 3, jer2_y - 12, 6, 8, (Color){255, 220, 180, 255});
    // Poignee
    DrawRectangle(jer2_x + 5, jer2_y - 10, 6, 4, (Color){80, 80, 80, 255});
    // Bandes de danger
    DrawRectangle(jer2_x - 8, jer2_y - 2, 16, 3, (Color){255, 255, 100, 255});
    DrawRectangle(jer2_x - 8, jer2_y + 4, 16, 3, (Color){255, 255, 100, 255});
  }

  EndMode2D(); // Fin de la camera (shake)

  // UI (ne bouge pas avec le shake)
  int barre_y = HAUTEUR_GRILLE * TAILLE_CASE + 18;

  if (modeActuel == DEUX_JOUEURS) {
    // Mode 2 joueurs - barres de carburant separees
    int barre_largeur = (LARGEUR_GRILLE * TAILLE_CASE - 140) / 2;

    // Barre J1 (gauche - cyan)
    float ratio1 = (float)carburant1 / CARBURANT_MAX;
    if (ratio1 < 0) ratio1 = 0;
    DrawText("P1", 5, barre_y, 16, (Color){0, 200, 255, 255});
    DrawRectangle(30, barre_y, barre_largeur, 16, (Color){50, 50, 50, 255});
    Color couleur1 = (ratio1 > 0.3f) ? (Color){0, 200, 255, 255} : (Color){200, 50, 50, 255};
    DrawRectangle(30, barre_y, (int)(barre_largeur * ratio1), 16, couleur1);
    DrawRectangleLines(30, barre_y, barre_largeur, 16, WHITE);
    DrawText(TextFormat("%d", score1), 30 + barre_largeur + 5, barre_y, 16, (Color){0, 200, 255, 255});

    // Barre J2 (droite - rouge)
    int barre_x2 = LARGEUR_GRILLE * TAILLE_CASE / 2 + 20;
    float ratio2 = (float)carburant2 / CARBURANT_MAX;
    if (ratio2 < 0) ratio2 = 0;
    DrawText("P2", barre_x2 - 25, barre_y, 16, (Color){255, 100, 100, 255});
    DrawRectangle(barre_x2, barre_y, barre_largeur, 16, (Color){50, 50, 50, 255});
    Color couleur2 = (ratio2 > 0.3f) ? (Color){255, 100, 100, 255} : (Color){200, 50, 50, 255};
    DrawRectangle(barre_x2, barre_y, (int)(barre_largeur * ratio2), 16, couleur2);
    DrawRectangleLines(barre_x2, barre_y, barre_largeur, 16, WHITE);
    DrawText(TextFormat("%d", score2), barre_x2 + barre_largeur + 5, barre_y, 16, (Color){255, 100, 100, 255});
  } else {
    // Mode 1 joueur - barre unique
    int barre_largeur = LARGEUR_GRILLE * TAILLE_CASE - 120;
    int barre_x = 110;
    float ratio_carburant = (float)carburant1 / CARBURANT_MAX;
    if (ratio_carburant < 0) ratio_carburant = 0;

    DrawRectangle(barre_x, barre_y, barre_largeur, 16, (Color){50, 50, 50, 255});
    Color couleur_fuel = (ratio_carburant > 0.3f) ? (Color){50, 200, 50, 255}
                                                  : (Color){200, 50, 50, 255};
    DrawRectangle(barre_x, barre_y, (int)(barre_largeur * ratio_carburant), 16, couleur_fuel);
    DrawRectangleLines(barre_x, barre_y, barre_largeur, 16, WHITE);
    DrawText("FUEL:", 60, barre_y, 16, WHITE);

    DrawText(TextFormat("Score: %d", score1), 10, HAUTEUR_GRILLE * TAILLE_CASE + 2, 14, WHITE);
  }

  const char *diff_text = (difficulte_actuelle == FACILE)  ? "EASY"
                          : (difficulte_actuelle == MOYEN) ? "MEDIUM"
                                                           : "HARD";
  Color diff_color = (difficulte_actuelle == FACILE)  ? GREEN
                     : (difficulte_actuelle == MOYEN) ? YELLOW
                                                      : RED;
  if (modeActuel != DEUX_JOUEURS) {
    DrawText(TextFormat("%s (%d holes)", diff_text, getMaxBlackHoles()), 150,
             HAUTEUR_GRILLE * TAILLE_CASE + 2, 14, diff_color);
  }

  // Afficher message de fin
  bool game_over = (modeActuel == DEUX_JOUEURS) ? (winner != 0) : jeu_termine1;
  if (game_over) {
    DrawRectangle(0, 0, LARGEUR_GRILLE * TAILLE_CASE,
                  HAUTEUR_GRILLE * TAILLE_CASE, Fade(BLACK, 0.85f));

    float pulse = (sinf((float)GetTime() * 5.0f) + 1.0f) * 0.5f;

    if (modeActuel == DEUX_JOUEURS && winner != 0) {
      // Afficher le gagnant
      const char *winner_text = (winner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
      Color winner_color = (winner == 1) ? (Color){0, 200, 255, 255} : (Color){255, 100, 100, 255};
      int text_width = MeasureText(winner_text, 40);
      DrawText(winner_text, (LARGEUR_GRILLE * TAILLE_CASE - text_width) / 2,
               HAUTEUR_GRILLE * TAILLE_CASE / 2 - 40, 40, ColorAlpha(winner_color, 0.5f + pulse * 0.5f));
    } else {
      // Mode 1 joueur - Game Over classique
      Color pulseColor = ColorAlpha(RED, 0.5f + pulse * 0.5f);
      DrawText("GAME OVER!", LARGEUR_GRILLE * TAILLE_CASE / 2 - 80,
               HAUTEUR_GRILLE * TAILLE_CASE / 2 - 40, 40, pulseColor);
      DrawText(TextFormat("Score final: %d", score1),
               LARGEUR_GRILLE * TAILLE_CASE / 2 - 90,
               HAUTEUR_GRILLE * TAILLE_CASE / 2 + 10, 20, WHITE);
    }
    DrawText("Press ESCAPE to Quit", LARGEUR_GRILLE * TAILLE_CASE / 2 - 100,
             HAUTEUR_GRILLE * TAILLE_CASE / 2 + 40, 20, LIGHTGRAY);
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
  Vector2 origin = {
      0, 0}; // Le coin haut-gauche de destRec est déjà calculé centré

  DrawTexturePro(snakeRenderTarget.texture, sourceRec, destRec, origin, 0.0f,
                 WHITE);

  // Afficher le temps global sur le côté (dans la zone noire)
  // On ne l'affiche que si on est proche de l'échelle 1 (pas trop petit)
  if (snakeGlobalTime != NULL && scale > 0.8f) {
    DrawText("TOTAL TIME", 20, HAUTEUR_FENETRE / 2 - 20, 20, GRAY);
    DrawText(TextFormat("%.1fs", *snakeGlobalTime), 20, HAUTEUR_FENETRE / 2 + 5,
             30, WHITE);
  }

  EndDrawing();
}

void RunSnakeGame(Difficulte difficulty, ModeJoueur mode, bool resume, bool *game_active,
                  const char *playerName, ScoreList *scores,
                  float timeout_seconds, float *total_time, float start_delay) {
  snakeGlobalTime = total_time;
  modeActuel = mode;

  // Configurer la fenetre
  SetWindowSize(LARGEUR_FENETRE, HAUTEUR_FENETRE);
  SetWindowTitle("Snake");
  SetTargetFPS(60);

  // Creer la render texture pour le rendu centre
  snakeRenderTarget = LoadRenderTexture(LARGEUR_JEU, HAUTEUR_JEU);

  // Utiliser la difficulte passee par le menu
  difficulte_actuelle = difficulty;

  // Initialiser le generateur de nombres aleatoires
  srand((unsigned int)time(NULL));

  // Initialiser les etoiles et les particules (toujours necessaire)
  InitStars();
  InitParticles();

  // Initialiser SEULEMENT si on ne reprend pas une partie
  if (!resume) {
    InitialiserJeu();
  }

  *game_active = true;
  float elapsed_time = 0.0f;

  // Boucle principale du jeu
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    elapsed_time += dt;

    // Determiner si le jeu est termine selon le mode
    bool game_over;
    if (modeActuel == DEUX_JOUEURS) {
      game_over = (winner != 0); // Un joueur a gagne
    } else {
      game_over = jeu_termine1;
    }

    // Gestion des transitions (Zoom/Shake)
    float current_scale = 1.0f;
    Vector2 shake_transition = {0.0f, 0.0f};

    // 1. Zoom In au debut (pendant start_delay)
    if (elapsed_time < start_delay) {
      float factor = elapsed_time / start_delay;
      current_scale = Lerp(0.1f, 1.0f, factor);
    }
    // 2. Shake + Zoom Out a la fin (0.5s avant fin)
    else if (!game_over && timeout_seconds > 0 &&
             (timeout_seconds - elapsed_time) < 0.5f) {
      float remaining = timeout_seconds - elapsed_time;
      float factor = remaining / 0.5f;
      current_scale = Lerp(0.1f, 1.0f, factor);

      float shake_amount = (1.0f - factor) * 10.0f;
      shake_transition.x = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
      shake_transition.y = (float)GetRandomValue(-100, 100) / 100.0f * shake_amount;
    }

    // Accumuler le temps global
    if (total_time != NULL && !game_over) {
      *total_time += dt;
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

    if (!game_over) {
      // Mise a jour de la logique
      if (elapsed_time >= start_delay) {
        GererEntrees();

        // Timer de jeu
        temps_derniere_maj += dt;
        if (temps_derniere_maj >= vitesse_jeu) {
          temps_derniere_maj = 0;
          DeplacerSerpents();
        }

        // Gestion de la consommation de carburant - Joueur 1
        if (!jeu_termine1) {
          carburant1 -= dt;
          if (carburant1 <= 0) {
            jeu_termine1 = true;
            shake_intensity = 5.0f;
            if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
              winner = 2;
            }
          }
        }

        // Gestion de la consommation de carburant - Joueur 2 (mode 2 joueurs)
        if (modeActuel == DEUX_JOUEURS && !jeu_termine2) {
          carburant2 -= dt;
          if (carburant2 <= 0) {
            jeu_termine2 = true;
            shake_intensity = 5.0f;
            if (!jeu_termine1) {
              winner = 1;
            }
          }
        }
      }
      UpdateStars();
      UpdateParticles();
    }

    DessinerJeu(current_scale, shake_transition);
  }

  // Liberer la render texture
  UnloadRenderTexture(snakeRenderTarget);
}
