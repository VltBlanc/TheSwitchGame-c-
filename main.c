
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

      // Temps global partagé entre tous les jeux
      float total_game_time = 0.0f;
      // Tracker pour savoir si chaque jeu a déjà été initialisé
      bool game_started[4] = {false, false, false, false};
      menu->partie_en_cours = true;
      while (menu->partie_en_cours && !WindowShouldClose()) {
        // Délai aléatoire entre 4 et 10 secondes
        float game_duration = 5.0f + (float)(rand() % 7); // 4 à 10 secondes

        const char *game_names[] = {"SURVIVOR", "SNAKE", "LUCA", "VALENTIN"};
        printf("\n=== ROTATION: Lancement de %s pour %.0f secondes ===\n",
               game_names[current_game], game_duration);
        printf("Temps global: %.1f secondes | Resume: %s\n", total_game_time,
               game_started[current_game] ? "OUI" : "NON");

        // Lancer le jeu actuel avec timeout
        // resume = true si le jeu a déjà été lancé (pour préserver l'état)
        bool resume = game_started[current_game];

        float start_delay = 0.7f;
        switch (current_game) {
        case GAME_SURVIVOR:
          RunSurvivorGame(menu->niveau, menu->mode, resume,
                          &menu->partie_en_cours, menu->playerName,
                          game_duration, &total_game_time, start_delay);
          break;
        case GAME_SNAKE:
          RunSnakeGame(menu->niveau, menu->mode, resume, &menu->partie_en_cours,
                       menu->playerName, game_duration, &total_game_time,
                       start_delay);
          break;
        case GAME_LUCA:
          RunLucaGame(menu->niveau, menu->mode, resume, &menu->partie_en_cours,
                      menu->playerName, game_duration, &total_game_time,
                      start_delay);
          break;
        case GAME_VALENTIN:
          RunValentinGame(menu->niveau, menu->mode, resume,
                          &menu->partie_en_cours, menu->playerName,
                          game_duration, &total_game_time, start_delay);
          break;
        }

        // Marquer le jeu comme déjà initialisé pour la prochaine fois
        game_started[current_game] = true;

        printf("Après jeu: partie_en_cours = %s, temps global = %.1f\n",
               menu->partie_en_cours ? "true" : "false", total_game_time);

        // Vérifier si on doit quitter (joueur mort ou ESC pressé)
        if (!menu->partie_en_cours || WindowShouldClose()) {
          printf("!!! ARRÊT DE LA ROTATION !!!\n");
          // Sauvegarder le score final (Temps Total)
          // Sauvegarder (Temps et Points)
          if (menu->scores_time) {
            int rankTime =
                add_score(menu->scores_time, menu->playerName, total_game_time,
                          menu->niveau, SCORE_BY_TIME);
            save_scores(menu->scores_time, SCORES_TIME_FILE);
            if (rankTime > 0)
              printf(">>> NOUVEAU RECORD TEMPS! Rang #%d <<<\n", rankTime);
          }
          if (menu->scores_points) {
            int rankPoints =
                add_score(menu->scores_points, menu->playerName,
                          total_game_time, menu->niveau, SCORE_BY_POINTS);
            save_scores(menu->scores_points, SCORES_POINTS_FILE);
            if (rankPoints > 0)
              printf(">>> NOUVEAU RECORD POINTS! Rang #%d <<<\n", rankPoints);
          }
          should_continue = false;
          break;
        }

        // Passer au jeu suivant
        current_game = (GameSelection)((current_game + 1) % 4);
        printf("=== Passage au jeu suivant: %s ===\n",
               game_names[current_game]);
      }

      printf("Retour au menu principal...\n");
      menu->etat = MENU_PRINCIPAL;
      if (!menu->partie_en_cours) {
        printf("Retour au menu principal...\n");
        menu->etat = MENU_PRINCIPAL;
      }
      // Rafraîchir les meilleurs scores affichés dans le menu
      if (menu->scores_time) {
        sort_scores_by_time(menu->scores_time);
        menu->count_time_scores = get_top_scores_by_time(
            menu->scores_time, menu->top_time_scores, NOMBRE_MEILLEURS_SCORES);
      }
      if (menu->scores_points) {
        sort_scores_by_points(menu->scores_points);
        menu->count_point_scores = get_top_scores_by_points(
            menu->scores_points, menu->top_point_scores,
            NOMBRE_MEILLEURS_SCORES);
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