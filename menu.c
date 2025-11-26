#include "include/menu.h"
#include <stdlib.h>

// Palette de couleurs pour le jeu
#define COULEUR_FOND      (Color){ 20, 20, 40, 255 }      // Fond bleu foncé
#define COULEUR_TEXTE     (Color){ 0, 255, 255, 255 }     // Texte cyan
#define COULEUR_SURBRILL  (Color){ 255, 255, 0, 255 }     // Jaune pour la sélection
#define COULEUR_BOUTON    (Color){ 255, 0, 255, 255 }     // Boutons magenta
#define COULEUR_SURVOL    (Color){ 0, 255, 0, 255 }       // Vert au survol
#define COULEUR_ROUGE     (Color){ 255, 0, 0, 255 }       // Rouge pour difficulté
#define COULEUR_ROUGE_FON (Color){ 139, 0, 0, 255 }       // Rouge foncé
#define COULEUR_SCORE     (Color){ 255, 165, 0, 255 }     // Orange pour scores

MenuJeu* init_menu(void) {
    // Allouer de la mémoire pour le menu
    MenuJeu* menu = (MenuJeu*)malloc(sizeof(MenuJeu));
    if (!menu) {
        printf("ERREUR: Impossible d'allouer la mémoire pour le menu!\n");
        return NULL;
    }

    // Initialiser les valeurs par défaut
    menu->etat = MENU_PRINCIPAL;
    menu->mode = UN_JOUEUR;
    menu->niveau = MOYEN;
    menu->option_selectionnee = 0;

    printf("Menu initialisé avec succès!\n");

    // Charger les meilleurs scores
    menu->scores = load_scores();
    if (menu->scores) {
        menu->nombre_scores = get_high_scores(menu->scores, menu->meilleurs_scores, NOMBRE_MEILLEURS_SCORES);
        printf("Chargé %d scores\n", menu->nombre_scores);
    } else {
        menu->nombre_scores = 0;
        printf("Aucun score trouvé\n");
    }

    return menu;
}

void free_menu(MenuJeu* menu) {
    if (menu) {
        printf("Libération du menu...\n");
        free_scores(menu->scores);
        free(menu);
        printf("Menu libéré!\n");
    }
}

void update_menu(MenuJeu* menu) {
    // Navigation avec les flèches haut/bas
    if (IsKeyPressed(KEY_DOWN)) {
        menu->option_selectionnee = (menu->option_selectionnee + 1) % 3;
    } else if (IsKeyPressed(KEY_UP)) {
        menu->option_selectionnee = (menu->option_selectionnee - 1 + 3) % 3;
    }

    // Gestion de la sélection avec Enter ou Espace
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (menu->option_selectionnee) {
            case 0: // Mode de jeu
                // Changer le mode de jeu
                if (menu->mode == UN_JOUEUR) {
                    menu->mode = DEUX_JOUEURS;
                } else {
                    menu->mode = UN_JOUEUR;
                }
                printf("Mode de jeu: %d joueur(s)\n", menu->mode + 1);
                break;

            case 1: // Difficulté
                // Passer à la difficulté suivante
                menu->niveau = (menu->niveau + 1) % 3;
                printf("Difficulté: %s\n", get_difficulty_text(menu->niveau));
                break;

            case 2: // Démarrer le jeu
                menu->etat = MENU_DEMARRAGE;
                break;
        }
    }

    // Flèches gauche/droite pour modifier les options
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        if (menu->option_selectionnee == 0) { // Mode de jeu
            // Changer le mode avec les flèches
            if (menu->mode == UN_JOUEUR) {
                menu->mode = DEUX_JOUEURS;
            } else {
                menu->mode = UN_JOUEUR;
            }
            printf("Mode: %d joueur(s)\n", menu->mode + 1);

        } else if (menu->option_selectionnee == 1) { // Difficulté
            // Changer la difficulté avec les flèches
            menu->niveau = (menu->niveau + 1) % 3;
            printf("Difficulté: %s\n", get_difficulty_text(menu->niveau));
        }
    }

    // Gestion de la souris pour la difficulté
    Vector2 position_souris = GetMousePosition();

    // Définir les zones de clic pour la difficulté
    Rectangle zone_facile = {LARGEUR_ECRAN/2 - 200, 350, 120, 30};
    Rectangle zone_moyen = {LARGEUR_ECRAN/2 - 60, 350, 120, 30};
    Rectangle zone_difficile = {LARGEUR_ECRAN/2 + 80, 350, 120, 30};

    // Vérifier les clics sur les boutons de difficulté
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(position_souris, zone_facile)) {
            menu->niveau = FACILE;
        } else if (CheckCollisionPointRec(position_souris, zone_moyen)) {
            menu->niveau = MOYEN;
        } else if (CheckCollisionPointRec(position_souris, zone_difficile)) {
            menu->niveau = DIFFICILE;
        }
    }

    // Définir les zones de clic pour le nombre de joueurs
    Rectangle zone_1joueur = {LARGEUR_ECRAN/2 - 150, 280, 120, 40};
    Rectangle zone_2joueurs = {LARGEUR_ECRAN/2 + 30, 280, 120, 40};

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(position_souris, zone_1joueur)) {
            menu->mode = UN_JOUEUR;
        } else if (CheckCollisionPointRec(position_souris, zone_2joueurs)) {
            menu->mode = DEUX_JOUEURS;
        }
    }
}

