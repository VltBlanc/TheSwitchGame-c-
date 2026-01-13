#ifndef STARFIELD_H
#define STARFIELD_H

#include "raylib.h"
#include "raymath.h"

#define NB_ETOILES 420 

extern Vector3 etoiles[NB_ETOILES]; 
extern Vector2 etoilesPosEcran[NB_ETOILES]; 
extern float vitesseStarfield; 

// --- Prototypes des fonctions ---
void Initialiseretoile(int largeur, int hauteur);
void Actualiseretoile(float tempsSurvie, int niveauDifficulte);
void Dessineretoile(void);
void LibererEtoile();

#endif