#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"      
#include <math.h>
#include "etoile.h" 

// --- CONFIGURATION ET ÉQUILIBRAGE ---
#define LARGEUR_ECRAN 800 
#define HAUTEUR_ECRAN 600 
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
    float z_distance;    // Position sur l'axe de profondeur
    int Voie_X;          // Index du couloir  horizontale 
    int Voie_Y;          // Index du couloir  verticale 
    bool actif;
    float rayon;       
    Color couleur;        
} Obstacle;

typedef struct {
    Rectangle frameRec;
    int currentFrame;
    float frameTimer;
} PlayerVisuals;

//Prototypes 
int LimiteEspace(int valeur, int min, int max); 
void InitGame(void);  
void ActualiseJoueur(void); 
void ApparitionAsteroide(void); 
void ActualiseAsteroides(void); 
void VerifieCollisions(void); 
void ActualiseJeu(void); 
void DessinerJeu(void); 

//Variables globales 
Vector3 positionJoueur = {0.0f, 0.0f, 0.0f};
int voieJoueurX = 1; // Milieu par défaut
int voieJoueurY = 0;

float positionsXVoies[NB_VOIES_X] = {-4.0f, 0.0f, 4.0f};
float positionsYVoies[NB_VOIES_Y] = { -2.0f, 2.0f };   
 
Obstacle tableauObstacles[MAX_OBSTACLES]; 
Camera3D cameraJeu = { 0 };           
bool jeuFini = false;
bool jeuEnPause = false;

float tempsSurvie = 0.0f;
int niveauDifficulte = 0;

Model vaisseauModel;
bool modelCharge = false;
Model asteroidModel;
bool asteroidModelCharge = false;

// Initialisation ou remise à zéro du jeu
void InitGame(void) {
    srand((unsigned int)time(NULL)); 

    // Configuration de la perspective 
    cameraJeu.position = (Vector3){0.0f, 6.0f, 12.0f}; 
    cameraJeu.target   = (Vector3){0.0f, 0.5f, 0.0f};  
    cameraJeu.up       = (Vector3){0.0f, 1.0f, 0.0f}; 
    cameraJeu.fovy     = 80.0f; 
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

    Initialiseretoile(LARGEUR_ECRAN, HAUTEUR_ECRAN);
}

// Définition des bordures, permet de limiter les mouvements du joueur
int LimiteEspace(int valeur, int min, int max) {
    if (valeur < min) return min;
    if (valeur > max) return max;
    return valeur;
}

void ActualiseJoueur(void) {
    bool isMoving = false;
    // Gestion des entrées clavier pour changer de voie
    if (IsKeyPressed(KEY_LEFT))  { voieJoueurX--; isMoving = true; } 
    if (IsKeyPressed(KEY_RIGHT)) { voieJoueurX++; isMoving = true; } 
    if (IsKeyPressed(KEY_DOWN))  { voieJoueurY--; isMoving = true; } 
    if (IsKeyPressed(KEY_UP))    { voieJoueurY++; isMoving = true; } 

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
    if (isMoving || fabsf(positionJoueur.x - cibleX) > 0.1f || fabsf(positionJoueur.y - cibleY) > 0.1f) {
        static float frameTimer = 0.0f;
        frameTimer += dt;
        if (frameTimer >= 0.1f) {
            frameTimer = 0.0f;
            static int currentFrame = 1;
            currentFrame++;
            if (currentFrame >= 3) currentFrame = 0;
        }
    }
}

void ApparitionAsteroide(void) {
    static float minuterie = 0.0f;
    
    // Calcul de la difficulté 
    float tauxApparition = TAUX_APPARITION_BASE;
    for (int i = 0; i < niveauDifficulte; i++) {
        tauxApparition *= MULTIPLIER_DIFFICULTE;
    }
    if (tauxApparition < 0.05f) tauxApparition = 0.05f;

    minuterie += GetFrameTime();

    // Si le délai est écoulé, on cherche un emplacement libre dans le tableau 
    if (minuterie >= tauxApparition) {
        minuterie = 0.0f;
        
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!tableauObstacles[i].actif) {
                tableauObstacles[i].z_distance = Z_INIT_ASTEROIDE; 
                tableauObstacles[i].Voie_X = rand() % NB_VOIES_X; 
                tableauObstacles[i].Voie_Y = rand() % NB_VOIES_Y;
                tableauObstacles[i].actif = true;
                tableauObstacles[i].rayon = RAYON_ASTEROIDE; 

                // Attribution d'une nuance de gris de façon aléatoire
                int choixCouleur = rand() % 8;
                if (choixCouleur == 0)      tableauObstacles[i].couleur = (Color){ 220, 220, 230, 255 };
                else if (choixCouleur == 1) tableauObstacles[i].couleur = (Color){ 200, 190, 170, 255 };
                else if (choixCouleur == 2) tableauObstacles[i].couleur = (Color){ 180, 200, 220, 255 };
                else if (choixCouleur == 3) tableauObstacles[i].couleur = (Color){ 210, 190, 160, 255 };
                else if (choixCouleur == 4) tableauObstacles[i].couleur = (Color){ 230, 210, 180, 255 };
                else if (choixCouleur == 5) tableauObstacles[i].couleur = (Color){ 190, 210, 190, 255 };
                else if (choixCouleur == 6) tableauObstacles[i].couleur = (Color){ 170, 180, 200, 255 };
                else                        tableauObstacles[i].couleur = (Color){ 240, 230, 200, 255 };

                return; 
            }
        }
    }
}

