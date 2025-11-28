#include <raylib.h>
#include <stdio.h>
#include <stdlib.h> 
#include <raymath.h> 

// --- DÉFINITIONS DU JEU ET VALEURS CLÉS À AJUSTER ---
// MODIFICATION : VOUS POUVEZ CHANGER CES 3 VALEURS
const float FALL_WARNING_TIME = 0.4f; // Temps en secondes pour l'avertissement ROUGE
const float FALL_DOWN_TIME = 0.2f;    // Temps en secondes où le bloc est GRIS (Zone de Mort)
const float WAIT_TIME = 0.1f;         // Temps en secondes d'attente (Bloc VERT)
// -----------------------------------------------------

// La durée totale du cycle est la somme des 3 temps ci-dessus
const float CYCLE_DURATION = FALL_WARNING_TIME + FALL_DOWN_TIME + WAIT_TIME; 

#define GRID_SIZE 8             
#define CELL_SIZE 60            
#define PLAYER_RADIUS 15        
#define BASE_PLAYER_SPEED 200.0f
#define TELEPORT_DISTANCE 2     

// ... Le reste des constantes de taille de fenêtre, etc., reste inchangé ...

const int screenWidth = (GRID_SIZE + 2) * CELL_SIZE; 
const int screenHeight = (GRID_SIZE + 2) * CELL_SIZE + 40; 
const int GRID_START_X = CELL_SIZE; 
const int GRID_START_Y = CELL_SIZE;

// États possibles pour chaque case de la grille
typedef enum {
    BLOCK_UP = 0,       
    BLOCK_WARNING = 1,  
    BLOCK_DOWN = 2,     
    BLOCK_PENDING = 3   
} BlockState;

// Structure pour le joueur
typedef struct Player {
    Vector2 position;
    bool isAlive;
    int score;
    Vector2 lastMovementDirection; 
} Player;

// Variables globales du jeu
BlockState grid[GRID_SIZE][GRID_SIZE];
float timer = 0.0f; 
int blocksToDrop = 8; 
bool scoreIncreasedThisCycle = false;

// --- Prototypes de Fonctions ---
void InitGrid();
Player InitPlayer();
void UpdatePlayerMovement(Player *player, float deltaTime);
void TeleportPlayer(Player *player);
void GetPlayerGridCoords(Vector2 playerPos, int *gridX, int *gridY);
void CheckDeath(Player *player);
void UpdateGridCycle(Player *player, float deltaTime);
void DrawCustomGrid();
void DrawPlayer(Player player);


// --- Initialisation et Mise à Jour ---

// Fonctions InitGrid, InitPlayer, TeleportPlayer, UpdatePlayerMovement, GetPlayerGridCoords, CheckDeath...
// ... (Ces fonctions restent inchangées, je les omettrai pour la concision) ...

void InitGrid() {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            grid[x][y] = BLOCK_UP;
        }
    }
}

Player InitPlayer() {
    Player player;
    player.position.x = GRID_START_X + (GRID_SIZE * CELL_SIZE) / 2;
    player.position.y = GRID_START_Y + (GRID_SIZE * CELL_SIZE) / 2;
    player.isAlive = true;
    player.score = 0;
    player.lastMovementDirection = (Vector2){0.0f, -1.0f}; 
    return player;
}

void TeleportPlayer(Player *player) {
    if (!player->isAlive) return;

    float teleportAmountX = player->lastMovementDirection.x * TELEPORT_DISTANCE * CELL_SIZE;
    float teleportAmountY = player->lastMovementDirection.y * TELEPORT_DISTANCE * CELL_SIZE;

    player->position.x += teleportAmountX;
    player->position.y += teleportAmountY;

    float minX = PLAYER_RADIUS;
    float maxX = screenWidth - PLAYER_RADIUS;
    float minY = PLAYER_RADIUS;
    float maxY = screenHeight - 40 - PLAYER_RADIUS; 

    if (player->position.x < minX) player->position.x = minX;
    if (player->position.x > maxX) player->position.x = maxX;
    if (player->position.y < minY) player->position.y = minY;
    if (player->position.y > maxY) player->position.y = maxY;
}


