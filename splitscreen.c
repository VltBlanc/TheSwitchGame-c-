#include "include/splitscreen.h"
#include <stdio.h>

// Initialise le contexte de rendu pour l'écran scindé
SplitScreenContext InitSplitScreen(ModeJoueur mode) {
    SplitScreenContext ctx = {0};
    ctx.is_split_screen = (mode == DEUX_JOUEURS);
    ctx.active_viewport = 0;

    if (ctx.is_split_screen) {
        // Deux cibles de rendu de 640x720 pour le mode deux joueurs
        ctx.player1_target = LoadRenderTexture(SPLIT_WIDTH, SPLIT_HEIGHT);
        ctx.player2_target = LoadRenderTexture(SPLIT_WIDTH, SPLIT_HEIGHT);
        printf("[SplitScreen] Mode 2 joueurs initialisé (2x %dx%d)\n",
               SPLIT_WIDTH, SPLIT_HEIGHT);
    } else {
        // Une seule cible de rendu de 1280x720 pour le mode solo
        ctx.player1_target = LoadRenderTexture(FULL_WIDTH, FULL_HEIGHT);
        printf("[SplitScreen] Mode solo initialisé (%dx%d)\n",
               FULL_WIDTH, FULL_HEIGHT);
    }

    return ctx;
}

// Commence le rendu dans la vue (viewport) d'un joueur spécifique
// player_id : 1 pour la vue de gauche, 2 pour la vue de droite
void BeginPlayerViewport(SplitScreenContext *ctx, int player_id) {
    ctx->active_viewport = player_id;

    if (player_id == 1) {
        BeginTextureMode(ctx->player1_target);
    } else if (player_id == 2 && ctx->is_split_screen) {
        BeginTextureMode(ctx->player2_target);
    }
}

// Termine le rendu dans la vue actuelle
void EndPlayerViewport(SplitScreenContext *ctx) {
    EndTextureMode();
    ctx->active_viewport = 0;
}

// Assemble les deux vues sur l'écran final avec une ligne de séparation
void DrawSplitScreen(SplitScreenContext *ctx) {
    BeginDrawing();
    ClearBackground(BLACK);

    if (ctx->is_split_screen) {
        // Vue du Joueur 1 sur la moitié gauche
        // Note : la hauteur est négative dans 'src' car les textures Raylib sont inversées verticalement
        Rectangle src1 = {0, 0, (float)SPLIT_WIDTH, -(float)SPLIT_HEIGHT};
        Rectangle dst1 = {0, 0, (float)SPLIT_WIDTH, (float)SPLIT_HEIGHT};
        DrawTexturePro(ctx->player1_target.texture, src1, dst1,
                       (Vector2){0, 0}, 0.0f, WHITE);

        // Vue du Joueur 2 sur la moitié droite
        Rectangle src2 = {0, 0, (float)SPLIT_WIDTH, -(float)SPLIT_HEIGHT};
        Rectangle dst2 = {(float)SPLIT_WIDTH, 0, (float)SPLIT_WIDTH, (float)SPLIT_HEIGHT};
        DrawTexturePro(ctx->player2_target.texture, src2, dst2,
                       (Vector2){0, 0}, 0.0f, WHITE);

        // Ligne de séparation au milieu
        DrawRectangle(SPLIT_WIDTH - 2, 0, 4, SPLIT_HEIGHT, WHITE);
    } else {
        // Mode solo - plein écran
        Rectangle src = {0, 0, (float)FULL_WIDTH, -(float)FULL_HEIGHT};
        Rectangle dst = {0, 0, (float)FULL_WIDTH, (float)FULL_HEIGHT};
        DrawTexturePro(ctx->player1_target.texture, src, dst,
                       (Vector2){0, 0}, 0.0f, WHITE);
    }

    EndDrawing();
}

// Affiche les étiquettes (P1, P2) sur chaque vue
void DrawPlayerLabels(SplitScreenContext *ctx) {
    if (!ctx->is_split_screen) return;

    // Dessine "P1" sur la vue de gauche
    BeginPlayerViewport(ctx, 1);
    DrawText("P1", 10, 10, 30, YELLOW);
    EndPlayerViewport(ctx);

    // Dessine "P2" sur la vue de droite
    BeginPlayerViewport(ctx, 2);
    DrawText("P2", 10, 10, 30, YELLOW);
    EndPlayerViewport(ctx);
}

// Affiche l'écran de victoire/défaite par-dessus les vues
// winner : 1 = P1 gagne, 2 = P2 gagne
void DrawWinOverlay(SplitScreenContext *ctx, int winner) {
    if (!ctx->is_split_screen || winner == 0) return;

    const char *p1_text = (winner == 1) ? "GAGNANT !" : "PERDU";
    const char *p2_text = (winner == 2) ? "GAGNANT !" : "PERDU";
    Color p1_color = (winner == 1) ? GREEN : RED;
    Color p2_color = (winner == 2) ? GREEN : RED;

    // Rendu du message sur la vue du Joueur 1
    BeginPlayerViewport(ctx, 1);
    DrawRectangle(0, 0, SPLIT_WIDTH, SPLIT_HEIGHT, Fade(BLACK, 0.7f));
    int text_width1 = MeasureText(p1_text, 60);
    DrawText(p1_text, (SPLIT_WIDTH - text_width1) / 2, SPLIT_HEIGHT / 2 - 30, 60, p1_color);
    DrawText("Appuyez sur ECHAP", (SPLIT_WIDTH - MeasureText("Appuyez sur ECHAP", 20)) / 2,
             SPLIT_HEIGHT / 2 + 50, 20, WHITE);
    EndPlayerViewport(ctx);

    // Rendu du message sur la vue du Joueur 2
    BeginPlayerViewport(ctx, 2);
    DrawRectangle(0, 0, SPLIT_WIDTH, SPLIT_HEIGHT, Fade(BLACK, 0.7f));
    int text_width2 = MeasureText(p2_text, 60);
    DrawText(p2_text, (SPLIT_WIDTH - text_width2) / 2, SPLIT_HEIGHT / 2 - 30, 60, p2_color);
    DrawText("Appuyez sur ECHAP", (SPLIT_WIDTH - MeasureText("Appuyez sur ECHAP", 20)) / 2,
             SPLIT_HEIGHT / 2 + 50, 20, WHITE);
    EndPlayerViewport(ctx);
}

// Libère la mémoire des textures de rendu
void UnloadSplitScreen(SplitScreenContext *ctx) {
    UnloadRenderTexture(ctx->player1_target);
    if (ctx->is_split_screen) {
        UnloadRenderTexture(ctx->player2_target);
    }
    printf("[SplitScreen] Textures de rendu déchargées\n");
}

// Utilitaire pour obtenir la taille d'une vue selon le mode
Vector2 GetViewportSize(ModeJoueur mode) {
    if (mode == DEUX_JOUEURS) {
        return (Vector2){SPLIT_WIDTH, SPLIT_HEIGHT};
    }
    return (Vector2){FULL_WIDTH, FULL_HEIGHT};
}

// Utilitaire pour obtenir le décalage (offset) de la caméra
Vector2 GetCameraOffset(ModeJoueur mode) {
    if (mode == DEUX_JOUEURS) {
        // Centre d'une vue de 640x720
        return (Vector2){SPLIT_WIDTH / 2.0f, SPLIT_HEIGHT / 2.0f};
    }
    // Centre d'une vue de 1280x720
    return (Vector2){FULL_WIDTH / 2.0f, FULL_HEIGHT / 2.0f};
}