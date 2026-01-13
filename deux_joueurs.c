#include "include/deux_joueurs.h"
#include "include/luca.h"
#include "include/menu.h"
#include "include/snake.h"
#include "include/survivor.h"
#include "include/valentin.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Helper to draw centered text
static void DrawCenteredText(const char *text, int y, int fontSize,
                             Color color) {
  int textWidth = MeasureText(text, fontSize);
  DrawText(text, (GetScreenWidth() - textWidth) / 2, y, fontSize, color);
}

// Function to run a single session (moved/adapted from main.c logic)
float PlaySingleSession(MenuJeu *menu, const char *playerName) {
  // Boucle de rotation des jeux (délai aléatoire entre 4-10 secondes)
  srand((unsigned int)time(NULL));
  GameSelection current_game = (GameSelection)(rand() % 4);

  float total_game_time = 0.0f;
  bool game_started[4] = {false, false, false, false};
  menu->partie_en_cours = true;

  while (menu->partie_en_cours && !WindowShouldClose()) {
    float game_duration = 4.0f + (float)(rand() % 7); // 4-10s
    if (menu->niveau == FACILE)
      game_duration +=
          2.0f; // More time on easy? or less frequency? Adjust as needed.

    // Temps global logic...
    bool resume = game_started[current_game];

    switch (current_game) {
    case GAME_SURVIVOR:
      RunSurvivorGame(menu->niveau, resume, &menu->partie_en_cours, playerName,
                      menu->scores, game_duration, &total_game_time);
      break;
    case GAME_SNAKE:
      RunSnakeGame(menu->niveau, resume, &menu->partie_en_cours, playerName,
                   menu->scores, game_duration, &total_game_time);
      break;
    case GAME_LUCA:
      RunLucaGame(menu->niveau, resume, &menu->partie_en_cours, playerName,
                  menu->scores, game_duration, &total_game_time);
      break;
    case GAME_VALENTIN:
      RunValentinGame(menu->niveau, resume, &menu->partie_en_cours, playerName,
                      menu->scores, game_duration, &total_game_time);
      break;
    }

    game_started[current_game] = true;

    // Check quit conditions
    if (!menu->partie_en_cours || WindowShouldClose()) {
      break;
    }

    current_game = (GameSelection)((current_game + 1) % 4);
  }

  // Restore window after session
  SetWindowSize(LARGEUR_ECRAN, HAUTEUR_ECRAN);
  SetWindowTitle("Jeu de Menu - Projet C");

  return total_game_time;
}

void RunTwoPlayerMode(MenuJeu *menu) {
  const char *p1Name = "JOUR 1";
  const char *p2Name = "JOUR 2";

  // --- JOUEUR 1 ---
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawCenteredText("TOUR DU JOUEUR 1", 200, 40, GREEN);
    DrawCenteredText("Appuyez sur ENTREE pour commencer", 400, 20, LIGHTGRAY);
    EndDrawing();
    if (IsKeyPressed(KEY_ENTER))
      break;
  }

  float scoreP1 = PlaySingleSession(menu, p1Name);

  if (WindowShouldClose())
    return;

  // --- JOUEUR 2 ---
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawCenteredText("TOUR DU JOUEUR 2", 200, 40, BLUE);
    DrawCenteredText("Appuyez sur ENTREE pour commencer", 400, 20, LIGHTGRAY);
    DrawCenteredText(TextFormat("Score à battre: %.2fs", scoreP1), 500, 20,
                     WHITE);
    EndDrawing();
    if (IsKeyPressed(KEY_ENTER))
      break;
  }

  float scoreP2 = PlaySingleSession(menu, p2Name);

  // --- RESULTATS ---
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawCenteredText("FIN DE LA PARTIE", 100, 50, BLACK);

    Color c1 = (scoreP1 > scoreP2) ? GREEN : GRAY;
    Color c2 = (scoreP2 > scoreP1) ? BLUE : GRAY;
    if (scoreP1 == scoreP2) {
      c1 = BLACK;
      c2 = BLACK;
    }

    DrawText(TextFormat("JOUEUR 1: %.2fs", scoreP1), 200, 250, 40, c1);
    DrawText(TextFormat("JOUEUR 2: %.2fs", scoreP2), 200, 350, 40, c2);

    const char *winner =
        (scoreP1 > scoreP2)
            ? "JOUEUR 1 GAGNE !"
            : (scoreP2 > scoreP1 ? "JOUEUR 2 GAGNE !" : "EGALITE !");
    DrawCenteredText(winner, 500, 60, RED);

    DrawCenteredText("Appuyez sur ENTREE pour revenir au menu", 650, 20,
                     DARKGRAY);

    EndDrawing();

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_SPACE))
      break;
  }

  // Save scores?
  add_score(menu->scores, p1Name, scoreP1);
  add_score(menu->scores, p2Name, scoreP2);
  save_scores(menu->scores);
}
