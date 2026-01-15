#ifndef LUCA_H
#define LUCA_H

#include "menu.h"

// timeout_seconds: 0 = pas de limite, >0 = durée max en secondes
// total_time: pointeur vers le temps total écoulé (tous jeux confondus)
void RunLucaGame(Difficulte difficulty, ModeJoueur mode, bool resume,
                 bool *game_active, const char *playerName, ScoreList *scores,
                 float timeout_seconds, float *total_time, float start_delay);

#endif
