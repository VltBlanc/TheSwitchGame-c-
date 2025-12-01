#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"

// --- Constantes Fondamentales du Jeu ---
#define SCREEN_WIDTH 800 //taille écran de jeu
#define SCREEN_HEIGHT 600 //taille écran de jeu 
#define NUM_LANES_X 3 // Nombre de voies horizontales (Gauche, Centre, Droite)
#define NUM_LANES_Y 2 // Nombre de voies verticales (Bas, Haut)

// --- Structure pour les Obstacles (Astéroïdes) ---
typedef struct Obstacle {
    float z;            // distance  entre lui et le joueur 
    int Voie_X;          // Voie horizontale de l'astéroïde
    int Voie_Y;          // Voie verticale de l'astéroïde
    bool active;        //  Détermine si visible et actif 
    float rayon;       // Rayon de l'astéroïde (doit être une constant finalement )
    Color color;        // Couleur astéoroïde
} Obstacle;

// --- Prototypes des Fonctions ---
 int LimiteEspace(int value, int min, int max); //vérifie que le joueur est toujours dans l'espace de jeu et l'empèche de sortir
 void InitGame(void);  //initialise le jeu 
 void ApparitionAsteroide(void); //Créer les astéroides 
 void ActualiseJeu(void); //met à jour le jeu sans cesse permet l'évolution de la partie + fluidité 
 void DessinerJeu(void); //affichage graphique du jeu 

// --- Variables Globales ---
 Vector3 playerPosition = {0.0f, 0.0f, 0.0f}; // position du vaisseau
 int JoeurligneX = 1;                         // voie X 
 int JoeurligneY = 0;                         // voie Y 
 float ligneXPositions[NUM_LANES_X] = {-2.5f, 0.0f, 2.5f}; // positions X des voies
 float ligneYPositions[NUM_LANES_Y] = { -1.5f, 1.5f };     // positions Y des voies


//--- Fonctions ---
 int LimiteEspace(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void actualiseJoueurs(void) {
    if (IsKeyPressed(KEY_LEFT))  JoeurligneX--;
    if (IsKeyPressed(KEY_RIGHT)) JoeurligneX++;
    if (IsKeyPressed(KEY_UP))    JoeurligneY++;
    if (IsKeyPressed(KEY_DOWN))  JoeurligneY--;

    JoeurligneX = LimiteEspace(JoeurligneX, 0, NUM_LANES_X - 1);
    JoeurligneY = LimiteEspace(JoeurligneY, 0, NUM_LANES_Y - 1);

    // Mise à jour position du joueur
    playerPosition.x = ligneXPositions[JoeurligneX];
    playerPosition.y = ligneYPositions[JoeurligneY];
}

void DessinerJeu(void) {
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D((Camera3D){ .position = (Vector3){0.0f, 0.0f, 10.0f},
                            .target   = (Vector3){0.0f, 0.0f, 0.0f},
                            .up       = (Vector3){0.0f, 1.0f, 0.0f},
                            .fovy     = 45.0f,
                            .projection = CAMERA_PERSPECTIVE });

    DrawCube(playerPosition, 1.0f, 1.0f, 1.0f, BLUE);

    EndMode3D();
    EndDrawing();
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Vaisseau 3D - Prototype");
    SetTargetFPS(60);

    // Boucle principale
    while (!WindowShouldClose()) {
        actualiseJoueurs();
        DessinerJeu();
    }

    CloseWindow();
    return 0;
}
