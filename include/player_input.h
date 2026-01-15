#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <raylib.h>
#include <stdbool.h>

// Player control configuration
typedef struct {
    int key_up;
    int key_down;
    int key_left;
    int key_right;
    int key_action;  // SPACE for P1, CTRL for P2
} PlayerControls;

// Predefined control schemes
// Player 1: ZQSD + SPACE (French AZERTY layout - use QWERTY physical positions)
static const PlayerControls P1_CONTROLS = {
    .key_up = KEY_W,      // Z on AZERTY keyboard
    .key_down = KEY_S,
    .key_left = KEY_A,    // Q on AZERTY keyboard
    .key_right = KEY_D,
    .key_action = KEY_SPACE
};

// Player 2: Arrow keys + RIGHT CTRL
static const PlayerControls P2_CONTROLS = {
    .key_up = KEY_UP,
    .key_down = KEY_DOWN,
    .key_left = KEY_LEFT,
    .key_right = KEY_RIGHT,
    .key_action = KEY_RIGHT_CONTROL
};

// Helper macros for cleaner code (AZERTY: Z=KEY_W, Q=KEY_A)
#define IS_P1_UP()     (IsKeyDown(KEY_W))
#define IS_P1_DOWN()   (IsKeyDown(KEY_S))
#define IS_P1_LEFT()   (IsKeyDown(KEY_A))
#define IS_P1_RIGHT()  (IsKeyDown(KEY_D))
#define IS_P1_ACTION() (IsKeyPressed(KEY_SPACE))

#define IS_P2_UP()     (IsKeyDown(KEY_UP))
#define IS_P2_DOWN()   (IsKeyDown(KEY_DOWN))
#define IS_P2_LEFT()   (IsKeyDown(KEY_LEFT))
#define IS_P2_RIGHT()  (IsKeyDown(KEY_RIGHT))
#define IS_P2_ACTION() (IsKeyPressed(KEY_RIGHT_CONTROL))

// For pressed (single-shot) inputs (AZERTY: Z=KEY_W, Q=KEY_A)
#define IS_P1_UP_PRESSED()     (IsKeyPressed(KEY_W))
#define IS_P1_DOWN_PRESSED()   (IsKeyPressed(KEY_S))
#define IS_P1_LEFT_PRESSED()   (IsKeyPressed(KEY_A))
#define IS_P1_RIGHT_PRESSED()  (IsKeyPressed(KEY_D))

#define IS_P2_UP_PRESSED()     (IsKeyPressed(KEY_UP))
#define IS_P2_DOWN_PRESSED()   (IsKeyPressed(KEY_DOWN))
#define IS_P2_LEFT_PRESSED()   (IsKeyPressed(KEY_LEFT))
#define IS_P2_RIGHT_PRESSED()  (IsKeyPressed(KEY_RIGHT))

#endif // PLAYER_INPUT_H
