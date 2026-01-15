#include "include/splitscreen.h"
#include <stdio.h>

SplitScreenContext InitSplitScreen(ModeJoueur mode) {
    SplitScreenContext ctx = {0};
    ctx.is_split_screen = (mode == DEUX_JOUEURS);
    ctx.active_viewport = 0;

    if (ctx.is_split_screen) {
        // Two 640x720 render targets for split-screen
        ctx.player1_target = LoadRenderTexture(SPLIT_WIDTH, SPLIT_HEIGHT);
        ctx.player2_target = LoadRenderTexture(SPLIT_WIDTH, SPLIT_HEIGHT);
        printf("[SplitScreen] Initialized 2-player mode (2x %dx%d)\n",
               SPLIT_WIDTH, SPLIT_HEIGHT);
    } else {
        // Single 1280x720 render target for single player
        ctx.player1_target = LoadRenderTexture(FULL_WIDTH, FULL_HEIGHT);
        printf("[SplitScreen] Initialized 1-player mode (%dx%d)\n",
               FULL_WIDTH, FULL_HEIGHT);
    }

    return ctx;
}

void BeginPlayerViewport(SplitScreenContext *ctx, int player_id) {
    ctx->active_viewport = player_id;

    if (player_id == 1) {
        BeginTextureMode(ctx->player1_target);
    } else if (player_id == 2 && ctx->is_split_screen) {
        BeginTextureMode(ctx->player2_target);
    }
}

void EndPlayerViewport(SplitScreenContext *ctx) {
    EndTextureMode();
    ctx->active_viewport = 0;
}

void DrawSplitScreen(SplitScreenContext *ctx) {
    BeginDrawing();
    ClearBackground(BLACK);

    if (ctx->is_split_screen) {
        // Player 1 viewport on left half
        Rectangle src1 = {0, 0, (float)SPLIT_WIDTH, -(float)SPLIT_HEIGHT};
        Rectangle dst1 = {0, 0, (float)SPLIT_WIDTH, (float)SPLIT_HEIGHT};
        DrawTexturePro(ctx->player1_target.texture, src1, dst1,
                       (Vector2){0, 0}, 0.0f, WHITE);

        // Player 2 viewport on right half
        Rectangle src2 = {0, 0, (float)SPLIT_WIDTH, -(float)SPLIT_HEIGHT};
        Rectangle dst2 = {(float)SPLIT_WIDTH, 0, (float)SPLIT_WIDTH, (float)SPLIT_HEIGHT};
        DrawTexturePro(ctx->player2_target.texture, src2, dst2,
                       (Vector2){0, 0}, 0.0f, WHITE);

        // Separator line in the middle
        DrawRectangle(SPLIT_WIDTH - 2, 0, 4, SPLIT_HEIGHT, WHITE);
    } else {
        // Single player - full screen
        Rectangle src = {0, 0, (float)FULL_WIDTH, -(float)FULL_HEIGHT};
        Rectangle dst = {0, 0, (float)FULL_WIDTH, (float)FULL_HEIGHT};
        DrawTexturePro(ctx->player1_target.texture, src, dst,
                       (Vector2){0, 0}, 0.0f, WHITE);
    }

    EndDrawing();
}

void DrawPlayerLabels(SplitScreenContext *ctx) {
    if (!ctx->is_split_screen) return;

    // Draw P1 label on left viewport
    BeginPlayerViewport(ctx, 1);
    DrawText("P1", 10, 10, 30, YELLOW);
    EndPlayerViewport(ctx);

    // Draw P2 label on right viewport
    BeginPlayerViewport(ctx, 2);
    DrawText("P2", 10, 10, 30, YELLOW);
    EndPlayerViewport(ctx);
}

void DrawWinOverlay(SplitScreenContext *ctx, int winner) {
    if (!ctx->is_split_screen || winner == 0) return;

    const char *p1_text = (winner == 1) ? "WINNER!" : "LOSER";
    const char *p2_text = (winner == 2) ? "WINNER!" : "LOSER";
    Color p1_color = (winner == 1) ? GREEN : RED;
    Color p2_color = (winner == 2) ? GREEN : RED;

    // Draw overlay on P1 viewport
    BeginPlayerViewport(ctx, 1);
    DrawRectangle(0, 0, SPLIT_WIDTH, SPLIT_HEIGHT, Fade(BLACK, 0.7f));
    int text_width1 = MeasureText(p1_text, 60);
    DrawText(p1_text, (SPLIT_WIDTH - text_width1) / 2, SPLIT_HEIGHT / 2 - 30, 60, p1_color);
    DrawText("Press ESCAPE", (SPLIT_WIDTH - MeasureText("Press ESCAPE", 20)) / 2,
             SPLIT_HEIGHT / 2 + 50, 20, WHITE);
    EndPlayerViewport(ctx);

    // Draw overlay on P2 viewport
    BeginPlayerViewport(ctx, 2);
    DrawRectangle(0, 0, SPLIT_WIDTH, SPLIT_HEIGHT, Fade(BLACK, 0.7f));
    int text_width2 = MeasureText(p2_text, 60);
    DrawText(p2_text, (SPLIT_WIDTH - text_width2) / 2, SPLIT_HEIGHT / 2 - 30, 60, p2_color);
    DrawText("Press ESCAPE", (SPLIT_WIDTH - MeasureText("Press ESCAPE", 20)) / 2,
             SPLIT_HEIGHT / 2 + 50, 20, WHITE);
    EndPlayerViewport(ctx);
}

void UnloadSplitScreen(SplitScreenContext *ctx) {
    UnloadRenderTexture(ctx->player1_target);
    if (ctx->is_split_screen) {
        UnloadRenderTexture(ctx->player2_target);
    }
    printf("[SplitScreen] Unloaded render targets\n");
}

Vector2 GetViewportSize(ModeJoueur mode) {
    if (mode == DEUX_JOUEURS) {
        return (Vector2){SPLIT_WIDTH, SPLIT_HEIGHT};
    }
    return (Vector2){FULL_WIDTH, FULL_HEIGHT};
}

Vector2 GetCameraOffset(ModeJoueur mode) {
    if (mode == DEUX_JOUEURS) {
        // Center of 640x720 viewport
        return (Vector2){SPLIT_WIDTH / 2.0f, SPLIT_HEIGHT / 2.0f};
    }
    // Center of 1280x720 viewport
    return (Vector2){FULL_WIDTH / 2.0f, FULL_HEIGHT / 2.0f};
}
