#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"

// --- Constantes Fondamentales du Jeu ---
#define SCREEN_WIDTH 800 // Largeur de l'écran (Ancien : taille écran de jeu)
#define SCREEN_HEIGHT 600 // Hauteur de l'écran (Ancien : taille écran de jeu)
#define NUM_LANES_X 3 // Nombre de voies horizontales (Gauche, Centre, Droite)
#define NUM_LANES_Y 2 // Nombre de voies verticales (Bas, Haut)
#define MAX_OBSTACLES 30 // Nombre maximum d'astéroïdes simultanés
#define ASTEROID_SPEED 0.50f // Vitesse de déplacement des astéroïdes
#define ASTEROID_SPAWN_Z -50.0f // Position Z initiale lointaine des astéroïdes
#define ASTEROID_RADIUS 1.0f // Rayon de l'astéroïde (Ancien : doit être une constant finalement)

// --- Structure pour les Obstacles (Astéroïdes) ---
typedef struct Obstacle {
    float z;            // Distance entre l'astéroïde et le joueur (Ancien : distance entre lui et le joueur)
    int Voie_X;          // Voie horizontale de l'astéroïde
    int Voie_Y;          // Voie verticale de l'astéroïde
    bool active;        // Détermine si l'astéroïde est visible et actif (Ancien : Détermine si visible et actif)
    float rayon;       // Rayon de l'astéroïde
    Color color;        // Couleur de l'astéroïde (Ancien : Couleur astéoroïde)
} Obstacle;

// --- Prototypes des Fonctions ---
int LimiteEspace(int value, int min, int max); // Limite la valeur entre min et max (Ancien : vérifie que le joueur est toujours dans l'espace de jeu et l'empèche de sortir)
void InitGame(void);  // Initialise le jeu (Ancien : initialise le jeu)
void ActualiseJoueurs(void); // Met à jour la position du joueur
void ApparitionAsteroide(void); // Tente de créer un astéroïde (Ancien : Créer les astéroides)
void ActualiseAsteroide(void); // Met à jour la position et l'état des astéroïdes
void CheckCollisions(void); // Vérifie les collisions entre le joueur et les astéroïdes
void ActualiseJeu(void); // Met à jour l'état du jeu (joueur, astéroïdes, collisions) (Ancien : met à jour le jeu sans cesse permet l'évolution de la partie + fluidité)
void DessinerJeu(void); // Rend le jeu (Ancien : affichage graphique du jeu)

// --- Variables Globales ---
Vector3 playerPosition = {0.0f, 0.0f, 0.0f}; // Position du vaisseau du joueur (Ancien : position du vaisseau)
int JoeurligneX = 1;                         // Voie X actuelle (0, 1, 2) (Ancien : voie X)
int JoeurligneY = 0;                         // Voie Y actuelle (0, 1) (Ancien : voie Y)
float ligneXPositions[NUM_LANES_X] = {-2.5f, 0.0f, 2.5f}; // Positions X des voies (Ancien : positions X des voies)
float ligneYPositions[NUM_LANES_Y] = { -1.5f, 1.5f };     // Positions Y des voies (Ancien : positions Y des voies)

Obstacle obstacles[MAX_OBSTACLES]; // Tableau des astéroïdes
Camera3D camera = { 0 };           // Caméra du jeu
bool gameOver = false;             // État du jeu

// --- Fonctions ---

// Initialisation du jeu
void InitGame(void) {
    // Initialise le générateur de nombres aléatoires
    srand((unsigned int)time(NULL)); 

    // Initialisation de la Caméra
    // Position de la caméra : légèrement au-dessus (5.0f) et derrière (10.0f) l'origine.
    camera.position = (Vector3){0.0f, 5.0f, 10.0f}; 
    camera.target   = (Vector3){0.0f, 0.0f, 0.0f};  // Regarde le centre
    camera.up       = (Vector3){0.0f, 1.0f, 0.0f}; // Vecteur 'haut'
    camera.fovy     = 75.0f; // Champ de vision
    camera.projection = CAMERA_PERSPECTIVE;

    // Initialisation du joueur
    JoeurligneX = 1; // Voie X centrale par défaut
    JoeurligneY = 0; // Voie Y inférieure par défaut
    playerPosition.x = ligneXPositions[JoeurligneX];
    playerPosition.y = ligneYPositions[JoeurligneY];
    playerPosition.z = 0.0f; // Le joueur reste à z=0

    // Initialisation des astéroïdes
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = false;
        obstacles[i].rayon = ASTEROID_RADIUS;
    }
    
    gameOver = false; // Réinitialisation de l'état du jeu
}

