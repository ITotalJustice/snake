#pragma once

#include "includes.h"

typedef enum
{
    BoardCellType_EMPTY         = '.',
    BoardCellType_WALL          = '#',
    BoardCellType_SNAKEBODY     = '-',
    BoardCellType_SNAKEHEAD     = 'O',
    BoardCellType_ITEM          = '*',
} BoardCellType;

typedef enum
{
    KeyType_NONE,
    KeyType_UP,
    KeyType_DOWN,
    KeyType_LEFT,
    KeyType_RIGHT,
} KeyType;

typedef enum
{
    GameState_PLAY,
    GameState_PAUSE,
    GameState_QUIT,
} GameState;

typedef enum
{
    SnakeDirection_LEFT,
    SnakeDirection_DOWN,
    SnakeDirection_RIGHT,
    SnakeDirection_UP,
} SnakeDirection;

typedef struct
{
    uint8_t x;
    uint8_t y;
    SnakeDirection direction;
} snake_body_t;

typedef struct
{
    uint16_t size;
    uint16_t size_max;

    SnakeDirection buffered_direction;

    uint16_t h_pos;
    uint16_t t_pos;
    snake_body_t *body;
} snake_t;

typedef enum
{
    ItemType_NONE,
    ItemType_FOOD,
    ItemType_POWERUP,
} ItemType;

typedef struct
{
    uint8_t x;
    uint8_t y;

    ItemType type;
} board_item_t;

typedef struct
{
    uint32_t score;

    uint16_t item_count;
    uint16_t item_max;
    board_item_t *items;

    uint8_t rows;
    uint8_t columns;
    uint8_t **board;
} board_t;

typedef struct
{
    bool opengl;

    rect_t clip;

    float scale;

    #ifdef ALLEGRO
    ALLEGRO_DISPLAY *display;
    ALLEGRO_FONT *font;
    ALLEGRO_EVENT_QUEUE *queue;
    #elif SDL2
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    #endif
} renderer_t;

typedef struct
{
    KeyType type;

    #ifdef ALLEGRO
    ALLEGRO_JOYSTICK *joystick;
    #elif SDL2
    SDL_Joystick *joystick;
    SDL_GameController *controller;
    #endif
} io_t;

typedef enum
{
    Player_NORMAL,
    Player_AI,
} Player;

typedef struct
{
    /// current render frame.
    uint8_t frame;

    /// how often to update the game (per frame).
    uint8_t update_freq;

    /// TODO: cleanly impliment this.
    Player player_type;

    /// play, pause or quit.
    GameState state;

    /// the main board.
    board_t *board;

    /// the snake size, body and direction.
    snake_t *snake;

    /// window, renderer and (unused) texture.
    renderer_t *renderer;

    /// joypad / controller structs.
    io_t *io;
} game_t;

bool snake_inbounds(board_t * board, const uint8_t x, const uint8_t y);
SnakeDirection snake_gen_rand_direction(void);
void board_gen_rand_item_pos(board_t * board, const ItemType type);

int snake_render_init(renderer_t * renderer, const uint32_t w, const uint32_t h);
void snake_render_exit(renderer_t * renderer);

int snake_new_game(game_t * game);
void snake_exit(game_t * game);

void snake_poll(game_t * game);
void snake_update(game_t * game);
void snake_render(game_t * game);

void snake_play();