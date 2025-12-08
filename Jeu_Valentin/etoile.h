
#ifndef STARFIELD_H
#define STARFIELD_H

#include "raylib.h"
#include "raymath.h"

// --- Constantes du Starfield ---
#define NB_ETOILES 420 

// --- Variables Externes (à définir dans starfield.c) ---
extern Vector3 etoiles[NB_ETOILES]; 
extern Vector2 etoilesPosEcran[NB_ETOILES]; 
extern float vitesseStarfield; 

// --- Prototypes des fonctions ---
void InitialiserStarfield(int largeur, int hauteur);
void ActualiserStarfield(float tempsSurvie, int niveauDifficulte);
void DessinerStarfield(void);

#endif // STARFIELD_H