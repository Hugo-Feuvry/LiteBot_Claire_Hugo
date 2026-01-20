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


static inline bool in_bounds(int x, int y) {
    return (x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS);
}

static inline bool is_walkable_for_ai(enum CellType t) {
    // Walkable pour l'IA : on EXCLUT EMPTY pour éviter de "tomber dans le vide"
    return (t == WALKABLE || t == START || t == END);
}

// Calcule la destination (nx, ny) d'un move sans effet de bord.
// dist = 1 pour MOVE_TO, 2 pour JUMP.
static inline void compute_dest(int x, int y, enum MovementType mt, enum Direction dir, int* nx, int* ny) {
    int dx = 0, dy = 0, dist = 1;
    switch (dir) {
    case NORTH: dy = -1; break;
    case EAST:  dx = 1; break;
    case SOUTH: dy = 1; break;
    case WEST:  dx = -1; break;
    default: break;
    }
    if (mt == JUMP) dist = 2;
    *nx = x + dx * dist;
    *ny = y + dy * dist;
}

// Vérifie si le move cible la case précédente (pour éviter l'aller‑retour immédiat)
static inline bool targets_prev(int x, int y, enum MovementType mt, enum Direction dir, sfVector2i prev) {
    int nx, ny;
    compute_dest(x, y, mt, dir, &nx, &ny);
    return (nx == prev.x && ny == prev.y);
}

// Essaie d'exécuter un move. Met à jour prevPos uniquement si la position a changé.
// - allow_backtrack=false : interdit de revenir sur prevPos
// - allow_backtrack=true  : autorise le retour si aucune autre option n'existe
static bool try_move_safely(struct Bot* bot, Grid* grid,
    enum MovementType mt, enum Direction dir,
    bool allow_backtrack, sfVector2i* prevPos,
    enum MoveResult* outResult)
{
    int x = bot->position.x;
    int y = bot->position.y;

    if (!allow_backtrack && targets_prev(x, y, mt, dir, *prevPos)) {
        return false;
    }

    // Vérification des bornes de la destination AVANT l’appel à MoveBot
    int nx, ny;
    compute_dest(x, y, mt, dir, &nx, &ny);
    if (!in_bounds(nx, ny)) return false;

    // Pour les JUMP, s'assurer que la destination est praticable pour l'IA
    if (mt == JUMP) {
        enum CellType dstT = grid->cell[ny][nx]->type;
        if (!is_walkable_for_ai(dstT)) {
            return false;
        }
        // La règle "mid doit être OBSTACLE" (ou non-walkable) est laissée à MoveBot si tu l'as implémentée.
    }

    sfVector2i before = bot->position;
    enum MoveResult r = MoveBot(bot, grid, mt, dir);
    if (outResult) *outResult = r;

    // S'arrêter immédiatement si la destination est END
    if (r == REACH_END) return true;

    // A-t-on vraiment bougé ?
    if (bot->position.x != before.x || bot->position.y != before.y) {
        *prevPos = before; // mémorise d'où l'on vient pour casser l'oscillation
        return true;
    }

    return false;
}

void MoveBot_AI(void* data)
{
    struct GameData* game = (struct GameData*)data;
    if (!game || !game->bot || !game->grid)
        return;

    struct Bot* bot = game->bot;
    Grid* grid = game->grid;

    game->pathResult = NOTHING;

    sfVector2i prevPos = (sfVector2i){ -1, -1 };

    while (1)
    {
        sfSleep(sfMilliseconds(300));

        int x = bot->position.x;
        int y = bot->position.y;

        if (grid->cell[y][x]->type == END) {
            game->pathResult = REACH_END;
            return;
        }

        enum CellType eastT = (x + 1 < GRID_COLS) ? grid->cell[y][x + 1]->type : OBSTACLE;
        enum CellType westT = (x - 1 >= 0) ? grid->cell[y][x - 1]->type : OBSTACLE;
        enum CellType northT = (y - 1 >= 0) ? grid->cell[y - 1][x]->type : OBSTACLE;
        enum CellType southT = (y + 1 < GRID_ROWS) ? grid->cell[y + 1][x]->type : OBSTACLE;

        bool east_ok = is_walkable_for_ai(eastT);
        bool west_ok = is_walkable_for_ai(westT);
        bool north_ok = is_walkable_for_ai(northT);
        bool south_ok = is_walkable_for_ai(southT);

        enum MoveResult r = NOTHING;
        bool moved = false;

        /* =======================
         *   PASS 1 : sans retour
         *  ======================= */

         // 1) Priorité à l'Est
        if (!moved && east_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, EAST, false, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        // 2) Descendre si possible
        if (!moved && south_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, SOUTH, false, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        // 3) Aller à l'Ouest
        if (!moved && west_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, WEST, false, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        // 4) Monter si possible
        if (!moved && north_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, NORTH, false, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        // 5) Saut vers le bas si nécessaire
        if (!moved && (y + 2) < GRID_ROWS) {
            enum CellType dstSouth = grid->cell[y + 2][x]->type;
            bool dst_ok = is_walkable_for_ai(dstSouth);
            if (!is_walkable_for_ai(southT) && dst_ok) {
                moved = try_move_safely(bot, grid, JUMP, SOUTH, false, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        // 6) Saut à l'EST si l'est immédiat est bloqué et x+2 praticable
        if (!moved && (x + 2) < GRID_COLS && !east_ok) {
            enum CellType dstEast2 = grid->cell[y][x + 2]->type;
            if (is_walkable_for_ai(dstEast2)) {
                moved = try_move_safely(bot, grid, JUMP, EAST, false, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        // 7) Saut à l'OUEST si l'ouest immédiat est bloqué et x-2 praticable
        if (!moved && (x - 2) >= 0 && !west_ok) {
            enum CellType dstWest2 = grid->cell[y][x - 2]->type;
            if (is_walkable_for_ai(dstWest2)) {
                moved = try_move_safely(bot, grid, JUMP, WEST, false, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        /* =======================
         *   PASS 2 : autoriser 1 retour
         *  ======================= */

        if (!moved && east_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, EAST, true, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        if (!moved && south_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, SOUTH, true, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        if (!moved && west_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, WEST, true, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        if (!moved && north_ok)
            moved = try_move_safely(bot, grid, MOVE_TO, NORTH, true, &prevPos, &r);
        if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }

        if (!moved && (y + 2) < GRID_ROWS) {
            enum CellType dstSouth = grid->cell[y + 2][x]->type;
            bool dst_ok = is_walkable_for_ai(dstSouth);
            if (!is_walkable_for_ai(southT) && dst_ok) {
                moved = try_move_safely(bot, grid, JUMP, SOUTH, true, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        if (!moved && (x + 2) < GRID_COLS && !east_ok) {
            enum CellType dstEast2 = grid->cell[y][x + 2]->type;
            if (is_walkable_for_ai(dstEast2)) {
                moved = try_move_safely(bot, grid, JUMP, EAST, true, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        if (!moved && (x - 2) >= 0 && !west_ok) {
            enum CellType dstWest2 = grid->cell[y][x - 2]->type;
            if (is_walkable_for_ai(dstWest2)) {
                moved = try_move_safely(bot, grid, JUMP, WEST, true, &prevPos, &r);
                if (moved) { if (r == REACH_END) { game->pathResult = REACH_END; } continue; }
            }
        }

        // Aucune option viable
        game->pathResult = NO_MOVE_LEFT;
        return;
    }
}














bool SearchPath_AI(struct Bot* bot, Grid* grid)
{
    // Implement pathfinding algorithm to fill bot's MoveQueue
    return false;
}

