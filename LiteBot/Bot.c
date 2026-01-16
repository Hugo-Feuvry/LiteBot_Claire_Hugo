#include "Bot.h"

#include "map.h"

bool IsWalkable(enum CellType type)
{
    return (type == WALKABLE || type == EMPTY || type == START || type == END);
}


struct Bot* CreateBot()
{
    struct Bot* bot = (struct Bot*)malloc(sizeof(struct Bot));
    if (!bot) return NULL;

    bot->position = (sfVector2i){ 0, 0 };

    bot->sprite = sfSprite_create();
    sfTexture* tex = sfTexture_createFromFile("./Assets/Characters/Bot01.png", NULL);
    sfSprite_setTexture(bot->sprite, tex, sfTrue);
    sfSprite_setPosition(bot->sprite, (sfVector2f) { 0, 0 });
    float scale = ((float)CELL_SIZE / 24.f) * 0.75f;
    sfSprite_setScale(bot->sprite, (sfVector2f) { scale, scale });

    // ⬇⬇⬇ IMPORTANT : initialiser la MoveQueue
    for (int i = 0; i < MOVE_QUEUE_SIZE; ++i) {
        bot->MoveQueue[i].type = INVALID;
        bot->MoveQueue[i].direction = NONE;
    }

    return bot;
}



void SpawnBotAtStartCell(struct Bot* bot, Grid* grid)
{
    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            if (grid->cell[i][j]->type == START)
            {
                bot->position = grid->cell[i][j]->coord;
                sfVector2f startCelPosition = sfSprite_getPosition(grid->cell[i][j]->sprite);
                startCelPosition.x += 5.f;
                startCelPosition.y += 5.f;
                sfSprite_setPosition(bot->sprite, startCelPosition);
                break;
            }
        }
    }
    
}

void DestroyBot(struct Bot* bot)
{
    if (!bot->sprite) return;
    sfSprite_destroy(bot->sprite);
    free(bot);
}

void DrawBot(sfRenderWindow* window, struct Bot* bot)
{
    if (!window || !bot || !bot->sprite) return;
    sfRenderWindow_drawSprite(window, bot->sprite, NULL);
}

int MoveBot(struct Bot* bot, Grid* grid, enum MovementType type, enum Direction direction)
{
    int distance = 1;
    if (type == JUMP) distance = 2;
    
    sfVector2i newPosition = bot->position;
    
    switch (direction)
    {
    case NORTH:
        if (newPosition.y > 0)
        {
            newPosition.y -= distance;
        }
        break;
    case EAST:
        if (newPosition.x < (GRID_COLS - 1))
        {
            newPosition.x += distance;
        }
        break;
    case SOUTH:
        if (newPosition.y < (GRID_ROWS - 1))
        {
            newPosition.y += distance;
        }
        break;
    case WEST:
        if (newPosition.x > 0)
        {
            newPosition.x -= distance;
        }
        break;
    default:
        break;
    }

    enum CellType destinationCellType = grid->cell[newPosition.y][newPosition.x]->type;

    if (destinationCellType != OBSTACLE)
    {
        bot->position = newPosition;
        sfVector2f newSpritePosition = sfSprite_getPosition(grid->cell[bot->position.y][bot->position.x]->sprite);
        newSpritePosition.x += 5.f;
        newSpritePosition.y += 5.f;
        sfSprite_setPosition(bot->sprite, newSpritePosition);
    } else
    {
        printf("can't go there ! \n");
    }

    switch (destinationCellType)
    {
    case END:
        return REACH_END;
    case EMPTY:
        return FAILURE;
    case START:
    case WALKABLE:
    case OBSTACLE:
    default:
        return NOTHING;
    }
}

void AddMovement(struct Bot* bot, enum MovementType type, enum Direction direction)
{
    if (!bot) return;
    // Add a new element Move to bot's MoveQueue
    int currentLength = 0;
    while (bot->MoveQueue[currentLength].type == MOVE_TO || bot->MoveQueue[currentLength].type == JUMP)
    {
        currentLength++;
    }
    bot->MoveQueue[currentLength].type = type;
    bot->MoveQueue[currentLength].direction = direction;
    bot->MoveQueue[currentLength + 1].type = INVALID;
}



void MoveBot_AI(void* data)
{
    struct GameData* game = (struct GameData*)data;
    if (!game || !game->bot || !game->grid)
        return;

    struct Bot* bot = game->bot;
    Grid* grid = game->grid;

    game->pathResult = NOTHING;

    while (1)
    {
        sfSleep(sfMilliseconds(300)); 

        int x = bot->position.x;
        int y = bot->position.y;


        if (x + 1 < GRID_COLS)
        {
            enum CellType nextCell = grid->cell[y][x + 1]->type;
            enum CellType nextcelly = grid->cell[y - 1][x]->type;


            if (nextCell == WALKABLE || nextCell == END)
            {
                enum MoveResult result = MoveBot(bot, grid, MOVE_TO, EAST);

                if (result == REACH_END)
                {
                    game->pathResult = REACH_END;
                    return; 
                }


                continue;
            }
            if (nextCell == EMPTY || nextcelly == WALKABLE)
            {
                enum MoveResult result = MoveBot(bot, grid, MOVE_TO, SOUTH);
                if (result == REACH_END)
                {
                    game->pathResult = REACH_END;
                    return;
                }
                continue;
            }
            if (nextCell == OBSTACLE || nextCell == WALKABLE )
            {
                enum MoveResult result = MoveBot(bot, grid, JUMP, EAST);
                if (result == REACH_END)
                {
                    game->pathResult = REACH_END;
                    
                    return;
                }
                continue;
            }
            if (nextcelly == WALKABLE || nextcelly == OBSTACLE )
            {
                enum MoveResult result = MoveBot(bot, grid, JUMP, SOUTH);
                if (result == REACH_END)
                {
                    game->pathResult = REACH_END;

                    return;
                }
                continue;
            }
        }


        game->pathResult = NO_MOVE_LEFT;
        return;
    }
}



bool SearchPath_AI(struct Bot* bot, Grid* grid)
{
    // Implement pathfinding algorithm to fill bot's MoveQueue
    return false;
}

