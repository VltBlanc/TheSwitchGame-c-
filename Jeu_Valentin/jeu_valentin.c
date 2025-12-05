#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"


// --- I. CONSTANTES DU JEU ---
#define LARGEUR_ECRAN 800 
#define HAUTEUR_ECRAN 600 
#define NB_VOIES_X 3       // Nombre de voies horizontales (Gauche, Centre, Droite)
#define NB_VOIES_Y 2       // Nombre de voies verticales (Bas, Haut)
#define MAX_OBSTACLES 30    
#define VITESSE_ASTEROIDE 0.50f
#define Z_INIT_ASTEROIDE -50.0f // Position Z initiale lointaine des astéroïdes
#define RAYON_ASTEROIDE 1.0f

// --- II. CONSTANTES DE DIFFICULTÉ ---
#define TAUX_APPARITION_BASE 0.35f 
#define TEMPS_PALIER_DIFFICULTE 5.0f // Temps (en secondes) après lequel la difficulté augmente
#define MULTIPLIER_DIFFICULTE 0.85f // Multiplicateur de réduction du taux d'apparition

// --- III. STRUCTURES ---
typedef struct Obstacle {
    float z_distance;            // Distance entre l'astéroïde et le joueur
    int Voie_X;          
    int Voie_Y;          
    bool actif;        // Détermine si l'astéroïde est visible et actif
    float rayon;       
    Color couleur;        
} Obstacle;

// --- IV. PROTOTYPES DES FONCTIONS ---
int LimiteEspace(int valeur, int min, int max); 
void InitGame(void);  
void ActualiseJoueur(void); 
void ApparitionAsteroide(void); 
void ActualiseAsteroides(void); 
void VerifieCollisions(void); 
void ActualiseJeu(void); 
void DessinerJeu(void); 

// --- V. VARIABLES GLOBALES ---

Vector3 positionJoueur = {0.0f, 0.0f, 0.0f}; // Position 3D du joueur
int voieJoueurX = 1;                         // Voie X actuelle (0, 1, 2)
int voieJoueurY = 0;                         // Voie Y actuelle (0, 1)
float positionsXVoies[NB_VOIES_X] = {-2.5f, 0.0f, 2.5f}; 
float positionsYVoies[NB_VOIES_Y] = { -1.5f, 1.5f };     

Obstacle tableauObstacles[MAX_OBSTACLES]; // Tableau des astéroïdes
Camera3D cameraJeu = { 0 };           
bool jeuFini = false;             

float tempsSurvie = 0.0f;          // Temps de survie (score)
int niveauDifficulte = 0;            // Niveau de difficulté actuel

// --- VI. IMPLÉMENTATION DES FONCTIONS ---

// Initialisation du jeu
void InitGame(void) {
    srand((unsigned int)time(NULL)); 

    // Configuration Caméra
    cameraJeu.position = (Vector3){0.0f, 4.0f, 10.0f}; 
    cameraJeu.target   = (Vector3){0.0f, 0.0f, 0.0f};  
    cameraJeu.up       = (Vector3){0.0f, 1.0f, 0.0f}; 
    cameraJeu.fovy     = 75.0f; 
    cameraJeu.projection = CAMERA_PERSPECTIVE;

    // Réinitialisation du joueur
    voieJoueurX = 1; 
    voieJoueurY = 0; 
    positionJoueur.x = positionsXVoies[voieJoueurX];
    positionJoueur.y = positionsYVoies[voieJoueurY];
    positionJoueur.z = 0.0f; 

    // Réinitialisation des astéroïdes
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        tableauObstacles[i].actif = false;
    }
    
    // Réinitialisation de l'état et de la difficulté
    jeuFini = false; 
    tempsSurvie = 0.0f;
    niveauDifficulte = 0;
}

// Limite la valeur entre min et max
int LimiteEspace(int valeur, int min, int max) {
    if (valeur < min) return min;
    if (valeur > max) return max;
    return valeur;
}

// Met à jour la position du joueur en fonction des entrées
void ActualiseJoueur(void) {
    // Entrées utilisateur et limitation de voie
    if (IsKeyPressed(KEY_LEFT))  voieJoueurX--;
    if (IsKeyPressed(KEY_RIGHT)) voieJoueurX++;
    if (IsKeyPressed(KEY_UP))    voieJoueurY++;
    if (IsKeyPressed(KEY_DOWN))  voieJoueurY--;

    voieJoueurX = LimiteEspace(voieJoueurX, 0, NB_VOIES_X - 1);
    voieJoueurY = LimiteEspace(voieJoueurY, 0, NB_VOIES_Y - 1);

    // Mise à jour de la position 3D
    positionJoueur.x = positionsXVoies[voieJoueurX];
    positionJoueur.y = positionsYVoies[voieJoueurY];
}

