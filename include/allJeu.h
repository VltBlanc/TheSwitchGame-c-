#ifndef ALLJEU_H
#define ALLJEU_H

#include "menu.h"

typedef enum GameScreen { JEU1, JEU2/*, JEU3, JEU4*/ } GameScreen;

int GameOverLuca();
void runLuca(Difficulte version,bool resume,bool *gameActive,bool *mort);

void RunValentinGame(Difficulte niveau, bool resume, bool *game_active,bool *mort);
int GameOverValentin();

#endif