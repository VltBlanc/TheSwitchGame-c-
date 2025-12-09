#ifndef SURVIVOR_H
#define SURVIVOR_H

// Lance la boucle de jeu survivor
// Cette fonction initialise sa propre fenêtre et la ferme à la fin
#include "menu.h"

// Lance la boucle de jeu survivor
// Cette fonction initialise sa propre fenêtre et la ferme à la fin
void RunSurvivorGame(Difficulte difficulty, bool resume, bool *game_active,
                     const char *playerName, ScoreList *scores);

#endif // SURVIVOR_H
