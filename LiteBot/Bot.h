#pragma once

#include <SFML/System.h>
#include <SFML/Graphics.h>
#include "basics.h"
#include "grid.h"

#define MOVE_QUEUE_SIZE 256   // valeur safe, modifiable



struct Map;

/// <summary>
/// Structure to represent a movement command for the Bot.
/// </summary>
/// <remarks>
/// This structure contains the type of movement and the direction.
/// </remarks>
struct Move
{
    enum MovementType  type;
    enum Direction direction;
};



/// <summary>
/// Structure to represent a Bot in the game.
/// </summary>
/// <remarks>
/// This structure contains the bot's position, sprite, and movement queue.
/// </remarks>
struct Bot
{
    sfVector2i position;
    sfSprite *sprite;
    struct Move MoveQueue[MOVE_QUEUE_SIZE];
};

/// <summary>
/// Structure to hold game data for AI movement.
/// </summary>
/// <remarks>
/// This structure contains pointers to the bot and grid,
/// as well as variables to track the current step and path result.
/// </remarks>
struct GameData
{
    struct Bot* bot;
    Grid* grid;
    int step;
    enum MoveResult pathResult ;
    sfClock* chrono_clock;  
    sfTime chrono_time;
    
    int chrono_seconds;
    int chrono_minutes;
    int chrono_hours;
    bool chrono_running;
    sfFont* font;       
    sfText* chrono_text; 
    bool** visited;
};
 


// ----------------------
// Fonctions du bot
// ----------------------
struct Bot* CreateBot();
void SpawnBotAtStartCell(struct Bot* bot, Grid* grid);
void DestroyBot(struct Bot* bot);
void DrawBot(sfRenderWindow* window, struct Bot* bot);

int MoveBot(struct Bot* bot, Grid* grid, enum MovementType type, enum Direction direction);
void AddMovement(struct Bot* bot, enum MovementType type, enum Direction direction);

// NOTE IMPORTANTE : CSFML impose void(*)(void*)
void MoveBot_AI(void* data);

bool SearchPath_AI(struct Bot* bot, Grid* grid);
