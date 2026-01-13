#include "include/menu.h"
#include <stdio.h>
#include <stdlib.h>

// Palette de couleurs pour le jeu
#define COULEUR_FOND (Color){20, 20, 40, 255}      // Fond bleu foncé
#define COULEUR_TEXTE (Color){0, 255, 255, 255}    // Texte cyan
#define COULEUR_SURBRILL (Color){255, 255, 0, 255} // Jaune pour la sélection
#define COULEUR_BOUTON (Color){255, 0, 255, 255}   // Boutons magenta
#define COULEUR_SURVOL (Color){0, 255, 0, 255}     // Vert au survol
#define COULEUR_ROUGE (Color){255, 0, 0, 255}      // Rouge pour difficulté
#define COULEUR_ROUGE_FON (Color){139, 0, 0, 255}  // Rouge foncé
#define COULEUR_SCORE (Color){255, 165, 0, 255}    // Orange pour scores

MenuJeu *init_menu(void) {
  // Allouer de la mémoire pour le menu
  MenuJeu *menu = (MenuJeu *)malloc(sizeof(MenuJeu));
  if (!menu) {
    printf("ERREUR: Impossible d'allouer la mémoire pour le menu!\n");
    return NULL;
  }

  // Initialiser les valeurs par défaut
  menu->etat = MENU_PRINCIPAL;
  menu->etat = MENU_PRINCIPAL;
  menu->mode = UN_JOUEUR;
  menu->niveau = MOYEN;
  strcpy(menu->playerName, "JOUEUR"); // Nom par défaut
  menu->niveau = MOYEN;
  menu->option_selectionnee = 0;
  menu->partie_en_cours = false;
  menu->jeu_selectionne = GAME_SURVIVOR; // Jeu par défaut

  printf("Menu initialisé avec succès!\n");

  // Charger les meilleurs scores
  menu->scores = load_scores();
  if (menu->scores) {
    menu->nombre_scores = get_high_scores(menu->scores, menu->meilleurs_scores,
                                          NOMBRE_MEILLEURS_SCORES);
    printf("Chargé %d scores\n", menu->nombre_scores);
  } else {
    menu->nombre_scores = 0;
    printf("Aucun score trouvé\n");
  }

  return menu;
}

void free_menu(MenuJeu *menu) {
  if (menu) {
    printf("Libération du menu...\n");
    free_scores(menu->scores);
    free(menu);
    printf("Menu libéré!\n");
  }
}

