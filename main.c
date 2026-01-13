#include "include/luca.h"
#include "include/menu.h"
#include "include/snake.h"
#include "include/survivor.h"
#include "include/valentin.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  printf("Démarrage du jeu...\n");

  // Initialisation de la fenêtre Raylib
  InitWindow(LARGEUR_ECRAN, HAUTEUR_ECRAN, "Jeu de Menu - Projet C");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL); // Empêche Raylib de fermer la fenêtre avec Echap

  // Initialisation du menu
  MenuJeu *menu = init_menu();
  if (!menu) {
    printf("ERREUR: L'initialisation du menu a échoué!\n");
    CloseWindow();
    return 1;
  }

  printf("=== MENU DE JEU DÉMARRÉ ===\n");
  printf("Contrôles:\n");
  printf("- Flèches: Naviguer dans le menu\n");
  printf("- Entrée: Sélectionner une option\n");
  printf("- Souris: Cliquer pour sélectionner\n");
  printf("- Échap: Quitter\n");
  printf("===============================\n");

  // Boucle principale du jeu
  printf("Entrée dans la boucle de jeu principale...\n");
  while (!WindowShouldClose()) {
    // Mettre à jour l'état du menu
    update_menu(menu);

    // Vérifier la touche Échap
    if (IsKeyPressed(KEY_ESCAPE)) {
      printf("Touche Échap pressée, préparation à quitter...\n");
      menu->etat = MENU_QUITTER;
    }

    // Commencer le dessin
    BeginDrawing();
    draw_menu(menu);
    EndDrawing();

    // Gérer les changements d'état du menu
    if (menu->etat == MENU_DEMARRAGE) {
      printf("DÉMARRAGE DU JEU!\n");
      printf("Paramètres: %d joueur(s), difficulté %s\n", menu->mode + 1,
             get_difficulty_text(menu->niveau));

      // Boucle de rotation des jeux (délai aléatoire entre 4-10 secondes)
      srand((unsigned int)time(NULL)); // Initialiser le générateur aléatoire
      GameSelection current_game =
          (GameSelection)(rand() % 4); // Jeu de départ aléatoire
      bool should_continue = true;

      while (should_continue && !WindowShouldClose()) {
        // Délai aléatoire entre 4 et 10 secondes
        float game_duration = 4.0f + (float)(rand() % 7); // 4 à 10 secondes
        printf("Lancement du jeu %d pour %.0f secondes...\n", current_game,
               game_duration);

        // Lancer le jeu actuel avec timeout
        switch (current_game) {
        case GAME_SURVIVOR:
          RunSurvivorGame(menu->niveau, false, &menu->partie_en_cours,
                          menu->playerName, menu->scores, game_duration);
          break;
        case GAME_SNAKE:
          RunSnakeGame(menu->niveau, false, &menu->partie_en_cours,
                       menu->playerName, menu->scores, game_duration);
          break;
        case GAME_LUCA:
          RunLucaGame(menu->niveau, false, &menu->partie_en_cours,
                      menu->playerName, menu->scores, game_duration);
          break;
        case GAME_VALENTIN:
          RunValentinGame(menu->niveau, false, &menu->partie_en_cours,
                          menu->playerName, menu->scores, game_duration);
          break;
        }

        // Vérifier si on doit quitter (joueur mort ou ESC pressé)
        if (!menu->partie_en_cours || WindowShouldClose()) {
          should_continue = false;
          break;
        }

        // Passer au jeu suivant
        current_game = (GameSelection)((current_game + 1) % 4);
        printf("Passage au jeu suivant: %d\n", current_game);
      }

      // Rafraîchir les meilleurs scores affichés dans le menu
      if (menu->scores) {
        menu->nombre_scores = get_high_scores(
            menu->scores, menu->meilleurs_scores, NOMBRE_MEILLEURS_SCORES);
      }

      // Restaurer la fenêtre du menu
      SetWindowSize(LARGEUR_ECRAN, HAUTEUR_ECRAN);
      SetWindowTitle("Jeu de Menu - Projet C");

      printf("Retour au menu principal...\n");
      menu->etat = MENU_PRINCIPAL;
    } else if (menu->etat == MENU_QUITTER) {
      printf("État QUITTER détecté, sortie de la boucle...\n");
      break;
    }
  }

  // Nettoyage
  printf("Nettoyage des ressources...\n");
  free_menu(menu);
  CloseWindow();

  printf("=== JEU TERMINÉ ===\n");
  printf("Merci d'avoir joué!\n");
  printf("===================\n");
  return 0;
}