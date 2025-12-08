#include <math.h>
#include <raylib.h>
#include <stdlib.h>
#include <time.h>

// Taille de la grille (plus petite pour elements plus gros)
#define LARGEUR_GRILLE 15
#define HAUTEUR_GRILLE 15
#define TAILLE_CASE 32

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

// Variables globales du jeu
Position serpent[225]; // tableau pour stocker le corps du serpent
int longueur_serpent;
int direction;
int direction_suivante; // buffer pour la prochaine direction
Position pomme;         // jerrican de carburant
int score;
bool jeu_termine;
double temps_derniere_maj;
double vitesse_jeu;

// File de mouvements (input queue)
#define TAILLE_FILE 3
int file_directions[TAILLE_FILE];
int file_tete;
int file_queue;

// Systeme de carburant
double carburant; // temps restant avant panne seche (5 secondes max)
#define CARBURANT_MAX 5.0

// Obstacles (trous noirs qui apparaissent)
Position murs[100];
int nombre_murs;

// Fonction pour generer une pomme a une position aleatoire
// Fonction pour generer une pomme a une position aleatoire
void GenererPomme() {
  bool position_valide = false;

  while (!position_valide) {
    pomme.x = GetRandomValue(0, LARGEUR_GRILLE - 1);
    pomme.y = GetRandomValue(0, HAUTEUR_GRILLE - 1);

    position_valide = true;
    // Verifier que la pomme n'apparait pas sur le serpent
    for (int i = 0; i < longueur_serpent; i++) {
      if (serpent[i].x == pomme.x && serpent[i].y == pomme.y) {
        position_valide = false;
        break;
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

// Fonction pour initialiser le jeu
void InitialiserJeu() {
  longueur_serpent = 3;
  direction = DROITE;
  direction_suivante = DROITE;
  score = 0;
  jeu_termine = false;
  vitesse_jeu = 0.15;
  temps_derniere_maj = 0;
  nombre_murs = 0;
  carburant = CARBURANT_MAX; // Reservoir plein au depart
  file_tete = 0;
  file_queue = 0;

  // Initialiser toutes les positions pour eviter le bug de trainee
  for (int i = 0; i < 225; i++) {
    serpent[i].x = -1;
    serpent[i].y = -1;
  }

  // Initialiser les murs
  for (int i = 0; i < 100; i++) {
    murs[i].x = -1;
    murs[i].y = -1;
  }

  // Placer le serpent au centre
  serpent[0].x = LARGEUR_GRILLE / 2;
  serpent[0].y = HAUTEUR_GRILLE / 2;
  serpent[1].x = serpent[0].x - 1;
  serpent[1].y = serpent[0].y;
  serpent[2].x = serpent[1].x - 1;
  serpent[2].y = serpent[1].y;

  GenererPomme();
}

// Fonction pour ajouter un mur aleatoire
void AjouterMur() {
  if (nombre_murs >= 100)
    return;

  bool position_valide = false;
  int tentatives = 0;

  while (!position_valide && tentatives < 200) {
    int nx = GetRandomValue(0, LARGEUR_GRILLE - 1);
    int ny = GetRandomValue(0, HAUTEUR_GRILLE - 1);
    position_valide = true;

    // Verifier que ce n'est pas sur le serpent
    for (int i = 0; i < longueur_serpent; i++) {
      if (serpent[i].x == nx && serpent[i].y == ny) {
        position_valide = false;
        break;
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

    // Verifier que ce n'est pas sur les 2 blocs devant le vaisseau
    for (int d = 1; d <= 2; d++) {
      int fx = serpent[0].x;
      int fy = serpent[0].y;
      if (direction == DROITE)
        fx += d;
      else if (direction == GAUCHE)
        fx -= d;
      else if (direction == HAUT)
        fy -= d;
      else if (direction == BAS)
        fy += d;

      if (nx == fx && ny == fy) {
        position_valide = false;
        break;
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
bool VerifierCollisionCorps() {
  for (int i = 1; i < longueur_serpent; i++) {
    if (serpent[0].x == serpent[i].x && serpent[0].y == serpent[i].y) {
      return true;
    }
  }
  return false;
}

// Fonction pour deplacer le serpent
void DeplacerSerpent() {
  // Consommer la prochaine direction de la file si disponible
  if (file_tete != file_queue) {
    int nouvelle_dir = file_directions[file_tete];
    // Verifier que ce n'est pas un demi-tour
    if (!((direction == HAUT && nouvelle_dir == BAS) ||
          (direction == BAS && nouvelle_dir == HAUT) ||
          (direction == GAUCHE && nouvelle_dir == DROITE) ||
          (direction == DROITE && nouvelle_dir == GAUCHE))) {
      direction = nouvelle_dir;
    }
    file_tete = (file_tete + 1) % TAILLE_FILE;
  }

  // Deplacer le corps
  for (int i = longueur_serpent - 1; i > 0; i--) {
    serpent[i] = serpent[i - 1];
  }

  // Deplacer la tete selon la direction
  if (direction == HAUT) {
    serpent[0].y--;
  } else if (direction == BAS) {
    serpent[0].y++;
  } else if (direction == GAUCHE) {
    serpent[0].x--;
  } else if (direction == DROITE) {
    serpent[0].x++;
  }

  // Verifier collision avec les murs
  if (serpent[0].x < 0 || serpent[0].x >= LARGEUR_GRILLE || serpent[0].y < 0 ||
      serpent[0].y >= HAUTEUR_GRILLE) {
    jeu_termine = true;
  }

  // Verifier collision avec le corps
  if (VerifierCollisionCorps()) {
    jeu_termine = true;
  }

  // Verifier collision avec les obstacles
  for (int i = 0; i < nombre_murs; i++) {
    if (serpent[0].x == murs[i].x && serpent[0].y == murs[i].y) {
      jeu_termine = true;
      break;
    }
  }

  // Verifier si le serpent prend le jerrican
  if (serpent[0].x == pomme.x && serpent[0].y == pomme.y) {
    longueur_serpent++;
    score += 10;
    carburant = CARBURANT_MAX; // Reservoir plein!
    GenererPomme();
    AjouterMur(); // Ajouter un trou noir pour rendre le jeu plus difficile

    // Augmenter la vitesse progressivement
    if (vitesse_jeu > 0.05) {
      vitesse_jeu -= 0.005;
    }
  }
}

// Fonction pour dessiner un vaisseau detaille
void DessinerVaisseau(int x, int y, int dir) {
  int cx = x * TAILLE_CASE + TAILLE_CASE / 2;
  int cy = y * TAILLE_CASE + TAILLE_CASE / 2;

  // Couleurs du vaisseau
  Color corps = (Color){60, 140, 220, 255};
  Color ailes = (Color){40, 100, 180, 255};
  Color cockpit = (Color){200, 240, 255, 230};
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

// Fonction pour ajouter une direction a la file
void AjouterDirectionFile(int dir) {
  int prochaine_queue = (file_queue + 1) % TAILLE_FILE;
  if (prochaine_queue != file_tete) { // File pas pleine
    file_directions[file_queue] = dir;
    file_queue = prochaine_queue;
  }
}

// Fonction pour gerer les entrees clavier
void GererEntrees() {
  // Determiner la derniere direction dans la file (ou la direction actuelle)
  int derniere_dir = direction;
  if (file_tete != file_queue) {
    int idx = (file_queue - 1 + TAILLE_FILE) % TAILLE_FILE;
    derniere_dir = file_directions[idx];
  }

  // Fleches directionnelles
  if ((IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_Z)) &&
      derniere_dir != BAS) {
    AjouterDirectionFile(HAUT);
  }
  if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) && derniere_dir != HAUT) {
    AjouterDirectionFile(BAS);
  }
  if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_Q)) &&
      derniere_dir != DROITE) {
    AjouterDirectionFile(GAUCHE);
  }
  if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) &&
      derniere_dir != GAUCHE) {
    AjouterDirectionFile(DROITE);
  }
}

// Fonction pour dessiner le jeu
void DessinerJeu() {
  BeginDrawing();
  ClearBackground((Color){10, 10, 30, 255});

  // Dessiner la grille
  for (int i = 0; i <= LARGEUR_GRILLE; i++) {
    DrawLine(i * TAILLE_CASE, 0, i * TAILLE_CASE, HAUTEUR_GRILLE * TAILLE_CASE,
             (Color){30, 30, 60, 255});
  }
  for (int i = 0; i <= HAUTEUR_GRILLE; i++) {
    DrawLine(0, i * TAILLE_CASE, LARGEUR_GRILLE * TAILLE_CASE, i * TAILLE_CASE,
             (Color){30, 30, 60, 255});
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

  // Dessiner la trainee du vaisseau (effet neon)
  for (int i = 1; i < longueur_serpent; i++) {
    if (serpent[i].x < 0 || serpent[i].y < 0)
      continue;

    float opacite = 1.0f - ((float)i / longueur_serpent) * 0.85f;
    int centre_x = serpent[i].x * TAILLE_CASE + TAILLE_CASE / 2;
    int centre_y = serpent[i].y * TAILLE_CASE + TAILLE_CASE / 2;

    // Effet de lueur neon cyan
    DrawCircle(centre_x, centre_y, 12, Fade((Color){0, 200, 255, 80}, opacite));
    DrawCircle(centre_x, centre_y, 8,
               Fade((Color){50, 220, 255, 150}, opacite));
    DrawCircle(centre_x, centre_y, 5,
               Fade((Color){150, 240, 255, 220}, opacite));
    DrawCircle(centre_x, centre_y, 2, Fade(WHITE, opacite));
  }

  // Dessiner le vaisseau detaille
  DessinerVaisseau(serpent[0].x, serpent[0].y, direction);

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

  // Barre de carburant en bas
  int barre_largeur = LARGEUR_GRILLE * TAILLE_CASE - 120;
  int barre_x = 110;
  int barre_y = HAUTEUR_GRILLE * TAILLE_CASE + 18;
  float ratio_carburant = (float)carburant / CARBURANT_MAX;
  if (ratio_carburant < 0)
    ratio_carburant = 0;

  // Fond de la barre
  DrawRectangle(barre_x, barre_y, barre_largeur, 16, (Color){50, 50, 50, 255});
  // Carburant restant
  Color couleur_fuel = (ratio_carburant > 0.3f) ? (Color){50, 200, 50, 255}
                                                : (Color){200, 50, 50, 255};
  DrawRectangle(barre_x, barre_y, (int)(barre_largeur * ratio_carburant), 16,
                couleur_fuel);
  // Contour
  DrawRectangleLines(barre_x, barre_y, barre_largeur, 16, WHITE);
  // Label
  DrawText("FUEL:", 60, barre_y, 16, WHITE);

  // Afficher le score
  DrawText(TextFormat("Score: %d", score), 10, HAUTEUR_GRILLE * TAILLE_CASE + 2,
           14, WHITE);

  // Afficher message de fin si le jeu est termine
  if (jeu_termine) {
    DrawRectangle(0, 0, LARGEUR_GRILLE * TAILLE_CASE,
                  HAUTEUR_GRILLE * TAILLE_CASE, Fade(BLACK, 0.7f));
    DrawText("GAME OVER!", LARGEUR_GRILLE * TAILLE_CASE / 2 - 80,
             HAUTEUR_GRILLE * TAILLE_CASE / 2 - 40, 40, RED);
    DrawText(TextFormat("Score final: %d", score),
             LARGEUR_GRILLE * TAILLE_CASE / 2 - 90,
             HAUTEUR_GRILLE * TAILLE_CASE / 2 + 10, 20, WHITE);
    DrawText("Appuyez sur ESPACE pour rejouer",
             LARGEUR_GRILLE * TAILLE_CASE / 2 - 150,
             HAUTEUR_GRILLE * TAILLE_CASE / 2 + 40, 20, WHITE);
  }

  EndDrawing();
}

int main(void) {
  // Initialiser la fenetre
  InitWindow(LARGEUR_GRILLE * TAILLE_CASE, HAUTEUR_GRILLE * TAILLE_CASE + 50,
             "Snake Game");

  SetTargetFPS(60);

  // Initialiser le generateur de nombres aleatoires
  srand(time(NULL));

  InitialiserJeu();

  // Boucle principale du jeu
  while (!WindowShouldClose()) {
    if (!jeu_termine) {
      GererEntrees();

      // Consommer le carburant
      carburant -= GetFrameTime();
      if (carburant <= 0) {
        jeu_termine = true;
      }

      // Mettre a jour le jeu selon la vitesse
      temps_derniere_maj += GetFrameTime();
      if (temps_derniere_maj >= vitesse_jeu) {
        DeplacerSerpent();
        temps_derniere_maj = 0;
      }
    } else {
      // Rejouer si on appuie sur espace
      if (IsKeyPressed(KEY_SPACE)) {
        InitialiserJeu();
      }
    }

    DessinerJeu();
  }

  CloseWindow();
  return 0;
}