// Limite la valeur entre min et max
int LimiteEspace(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Met à jour la position du joueur en fonction des entrées
void ActualiseJoueurs(void) {
    // Changement de voie X (gauche/droite)
    if (IsKeyPressed(KEY_LEFT))  JoeurligneX--;
    if (IsKeyPressed(KEY_RIGHT)) JoeurligneX++;
    // Changement de voie Y (haut/bas)
    if (IsKeyPressed(KEY_UP))    JoeurligneY++;
    if (IsKeyPressed(KEY_DOWN))  JoeurligneY--;

    // Limiter le joueur aux voies disponibles
    JoeurligneX = LimiteEspace(JoeurligneX, 0, NUM_LANES_X - 1);
    JoeurligneY = LimiteEspace(JoeurligneY, 0, NUM_LANES_Y - 1);

    // Mise à jour de la position 3D du joueur
    playerPosition.x = ligneXPositions[JoeurligneX];
    playerPosition.y = ligneYPositions[JoeurligneY];
}

// Tente de faire apparaître un nouvel astéroïde aléatoirement
void ApparitionAsteroide(void) {
    static float timer = 0.0f;
    // Cadence d'apparition plus rapide
    const float spawnRate = 0.15f; 

    timer += GetFrameTime();

    // Condition d'apparition: temps écoulé
    if (timer >= spawnRate) {
        timer = 0.0f; // Réinitialiser le minuteur
        
        // Trouver un astéroïde inactif
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!obstacles[i].active) {
                // Initialisation de l'astéroïde
                obstacles[i].z = ASTEROID_SPAWN_Z; // Loin devant
                // Choix aléatoire de la voie
                obstacles[i].Voie_X = rand() % NUM_LANES_X; 
                obstacles[i].Voie_Y = rand() % NUM_LANES_Y;
                obstacles[i].active = true;
                
                // Couleur aléatoire simple
                int color_choice = rand() % 3;
                if (color_choice == 0) obstacles[i].color = RED;
                else if (color_choice == 1) obstacles[i].color = GREEN;
                else obstacles[i].color = YELLOW;
                
                return; 
            }
        }
    }
}

// Met à jour la position des astéroïdes et les désactive s'ils dépassent le joueur
void ActualiseAsteroide(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            // L'astéroïde se déplace vers le joueur (z augmente)
            obstacles[i].z += ASTEROID_SPEED;

            // Si l'astéroïde a dépassé le joueur (z > playerPosition.z + rayon)
            if (obstacles[i].z > playerPosition.z + obstacles[i].rayon) {
                obstacles[i].active = false; // Désactiver l'astéroïde
            }
        }
    }
}

// Vérifie les collisions entre le joueur et les astéroïdes
void CheckCollisions(void) {
    float playerRadius = 0.5f;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            
            // 1. Vérifier si l'astéroïde est sur la même voie que le joueur
            if (obstacles[i].Voie_X == JoeurligneX && obstacles[i].Voie_Y == JoeurligneY) {
                
                // 2. Calculer la position 3D de l'astéroïde
                // L'astéroïde est aux coordonnées x et y de sa voie
                Vector3 asteroidPosition = { 
                    ligneXPositions[obstacles[i].Voie_X], 
                    ligneYPositions[obstacles[i].Voie_Y], 
                    obstacles[i].z 
                };
                
                float distance = Vector3Distance(playerPosition, asteroidPosition);
                
                // 3. Collision si la distance est inférieure à la somme des rayons
                float minDistance = playerRadius + obstacles[i].rayon;
                
                if (distance < minDistance) {
                    gameOver = true; // Collision détectée ! Fin du jeu
                }
            }
        }
    }
}

// Met à jour tout l'état du jeu
void ActualiseJeu(void) {
    if (!gameOver) {
        ActualiseJoueurs(); // Mouvement du joueur
        ApparitionAsteroide(); // Tentative d'apparition d'un nouvel astéroïde
        ActualiseAsteroide(); // Mouvement des astéroïdes
        CheckCollisions(); // Vérification des collisions
    } else {
        //'Fin du jeu' (touche ENTREE pour recommencer)
        if (IsKeyPressed(KEY_ENTER)) {
            InitGame(); // Réinitialiser le jeu
        }
    }
}



void DessinerJeu(void) {
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(camera);
    

    
    // Dessiner le vaisseau (joueur).
    DrawCube(playerPosition, 1.0f, 1.0f, 1.0f, BLUE); 
    
    // Dessiner les astéroïdes actifs
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            // Position de dessin de l'astéroïde
            Vector3 drawPosition = { 
                ligneXPositions[obstacles[i].Voie_X], 
                ligneYPositions[obstacles[i].Voie_Y], 
                obstacles[i].z 
            };
            DrawSphere(drawPosition, obstacles[i].rayon, obstacles[i].color);
        }
    }

    EndMode3D();
    

    
    // Affichage 'Fin du jeu'
    if (gameOver) {
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 50, 40, RED);
        DrawText("Appuyez sur ENTREE pour recommencer", SCREEN_WIDTH / 2 - MeasureText("Appuyez sur ENTREE pour recommencer", 20) / 2, SCREEN_HEIGHT / 2 + 20, 20, WHITE);
    }

    EndDrawing();
}

int main(void) {
    // Initialisation de la fenêtre
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vaisseau 3D - Esquive d'Astéroïdes");
    InitGame(); 
    SetTargetFPS(60); 

    // Boucle principale
    while (!WindowShouldClose()) {
        ActualiseJeu(); 
        DessinerJeu();  
    }

    CloseWindow(); 
}