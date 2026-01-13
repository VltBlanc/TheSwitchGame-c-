#include "etoile.h"
#include <stdlib.h> 

// Variables - Changées en pointeurs pour permettre la libération mémoire
Vector3 *etoiles = NULL; 
Vector2 *etoilesPosEcran = NULL; 
Vector2 *etoilesPosEcranPrecedente = NULL; 
float vitesseStarfield = 0.5f; 

// Initialise les étoiles et alloue la mémoire
void Initialiseretoile(int largeur, int hauteur) {
    // Allocation de la mémoire pour le nombre d'étoiles défini
    etoiles = (Vector3 *)malloc(NB_ETOILES * sizeof(Vector3));
    etoilesPosEcran = (Vector2 *)malloc(NB_ETOILES * sizeof(Vector2));
    etoilesPosEcranPrecedente = (Vector2 *)malloc(NB_ETOILES * sizeof(Vector2));

    // Vérification de sécurité pour s'assurer que l'allocation a réussi
    if (etoiles == NULL || etoilesPosEcran == NULL || etoilesPosEcranPrecedente == NULL) return;

    for (int i = 0; i < NB_ETOILES; i++) {
        etoiles[i].x = (float)GetRandomValue(-largeur * 0.5f, largeur * 0.5f);
        etoiles[i].y = (float)GetRandomValue(-hauteur * 0.5f, hauteur * 0.5f);
        etoiles[i].z = (float)GetRandomValue(100, 200) / 100.0f; 
        
        // Calcul de la position 2D initiale
        etoilesPosEcran[i].x = (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z;
        etoilesPosEcran[i].y = (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z;
        
        etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
    }
}

// Libère la mémoire allouée
void LibererEtoile(void) {
    if (etoiles != NULL) {
        free(etoiles);
        etoiles = NULL;
    }
    if (etoilesPosEcran != NULL) {
        free(etoilesPosEcran);
        etoilesPosEcran = NULL;
    }
    if (etoilesPosEcranPrecedente != NULL) {
        free(etoilesPosEcranPrecedente);
        etoilesPosEcranPrecedente = NULL;
    }
}

// Met à jour la position && gère la vitesse (évolutive)
void Actualiseretoile(float tempsSurvie, int niveauDifficulte) {
    if (etoiles == NULL) return; // Sécurité si non initialisé

    float dt = GetFrameTime();
    
    // Calcul de la vitesse 
    float vitesseActuelle = (vitesseStarfield * 1.5f) + (niveauDifficulte * 0.15f) + (tempsSurvie * 0.02f);
        
    for (int i = 0; i < NB_ETOILES; i++) {
        // Sauvegarde de l'ancienne position (permet effet de sillage)
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
            etoiles[i].x = (float)GetRandomValue(-GetScreenWidth() * 0.5f, GetScreenWidth() * 0.5f);
            etoiles[i].y = (float)GetRandomValue(-GetScreenHeight() * 0.5f, GetScreenHeight() * 0.5f);
            etoiles[i].z = (float)GetRandomValue(150, 250) / 100.0f; 
            
            // Recalcul immédiat pour éviter un trait qui traverse tout l'écran
            etoilesPosEcran[i].x = (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z;
            etoilesPosEcran[i].y = (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z;
            etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
        }
    }
}

// Affiche les étoiles
void Dessineretoile(void) {
    if (etoiles == NULL) return;

    for (int i = 0; i < NB_ETOILES; i++) {
        // Plus l'étoile est proche (Z petit) --> le trait est épais
        float epaisseur = 1.5f / etoiles[i].z; 
        if (epaisseur < 0.5f) epaisseur = 0.5f; 
        
        // Effet brillance (Fade diminue l'alpha selon la distance)
        Color couleur = Fade(RAYWHITE, 1.0f / etoiles[i].z); 
        
        // Dessine un trait entre l'ancienne et la nouvelle position
        DrawLineEx(etoilesPosEcranPrecedente[i], etoilesPosEcran[i], epaisseur, couleur);
    }
}