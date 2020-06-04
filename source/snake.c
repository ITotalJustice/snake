#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "snake.h"


#define ROWS    40
#define COLUMNS 40
#define SCALE   15

#define WIN_W    ROWS * SCALE
#define WIN_H    COLUMNS * SCALE

typedef enum
{
    BoardCellType_EMPTY         = '.',
    BoardCellType_WALL          = '#',
    BoardCellType_SNAKEBODY     = '-',
    BoardCellType_SNAKEHEAD     = 'O',
    BoardCellType_EAT           = '*',
} BoardCellType;

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
} snake_body_t;

typedef struct
{
    uint16_t size;

    SnakeDirection direction;
    SnakeDirection prev_direction;

    uint16_t h_pos;
    uint16_t t_pos;
    snake_body_t *body;
} snake_t;

typedef enum
{
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

    uint8_t item_count;
    board_item_t *items;

    uint8_t rows;
    uint8_t columns;
    uint8_t **board;
} board_t;

typedef struct
{
    uint32_t win_x;
    uint32_t win_y;
    uint32_t win_w;
    uint32_t win_h;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} renderer_t;

typedef struct
{
    SDL_Joystick *joystick;
    SDL_GameController *controller;
} io_t;

typedef struct
{
    /// current render frame.
    uint8_t frame;

    /// how often to update the game (per frame).
    uint8_t update_freq;

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

static int render_init_sdl(renderer_t * renderer)
{
    assert(renderer);

    renderer->window = SDL_CreateWindow("snake",
        0, 0, WIN_W, WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    assert(renderer->window);

    renderer->renderer = SDL_CreateRenderer(renderer->window,
        -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(renderer->renderer);

    renderer->texture = SDL_CreateTexture(renderer->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        WIN_W, WIN_H);
    assert(renderer->texture);

    return 0;
}

static void render_exit_sdl(renderer_t * renderer)
{
    assert(renderer);

    if (renderer->texture)
    {
        SDL_DestroyTexture(renderer->texture);
        renderer->texture = NULL;
    }
    if (renderer->renderer)
    {
        SDL_DestroyRenderer(renderer->renderer);
        renderer->renderer = NULL;
    }
    if (renderer->window)
    {
        SDL_DestroyWindow(renderer->window);
        renderer->window = NULL;
    }
}

static void draw_grid(const renderer_t * renderer, const board_t * board)
{
    assert(renderer); assert(board);

    SDL_SetRenderDrawColor(renderer->renderer,
        80, 80, 80, 0xFF);

    for (uint8_t r = 1; r < board->rows; r++)
    {
        SDL_RenderDrawLine(renderer->renderer,
            0, r * SCALE, 40 * SCALE, r * SCALE);
    }

    for (uint8_t c = 1; c < board->columns; c++)
    {
        SDL_RenderDrawLine(renderer->renderer,
            c * SCALE, 0, c * SCALE, 40 * SCALE);
    }
}

static void render_set_colour_type(const renderer_t * renderer, BoardCellType type)
{
    assert(renderer);

    switch (type)
    {
        case BoardCellType_EMPTY:       SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 0xFF);      break;
        case BoardCellType_WALL:        SDL_SetRenderDrawColor(renderer->renderer, 153, 0, 0, 0xFF);    break;
        case BoardCellType_SNAKEHEAD:   SDL_SetRenderDrawColor(renderer->renderer, 56, 153, 56, 0xFF);  break;
        case BoardCellType_SNAKEBODY:   SDL_SetRenderDrawColor(renderer->renderer, 36, 93, 36, 0xFF);   break;
        case BoardCellType_EAT:         SDL_SetRenderDrawColor(renderer->renderer, 56, 153, 153, 0xFF); break;

        default: break;
    }
}

static void draw_board(const renderer_t * renderer, const board_t * board)
{
    assert(renderer); assert(board);

    for (uint8_t r = 0; r < board->rows; r++)
    {
        for (uint8_t c = 0; c < board->columns; c++)
        {
            if (board->board[r][c] == BoardCellType_EMPTY)
            {
                continue;
            }

            const SDL_Rect rect = { r * SCALE, c * SCALE, SCALE, SCALE };
            render_set_colour_type(renderer, board->board[r][c]);
            SDL_RenderFillRect(renderer->renderer, &rect);
        }
    }
}

static void render(game_t * game)
{
    assert(game);

    SDL_SetRenderDrawColor(game->renderer->renderer, 0,0,0,0xFF);
    SDL_RenderClear(game->renderer->renderer);

    //draw_grid(game->renderer, game->board);
    draw_board(game->renderer, game->board);

    SDL_RenderPresent(game->renderer->renderer);
}

static void board_free(board_t * board)
{
    assert(board);

    if (board->board && *board->board)
    {
        for (uint8_t i = 0; i < board->rows; i++)
        {
            free(board->board[i]);
            board->board[i] = NULL;
        }

        free(board->board);
        board->board = NULL;
    }

    if (board->items)
    {
        free(board->items);
        board->items = NULL;
    }

    board->rows = 0;
    board->columns = 0;
    board->item_count = 0;
}

static void snake_free(snake_t * snake)
{
    assert(snake);

    snake->size = 0;
    snake->h_pos = 0;
    snake->t_pos = 0;
    snake->direction = 0;
    snake->prev_direction = 0;

    if (snake->body)
    {
        free(snake->body);
        snake->body = NULL;
    }
}

static void snake_end_game(game_t * game)
{
    snake_free(game->snake);
    board_free(game->board);
}

static game_t * snake_init(void)
{
    game_t *game = calloc(1, sizeof(game_t));
    assert(game);
    game->board = calloc(1, sizeof(board_t));
    assert(game->board);
    game->snake = calloc(1, sizeof(snake_t));
    assert(game->snake);
    game->renderer = calloc(1, sizeof(renderer_t));
    assert(game->renderer);
    game->io = calloc(1, sizeof(io_t));
    assert(game->io);

    return game;
}

static void snake_exit(game_t * game)
{
    assert(game);

    /// end any current running games.
    snake_end_game(game);

    if (game->io)
    {
        free(game->io);
        game->io = NULL;
    }
    if (game->snake)
    {
        free(game->snake);
        game->snake = NULL;
    }
    if (game->board)
    {
        free(game->board);
        game->board = NULL;
    }
    if (game->renderer)
    {
        free(game->renderer);
        game->renderer = NULL;
    }
    if (game)
    {
        free(game);
        game = NULL;
    }
}

static void board_create(board_t * board, const uint8_t rows, const uint8_t columns)
{
    assert(board);

    board->rows = rows;
    board->columns = columns;
    board->board = calloc(board->rows, sizeof(uint8_t *));
    assert(board->board);

    for (uint8_t r = 0; r < board->rows; r++)
    {
        board->board[r] = malloc(board->columns);
        assert(board->board[r]);

        for (uint8_t c = 0; c < board->columns; c++)
        {
            board->board[r][c] = BoardCellType_EMPTY;
        }
    }

    for (uint8_t i = 0; i < rows; i++)
    {
        board->board[i][0] = BoardCellType_WALL;
        board->board[0][i] = BoardCellType_WALL;
        board->board[i][rows - 1] = BoardCellType_WALL;
        board->board[rows - 1][i] = BoardCellType_WALL;
    }

}

static SnakeDirection snake_gen_rand_direction(void)
{
    return (SnakeDirection)(rand() % 4);
}

static void snake_create(board_t * board, snake_t * snake)
{
    assert(board); assert(snake);

    /// create the snake body.
    /// set the size to the size of the board (max size).
    snake->body = calloc(board->rows * board->columns, sizeof(snake_body_t));
    assert(snake->body);

    /// give it a random direction.
    /// start the snake in the middle of the board and set the size to 3 (H|M|T).
    snake->direction = snake_gen_rand_direction();
    snake->size = 3;
    snake->body[0].x = board->rows/2;
    snake->body[0].y = board->columns/2;

    /// calculate the rest of the snake body position based on the initial direction.
    switch (snake->direction)
    {
        case SnakeDirection_LEFT:
            snake->body[1].x = snake->body[0].x + 1;
            snake->body[1].y = snake->body[0].y;
            snake->body[2].x = snake->body[1].x + 1;
            snake->body[2].y = snake->body[1].y;
            break;

        case SnakeDirection_DOWN:
            snake->body[1].x = snake->body[0].x;
            snake->body[1].y = snake->body[0].y - 1;
            snake->body[2].x = snake->body[1].x;
            snake->body[2].y = snake->body[1].y - 1;
            break;

        case SnakeDirection_RIGHT:
            snake->body[1].x = snake->body[0].x - 1;
            snake->body[1].y = snake->body[0].y;
            snake->body[2].x = snake->body[1].x - 1;
            snake->body[2].y = snake->body[1].y;
            break;

        case SnakeDirection_UP:
            snake->body[1].x = snake->body[0].x;
            snake->body[1].y = snake->body[0].y + 1;
            snake->body[2].x = snake->body[1].x;
            snake->body[2].y = snake->body[1].y + 1;
            break;
    }

    /// head, mid, tail
    board->board[snake->body[0].x][snake->body[0].y] = BoardCellType_SNAKEHEAD;
    board->board[snake->body[1].x][snake->body[1].y] = BoardCellType_SNAKEBODY;
    board->board[snake->body[2].x][snake->body[2].y] = BoardCellType_SNAKEBODY;
}

static const char *str_snake_direction(const SnakeDirection direction)
{
    switch (direction)
    {
        case SnakeDirection_LEFT:   return "LEFT";
        case SnakeDirection_DOWN:   return "DOWN";
        case SnakeDirection_RIGHT:  return "RIGHT";
        case SnakeDirection_UP:     return "UP";
        default:                    return "NULL";
    }
}

static void board_gen_rand_item_pos(board_t * board)
{
    assert(board);

    /// FIX: this can cause a pretty big loop if the board is nearly full.
    uint8_t x, y;
    do
    {
        x = rand() % board->rows;
        y = rand() % board->columns;
    } while (board->board[x][y] != BoardCellType_EMPTY);
    board->board[x][y] = BoardCellType_EAT;
}

static void snake_swap_direction(game_t * game)
{

}

static void snake_move(game_t * game)
{
    assert(game);

    snake_body_t new_head = game->snake->body[0];
    const snake_body_t old_head = game->snake->body[0];
    const snake_body_t old_tail = game->snake->body[game->snake->size - 1];

    switch (game->snake->direction)
    {
        case SnakeDirection_LEFT:   new_head.x--;   break;
        case SnakeDirection_DOWN:   new_head.y++;   break;
        case SnakeDirection_RIGHT:  new_head.x++;   break;
        case SnakeDirection_UP:     new_head.y--;   break;
    }

    /// hit detection.
    switch (game->board->board[new_head.x][new_head.y])
    {
        /// game over.
        case BoardCellType_WALL: case BoardCellType_SNAKEBODY:
            printf("hit\n");
            game->state = GameState_QUIT;
            return;

        /// eat an item.
        case BoardCellType_EAT:
            game->snake->body[game->snake->size] = game->snake->body[game->snake->size - 1];
            ++game->snake->size;
            --game->board->item_count;
            break;
        
        /// hit nothing.
        default:
            break;
    }

    /// NOTE: not sure which is faster.
    /// i think both options are inneficient, though it's not like snake is
    /// starved for performance.
    /// memcpy is probably faster because it can be easily optimised by the compiler.
    /// also no loops and no more 3 new temp vars.
    memcpy(&game->snake->body[1], game->snake->body, sizeof(snake_body_t) * game->snake->size - 2);

    // snake_body_t push_body = game->snake->body[0];
    // for (uint16_t i = 0; i < game->snake->size - 1; i++)
    // {
    //     snake_body_t temp_body = game->snake->body[i+1];
    //     game->snake->body[i+1] = push_body;
    //     push_body = temp_body;
    // }

    /// update head pos
    game->snake->body[0] = new_head;

    /// update new head
    game->board->board[new_head.x][new_head.y] = BoardCellType_SNAKEHEAD;
    /// fill in empty space between body and head.
    game->board->board[old_head.x][old_head.y] = BoardCellType_SNAKEBODY;
    /// remove old tail.
    game->board->board[old_tail.x][old_tail.y] = BoardCellType_EMPTY;
}

static int snake_new_game(game_t * game)
{
    assert(game);

    /// clear game if already playing.
    snake_end_game(game);

    game->update_freq = 5;
    board_create(game->board, ROWS, COLUMNS);
    snake_create(game->board, game->snake);

    return 0;
}

static void snake_update_direction(snake_t * snake, const SnakeDirection new_direction)
{
    if (snake->direction != new_direction && ((snake->direction + 2) % 4) != new_direction)
    {
        //fprintf(stdout, "changed direction. OLD:%s NEW:%s\n", str_snake_direction(snake->direction), str_snake_direction(new_direction));
        snake->direction = new_direction;
    }
}

static void keyboard_update(game_t * game, const SDL_KeyboardEvent * e)
{
    assert(game); assert(e);

    if (e->repeat)
    {
        return;
    }

    switch (e->keysym.sym)
    {
        case SDLK_LEFT: case SDLK_a:
            snake_update_direction(game->snake, SnakeDirection_LEFT);
            break;
        case SDLK_DOWN: case SDLK_s:
            snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDLK_RIGHT: case SDLK_d:
            snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;
        case SDLK_UP: case SDLK_w:
            snake_update_direction(game->snake, SnakeDirection_UP);
            break;

        case SDLK_SPACE:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;

        case SDLK_r:
            snake_new_game(game);
            break;

        case SDLK_ESCAPE:
            game->state = GameState_QUIT;
            break;

        default:
            break;
    }
}

static void joypad_connect(io_t * io, const SDL_JoyDeviceEvent * e)
{
    assert(io); assert(e);

    if (e->type == SDL_JOYDEVICEADDED)
    {
        if (io->joystick == NULL)
        {
            io->joystick = SDL_JoystickOpen(e->which);
        }
    }
    else
    {
        SDL_JoystickClose(io->joystick);
        io->joystick = NULL;
    }
}

static void jhat_update(game_t * game, const SDL_JoyHatEvent * e)
{
    assert(game); assert(e);

    switch (e->value)
    {
        case SDL_HAT_LEFT:
            snake_update_direction(game->snake, SnakeDirection_LEFT);
            break;
        case SDL_HAT_DOWN:
            snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDL_HAT_RIGHT:
            snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;
        case SDL_HAT_UP:
            snake_update_direction(game->snake, SnakeDirection_UP);
            break;

        default:
            break;
    }
}

static void jbutton_update(game_t * game, const SDL_JoyButtonEvent * e)
{
    assert(game); assert(e);

    switch (e->button)
    {
        case 7:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;

        default:
            break;
    }
}

static void controller_connect(io_t * io, const SDL_ControllerDeviceEvent * e)
{
    assert(io); assert(e);

    if (e->type == SDL_CONTROLLERDEVICEADDED)
    {
        if (io->controller == NULL)
        {
            io->controller = SDL_GameControllerOpen(e->which);
        }
    }
    else
    {
        SDL_GameControllerClose(io->controller);
        io->controller = NULL;
    }
}

static void cbutton_update(game_t * game, const SDL_ControllerButtonEvent * e)
{
    assert(game); assert(e);

    switch (e->button)
    {
        case SDL_CONTROLLER_BUTTON_A:
            break;

        case SDL_CONTROLLER_BUTTON_B:
            break;

        case SDL_CONTROLLER_BUTTON_X:
            break;

        case SDL_CONTROLLER_BUTTON_Y:
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            snake_update_direction(game->snake, SnakeDirection_UP);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            snake_update_direction(game->snake, SnakeDirection_LEFT);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;

        case SDL_CONTROLLER_BUTTON_START:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;
    }
}

static void poll(game_t * game)
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                game->state = GameState_QUIT;
                break;

            case SDL_KEYDOWN:
                keyboard_update(game, &event.key);
                break;

            // case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
            //     joypad_connect(game->io, &event.jdevice);
            //     break;

            // case SDL_JOYHATMOTION:
            //     jhat_update(game, &event.jhat);
            //     break;

            // case SDL_JOYBUTTONDOWN:
            //     jbutton_update(game, &event.jbutton);
            //     break;

            // case SDL_JOYAXISMOTION:
            //     break;

            case SDL_CONTROLLERDEVICEADDED: case SDL_CONTROLLERDEVICEREMOVED:
                controller_connect(game->io, &event.cdevice);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                cbutton_update(game, &event.cbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                break;

            default:
                break;
        }
    }
}

