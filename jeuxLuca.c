#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <raymath.h>
#include <time.h>

//---VARIABLE---

#define Taille_Grid 10
#define Taille_Case 60
#define Taille_Marge_Grille 200

#define Demi_Cote_Joueur_X 14
#define Demi_Cote_Joueur_Y 19
#define HITBOX_PIEDS_HAUTEUR 15.0f
#define HITBOX_PIEDS_LARGEUR 20.0f

#define Vitesse_Marche 200.0f
#define CoefCourse 2.0f
#define Distance_Case_Dash 2

const int screenWidth = Taille_Marge_Grille * 2 + Taille_Grid * Taille_Case; 
const int screenHeight = Taille_Marge_Grille * 2 + Taille_Grid * Taille_Case; 
const int Debut_Grille_X = Taille_Marge_Grille; 
const int Debut_Grille_Y = Taille_Marge_Grille;

// etats possibles pour chaque case
typedef enum
{
    Block_Normal = 0,
    Block_Alerte = 1,
    Block_Tomber = 2,
    Block_Prochain_Cycle = 3
} Block;

//phases possibles
typedef enum {
    Phase_Attente = 0,
    Phase_Alerte = 1,
    Phase_Tomber = 2
} Phase;

typedef struct Joueur
{
    Vector2 position;
    bool vivant;
    int score;
    Vector2 dernierMouvement;
} Joueur;

Phase phaseActuelle = Phase_Attente;
Block **grid = NULL; //initialisation du tableau a Null
float timer = 0.0f;
const float Temps_Alerte = 0.6f; // temps en secondes pour l'avertissement
const float Temps_Tomber = 0.2f; 
const float Temps_Attente = 0.1f;   // temps en secondes avant prochaine phase
int nbrBlockQuiTombe = 8;
bool scoreDoitAugmenter = false;

Texture2D playerTexture;
Texture2D fondTexture;
Texture2D solTexture;
Texture2D solFissureTexture;

//---PROTOTYPES---
//utile pour écrire les fonctions dans l'ordre qu'on veut
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