void update_menu(MenuJeu *menu) {
  // Navigation avec les flèches haut/bas
  if (menu->etat == MENU_PRINCIPAL) {
    int max_options = menu->partie_en_cours ? 4 : 3;
    if (IsKeyPressed(KEY_DOWN)) {
      menu->option_selectionnee = (menu->option_selectionnee + 1) % max_options;
    } else if (IsKeyPressed(KEY_UP)) {
      menu->option_selectionnee =
          (menu->option_selectionnee - 1 + max_options) % max_options;
    }
  } else if (menu->etat == MENU_NAME_INPUT) {
    // Gestion de la saisie de texte
    int key = GetCharPressed();
    while (key > 0) {
      // Seulement les caractères alphanumériques
      if ((key >= 32) && (key <= 125) && (strlen(menu->playerName) < 15)) {
        int len = strlen(menu->playerName);
        menu->playerName[len] = (char)key;
        menu->playerName[len + 1] = '\0';
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = strlen(menu->playerName);
      if (len > 0) {
        menu->playerName[len - 1] = '\0';
      }
    }

    if (IsKeyPressed(KEY_ENTER)) {
      menu->etat = MENU_PRINCIPAL; // Valider et retourner au menu
    }
  }

  // --- Gestion Clavier ---
  if (menu->etat == MENU_PRINCIPAL) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      switch (menu->option_selectionnee) {
      case 0: // Mode de jeu
        if (menu->mode == UN_JOUEUR)
          menu->mode = DEUX_JOUEURS;
        else
          menu->mode = UN_JOUEUR;
        break;
      case 1: // Difficulté
        menu->niveau = (menu->niveau + 1) % 3;
        break;
      case 2: // Démarrer
        menu->etat = MENU_DEMARRAGE;
        break;
      case 3: // Restart
        if (menu->partie_en_cours) {
          menu->partie_en_cours = false;
          menu->etat = MENU_DEMARRAGE;
        }
        break;
      }
    }

    // Flèches gauche/droite
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
      if (menu->option_selectionnee == 0) { // Mode
        if (menu->mode == UN_JOUEUR)
          menu->mode = DEUX_JOUEURS;
        else
          menu->mode = UN_JOUEUR;
      } else if (menu->option_selectionnee == 1) { // Difficulté
        menu->niveau = (menu->niveau + 1) % 3;
      }
    }
  }

  // --- Gestion Souris ---
  Vector2 position_souris = GetMousePosition();
  int centerLeft = 250;

  // 1. Nom (Name Input)
  Rectangle nomBox = {centerLeft - 100, 150, 200, 30};
  if (menu->etat == MENU_PRINCIPAL) {
    if (CheckCollisionPointRec(position_souris, nomBox) &&
        IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      menu->etat = MENU_NAME_INPUT;
    }
  } else if (menu->etat == MENU_NAME_INPUT) {
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        !CheckCollisionPointRec(position_souris, nomBox)) {
      menu->etat = MENU_PRINCIPAL;
    }
  }

  // Si on est en saisie de nom, on ne gère pas les autres clics
  if (menu->etat != MENU_PRINCIPAL)
    return;

  // 2. Boutons 1J / 2J
  int yPosMode = 230;
  Rectangle zone_1joueur = {centerLeft - 105, yPosMode + 40, 100, 30};
  Rectangle zone_2joueurs = {centerLeft + 5, yPosMode + 40, 100, 30};

  // 3. Difficulté
  // const char *difficultes[] = {"FACILE", "MOYEN", "DIFF."};
  int btnWidth = 80;
  int gap = 10;
  int startXDiff = centerLeft - (3 * btnWidth + 2 * gap) / 2;
  int yPosDiff = 320;
  Rectangle zone_facile = {startXDiff, yPosDiff + 40, btnWidth, 30};
  Rectangle zone_moyen = {startXDiff + btnWidth + gap, yPosDiff + 40, btnWidth,
                          30};
  Rectangle zone_difficile = {startXDiff + 2 * (btnWidth + gap), yPosDiff + 40,
                              btnWidth, 30};

  // 4. Start / Restart
  int yPosStart = 420;
  Rectangle bouton_demarrer = {centerLeft - 90, yPosStart, 180, 50};
  Rectangle bouton_new = {centerLeft - 70, yPosStart + 60, 140, 30};

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    // Mode
    if (CheckCollisionPointRec(position_souris, zone_1joueur))
      menu->mode = UN_JOUEUR;
    else if (CheckCollisionPointRec(position_souris, zone_2joueurs))
      menu->mode = DEUX_JOUEURS;

    // Difficulté
    if (CheckCollisionPointRec(position_souris, zone_facile))
      menu->niveau = FACILE;
    else if (CheckCollisionPointRec(position_souris, zone_moyen))
      menu->niveau = MOYEN;
    else if (CheckCollisionPointRec(position_souris, zone_difficile))
      menu->niveau = DIFFICILE;

    // Start
    if (CheckCollisionPointRec(position_souris, bouton_demarrer))
      menu->etat = MENU_DEMARRAGE;

    // New Game
    if (menu->partie_en_cours &&
        CheckCollisionPointRec(position_souris, bouton_new)) {
      menu->partie_en_cours = false;
      menu->etat = MENU_DEMARRAGE;
    }

    // 5. Sélection Score (Droit)
    int centerRight = 600;
    for (int i = 0; i < menu->nombre_scores; i++) {
      int scoreY = 180 + i * 30;
      Rectangle scoreRec = {centerRight - 100, scoreY, 200, 25};
      if (CheckCollisionPointRec(position_souris, scoreRec)) {
        // Copier le nom
        strncpy(menu->playerName, menu->meilleurs_scores[i].playername, 15);
        menu->playerName[15] = '\0';
      }
    }
  }
}