static void update_board(game_t * game)
{
    assert(game);

    /// move snake.
    if (game->frame % game->update_freq == 0)
    {
        snake_move(game);
    }

    /// create new eat item on board
    if (game->board->item_count == 0)
    {
        board_gen_rand_item_pos(game->board);
        game->board->item_count++;
    }
}

static void update(game_t * game)
{
    assert(game);

    game->frame = (game->frame + 1) % 60;

    if (game->state == GameState_PLAY)
    {
        update_board(game);
    }
}

#ifdef __EMSCRIPTEN__
static void snake_run(void * game_void)
{
    game_t *game = (game_t*)game_void;
#else
static void snake_run(game_t * game)
{
#endif

    poll(game);
    update(game);
    render(game);

    #if __EMSCRIPTEN__
    if (game->state == GameState_QUIT)
    {
        emscripten_cancel_main_loop();
    }
    #endif
}

void snake_play()
{
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    game_t *game = snake_init();

    render_init_sdl(game->renderer);

    snake_new_game(game);

    #if __EMSCRIPTEN__
    emscripten_set_main_loop_arg(snake_run, game, -1, 1);
    #else
    while (game->state != GameState_QUIT)
    #endif
    {
        snake_run(game);
    }

    render_exit_sdl(game->renderer);
    snake_exit(game);
    SDL_Quit();
}