#include "menu.h"

typedef enum GameScreen { JEU1, JEU2/*, JEU3, JEU4*/ } GameScreen;

int GameOverLuca();
void runLuca(Difficulte version,bool resume,bool *gameActive);

void RunValentinGame(Difficulte niveau, bool resume, bool *game_active);
int GameOverValentin();