#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "scores.h"
#include <string.h> // Nécessaire pour strcpy, strlen, etc.

#define LARGEUR_ECRAN 800
#define HAUTEUR_ECRAN 600
#define NOMBRE_MEILLEURS_SCORES 5

// Etats du menu
typedef enum {
    MENU_PRINCIPAL,
    MENU_NAME_INPUT,   // <--- AJOUTÉ
    MENU_DEMARRAGE,
    MENU_QUITTER
} EtatMenu;

// Modes de jeu
typedef enum {
    UN_JOUEUR,
    DEUX_JOUEURS
} ModeJoueur;

// Niveaux de difficulté
typedef enum {
    FACILE,
    MOYEN,
    DIFFICILE
} Difficulte;

// Structure du menu
typedef struct {
    EtatMenu etat;
    ModeJoueur mode;
    Difficulte niveau;
    int option_selectionnee;
    char playerName[16];      // <--- AJOUTÉ
    bool partie_en_cours;     // <--- AJOUTÉ
    ScoreList* scores;
    ScoreEntry meilleurs_scores[NOMBRE_MEILLEURS_SCORES];
    int nombre_scores;
} MenuJeu;

// Prototypes (on garde les tiens)
MenuJeu* init_menu(void);
void free_menu(MenuJeu* menu);
void update_menu(MenuJeu* menu);
void draw_menu(const MenuJeu* menu);
void draw_high_scores(const MenuJeu* menu);
void draw_player_selection(const MenuJeu* menu);
void draw_difficulty_selection(const MenuJeu* menu);
void draw_start_button(const MenuJeu* menu);
const char* get_difficulty_text(Difficulte difficulte);

#endif