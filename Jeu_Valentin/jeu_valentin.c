#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"      
#include <math.h>
#include "etoile.h" 
#include "../include/menu.h" // Ajouté pour les types Difficulte et ScoreList

// --- CONFIGURATION ET ÉQUILIBRAGE ---
#define LARGEUR_ECRAN 800 
#define HAUTEUR_ECRAN 600 
#define NB_VOIES_X 3       
#define NB_VOIES_Y 2       
#define MAX_OBSTACLES 30    
#define VITESSE_ASTEROIDE 0.50f
#define Z_INIT_ASTEROIDE -50.0f 
#define RAYON_ASTEROIDE 1.0f

#define TAUX_APPARITION_BASE 0.35f 
#define TEMPS_PALIER_DIFFICULTE 5.0f
#define MULTIPLIER_DIFFICULTE 0.85f

// --- STRUCTURES DE DONNÉES ---
typedef struct Obstacle {
    float z_distance;
    int Voie_X;
    int Voie_Y;
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

void RunValentinGame(Difficulte niveau, bool resume, bool *game_active,bool *mort);
int GameOverValentin();

//Variables globales 
Vector3 positionJoueur = {0.0f, 0.0f, 0.0f};
int voieJoueurX = 1; 
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



void InitGame(void) {
    srand((unsigned int)time(NULL)); 
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
    for (int i = 0; i < MAX_OBSTACLES; i++) tableauObstacles[i].actif = false;
    jeuFini = false; 
    jeuEnPause = false; 
    tempsSurvie = 0.0f;
    Initialiseretoile(LARGEUR_ECRAN, HAUTEUR_ECRAN);
}

int LimiteEspace(int valeur, int min, int max) {
    if (valeur < min) return min;
    if (valeur > max) return max;
    return valeur;
}

void ActualiseJoueur(void) {
    bool isMoving = false;
    if (IsKeyPressed(KEY_LEFT))  { voieJoueurX--; isMoving = true; } 
    if (IsKeyPressed(KEY_RIGHT)) { voieJoueurX++; isMoving = true; } 
    if (IsKeyPressed(KEY_DOWN))  { voieJoueurY--; isMoving = true; } 
    if (IsKeyPressed(KEY_UP))    { voieJoueurY++; isMoving = true; } 
    voieJoueurX = LimiteEspace(voieJoueurX, 0, NB_VOIES_X - 1);
    voieJoueurY = LimiteEspace(voieJoueurY, 0, NB_VOIES_Y - 1);
    float cibleX = positionsXVoies[voieJoueurX];
    float cibleY = positionsYVoies[voieJoueurY];
    positionJoueur.x = Lerp(positionJoueur.x, cibleX, 0.2f);
    positionJoueur.y = Lerp(positionJoueur.y, cibleY, 0.2f);
}

void ApparitionAsteroide(void) {
    static float minuterie = 0.0f;
    float tauxApparition = TAUX_APPARITION_BASE;
    for (int i = 0; i < niveauDifficulte; i++) tauxApparition *= MULTIPLIER_DIFFICULTE;
    if (tauxApparition < 0.05f) tauxApparition = 0.05f;
    minuterie += GetFrameTime();
    if (minuterie >= tauxApparition) {
        minuterie = 0.0f;
        int obstaclesSurMemeLigne = 0;
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (tableauObstacles[i].actif && fabsf(tableauObstacles[i].z_distance - Z_INIT_ASTEROIDE) < 5.0f) obstaclesSurMemeLigne++;
        }
        if (obstaclesSurMemeLigne >= (NB_VOIES_X * NB_VOIES_Y) - 1) return; 
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (!tableauObstacles[i].actif) {
                tableauObstacles[i].z_distance = Z_INIT_ASTEROIDE; 
                tableauObstacles[i].Voie_X = rand() % NB_VOIES_X; 
                tableauObstacles[i].Voie_Y = rand() % NB_VOIES_Y;
                tableauObstacles[i].actif = true;
                tableauObstacles[i].rayon = RAYON_ASTEROIDE; 
                tableauObstacles[i].couleur = (Color[]){WHITE, LIGHTGRAY, RAYWHITE, SKYBLUE}[rand()%4];
                return; 
            }
        }
    }
}

void ActualiseAsteroides(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            tableauObstacles[i].z_distance += VITESSE_ASTEROIDE;
            if (tableauObstacles[i].z_distance > positionJoueur.z + tableauObstacles[i].rayon) tableauObstacles[i].actif = false; 
        }
    }
}