//---FONCTIONS---
void InitGrid() {
    grid = (Block**)malloc(Taille_Grid * sizeof(Block*));
    if (grid == NULL) {
        printf("Erreur malloc lignes initGrid\n");
        exit(1); 
    }

    for (int x = 0; x < Taille_Grid; x++) {
        grid[x] = (Block*)malloc(Taille_Grid * sizeof(Block));

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

Joueur InitJoueur()
{
    Joueur Joueur;
    Joueur.position.x = screenWidth / 2;
    Joueur.position.y = screenHeight / 2;
    Joueur.vivant = true;
    Joueur.score = 0;
    Joueur.dernierMouvement = (Vector2){0.0f, 0.0f}; //utile pour le dash et savoir où regarde le joueur
    return Joueur;
}

void DashJoueur(Joueur *Joueur)
{
    if (!Joueur->vivant){
        return;
    }

    float DashX = Joueur->dernierMouvement.x * Distance_Case_Dash * Taille_Case;
    float DashY = Joueur->dernierMouvement.y * Distance_Case_Dash * Taille_Case;

    Joueur->position.x += DashX;
    Joueur->position.y += DashY;
}

void UpdateJoueurMouvement(Joueur *Joueur, float deltaTime)
{
    if (!Joueur->vivant){
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

    if (mouvement.x != 0.0f || mouvement.y != 0.0f)
    {
        Joueur->dernierMouvement = Vector2Normalize(mouvement);
        mouvement = Joueur->dernierMouvement;
    }

    float vitesseActuel = Vitesse_Marche;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
    {
        vitesseActuel *= CoefCourse;
    }

    Joueur->position.x += mouvement.x * vitesseActuel * deltaTime;
    Joueur->position.y += mouvement.y * vitesseActuel * deltaTime;

    if (IsKeyPressed(KEY_SPACE))
    {
        DashJoueur(Joueur);
    }
}

void VerifieMort(Joueur *Joueur)
{
    if (!Joueur->vivant){
        return;
    }

    float BottomY = Joueur->position.y + Demi_Cote_Joueur_Y; 
    float HitboxTopY = BottomY - HITBOX_PIEDS_HAUTEUR; 
    float HitboxLeftX = Joueur->position.x - (HITBOX_PIEDS_LARGEUR / 2.0f); 

    Rectangle JoueurRec = {
        HitboxLeftX,                         // X du coin supérieur gauche (centrée)
        HitboxTopY,                          // Y du coin supérieur gauche (haut des pieds)
        HITBOX_PIEDS_LARGEUR,                // Largeur de la hitbox
        HITBOX_PIEDS_HAUTEUR                 // Hauteur de la hitbox
    };

    // Vérification chute de la grille
   bool horsGrilleX = (JoueurRec.x < Debut_Grille_X) || (JoueurRec.x + JoueurRec.width > (Debut_Grille_X + Taille_Grid * Taille_Case));
    bool horsGrilleY = (JoueurRec.y < Debut_Grille_Y) || (JoueurRec.y + JoueurRec.height > (Debut_Grille_Y + Taille_Grid * Taille_Case));

    if (horsGrilleX || horsGrilleY)
    {
        Joueur->vivant = false;
        return;
    }

    //converit le cercle joueur en ensemble de case qu'il touche
    // et on vérifie qu'on ne sort pas du tableau

   int minGridX = (int)floor((JoueurRec.x - Debut_Grille_X) / (float)Taille_Case);
    if(minGridX < 0) {
        minGridX=0;
    }

    int maxGridX = (int)floor(((JoueurRec.x + JoueurRec.width) - Debut_Grille_X) / (float)Taille_Case);
    if(maxGridX >= Taille_Grid){
        maxGridX = Taille_Grid - 1;
    }

    int minGridY = (int)floor((JoueurRec.y - Debut_Grille_Y) / (float)Taille_Case);
    if(minGridY < 0) {
        minGridY=0;
    }

    int maxGridY = (int)floor(((JoueurRec.y + JoueurRec.height) - Debut_Grille_Y) / (float)Taille_Case);
    if(maxGridY >= Taille_Grid){
        maxGridY = Taille_Grid - 1;
    }

    //vérifie collision Cercle-Rectangle
    for (int y = minGridY; y <= maxGridY; y++)
    {
        for (int x = minGridX; x <= maxGridX; x++)
        {

            if (grid[x][y] == Block_Tomber)
            {

                // rectangle = (poisition x haut gauche,poisition y haut gauche,longueur,largeur)
                Rectangle Case = {
                    Debut_Grille_X + x * Taille_Case,
                    Debut_Grille_Y + y * Taille_Case,
                    Taille_Case,
                    Taille_Case};

                //(centre cercle,rayon,rectangle)
                if (CheckCollisionRecs(JoueurRec, Case))
                {
                    Joueur->vivant = false;
                    return;
                }
            }
        }
    }
}

void UpdateGridPhase(Joueur *Joueur, float deltaTime)
{
    if (!Joueur->vivant){
        return;
    }

    timer += deltaTime;

    // avertissement (début phase rouge)
    if (phaseActuelle == Phase_Attente && timer >= Temps_Attente)
    {
        phaseActuelle = Phase_Alerte;
        timer = 0.0f;

        if (!scoreDoitAugmenter)
        {
            if (nbrBlockQuiTombe <= (Taille_Grid * Taille_Grid) / 2)//le nombre de block évolue avec le score et ne peut pas dépasser la moitié des cases
            {
                nbrBlockQuiTombe = 8 + (Joueur->score);
            }
            scoreDoitAugmenter = true;
            Joueur->score++;
        }

        for (int y = 0; y < Taille_Grid; y++)
        {
            for (int x = 0; x < Taille_Grid; x++)
            {
                if (grid[x][y] == Block_Tomber)
                {
                    grid[x][y] = Block_Normal;
                }
                if (grid[x][y] == Block_Prochain_Cycle)
                {
                    grid[x][y] = Block_Alerte;
                }
            }
        }
    }

    // chute block
    else if (phaseActuelle == Phase_Alerte && timer >= Temps_Alerte)
    {

        phaseActuelle = Phase_Tomber;
        timer = 0.0f;

        for (int y = 0; y < Taille_Grid; y++)
        {
            for (int x = 0; x < Taille_Grid; x++)
            {
                if (grid[x][y] == Block_Alerte)
                {
                    grid[x][y] = Block_Tomber;
                }
            }
        }
    }

    // nouveau cycle
    else if (phaseActuelle == Phase_Tomber && timer >= Temps_Tomber)
    {
        phaseActuelle = Phase_Attente;
        timer = 0.0f; 
        scoreDoitAugmenter = false;

        for (int y = 0; y < Taille_Grid; y++)
        {
            for (int x = 0; x < Taille_Grid; x++)
            {
                if (grid[x][y] == Block_Prochain_Cycle || grid[x][y] == Block_Alerte)
                {
                    grid[x][y] = Block_Normal;
                }
            }
        }

        // Sélection aléatoire des blocs pour le prochain cycle
        int cpt = 0;
        while (cpt < nbrBlockQuiTombe)
        {
            int randX = GetRandomValue(0, Taille_Grid - 1);
            int randY = GetRandomValue(0, Taille_Grid - 1);

            if (grid[randX][randY] == Block_Normal)
            {
                grid[randX][randY] = Block_Prochain_Cycle;
                cpt++;
            }
        }
    }
}

void DessinePartie()
{
    //DrawRectangle(0, 0, screenWidth, screenHeight, DARKGRAY); //le fond
    DrawTextureEx(fondTexture, (Vector2){0, 0}, 0.0f, 1.0f, WHITE);//le fond

    for (int y = 0; y < Taille_Grid; y++)
    {
        for (int x = 0; x < Taille_Grid; x++)
        {

            Rectangle rect = {
                Debut_Grille_X + x * Taille_Case,
                Debut_Grille_Y + y * Taille_Case,
                Taille_Case,
                Taille_Case};


            switch (grid[x][y])
            {
            case Block_Normal:
            case Block_Prochain_Cycle:
                DrawTexturePro(solTexture, 
                               (Rectangle){0.0f, 0.0f, (float)solTexture.width, (float)solTexture.height},
                               rect, 
                               (Vector2){0, 0}, 
                               0.0f, 
                               WHITE);

                break;
            case Block_Tomber:
                    continue;
            case Block_Alerte:
                    DrawTexturePro(solFissureTexture, 
                               (Rectangle){0.0f, 0.0f, (float)solTexture.width, (float)solTexture.height},
                               rect, 
                               (Vector2){0, 0}, 
                               0.0f, 
                               WHITE);
            
            default:
                break;
            }

            // 3. Dessiner le contour (pour tous les blocs visibles)
            DrawRectangleLines(rect.x, rect.y, Taille_Case, Taille_Case, (Color){0,0,0,0});//contour case utile pour voir bug
        }
    }
}

void DessineJoueur(Joueur Joueur)
{
    if (Joueur.vivant)
    {
        // 1. Définition du rectangle source (l'image entière)
        Rectangle sourceRec = { 0.0f, 0.0f, (float)playerTexture.width, (float)playerTexture.height };

        // 2. Définition du rectangle de destination (taille et position à l'écran)
        // La largeur et la hauteur du dessin seront 2 * Demi_Cote_Joueur.
        // La position (X, Y) est le coin supérieur gauche du dessin.
        Rectangle destRec = {
            Joueur.position.x - Demi_Cote_Joueur_X,  // X du coin supérieur gauche (centre - rayon)
            Joueur.position.y - Demi_Cote_Joueur_Y,  // Y du coin supérieur gauche (centre - rayon)
            Demi_Cote_Joueur_X * 2.0f,               // Largeur du dessin (Diamètre)
            Demi_Cote_Joueur_Y * 2.0f                // Hauteur du dessin (Diamètre)
        };
        
        // 3. Dessin de la texture redimensionnée
        DrawTexturePro(playerTexture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
        // On utilise {0, 0} comme origine et 0.0f comme rotation, et WHITE pour l'opacité par défaut.

        float BottomY = Joueur.position.y + Demi_Cote_Joueur_Y;
        float HitboxTopY = BottomY - HITBOX_PIEDS_HAUTEUR;
        float HitboxLeftX = Joueur.position.x - (HITBOX_PIEDS_LARGEUR / 2.0f);

        // Dessinez le rectangle de collision en magenta
        //DrawRectangleLinesEx((Rectangle){HitboxLeftX, HitboxTopY, HITBOX_PIEDS_LARGEUR, HITBOX_PIEDS_HAUTEUR}, 1, MAGENTA);
    }
}

// --- Boucle principale ---

int main(void)
{
    srand((unsigned int)time(NULL)); //graine choisi aléatoirement pour éviter répétition du même motif

    InitWindow(screenWidth, screenHeight, "Jeux Luca");
    SetTargetFPS(60);

    playerTexture = LoadTexture("assets/playerLuca.png");
    fondTexture = LoadTexture("assets/fond.png");
    solTexture = LoadTexture("assets/sol.png");
    solFissureTexture = LoadTexture("assets/solFissure.png");

    InitGrid();
    Joueur Joueur = InitJoueur();


    while (!WindowShouldClose())
    {

        float deltaTime = GetFrameTime();

        if (Joueur.vivant)
        {
            UpdateJoueurMouvement(&Joueur, deltaTime);
            UpdateGridPhase(&Joueur, deltaTime);
            VerifieMort(&Joueur);
        }
        else
        {
            if (IsKeyPressed(KEY_SPACE)) // = relance le jeu
            {
                ResetGridState();
                Joueur = InitJoueur();
                nbrBlockQuiTombe = 8;
                timer = 0.0f;
                phaseActuelle=Phase_Attente;
                scoreDoitAugmenter = false;
                
            }
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DessinePartie();

        DessineJoueur(Joueur);

        if (!Joueur.vivant)
        {
            DrawText("GAME OVER!", (screenWidth / 2)- MeasureText("GAME OVER!", 60)/2, screenHeight / 3, 60, RED);
            DrawText("Appuyez sur ESPACE pour recommencer", (screenWidth / 2) - MeasureText("Appuyez sur ESPACE pour recommencer", 25) / 2, screenHeight / 2, 25, RAYWHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    UnloadTexture(playerTexture);
    UnloadTexture(fondTexture);
    UnloadTexture(solTexture);
    UnloadTexture(solFissureTexture);
    FreeGrid();
    return 0;
}