void draw_menu(const MenuJeu *menu) {
  ClearBackground(COULEUR_FOND);

  // Dessiner le titre
  const char *titre = "MENU JEU";
  int taille_titre = MeasureText(titre, 60);
  DrawText(titre, (LARGEUR_ECRAN - taille_titre) / 2, 30, 60, COULEUR_SURBRILL);

  // Colonne Gauche: Contrôles (Centre ~ x=250)
  int centerLeft = 250;

  // Affichage du nom du joueur
  Rectangle nomBox = {centerLeft - 100, 150, 200, 30};
  bool editing = (menu->etat == MENU_NAME_INPUT);
  Color boxColor = editing ? COULEUR_SURBRILL : COULEUR_BOUTON;

  DrawText("NOM DU JOUEUR:", centerLeft - MeasureText("NOM DU JOUEUR:", 20) / 2,
           125, 20, COULEUR_TEXTE);
  DrawRectangleRec(nomBox, boxColor);
  DrawRectangleLinesEx(nomBox, 2, COULEUR_TEXTE);
  int nameWidth = MeasureText(menu->playerName, 20);
  DrawText(menu->playerName, nomBox.x + (nomBox.width - nameWidth) / 2,
           nomBox.y + 5, 20, BLACK);

  if (editing) {
    const char *msg = "ENTRÉE pour valider";
    DrawText(msg, centerLeft - MeasureText(msg, 16) / 2, 185, 16,
             COULEUR_SURBRILL);
  }

  // Draw other left column controls
  draw_player_selection(menu);
  draw_difficulty_selection(menu);
  draw_start_button(menu);

  // Colonne Droite: Scores (Centre ~ x=600)
  draw_high_scores(menu);

  // Séparateur vertical
  DrawLine(425, 120, 425, 500, GRAY);

  // Instructions
  DrawText("↑ ↓ Naviguer  |  ← → Modifier  |  ENTRÉE Sélectionner",
           LARGEUR_ECRAN / 2 - 220, 570, 16, COULEUR_TEXTE);
  DrawText("Échap pour quitter", LARGEUR_ECRAN / 2 - 70, 590, 16,
           COULEUR_TEXTE);
}

void draw_high_scores(const MenuJeu *menu) {
  int centerRight = 600;
  DrawText("MEILLEURS SCORES",
           centerRight - MeasureText("MEILLEURS SCORES", 30) / 2, 130, 30,
           COULEUR_TEXTE);

  if (menu->nombre_scores > 0) {
    for (int i = 0; i < menu->nombre_scores; i++) {
      char texte_score[64];
      sprintf(texte_score, "%d. %s - %.2fs", i + 1,
              menu->meilleurs_scores[i].playername,
              menu->meilleurs_scores[i].time);

      // Hitbox pour le clic (visualisation implicite)
      int scoreY = 180 + i * 30;

      Color scoreColor = COULEUR_SCORE;
      // Petit effet de survol si on veut (optionnel, géré dans update serait
      // mieux mais ici pour simple feedback visuel)
      if (CheckCollisionPointRec(
              GetMousePosition(),
              (Rectangle){centerRight - 100, scoreY, 200, 25})) {
        scoreColor = COULEUR_SURVOL;
      }

      DrawText(texte_score, centerRight - MeasureText(texte_score, 20) / 2,
               scoreY, 20, scoreColor);
    }
    DrawText("(Cliquez pour copier)",
             centerRight - MeasureText("(Cliquez pour copier)", 14) / 2,
             180 + menu->nombre_scores * 30 + 10, 14, GRAY);
  } else {
    DrawText("Aucun score disponible",
             centerRight - MeasureText("Aucun score disponible", 18) / 2, 170,
             18, COULEUR_TEXTE);
  }
}

void draw_player_selection(const MenuJeu *menu) {
  int centerLeft = 250;
  int yPos = 230;

  // Indicateur de sélection
  if (menu->option_selectionnee == 0) {
    DrawText("▶ JOUEURS", centerLeft - 50, yPos, 25, COULEUR_SURBRILL);
  } else {
    DrawText("JOUEURS", centerLeft - MeasureText("JOUEURS", 25) / 2, yPos, 25,
             COULEUR_TEXTE);
  }

  // Bouton 1 joueur
  Rectangle bouton_1j = {centerLeft - 105, yPos + 40, 100, 30};
  Color couleur_1j =
      (menu->mode == UN_JOUEUR) ? COULEUR_SURBRILL : COULEUR_BOUTON;
  DrawRectangleRec(bouton_1j, couleur_1j);
  DrawRectangleLinesEx(bouton_1j, 2, COULEUR_TEXTE);
  DrawText("1 JOUEUR",
           bouton_1j.x + (bouton_1j.width - MeasureText("1 JOUEUR", 10)) / 2 -
               15,
           bouton_1j.y + 8, 16, BLACK);

  // Bouton 2 joueurs
  Rectangle bouton_2j = {centerLeft + 5, yPos + 40, 100, 30};
  Color couleur_2j =
      (menu->mode == DEUX_JOUEURS) ? COULEUR_SURBRILL : COULEUR_BOUTON;
  DrawRectangleRec(bouton_2j, couleur_2j);
  DrawRectangleLinesEx(bouton_2j, 2, COULEUR_TEXTE);
  DrawText("2 JOUEURS",
           bouton_2j.x + (bouton_2j.width - MeasureText("2 JOUEURS", 10)) / 2 -
               15,
           bouton_2j.y + 8, 16, BLACK);
}

