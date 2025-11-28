#include "include/menu.h"
#include "include/survivor.h"
#include <raylib.h>
#include <stdio.h>

int main(void) {
  printf("Démarrage du jeu...\n");

  // Initialisation de la fenêtre Raylib
  InitWindow(LARGEUR_ECRAN, HAUTEUR_ECRAN, "Jeu de Menu - Projet C");
  SetTargetFPS(60);

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

      // Lancer le jeu
      RunSurvivorGame(menu->niveau);

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