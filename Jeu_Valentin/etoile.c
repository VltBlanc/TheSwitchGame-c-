#include "etoile.h"
#include <stdlib.h> 

Vector3 etoiles[NB_ETOILES] = { 0 }; 
Vector2 etoilesPosEcran[NB_ETOILES] = { 0 }; 
Vector2 etoilesPosEcranPrecedente[NB_ETOILES] = { 0 }; // Stocke la position du frame précédent
float vitesseStarfield = 0.5f; 

// Initialise les positions aléatoires des étoiles
void Initialiseretoile(int largeur, int hauteur) {
    for (int i = 0; i < NB_ETOILES; i++) {
        etoiles[i].x = GetRandomValue(-largeur * 0.5f, largeur * 0.5f);
        etoiles[i].y = GetRandomValue(-hauteur * 0.5f, hauteur * 0.5f);
        etoiles[i].z = GetRandomValue(100, 200) / 100.0f; // Commence entre z=1.0 et z=2.0
        
        // Initiall'identique iser les positions écran à pour éviter --> un trait au démarrage
        etoilesPosEcran[i] = (Vector2){
            (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z,
            (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z,
        };
        etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
    }
}

// Met à jour la position des étoiles 
void Actualiseretoile(float tempsSurvie, int niveauDifficulte) {
        float dt = GetFrameTime();
        
        // Utiliser le tempsSurvie pour augmenter la vitesse progressivement
        float vitesseBonusTemps = tempsSurvie * 0.02f; 
        
        // Vitesse totale
        // Vitesse de base augmentée pour un  effet filant
        float vitesseActuelle = (vitesseStarfield * 1.5f) + (niveauDifficulte * 0.15f) + vitesseBonusTemps;
        
    for (int i = 0; i < NB_ETOILES; i++) {
        //1) : Sauvegarder la position actuelle comme position précédente
        etoilesPosEcranPrecedente[i] = etoilesPosEcran[i]; // <--- MODIFICATION

        //2) : Avancement des étoiles (z diminue)
        etoiles[i].z -= dt * vitesseActuelle;

        //3) : Calcul de la NOUVELLE position 2D à l'écran
        etoilesPosEcran[i] = (Vector2){
            (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z,
            (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z,
        };

        //4) : Réinitialisation si l'étoile est hors champ ou trop proche
        if ((etoiles[i].z < 0.0f) || (etoilesPosEcran[i].x < 0) || (etoilesPosEcran[i].y < 0.0f) ||
            (etoilesPosEcran[i].x > GetScreenWidth()) || (etoilesPosEcran[i].y > GetScreenHeight()))
        {
            // Réinitialisation de X et Y (hors centre) et Z (au loin)
            etoiles[i].x = GetRandomValue(-GetScreenWidth() * 0.5f, GetScreenWidth() * 0.5f);
            etoiles[i].y = GetRandomValue(-GetScreenHeight() * 0.5f, GetScreenHeight() * 0.5f);
            etoiles[i].z = GetRandomValue(150, 250) / 100.0f; // Nouvelle étoile commence plus loin (z > 1.0)
            
            // Pour éviter un long trait au moment de la réinitialisation, on aligne les positions
            etoilesPosEcran[i] = (Vector2){
                (float)GetScreenWidth() * 0.5f + etoiles[i].x / etoiles[i].z,
                (float)GetScreenHeight() * 0.5f + etoiles[i].y / etoiles[i].z,
            };
            etoilesPosEcranPrecedente[i] = etoilesPosEcran[i];
        }
    }
}

// Rend les étoiles à l'écran
void Dessineretoile(void) {
    for (int i = 0; i < NB_ETOILES; i++) {
        float taille = 1.5f / etoiles[i].z; // Taille base pour le trait
        if (taille < 0.5f) taille = 0.5f; 
        
        Color couleurEtoile = Fade(RAYWHITE, 1.0f / etoiles[i].z); 
        
        DrawLineEx(etoilesPosEcranPrecedente[i], etoilesPosEcran[i], taille, couleurEtoile);
    }
}