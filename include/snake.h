#ifndef SNAKE_H
#define SNAKE_H

#include "menu.h"

void RunSnakeGame(Difficulte difficulty, bool resume, bool *game_active,
                  const char *playerName, ScoreList *scores);

#endif
