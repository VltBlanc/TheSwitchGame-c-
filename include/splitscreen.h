#ifndef SPLITSCREEN_H
#define SPLITSCREEN_H

#include <raylib.h>
#include <stdbool.h>
#include "menu.h"

// Dimensions du split-screen (séparation verticale)
#define SPLIT_WIDTH 640
#define SPLIT_HEIGHT 720
#define FULL_WIDTH 1280
#define FULL_HEIGHT 720

// Contexte de rendu pour le split-screen
typedef struct {
    RenderTexture2D player1_target;
    RenderTexture2D player2_target;
    bool is_split_screen;
    int active_viewport;  // Zone actuellement utilisée pour le rendu : 1 ou 2
} SplitScreenContext;

// Initialise le contexte de rendu du split-screen
// Pour DEUX_JOUEURS : crée deux cibles de rendu de 640x720
// Pour UN_JOUEUR   : crée une seule cible de rendu de 1280x720
SplitScreenContext InitSplitScreen(ModeJoueur mode);

// Démarre le rendu dans la zone du joueur
// player_id : 1 pour la zone de gauche, 2 pour la zone de droite
void BeginPlayerViewport(SplitScreenContext *ctx, int player_id);

// Termine le rendu dans la zone actuelle
void EndPlayerViewport(SplitScreenContext *ctx);

// Affiche les deux zones à l’écran avec une ligne de séparation
// À appeler après avoir rendu les deux zones
void DrawSplitScreen(SplitScreenContext *ctx);

// Affiche les étiquettes des joueurs (P1, P2) dans leurs zones respectives
void DrawPlayerLabels(SplitScreenContext *ctx);

// Affiche une surcouche indiquant le gagnant et le perdant
// winner : 1 = P1 gagne, 2 = P2 gagne
void DrawWinOverlay(SplitScreenContext *ctx, int winner);

// Libère les textures de rendu
void UnloadSplitScreen(SplitScreenContext *ctx);

// Renvoie les dimensions de la zone d’affichage selon le mode de jeu
Vector2 GetViewportSize(ModeJoueur mode);

// Renvoie le décalage de la caméra pour le split-screen
Vector2 GetCameraOffset(ModeJoueur mode);

#endif // SPLITSCREEN_H