void draw_difficulty_selection(const MenuJeu *menu) {
  int centerLeft = 250;
  int yPos = 320;

  // Indicateur de sélection
  if (menu->option_selectionnee == 1) {
    DrawText("▶ DIFFICULTÉ", centerLeft - 60, yPos, 25, COULEUR_SURBRILL);
  } else {
    DrawText("DIFFICULTÉ", centerLeft - MeasureText("DIFFICULTÉ", 25) / 2, yPos,
             25, COULEUR_TEXTE);
  }

  // Boutons de difficulté (plus petits)
  const char *difficultes[] = {"FACILE", "MOYEN", "DIFF."};
  int btnWidth = 80;
  int gap = 10;
  int startX = centerLeft - (3 * btnWidth + 2 * gap) / 2;

  for (int i = 0; i < 3; i++) {
    Rectangle btnRec = {startX + i * (btnWidth + gap), yPos + 40, btnWidth, 30};
    Color couleur_btn =
        (menu->niveau == (Difficulte)i) ? COULEUR_SURBRILL : COULEUR_BOUTON;
    DrawRectangleRec(btnRec, couleur_btn);
    DrawRectangleLinesEx(btnRec, 2, COULEUR_TEXTE);

    int textX = btnRec.x + (btnRec.width - MeasureText(difficultes[i], 16)) / 2;
    DrawText(difficultes[i], textX, btnRec.y + 7, 16, BLACK);
  }
}

void draw_start_button(const MenuJeu *menu) {
  int centerLeft = 250;
  int yPos = 420;

  Rectangle bouton_demarrer = {centerLeft - 90, yPos, 180, 50};
  Color couleur_demarrer =
      (menu->option_selectionnee == 2) ? COULEUR_SURBRILL : COULEUR_SURVOL;

  Vector2 position_souris = GetMousePosition();
  if (CheckCollisionPointRec(position_souris, bouton_demarrer)) {
    couleur_demarrer = COULEUR_SURBRILL;
  }

  DrawRectangleRec(bouton_demarrer, couleur_demarrer);
  DrawRectangleLinesEx(bouton_demarrer, 3, COULEUR_TEXTE);

  const char *texte_bouton = menu->partie_en_cours ? "REPRENDRE" : "DÉMARRER";
  int largeur_texte_demarrer = MeasureText(texte_bouton, 24);
  DrawText(texte_bouton,
           bouton_demarrer.x +
               (bouton_demarrer.width - largeur_texte_demarrer) / 2,
           bouton_demarrer.y + 13, 24, BLACK);

  if (menu->partie_en_cours) {
    // Bouton nouvelle partie
    Rectangle bouton_new = {centerLeft - 70, yPos + 60, 140, 30};
    Color col_new =
        (menu->option_selectionnee == 3) ? COULEUR_SURBRILL : COULEUR_BOUTON;
    if (CheckCollisionPointRec(position_souris, bouton_new))
      col_new = COULEUR_SURBRILL;
    DrawRectangleRec(bouton_new, col_new);
    DrawRectangleLinesEx(bouton_new, 2, COULEUR_TEXTE);
    DrawText("NOUVELLE PARTIE", bouton_new.x + 10, bouton_new.y + 7, 14, BLACK);
  }
}

const char *get_difficulty_text(Difficulte niveau) {
  switch (niveau) {
  case FACILE:
    return "FACILE";
  case MOYEN:
    return "MOYEN";
  case DIFFICILE:
    return "DIFFICILE";
  default:
    return "INCONNU";
  }
}

float get_difficulty_percentage(Difficulte niveau) {
  switch (niveau) {
  case FACILE:
    return 0.33f;
  case MOYEN:
    return 0.66f;
  case DIFFICILE:
    return 1.0f;
  default:
    return 0.0f;
  }
}

Color get_difficulty_color(Difficulte niveau) {
  switch (niveau) {
  case FACILE:
    return (Color){255, 100, 100, 255};
  case MOYEN:
    return (Color){255, 50, 50, 255};
  case DIFFICILE:
    return COULEUR_ROUGE;
  default:
    return COULEUR_TEXTE;
  }
}