void UpdatePlayerMovement(Player *player, float deltaTime) {
    if (!player->isAlive) return;

    Vector2 movement = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) movement.x += 1.0f;
    if (IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))  movement.x -= 1.0f;
    if (IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))  movement.y += 1.0f;
    if (IsKeyDown(KEY_UP)||IsKeyDown(KEY_W))    movement.y -= 1.0f;

    if (movement.x != 0.0f || movement.y != 0.0f) {
        player->lastMovementDirection = Vector2Normalize(movement); 
        movement = player->lastMovementDirection; 
    }
    
    float currentSpeed = BASE_PLAYER_SPEED;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        currentSpeed *= 2.0f; 
    }

    player->position.x += movement.x * currentSpeed * deltaTime;
    player->position.y += movement.y * currentSpeed * deltaTime;

    float minX = PLAYER_RADIUS;
    float maxX = screenWidth - PLAYER_RADIUS;
    float minY = PLAYER_RADIUS;
    float maxY = screenHeight - 40 - PLAYER_RADIUS; 

    if (player->position.x < minX) player->position.x = minX;
    if (player->position.x > maxX) player->position.x = maxX;
    if (player->position.y < minY) player->position.y = minY;
    if (player->position.y > maxY) player->position.y = maxY;
    
    if (IsKeyPressed(KEY_SPACE)) {
        TeleportPlayer(player);
    }
}

void GetPlayerGridCoords(Vector2 playerPos, int *gridX, int *gridY) {
    int relativeX = (int)playerPos.x - GRID_START_X;
    int relativeY = (int)playerPos.y - GRID_START_Y;

    *gridX = relativeX / CELL_SIZE;
    *gridY = relativeY / CELL_SIZE;
}

void CheckDeath(Player *player) {
    if (!player->isAlive) return;

    bool outsideGridX = player->position.x < GRID_START_X || player->position.x > (GRID_START_X + GRID_SIZE * CELL_SIZE);
    bool outsideGridY = player->position.y < GRID_START_Y || player->position.y > (GRID_START_Y + GRID_SIZE * CELL_SIZE);

    if (outsideGridX || outsideGridY) {
        player->isAlive = false;
        return;
    }

    int pGridX, pGridY;
    GetPlayerGridCoords(player->position, &pGridX, &pGridY);

    if (pGridX >= 0 && pGridX < GRID_SIZE && pGridY >= 0 && pGridY < GRID_SIZE) {
        if (grid[pGridX][pGridY] == BLOCK_DOWN) {
            player->isAlive = false;
        }
    }
}


// MODIFICATION CRUCIALE DANS CETTE FONCTION
void UpdateGridCycle(Player *player, float deltaTime) {
    if (!player->isAlive) return;

    // Calcul des points de déclenchement basés sur les constantes ajustables
    const float TIME_TO_WARNING_START = WAIT_TIME; 
    const float TIME_TO_DROP_START = WAIT_TIME + FALL_WARNING_TIME; 
    const float TIME_TO_RESET_START = CYCLE_DURATION; 

    // Détection des événements par croisement de seuil
    bool shouldTriggerWarning = (timer < TIME_TO_WARNING_START && timer + deltaTime >= TIME_TO_WARNING_START);
    bool shouldTriggerDrop = (timer < TIME_TO_DROP_START && timer + deltaTime >= TIME_TO_DROP_START);
    bool shouldTriggerReset = (timer < TIME_TO_RESET_START && timer + deltaTime >= TIME_TO_RESET_START);

    timer += deltaTime;

    // --- ÉVÉNEMENT 1 : AVERTISSEMENT (Début de la phase ROUGE) ---
    if (shouldTriggerWarning) {
        if (!scoreIncreasedThisCycle) {
            player->score++;
            blocksToDrop = 8 + (player->score / 3); 
            scoreIncreasedThisCycle = true;
        }

        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                // Les blocs qui étaient DOWN remontent et deviennent UP
                if (grid[x][y] == BLOCK_DOWN) {
                    grid[x][y] = BLOCK_UP;
                }
                // Les blocs PENDING (choisis) passent en WARNING (ROUGE)
                if (grid[x][y] == BLOCK_PENDING) {
                    grid[x][y] = BLOCK_WARNING;
                }
            }
        }
    }

    // --- ÉVÉNEMENT 2 : CHUTE (Début de la phase GRISE/Mort) ---
    if (shouldTriggerDrop) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                // Les blocs WARNING passent en DOWN (GRIS/MORT).
                if (grid[x][y] == BLOCK_WARNING) {
                    grid[x][y] = BLOCK_DOWN;
                }
            }
        }
    }

    // --- ÉVÉNEMENT 3 : NOUVEAU CYCLE (TIMER atteint la durée totale) ---
    if (shouldTriggerReset) {
        timer = 0.0f;
        scoreIncreasedThisCycle = false;

        // Réinitialisation des blocs non tombés au vert
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                 if (grid[x][y] == BLOCK_PENDING || grid[x][y] == BLOCK_WARNING) {
                    grid[x][y] = BLOCK_UP;
                 }
            }
        }

        // Sélection aléatoire des blocs pour le prochain cycle (PENDING)
        int drops = 0;
        while (drops < blocksToDrop) {
            int randX = GetRandomValue(0, GRID_SIZE - 1);
            int randY = GetRandomValue(0, GRID_SIZE - 1);

            if (grid[randX][randY] == BLOCK_UP) {
                grid[randX][randY] = BLOCK_PENDING;
                drops++;
            }
        }
    }
    
    // Si le timer dépasse la durée totale (sécurité)
    if (timer >= CYCLE_DURATION) {
        timer = 0.0f;
    }
}


