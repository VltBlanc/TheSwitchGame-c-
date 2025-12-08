
#include "etoile.h"
#include <stdlib.h> 

// --- Définition des Variables Globales du Starfield ---
Vector3 etoiles[NB_ETOILES] = { 0 }; 
Vector2 etoilesPosEcran[NB_ETOILES] = { 0 }; 
float vitesseStarfield = 0.5f; 

// Initialise les positions aléatoires des étoiles
void InitialiserStarfield(int largeur, int hauteur) {
    for (int i = 0; i < NB_ETOILES; i++) {
        etoiles[i].x = GetRandomValue(-largeur * 0.5f, largeur * 0.5f);
        etoiles[i].y = GetRandomValue(-hauteur * 0.5f, hauteur * 0.5f);
        etoiles[i].z = 1.0f;
    }
}

// Met à jour la position des étoiles pour l'effet Starfield
void ActualiserStarfield(float tempsSurvie, int niveauDifficulte) {
        float dt = GetFrameTime();
        
        // Utiliser le tempsSurvie pour augmenter la vitesse progressivement
        // Utilisation simple : chaque seconde ajoute un petit bonus de vitesse.
        float vitesseBonusTemps = tempsSurvie * 0.02f; 
        
        // Vitesse totale
        float vitesseActuelle = vitesseStarfield + (niveauDifficulte * 0.1f) + vitesseBonusTemps;
    for (int i = 0; i < NB_ETOILES; i++) {
        // Avancement des étoiles (z diminue)
        etoiles[i].z -= dt * vitesseActuelle;

        // Calcul de la position 2D à l'écran
        etoilesPosEcran[i] = (Vector2){
            (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z,
            (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z,
        };

        // Réinitialisation si l'étoile est hors champ ou trop proche
        if ((etoiles[i].z < 0.0f) || (etoilesPosEcran[i].x < 0) || (etoilesPosEcran[i].y < 0.0f) ||
            (etoilesPosEcran[i].x > GetScreenWidth()) || (etoilesPosEcran[i].y > GetScreenHeight()))
        {
            etoiles[i].x = GetRandomValue(-GetScreenWidth() * 0.5f, GetScreenWidth() * 0.5f);
            etoiles[i].y = GetRandomValue(-GetScreenHeight() * 0.5f, GetScreenHeight() * 0.5f);
            etoiles[i].z = 1.0f;
        }
    }
}

// Rend les étoiles à l'écran
void DessinerStarfield(void) {
    for (int i = 0; i < NB_ETOILES; i++) {
        float radius = 1.0f / etoiles[i].z; 
        if (radius < 0.5f) radius = 0.5f; 
        
        Color couleurEtoile = Fade(RAYWHITE, 1.0f / etoiles[i].z); 
        
        DrawCircleV(etoilesPosEcran[i], radius, couleurEtoile);
    }
}