#ifndef SNAKE_H
#define SNAKE_H

#include "menu.h"

// timeout_seconds: 0 = pas de limite, >0 = durée max en secondes
void RunSnakeGame(Difficulte difficulty, bool resume, bool *game_active,
                  const char *playerName, ScoreList *scores,
                  float timeout_seconds);

#endif