void draw_menu(const MenuJeu* menu) {
    ClearBackground(COULEUR_FOND);

    // Dessiner le titre
    const char* titre = "MENU JEU";
    int taille_titre = MeasureText(titre, 60);
    DrawText(titre, (LARGEUR_ECRAN - taille_titre) / 2, 50, 60, COULEUR_SURBRILL);

    // Dessiner toutes les sections
    draw_high_scores(menu);
    draw_player_selection(menu);
    draw_difficulty_selection(menu);
    draw_start_button(menu);
}

void draw_high_scores(const MenuJeu* menu) {
    DrawText("MEILLEURS SCORES", LARGEUR_ECRAN/2 - 100, 130, 30, COULEUR_TEXTE);

    if (menu->nombre_scores > 0) {
        for (int i = 0; i < menu->nombre_scores; i++) {
            char texte_score[64];
            sprintf(texte_score, "%d. %s - %d",
                    i + 1,
                    menu->meilleurs_scores[i].playername,
                    menu->meilleurs_scores[i].score);
            DrawText(texte_score, LARGEUR_ECRAN/2 - 80, 170 + i * 20, 18, COULEUR_SCORE);
        }
    } else {
        DrawText("Aucun score disponible", LARGEUR_ECRAN/2 - 80, 170, 18, COULEUR_TEXTE);
    }
}

void draw_player_selection(const MenuJeu* menu) {
    // Indicateur de sélection si cette section est sélectionnée
    if (menu->option_selectionnee == 0) {
        DrawText("▶ JOUEURS", LARGEUR_ECRAN/2 - 80, 250, 25, COULEUR_SURBRILL);
    } else {
        DrawText("JOUEURS", LARGEUR_ECRAN/2 - 50, 250, 25, COULEUR_TEXTE);
    }

    // Bouton 1 joueur
    Rectangle bouton_1j = {LARGEUR_ECRAN/2 - 150, 280, 120, 40};
    Color couleur_1j = (menu->mode == UN_JOUEUR) ? COULEUR_SURBRILL : COULEUR_BOUTON;
    DrawRectangleRec(bouton_1j, couleur_1j);
    DrawRectangleLinesEx(bouton_1j, 3, COULEUR_TEXTE);

    int largeur_texte_1j = MeasureText("1 JOUEUR", 20);
    DrawText("1 JOUEUR",
             bouton_1j.x + (bouton_1j.width - largeur_texte_1j) / 2,
             bouton_1j.y + 10, 20, BLACK);

    // Bouton 2 joueurs
    Rectangle bouton_2j = {LARGEUR_ECRAN/2 + 30, 280, 120, 40};
    Color couleur_2j = (menu->mode == DEUX_JOUEURS) ? COULEUR_SURBRILL : COULEUR_BOUTON;
    DrawRectangleRec(bouton_2j, couleur_2j);
    DrawRectangleLinesEx(bouton_2j, 3, COULEUR_TEXTE);

    int largeur_texte_2j = MeasureText("2 JOUEURS", 20);
    DrawText("2 JOUEURS",
             bouton_2j.x + (bouton_2j.width - largeur_texte_2j) / 2,
             bouton_2j.y + 10, 20, BLACK);

    // Indication de navigation si sélectionné
    if (menu->option_selectionnee == 0) {
        DrawText("Utilisez ← → ou ENTRÉE", LARGEUR_ECRAN/2 - 100, 330, 14, COULEUR_SURBRILL);
    }
}

