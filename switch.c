#include "include/menu.h"
#include "include/allJeu.h"
#include "include/switch.h"
#include <raylib.h>
#include <stdio.h>

int nbJeu=2;

int switchMeca(Difficulte version)
{
    SetTargetFPS(60);

    GameScreen currentScreen = GetRandomValue(0, nbJeu);  // Jeu de départ
    float timeToSwitch = (float)GetRandomValue(4, 7); // Entre 5 et 15 secondes
    float timer = 0.0f;

    // Initialisation globale (ou spécifique au premier jeu)
    bool gameActive = false;
    runLuca(version, false, &gameActive);
    RunValentinGame(version, false, &gameActive);
    gameActive=true;

    bool estMort=false;

    while (!WindowShouldClose() && !estMort)
    {
        // --- LOGIQUE DE TRANSITION ---
        timer += GetFrameTime();

        if (timer >= timeToSwitch)
        {
            currentScreen = GetRandomValue(0, nbJeu);        // Nouveau jeu au hasard
            timer = 0.0f;                                // Reset chrono
            timeToSwitch = (float)GetRandomValue(3, 7); // Nouveau délai

            // Optionnel : Réinitialiser le jeu choisi pour repartir de zéro
            // SwitchInit(currentScreen);
        }

        // --- UPDATE & DRAW ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScreen)
        {
        case JEU1:
            gameActive=true;
            runLuca(version, true, &gameActive);
            gameActive=false;
            RunValentinGame(version, true, &gameActive);
            //2autre jeu
            break;
        case JEU2:
            gameActive=true;
            RunValentinGame(version, true, &gameActive);
            gameActive=false;
            RunLuca(version, true, &gameActive);
            //2autre jeu
            break;
        /*case JEU3:
            UpdateJeu3();
            DrawJeu3();
            break;
        case JEU4:
            UpdateJeu4();
            DrawJeu4();
            break;*/
        }

        // Afficher le temps restant pour le debug
        DrawText(TextFormat("Switch dans: %.1f s", timeToSwitch - timer), 10, 10, 20, DARKGRAY);

        EndDrawing();
    }
    CloseWindow();

    int score=0;
    score+=GameOverLuca();
    score+=GameOverValentin();
    //2autrejeu

    return score;
}