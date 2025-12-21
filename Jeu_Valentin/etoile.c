#include "etoile.h"
#include <stdlib.h> 

//Variables  
Vector3 etoiles[NB_ETOILES] = { 0 }; 
Vector2 etoilesPosEcran[NB_ETOILES] = { 0 }; 
Vector2 etoilesPosEcranPrecedente[NB_ETOILES] = { 0 }; 
float vitesseStarfield = 0.5f; 

// Initialise les étoiles 
void Initialiseretoile(int largeur, int hauteur) {
    for (int i = 0; i < NB_ETOILES; i++) {
        etoiles[i].x = GetRandomValue(-largeur * 0.5f, largeur * 0.5f);
        etoiles[i].y = GetRandomValue(-hauteur * 0.5f, hauteur * 0.5f);
        etoiles[i].z = (float)GetRandomValue(100, 200) / 100.0f; 
        
        // Calcul de la position 2D initiale
        etoilesPosEcran[i].x = (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z;
        etoilesPosEcran[i].y = (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z;
        
        etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
    }
}

// Met à jour la position && gère la vitesse (évolutive)
void Actualiseretoile(float tempsSurvie, int niveauDifficulte) {
    float dt = GetFrameTime();
    
    // Calcul de la vitesse 
    float vitesseActuelle = (vitesseStarfield * 1.5f) + (niveauDifficulte * 0.15f) + (tempsSurvie * 0.02f);
        
    for (int i = 0; i < NB_ETOILES; i++) {
        // Sauvegarde de l'ancienne position (permet effet de sillage )
        etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];

        // L'étoile avance vers la caméra 
        etoiles[i].z -= dt * vitesseActuelle;

        // Projection 3D vers 2D
        etoilesPosEcran[i].x = (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z;
        etoilesPosEcran[i].y = (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z;

        // Si l'étoile sort de l'écran ou nous dépasse, on la réinitialise au loin
        if (etoiles[i].z < 0.1f || etoilesPosEcran[i].x < 0 || etoilesPosEcran[i].x > GetScreenWidth() ||
            etoilesPosEcran[i].y < 0 || etoilesPosEcran[i].y > GetScreenHeight()) 
        {
            etoiles[i].x = GetRandomValue(-GetScreenWidth() * 0.5f, GetScreenWidth() * 0.5f);
            etoiles[i].y = GetRandomValue(-GetScreenHeight() * 0.5f, GetScreenHeight() * 0.5f);
            etoiles[i].z = (float)GetRandomValue(150, 250) / 100.0f; 
            
            // Recalcul immédiat pour éviter un trait qui traverse tout l'écran
            etoilesPosEcran[i].x = (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z;
            etoilesPosEcran[i].y = (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z;
            etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
        }
    }
}

// Affiche les étoiles sous forme de lignes pour l'effet de vitesse
void Dessineretoile(void) {
    for (int i = 0; i < NB_ETOILES; i++) {
        // Plus l'étoile est proche (Z petit) -->  le trait est épais
        float epaisseur = 1.5f / etoiles[i].z; 
        if (epaisseur < 0.5f) epaisseur = 0.5f; 
        
        // Effet brillance
        Color couleur = Fade(RAYWHITE, 1.0f / etoiles[i].z); 
        
        // Dessine un trait entre l'ancienne et la nouvelle position
        DrawLineEx(etoilesPosEcranPrecedente[i], etoilesPosEcran[i], epaisseur, couleur);
    }
}