void draw_difficulty_selection(const MenuJeu* menu) {
    // Indicateur de sélection si cette section est sélectionnée
    if (menu->option_selectionnee == 1) {
        DrawText("▶ DIFFICULTÉ", LARGEUR_ECRAN/2 - 90, 320, 25, COULEUR_SURBRILL);
    } else {
        DrawText("DIFFICULTÉ", LARGEUR_ECRAN/2 - 60, 320, 25, COULEUR_TEXTE);
    }

    // Barre de difficulté
    Rectangle barre_fond = {LARGEUR_ECRAN/2 - 200, 370, 400, 20};
    DrawRectangleRec(barre_fond, COULEUR_ROUGE_FON);
    DrawRectangleLinesEx(barre_fond, 2, COULEUR_TEXTE);

    // Remplissage de la barre selon la difficulté
    float pourcentage = get_difficulty_percentage(menu->niveau);
    Rectangle barre_remplie = {barre_fond.x, barre_fond.y, barre_fond.width * pourcentage, barre_fond.height};
    DrawRectangleRec(barre_remplie, COULEUR_ROUGE);

    // Boutons de difficulté
    const char* difficultes[] = {"FACILE", "MOYEN", "DIFFICILE"};
    Rectangle rectangles_difficulte[] = {
        {LARGEUR_ECRAN/2 - 200, 350, 120, 30},
        {LARGEUR_ECRAN/2 - 60, 350, 120, 30},
        {LARGEUR_ECRAN/2 + 80, 350, 120, 30}
    };

    for (int i = 0; i < 3; i++) {
        Color couleur_btn = (menu->niveau == i) ? COULEUR_SURBRILL : COULEUR_BOUTON;
        DrawRectangleRec(rectangles_difficulte[i], couleur_btn);
        DrawRectangleLinesEx(rectangles_difficulte[i], 2, COULEUR_TEXTE);

        int largeur_texte = MeasureText(difficultes[i], 18);
        DrawText(difficultes[i],
                 rectangles_difficulte[i].x + (rectangles_difficulte[i].width - largeur_texte) / 2,
                 rectangles_difficulte[i].y + 6, 18, BLACK);
    }

    // Indication de navigation si sélectionné
    if (menu->option_selectionnee == 1) {
        DrawText("Utilisez ← → ou ENTRÉE", LARGEUR_ECRAN/2 - 100, 400, 14, COULEUR_SURBRILL);
    }
}

void draw_start_button(const MenuJeu* menu) {
    // Bouton démarrer
    Rectangle bouton_demarrer = {LARGEUR_ECRAN/2 - 100, 450, 200, 60};
    Color couleur_demarrer = (menu->option_selectionnee == 2) ? COULEUR_SURBRILL : COULEUR_SURVOL;

    // Vérifier le survol de la souris
    Vector2 position_souris = GetMousePosition();
    if (CheckCollisionPointRec(position_souris, bouton_demarrer)) {
        couleur_demarrer = COULEUR_SURBRILL;
    }

    DrawRectangleRec(bouton_demarrer, couleur_demarrer);
    DrawRectangleLinesEx(bouton_demarrer, 4, COULEUR_TEXTE);

    int largeur_texte_demarrer = MeasureText("DÉMARRER", 30);
    DrawText("DÉMARRER",
             bouton_demarrer.x + (bouton_demarrer.width - largeur_texte_demarrer) / 2,
             bouton_demarrer.y + 15, 30, BLACK);

    // Indication de navigation si sélectionné
    if (menu->option_selectionnee == 2) {
        DrawText("▶ Appuyez sur ENTRÉE pour démarrer", LARGEUR_ECRAN/2 - 120, 430, 16, COULEUR_SURBRILL);
    }

    // Instructions
    DrawText("↑ ↓ Naviguer  |  ← → Modifier  |  ENTRÉE Sélectionner",
             LARGEUR_ECRAN/2 - 220, 530, 16, COULEUR_TEXTE);
    DrawText("Échap pour quitter",
             LARGEUR_ECRAN/2 - 70, 550, 16, COULEUR_TEXTE);
}

const char* get_difficulty_text(Difficulte niveau) {
    switch (niveau) {
        case FACILE: return "FACILE";
        case MOYEN: return "MOYEN";
        case DIFFICILE: return "DIFFICILE";
        default: return "INCONNU";
    }
}

float get_difficulty_percentage(Difficulte niveau) {
    switch (niveau) {
        case FACILE: return 0.33f;
        case MOYEN: return 0.66f;
        case DIFFICILE: return 1.0f;
        default: return 0.0f;
    }
}

Color get_difficulty_color(Difficulte niveau) {
    switch (niveau) {
        case FACILE: return (Color){ 255, 100, 100, 255 };
        case MOYEN: return (Color){ 255, 50, 50, 255 };
        case DIFFICILE: return COULEUR_ROUGE;
        default: return COULEUR_TEXTE;
    }
}