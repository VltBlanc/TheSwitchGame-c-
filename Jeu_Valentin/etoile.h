#ifndef STARFIELD_H
#define STARFIELD_H

#include "raylib.h"
#include "raymath.h"

#define NB_ETOILES 420 

// On déclare que ce sont des pointeurs (pour correspondre au .c)
extern Vector3 *etoiles; 
extern Vector2 *etoilesPosEcran; 
extern float vitesseStarfield; 

// --- Prototypes des fonctions ---
void Initialiseretoile(int largeur, int hauteur);
void Actualiseretoile(float tempsSurvie, int niveauDifficulte);
void Dessineretoile(void);
void LibererEtoile(void);

#endif