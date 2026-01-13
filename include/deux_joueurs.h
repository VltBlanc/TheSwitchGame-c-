#ifndef DEUX_JOUEURS_H
#define DEUX_JOUEURS_H

#include "menu.h"

// Lance le mode 2 joueurs (tour par tour)
void RunTwoPlayerMode(MenuJeu *menu);

// Expose the single session runner for reusability
float PlaySingleSession(MenuJeu *menu, const char *playerName);

#endif
