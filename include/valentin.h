#ifndef VALENTIN_H
#define VALENTIN_H

#include "menu.h"

// timeout_seconds: 0 = pas de limite, >0 = durée max en secondes
void RunValentinGame(Difficulte difficulty, bool resume, bool *game_active,
                     const char *playerName, ScoreList *scores,
                     float timeout_seconds);

#endif
