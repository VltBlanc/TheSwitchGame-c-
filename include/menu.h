#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "scores.h"
#include <stdbool.h>

#define LARGEUR_ECRAN 1280
#define HAUTEUR_ECRAN 720
#define NOMBRE_MEILLEURS_SCORES 5

// Etats du menu
typedef enum {
  MENU_PRINCIPAL,
  MENU_DEMARRAGE,
  MENU_QUITTER,
  MENU_NAME_INPUT // État pour la saisie du nom du joueur
} EtatMenu;

// Modes de jeu
typedef enum { UN_JOUEUR, DEUX_JOUEURS } ModeJoueur;

// Niveaux de difficulté
typedef enum { FACILE, MOYEN, DIFFICILE } Difficulte;

// Sélection du jeu
typedef enum {
  GAME_SURVIVOR,
  GAME_SNAKE,
  GAME_LUCA,
  GAME_VALENTIN
} GameSelection;

#define MAX_PLAYER_NAME 32

// Structure du menu
typedef struct {
  EtatMenu etat;
  ModeJoueur mode;
  Difficulte niveau;
  int option_selectionnee;
  ScoreList *scores;
  ScoreEntry meilleurs_scores[NOMBRE_MEILLEURS_SCORES];
  int nombre_scores;
  bool partie_en_cours; // Indique si une partie est en cours (pour reprendre)
  char playerName[MAX_PLAYER_NAME]; // Nom du joueur pour les scores
  GameSelection jeu_selectionne;    // Le jeu actuellement sélectionné
} MenuJeu;

// Structure pour les boutons
typedef struct {
  Rectangle rect;
  const char *texte;
  bool survole;
  bool selectionne;
} Bouton;

// Prototypes pour les fonctions
MenuJeu *init_menu(void);
void free_menu(MenuJeu *menu);
void update_menu(MenuJeu *menu);
void draw_menu(const MenuJeu *menu);
void draw_high_scores(const MenuJeu *menu);
void draw_player_selection(const MenuJeu *menu);
void draw_difficulty_selection(const MenuJeu *menu);
void draw_start_button(const MenuJeu *menu);
void update_button_hover(Bouton *bouton);
void draw_button(const Bouton *bouton, Color couleur_normale,
                 Color couleur_survol);
const char *get_difficulty_text(Difficulte difficulte);
float get_difficulty_percentage(Difficulte difficulte);
Color get_difficulty_color(Difficulte difficulte);

#endif // MENU_H