// --- Fonctions de Dessin ---

// ... (DrawCustomGrid et DrawPlayer restent inchangées) ...

void DrawCustomGrid() {
    DrawRectangle(0, 0, screenWidth, screenHeight - 40, DARKGRAY);

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            
            Rectangle rect = {
                GRID_START_X + x * CELL_SIZE, 
                GRID_START_Y + y * CELL_SIZE, 
                CELL_SIZE, 
                CELL_SIZE
            };
            
            Color blockColor;
            switch (grid[x][y]) {
                case BLOCK_UP:
                case BLOCK_PENDING: 
                    blockColor = GREEN;
                    break;
                case BLOCK_WARNING:
                    blockColor = RED; 
                    break;
                case BLOCK_DOWN:
                    blockColor = DARKGRAY; 
                    break;
                default:
                    blockColor = WHITE;
            }
            
            DrawRectangleRec(rect, blockColor);
            
            DrawRectangleLines(rect.x, rect.y, CELL_SIZE, CELL_SIZE, GRAY);
        }
    }
}

void DrawPlayer(Player player) {
    if (player.isAlive) {
        DrawCircleV(player.position, PLAYER_RADIUS, SKYBLUE);
    }
}


// --- Boucle Principale ---

int main(void) {
    InitWindow(screenWidth, screenHeight, "Raylib - Blocs Dynamiques (Paramètres de Temps Flexibles)");
    SetTargetFPS(60); 

    InitGrid();
    Player player = InitPlayer(); 

    // Calcul des points de déclenchement pour l'affichage uniquement
    const float TIME_TO_WARNING_START = WAIT_TIME; 
    const float TIME_TO_DROP_START = WAIT_TIME + FALL_WARNING_TIME; 

    while (!WindowShouldClose()) {
        
        float deltaTime = GetFrameTime();
        
        if (player.isAlive) {
            UpdatePlayerMovement(&player, deltaTime);
            UpdateGridCycle(&player, deltaTime);
            CheckDeath(&player);
        } else {
            if (IsKeyPressed(KEY_SPACE)) {
                InitGrid();
                player = InitPlayer();
                blocksToDrop = 8; 
                timer = 0.0f;
                scoreIncreasedThisCycle = false;
            }
        }
        
        BeginDrawing();
            
            ClearBackground(RAYWHITE); 
            DrawCustomGrid();

            DrawPlayer(player);

            // Affichage du score et du statut
            DrawText(TextFormat("Score: %d", player.score), 10, screenHeight - 30, 20, BLACK);
            
            // Affichage précis des phases
            if (timer < TIME_TO_WARNING_START) {
                DrawText(TextFormat("Attente (%.2f s)", TIME_TO_WARNING_START - timer), 200, screenHeight - 30, 20, BLACK);
            } else if (timer < TIME_TO_DROP_START) {
                DrawText(TextFormat("Avertissement ROUGE! (%.2f s)", TIME_TO_DROP_START - timer), 200, screenHeight - 30, 20, RED);
            } else if (timer < CYCLE_DURATION) {
                DrawText(TextFormat("CHUTE! (%.2f s)", CYCLE_DURATION - timer), 200, screenHeight - 30, 20, RAYWHITE);
            } else {
                DrawText(TextFormat("NOUVEAU CYCLE!"), 200, screenHeight - 30, 20, BLACK);
            }
            
            if (!player.isAlive) {
                DrawText("GAME OVER!", screenWidth/2 - MeasureText("GAME OVER!", 40)/2, screenHeight/2 - 20, 40, RED);
                DrawText("Appuyez sur ESPACE pour recommencer", screenWidth/2 - MeasureText("Appuyez sur ESPACE pour recommencer", 20)/2, screenHeight/2 + 30, 20, RAYWHITE);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}