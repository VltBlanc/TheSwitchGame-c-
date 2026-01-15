#ifndef SPLITSCREEN_H
#define SPLITSCREEN_H

#include <raylib.h>
#include <stdbool.h>
#include "menu.h"

// Split-screen dimensions (vertical split)
#define SPLIT_WIDTH 640
#define SPLIT_HEIGHT 720
#define FULL_WIDTH 1280
#define FULL_HEIGHT 720

// Split-screen rendering context
typedef struct {
    RenderTexture2D player1_target;
    RenderTexture2D player2_target;
    bool is_split_screen;
    int active_viewport;  // Currently rendering to: 1 or 2
} SplitScreenContext;

// Initialize split-screen rendering context
// For DEUX_JOUEURS: creates two 640x720 render targets
// For UN_JOUEUR: creates one 1280x720 render target
SplitScreenContext InitSplitScreen(ModeJoueur mode);

// Begin rendering to a player's viewport
// player_id: 1 for left viewport, 2 for right viewport
void BeginPlayerViewport(SplitScreenContext *ctx, int player_id);

// End rendering to the current viewport
void EndPlayerViewport(SplitScreenContext *ctx);

// Composite both viewports to screen with separator line
// Call this after rendering both viewports
void DrawSplitScreen(SplitScreenContext *ctx);

// Draw player labels (P1, P2) on viewports
void DrawPlayerLabels(SplitScreenContext *ctx);

// Draw winner/loser overlay on viewports
// winner: 1 = P1 wins, 2 = P2 wins
void DrawWinOverlay(SplitScreenContext *ctx, int winner);

// Cleanup render textures
void UnloadSplitScreen(SplitScreenContext *ctx);

// Helper to get viewport dimensions based on mode
Vector2 GetViewportSize(ModeJoueur mode);

// Helper to get camera offset for split-screen
Vector2 GetCameraOffset(ModeJoueur mode);

#endif // SPLITSCREEN_H
