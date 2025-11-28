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
static int LimitEspace(int value, int min, int max); //vérifie que le joueur est toujours dans l'espace de jeu et l'empèche de sortir
static void InitGame(void);  //initialise le jeu 
static void ApparitionAsteroide(void); //Créer les astéroides 
static void ActualiseJeu(void); //met à jour le jeu sans cesse permet l'évolution de la partie + fluidité 
static void DessinerJeu(void); //affichage graphique du jeu 
