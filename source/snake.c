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
    uint16_t size_max;

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
    board->board = calloc(board->rows, sizeof(uint8_t*));
    assert(board->board);

    /// allocate the columns, fill empty.
    for (uint8_t r = 0; r < board->rows; r++)
    {
        board->board[r] = calloc(board->columns, sizeof(uint8_t));
        assert(board->board[r]);

        for (uint8_t c = 0; c < board->columns; c++)
        {
            board->board[r][c] = BoardCellType_EMPTY;
        }
    }

    /// set a basic wall around the board.
    /// levels will have layout of their own.
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
    snake->size_max = board->rows * board->columns;
    snake->body = calloc(snake->size_max, sizeof(snake_body_t));
    assert(snake->body);

    /// random starting direction.
    snake->direction = snake_gen_rand_direction();
    snake->size = 3;
    snake->h_pos = 0;
    snake->t_pos = 2;

    /// set the head to start in the middle.
    /// the body will be set to the same position as the head.
    snake->body[0].x = board->rows/2;
    snake->body[0].y = board->columns/2;
    snake->body[1] = snake->body[0];
    snake->body[2] = snake->body[0];

    /// calculate the rest of the snake body position based on the initial direction.
    switch (snake->direction)
    {
        case SnakeDirection_LEFT:
            snake->body[1].x += 1;
            snake->body[2].x += 2;
            break;

        case SnakeDirection_DOWN:
            snake->body[1].y -= 1;
            snake->body[2].y -= 2;
            break;

        case SnakeDirection_RIGHT:
            snake->body[1].x -= 1;
            snake->body[2].x -= 2;
            break;

        case SnakeDirection_UP:
            snake->body[1].y += 1;
            snake->body[2].y += 2;
            break;
    }

    /// head, mid, tail
    board->board[snake->body[0].x][snake->body[0].y] = BoardCellType_SNAKEHEAD;
    board->board[snake->body[1].x][snake->body[1].y] = BoardCellType_SNAKEBODY;
    board->board[snake->body[2].x][snake->body[2].y] = BoardCellType_SNAKEBODY;
}

static void board_gen_rand_item_pos(board_t * board)
{
    assert(board);

    /// FIX: this can cause a pretty big loop if the board is nearly full.
    uint8_t x = 0, y = 0;
    do
    {
        x = rand() % board->rows;
        y = rand() % board->columns;
    } while (board->board[x][y] != BoardCellType_EMPTY);
    board->board[x][y] = BoardCellType_EAT;
}

static void snake_invert_direction(snake_t * snake)
{
    assert(snake);

    /// invert the direction.
    snake->direction = (snake->direction + 2) % 4;
    
    /// swap the body emelments around.
    for (uint16_t i = 0, sz = snake->size / 2; i < sz; i++)
    {
        snake_body_t temp_body = snake->body[(snake->h_pos + i) % snake->size_max];
        snake->body[(snake->h_pos + i) % snake->size_max] = snake->body[(snake->t_pos - i) % snake->size_max];
        snake->body[(snake->t_pos - i) % snake->size_max] = temp_body;
    }
}

static void snake_move(game_t * game)
{
    assert(game);

    snake_body_t new_head = game->snake->body[game->snake->h_pos];
    const snake_body_t old_head = game->snake->body[game->snake->h_pos];
    const snake_body_t old_tail = game->snake->body[game->snake->t_pos];

    /// update new position.
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
        /// hit nothing.
        case BoardCellType_EMPTY:
            break;

        /// game over.
        case BoardCellType_WALL: case BoardCellType_SNAKEBODY:
            game->state = GameState_QUIT;
            return;
            break;

        /// eat an item.
        case BoardCellType_EAT:
            game->snake->t_pos = (game->snake->t_pos + 1) % game->snake->size_max;
            game->snake->body[game->snake->t_pos] = old_tail;
            ++game->snake->size;
            --game->board->item_count;
            break;

        default:
            break;
    }

    /// update new head / tail pos in the body array.
    game->snake->h_pos = (game->snake->h_pos - 1) % game->snake->size_max;
    game->snake->t_pos = (game->snake->t_pos - 1) % game->snake->size_max;

    /// move head to the new position in the array.
    game->snake->body[game->snake->h_pos] = new_head;

    /// update new head on the board.
    game->board->board[new_head.x][new_head.y] = BoardCellType_SNAKEHEAD;
    /// fill in empty space between body and head on the board.
    game->board->board[old_head.x][old_head.y] = BoardCellType_SNAKEBODY;
    /// remove old tail from the board.
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
        /// up,down,left,right
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

        /// pause.
        case SDLK_SPACE:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;

        /// test invertting direction.
        case SDLK_k:
            snake_invert_direction(game->snake);
            break;

        /// test reset.
        case SDLK_r:
            snake_new_game(game);
            break;

        /// quit
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

static void snake_run(game_t * game)
{
    poll(game);
    update(game);
    render(game);
}

void snake_play()
{
    srand(time(NULL));

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    game_t *game = snake_init();
    render_init_sdl(game->renderer);
    snake_new_game(game);

    while (game->state != GameState_QUIT)
    {
        snake_run(game);
    }

    render_exit_sdl(game->renderer);
    snake_exit(game);
    SDL_Quit();
}