// Tente de faire apparaître un nouvel astéroïde aléatoirement
void ApparitionAsteroide(void) {
    static float minuterie = 0.0f;
    
    // Calcul du taux d'apparition (tauxApparition)
    float tauxApparition = TAUX_APPARITION_BASE;
    // Diminue le tauxApparition à chaque niveau de difficulté
    for (int i = 0; i < niveauDifficulte; i++) {
        tauxApparition *= MULTIPLIER_DIFFICULTE;
    }
    if (tauxApparition < 0.05f) tauxApparition = 0.05f; // Limite minimale

    minuterie += GetFrameTime();

    if (minuterie >= tauxApparition) {
        minuterie = 0.0f;
        
        // Chercher et initialiser un astéroïde inactif
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!tableauObstacles[i].actif) {
                tableauObstacles[i].z_distance = Z_INIT_ASTEROIDE; 
                tableauObstacles[i].Voie_X = rand() % NB_VOIES_X; 
                tableauObstacles[i].Voie_Y = rand() % NB_VOIES_Y;
                tableauObstacles[i].actif = true;
                tableauObstacles[i].rayon = RAYON_ASTEROIDE; // Assurer que le rayon est défini

                
                int choixCouleur = rand() % 3;
                if (choixCouleur == 0) tableauObstacles[i].couleur = RED;
                else if (choixCouleur == 1) tableauObstacles[i].couleur = GREEN;
                else tableauObstacles[i].couleur = YELLOW;
                
                return; 
            }
        }
    }
}

// Met à jour la position des astéroïdes
void ActualiseAsteroides(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            // L'astéroïde se déplace vers le joueur (z augmente)
            tableauObstacles[i].z_distance += VITESSE_ASTEROIDE;

            // Désactivation si l'astéroïde est dépassé
            if (tableauObstacles[i].z_distance > positionJoueur.z + tableauObstacles[i].rayon) {
                tableauObstacles[i].actif = false; 
            }
        }
    }
}

// Vérifie les collisions entre le joueur et les astéroïdes
void VerifieCollisions(void) {
    float rayonJoueur = 0.5f;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            
            // Collision possible uniquement si l'astéroïde est sur la même voie
            if (tableauObstacles[i].Voie_X == voieJoueurX && tableauObstacles[i].Voie_Y == voieJoueurY) {
                
                // Calculer la position 3D réelle de l'astéroïde
                Vector3 positionAsteroide = { 
                    positionsXVoies[tableauObstacles[i].Voie_X], 
                    positionsYVoies[tableauObstacles[i].Voie_Y], 
                    tableauObstacles[i].z_distance 
                };
                
                float distance = Vector3Distance(positionJoueur, positionAsteroide);
                float distanceMinimale = rayonJoueur + tableauObstacles[i].rayon;
                
                // Fin du jeu si la distance est inférieure à la somme des rayons
                if (distance < distanceMinimale) {
                    jeuFini = true; 
                }
            }
        }
    }
}

// Met à jour tout l'état du jeu (logique principale)
void ActualiseJeu(void) {
    if (!jeuFini) {
        // Mise à jour du temps de survie et du niveau de difficulté
        tempsSurvie += GetFrameTime();
        
        int nouveauNiveauDifficulte = (int)(tempsSurvie / TEMPS_PALIER_DIFFICULTE);
        if (nouveauNiveauDifficulte > niveauDifficulte) {
            niveauDifficulte = nouveauNiveauDifficulte;
        }

        ActualiseJoueur(); 
        ApparitionAsteroide(); 
        ActualiseAsteroides(); 
        VerifieCollisions(); 
    } else {
        // Redémarrage du jeu
        if (IsKeyPressed(KEY_ENTER)) {
            InitGame(); 
        }
    }
}

// Rend le jeu (affichage)
void DessinerJeu(void) {
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(cameraJeu);
    
    // Dessiner le vaisseau
    DrawCube(positionJoueur, 1.0f, 1.0f, 1.0f, BLUE); 
    
    // Dessiner les astéroïdes actifs
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            Vector3 positionDessin = { 
                positionsXVoies[tableauObstacles[i].Voie_X], 
                positionsYVoies[tableauObstacles[i].Voie_Y], 
                tableauObstacles[i].z_distance 
            };
            DrawSphere(positionDessin, tableauObstacles[i].rayon, tableauObstacles[i].couleur);
        }
    }
    
    EndMode3D();
    
    // Affichage du Score/Temps de Survie
    char texteTemps[64];
    sprintf(texteTemps, "Temps: %.1f s | Difficulte: %d", tempsSurvie, niveauDifficulte);
    DrawText(texteTemps, 10, 10, 20, WHITE);
    
    // Affichage 'Fin du jeu'
    if (jeuFini) {
        DrawText("GAME OVER", LARGEUR_ECRAN / 2 - MeasureText("GAME OVER", 40) / 2, HAUTEUR_ECRAN / 2 - 50, 40, RED);
        DrawText(texteTemps, LARGEUR_ECRAN / 2 - MeasureText(texteTemps, 25) / 2, HAUTEUR_ECRAN / 2 + 5, 25, YELLOW);
        DrawText("Appuyez sur ENTREE pour recommencer", LARGEUR_ECRAN / 2 - MeasureText("Appuyez sur ENTREE pour recommencer", 20) / 2, HAUTEUR_ECRAN / 2 + 40, 20, WHITE);
    }

    EndDrawing();
}

int main(void) {
    InitWindow(LARGEUR_ECRAN, HAUTEUR_ECRAN, "Vaisseau 3D - Esquive d'Astéroïdes");
    InitGame(); 
    SetTargetFPS(60); 

    // Boucle principale
    while (!WindowShouldClose()) {
        ActualiseJeu(); 
        DessinerJeu();  
    }

    CloseWindow(); 
}