void ActualiseAsteroides(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            // Les astéroïdes avancent vers le joueur (z augmente)
            tableauObstacles[i].z_distance += VITESSE_ASTEROIDE;
            // Si l'astéroïde dépasse le joueur, on le désactive pour le réutiliser
            if (tableauObstacles[i].z_distance > positionJoueur.z + tableauObstacles[i].rayon) {
                tableauObstacles[i].actif = false; 
            }
        }
    }
}

void VerifieCollisions(void) {
    float rayonJoueur = 0.5f; 

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            // On ne vérifie la distance que s'ils sont sur la même voie (petite optmisation )
            if (tableauObstacles[i].Voie_X == voieJoueurX && tableauObstacles[i].Voie_Y == voieJoueurY) {
                Vector3 positionAsteroide = { 
                    positionsXVoies[tableauObstacles[i].Voie_X], 
                    positionsYVoies[tableauObstacles[i].Voie_Y], 
                    tableauObstacles[i].z_distance 
                };
                
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
void ActualiseJeu(void) {
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
        if (!jeuFini) jeuEnPause = !jeuEnPause;
    }
    
    if (!jeuFini && !jeuEnPause) {
        tempsSurvie += GetFrameTime();
        
        // Progression de la difficulté par paliers de temps ! (trouver un équilibre)
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
        if (IsKeyPressed(KEY_ENTER)) InitGame(); 
    }
}

void DessinerJeu(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    Dessineretoile(); // Fond étoilé 2D ( géré par etoile.h)

    BeginMode3D(cameraJeu);
    
    // Rendu des astéroïdes
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            Vector3 positionDessin = { 
                positionsXVoies[tableauObstacles[i].Voie_X], 
                positionsYVoies[tableauObstacles[i].Voie_Y], 
                tableauObstacles[i].z_distance 
            };

            if (asteroidModelCharge) {
                float echelleAsteroide = 2.0f;
                float angle = (float)(GetTime() * 50.0); // Rotation continue
                DrawModelEx(asteroidModel, positionDessin, (Vector3){0.0f, 1.0f, 0.0f}, angle,
                           (Vector3){echelleAsteroide, echelleAsteroide, echelleAsteroide}, tableauObstacles[i].couleur);
            } else {
                DrawSphere(positionDessin, tableauObstacles[i].rayon, tableauObstacles[i].couleur);
            }
        }
    }
    
    // Rendu du vaisseau
    if (modelCharge) {
        float echelleVaisseau = 0.004f;
        DrawModelEx(vaisseauModel, positionJoueur, (Vector3){0.0f, 1.0f, 0.0f}, 180.0f,
                   (Vector3){echelleVaisseau, echelleVaisseau, echelleVaisseau}, WHITE);
    } else {
        // Si le modèle png ne fonctionne pas remplacement par le cube (éphémère à enlever à terme )
        DrawCube(positionJoueur, 1.5f, 1.5f, 2.0f, BLUE);
        DrawCubeWires(positionJoueur, 1.5f, 1.5f, 2.0f, DARKBLUE);
    }
    
    EndMode3D();
    
    //INTERFACE utilisateur

    char texteTemps[64];
    sprintf(texteTemps, "Temps: %.1f s | Difficulte: %d", tempsSurvie, niveauDifficulte);
    DrawText(texteTemps, 10, 10, 20, WHITE);
    
    if (jeuEnPause && !jeuFini) {
        DrawRectangle(0, 0, LARGEUR_ECRAN, HAUTEUR_ECRAN, (Color){0, 0, 0, 150});
        DrawText("PAUSE", LARGEUR_ECRAN / 2 - MeasureText("PAUSE", 60) / 2, HAUTEUR_ECRAN / 2 - 80, 60, YELLOW);
    }
    
    if (jeuFini) {
        DrawRectangle(0, 0, LARGEUR_ECRAN, HAUTEUR_ECRAN, (Color){0, 0, 0, 200});
        DrawText("GAME OVER", LARGEUR_ECRAN / 2 - MeasureText("GAME OVER", 40) / 2, HAUTEUR_ECRAN / 2 - 50, 40, RED);
        DrawText("Appuyez sur ENTREE pour recommencer", LARGEUR_ECRAN / 2 - MeasureText("Appuyez sur ENTREE pour recommencer", 20) / 2, HAUTEUR_ECRAN / 2 + 40, 20, WHITE);
    }

    EndDrawing();
}

int main(void) {
    // Initialisation de la fenêtre 
    InitWindow(LARGEUR_ECRAN, HAUTEUR_ECRAN, "Vaisseau 3D - Esquive d'Astéroïdes");
    
    // Chargement des ressources visuelles 
    vaisseauModel = LoadModel("/Users/vb/Desktop/projetC_doc/projet-c/Jeu_Valentin/Lo_poly_Spaceship_01_by_Liz_Reddington.obj");
    modelCharge = (vaisseauModel.meshCount > 0);

    asteroidModel = LoadModel("/Users/vb/Desktop/projetC_doc/projet-c/Jeu_Valentin/Asteroid.obj");
    asteroidModelCharge = (asteroidModel.meshCount > 0);
    
    InitGame(); 
    SetTargetFPS(60); 

    // Boucle principale du jeu
    while (!WindowShouldClose()) {
        ActualiseJeu(); 
        DessinerJeu();  
    }

    // Déchargement de la mémoire ! 
    if (modelCharge) UnloadModel(vaisseauModel);
    if (asteroidModelCharge) UnloadModel(asteroidModel);
    CloseWindow();
    
    return 0;
}