void VerifieCollisions(void) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif && tableauObstacles[i].Voie_X == voieJoueurX && tableauObstacles[i].Voie_Y == voieJoueurY) {
            Vector3 posAst = { positionsXVoies[tableauObstacles[i].Voie_X], positionsYVoies[tableauObstacles[i].Voie_Y], tableauObstacles[i].z_distance };
            if (Vector3Distance(positionJoueur, posAst) < (0.5f + tableauObstacles[i].rayon)) jeuFini = true; 
        }
    }
}

void ActualiseJeu(void) {
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
        if (!jeuFini) jeuEnPause = !jeuEnPause;
    }
    if (!jeuFini && !jeuEnPause) {
        tempsSurvie += GetFrameTime();
        niveauDifficulte = (int)(tempsSurvie / TEMPS_PALIER_DIFFICULTE);
        ActualiseJoueur(); 
        ApparitionAsteroide(); 
        ActualiseAsteroides(); 
        VerifieCollisions(); 
        Actualiseretoile(tempsSurvie, niveauDifficulte); 
    }
}

void DessinerJeu(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    Dessineretoile();
    BeginMode3D(cameraJeu);
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (tableauObstacles[i].actif) {
            Vector3 p = { positionsXVoies[tableauObstacles[i].Voie_X], positionsYVoies[tableauObstacles[i].Voie_Y], tableauObstacles[i].z_distance };
            if (asteroidModelCharge) DrawModelEx(asteroidModel, p, (Vector3){0, 1, 0}, (float)GetTime()*50, (Vector3){2, 2, 2}, tableauObstacles[i].couleur);
            else DrawSphere(p, tableauObstacles[i].rayon, tableauObstacles[i].couleur);
        }
    }
    if (modelCharge) DrawModelEx(vaisseauModel, positionJoueur, (Vector3){0, 1, 0}, 180.0f, (Vector3){0.004f, 0.004f, 0.004f}, WHITE);
    else { DrawCube(positionJoueur, 1.5f, 1.5f, 2.0f, BLUE); DrawCubeWires(positionJoueur, 1.5f, 1.5f, 2.0f, DARKBLUE); }
    EndMode3D();
    DrawText(TextFormat("Temps: %.1f s | Difficulte: %d", tempsSurvie, niveauDifficulte), 10, 10, 20, WHITE);
    if (jeuEnPause && !jeuFini) DrawText("PAUSE", LARGEUR_ECRAN/2 - 80, HAUTEUR_ECRAN/2 - 30, 60, YELLOW);
    if (jeuFini) {
        DrawRectangle(0, 0, LARGEUR_ECRAN, HAUTEUR_ECRAN, (Color){0, 0, 0, 200});
        DrawText("GAME OVER", LARGEUR_ECRAN/2 - 100, HAUTEUR_ECRAN/2 - 50, 40, RED);
        DrawText("ENTREE pour recommencer / ESC pour menu", LARGEUR_ECRAN/2 - 180, HAUTEUR_ECRAN/2 + 40, 20, WHITE);
    }
    EndDrawing();
}
int GameOverValentin() {
    int scoreFinal = (int)tempsSurvie;


    LibererEtoile(); 

    jeuFini = true;

    return scoreFinal;
}

void RunValentinGame(Difficulte niveau, bool resume, bool *game_active,bool *mort) {
    // Chargement des modèles si pas encore fait
    if (!modelCharge) {
        vaisseauModel = LoadModel("Jeu_Valentin/Spaceship.obj");
        modelCharge = (vaisseauModel.meshCount > 0);
        asteroidModel = LoadModel("Jeu_Valentin/Asteroid.obj");
        asteroidModelCharge = (asteroidModel.meshCount > 0);
    }

    // Gestion du resume
    if (!resume) {
        InitGame();
        // Adaptation difficulté
        if (niveau == FACILE) niveauDifficulte = 0;
        else if (niveau == MOYEN) niveauDifficulte = 2;
        else if (niveau == DIFFICILE) niveauDifficulte = 5;
    }

    //*game_active = true;
    bool scoreEnregistre = false;

    //while (!WindowShouldClose()) {
    if (*game_active){
         

        if (!jeuFini && !jeuEnPause) {
            ActualiseJeu(); 
            
            // Si le jeu vient de passer en 'fini' après l'actualisation
            if (jeuFini) {
                *mort=jeuFini;
                //GameOverValentin();
            }
        }
        /* else if (jeuFini) {
            // Permet de relancer une partie directement sans repasser par le menu
            if (IsKeyPressed(KEY_ENTER)) {
                InitGame();
                *game_active = true;
            }
        }*/

        DessinerJeu();  
    }
 }