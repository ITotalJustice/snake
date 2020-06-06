#include "snake.h"

#define ROWS    20
#define COLUMNS 20
#define SCALE   30

#define WIN_W    ROWS * SCALE
#define WIN_H    COLUMNS * SCALE

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
    //#ifdef ALLEGRO

    //#elif SDL2
    #ifdef SDL2
    SDL_Joystick *joystick;
    SDL_GameController *controller;
    #endif
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

static int allegro_init(renderer_t * renderer)
{
    #ifdef ALLEGRO
    assert(renderer);

    bool result;
    result = al_init(); assert(result);
    result = al_install_keyboard(); assert(result);
    result = al_install_mouse(); assert(result);
    result = al_install_joystick(); assert(result);
    result = al_init_primitives_addon(); assert(result);
    result = al_init_font_addon(); assert(result);
    result = al_init_ttf_addon(); assert(result);

    al_set_new_display_flags(ALLEGRO_RESIZABLE | ALLEGRO_OPENGL_3_0);
    renderer->display = al_create_display(renderer->clip.w = WIN_W, renderer->clip.h = WIN_H);
    assert(renderer->display);

    renderer->queue = al_create_event_queue();
    assert(renderer->queue);

    al_register_event_source(renderer->queue, al_get_display_event_source(renderer->display));
    al_register_event_source(renderer->queue, al_get_keyboard_event_source());
    al_register_event_source(renderer->queue, al_get_mouse_event_source());
    al_register_event_source(renderer->queue, al_get_joystick_event_source());

    renderer->opengl = true;
    #endif

    return 0;
}

static void allegro_exit(renderer_t * renderer)
{
    #ifdef ALLEGRO
    al_destroy_display(renderer->display);
    al_destroy_event_queue(renderer->queue);
    al_destroy_font(renderer->font);

    if (al_is_keyboard_installed()) al_uninstall_keyboard();
    if (al_is_mouse_installed()) al_uninstall_mouse();
    if (al_is_joystick_installed()) al_uninstall_joystick();
    if (al_is_primitives_addon_initialized()) al_shutdown_primitives_addon();
    if (al_is_ttf_addon_initialized()) al_shutdown_ttf_addon();
    if (al_is_font_addon_initialized()) al_shutdown_font_addon();
    #endif
}

static int sdl2_init(renderer_t * renderer)
{
    #ifdef SDL2
    assert(renderer);

    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    assert(result == 0);

    renderer->window = SDL_CreateWindow("snake",
        0, 0, renderer->clip.w = WIN_W, renderer->clip.h = WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    assert(renderer->window);

    renderer->renderer = SDL_CreateRenderer(renderer->window,
        -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(renderer->renderer);

    renderer->texture = SDL_CreateTexture(renderer->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        WIN_W, WIN_H);
    assert(renderer->texture);

    renderer->opengl = false;
    #endif

    return 0;
}

static void sdl2_exit(renderer_t * renderer)
{
    #ifdef SDL2
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

    SDL_Quit();
    #endif
}

static int render_init(renderer_t * renderer)
{
    #ifdef ALLEGRO
        return allegro_init(renderer);
    #elif SDL2
        return sdl2_init(renderer);
    #endif

    return -1;
}

static void render_exit(renderer_t * renderer)
{
    #ifdef ALLEGRO
        allegro_exit(renderer);
    #elif SDL2
        sdl2_exit(renderer);
    #endif
}

static void render_clear(const renderer_t * renderer, const colour_t colour)
{
    #ifdef ALLEGRO
        al_clear_to_color(al_map_rgba(colour.r, colour.g, colour.b, colour.a));
    #elif SDL2
        SDL_SetRenderDrawColor(renderer->renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderClear(renderer->renderer);
    #endif
}

static void render_update(const renderer_t * renderer)
{
    #ifdef ALLEGRO
        al_flip_display();
    #elif SDL2
        SDL_RenderPresent(renderer->renderer);
    #endif
}

static void draw_line(const renderer_t * renderer, const rect_t rect, const colour_t colour)
{
    #ifdef ALLEGRO
        al_draw_line(rect.x, rect.y, rect.w, rect.h, al_map_rgba(colour.r, colour.g, colour.b, colour.a), 1);
    #elif SDL2
        SDL_SetRenderDrawColor(renderer->renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderDrawLine(renderer->renderer, rect.x, rect.y, rect.w, rect.h);
    #endif
}

static void draw_rect(const renderer_t * renderer, const rect_t rect, const colour_t colour)
{
    #ifdef ALLEGRO
        al_draw_rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, al_map_rgba(colour.r, colour.g, colour.b, colour.a), 1);
    #elif SDL2
        const SDL_Rect sdl_rect = { .x = rect.x, .y = rect.y, .w = rect.w, .h = rect.h };
        SDL_SetRenderDrawColor(renderer->renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderDrawRect(renderer->renderer, &sdl_rect);
    #endif
}

static void draw_filled_rect(const renderer_t * renderer, const rect_t rect, const colour_t colour)
{
    #ifdef ALLEGRO
        al_draw_filled_rectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, al_map_rgba(colour.r, colour.g, colour.b, colour.a));
    #elif SDL2
        const SDL_Rect sdl_rect = { .x = rect.x, .y = rect.y, .w = rect.w, .h = rect.h };
        SDL_SetRenderDrawColor(renderer->renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderFillRect(renderer->renderer, &sdl_rect);
    #endif
}

static void draw_grid(const renderer_t * renderer, const uint32_t rows, const uint32_t columns, const rect_t clip, const colour_t colour)
{
    assert(renderer);

    if (rows == 0 || columns == 0 || clip.w == 0 || clip.h == 0)
    {
        return;
    }

    const uint32_t r_scale = clip.h / rows;
    for (uint32_t r = 1; r < rows; r++)
    {
        draw_line(renderer, map_rect(clip.x, r * r_scale, clip.w, r * r_scale), colour);
    }

    const uint32_t c_scale = clip.w / columns;
    for (uint32_t c = 1; c < columns; c++)
    {
        draw_line(renderer, map_rect(c * c_scale, clip.y, c * c_scale, clip.h), colour);
    }
}

static void draw_board_rect(const renderer_t * renderer, const rect_t rect, BoardCellType type)
{
    assert(renderer);

    switch (type)
    {
        case BoardCellType_EMPTY:       draw_filled_rect(renderer, rect, map_rgb(0, 0, 0));      break;
        case BoardCellType_WALL:        draw_filled_rect(renderer, rect, map_rgb(153, 0, 0));    break;
        case BoardCellType_SNAKEHEAD:   draw_filled_rect(renderer, rect, map_rgb(56, 153, 56));  break;
        case BoardCellType_SNAKEBODY:   draw_filled_rect(renderer, rect, map_rgb(36, 93, 36));   break;
        case BoardCellType_ITEM:        draw_filled_rect(renderer, rect, map_rgb(56, 153, 153)); break;

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

            draw_board_rect(renderer, map_rect(r * SCALE, c * SCALE, SCALE, SCALE), board->board[r][c]);
        }
    }
}

static void render(game_t * game)
{
    assert(game);

    render_clear(game->renderer, map_rgb(0,0,0));

    //draw_grid(game->renderer, game->board->rows, game->board->columns, game->renderer->clip, map_rgb(255,255,255));
    draw_board(game->renderer, game->board);

    render_update(game->renderer);
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

    board->item_count = 0;
    board->item_max = 321;
    board->items = calloc(board->item_max, sizeof(board_item_t));
    assert(board->items);

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

    snake->size = 3;
    snake->h_pos = 0;
    snake->t_pos = 2;

    /// set the head to start in the middle.
    /// the body will be set to the same position as the head.
    snake->body[0].direction = snake_gen_rand_direction();
    snake->body[0].x = board->rows/2;
    snake->body[0].y = board->columns/2;
    snake->body[1] = snake->body[0];
    snake->body[2] = snake->body[0];

    /// calculate the rest of the snake body position based on the initial direction.
    switch (snake->body[0].direction)
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

static void board_gen_rand_item_pos(board_t * board, const ItemType type)
{
    assert(board);

    /// FIX: this can cause a pretty big loop if the board is nearly full.
    uint8_t x = 0, y = 0;
    do
    {
        x = rand() % board->rows;
        y = rand() % board->columns;
    } while (board->board[x][y] != BoardCellType_EMPTY);

    board->board[x][y] = BoardCellType_ITEM;
    board->items[board->item_count].type = type;
    board->items[board->item_count].x = x;
    board->items[board->item_count].y = y;
}

#define WRAP(v,x,max) ((((uint16_t)(v + x)) % max))
#define DIRECTION_INVERT(x) (((x + 2) % 4))

static void snake_invert_direction(snake_t * snake)
{
    assert(snake);

    /// swap the body emelments around.
    for (uint16_t i = 0, sz = snake->size / 2; i < sz; i++)
    {
        snake_body_t temp_body = snake->body[WRAP(snake->h_pos, i, snake->size_max)];
        temp_body.direction = DIRECTION_INVERT(temp_body.direction);

        snake->body[WRAP(snake->h_pos, i, snake->size_max)] = snake->body[WRAP(snake->t_pos, -i, snake->size_max)];
        snake->body[WRAP(snake->h_pos, i, snake->size_max)].direction = DIRECTION_INVERT(snake->body[WRAP(snake->h_pos, i, snake->size_max)].direction);

        snake->body[WRAP(snake->t_pos, -i, snake->size_max)] = temp_body;
    }
}

static void snake_move(game_t * game)
{
    assert(game);

    snake_body_t new_head = game->snake->body[game->snake->h_pos];
    const snake_body_t old_head = game->snake->body[game->snake->h_pos];
    const snake_body_t old_tail = game->snake->body[game->snake->t_pos];

    /// update new position.
    switch (old_head.direction)
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
            game->state = GameState_PAUSE;
            game->board->board[new_head.x][new_head.y] = BoardCellType_SNAKEHEAD;
            return;

        /// eat an item.
        case BoardCellType_ITEM:
            for (uint16_t i = 0; i < game->board->item_count; i++)
            {
                if (game->board->items[i].x == new_head.x && game->board->items[i].y == new_head.y && game->board->items[i].type != ItemType_NONE)
                {
                    game->snake->t_pos = WRAP(game->snake->t_pos, 1, game->snake->size_max);
                    game->snake->body[game->snake->t_pos] = old_tail;

                    ++game->snake->size;
                    --game->board->item_count;
                    game->board->items[i].type = ItemType_NONE;
                    break;
                }
            }
            break;

        default:
            break;
    }

    /// update new head / tail pos in the body array.
    game->snake->h_pos = WRAP(game->snake->h_pos, -1, game->snake->size_max);
    game->snake->t_pos = WRAP(game->snake->t_pos, -1, game->snake->size_max);


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

    /// how often the board should be updated (frame tick).
    game->update_freq = 6;

    board_create(game->board, ROWS, COLUMNS);
    snake_create(game->board, game->snake);

    return 0;
}

static void snake_update_direction(snake_t * snake, const SnakeDirection new_direction)
{
    if (snake->body[snake->h_pos].direction != new_direction && DIRECTION_INVERT(snake->body[snake->h_pos].direction) != new_direction)
    {
        snake->body[snake->h_pos].direction = new_direction;
    }
}

#ifdef SDL2
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
#endif

static void poll(game_t * game)
{
    #ifdef SDL2
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
    #endif
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
        board_gen_rand_item_pos(game->board, ItemType_FOOD);
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

static inline void snake_run(game_t * game)
{
    poll(game);
    update(game);
    render(game);
}

void snake_play()
{
    srand(time(NULL));

    game_t *game = snake_init();
    render_init(game->renderer);

    snake_new_game(game);
    //game->state = GameState_PAUSE;

    while (game->state != GameState_QUIT)
    {
        snake_run(game);
    }

    render_exit(game->renderer);
    snake_exit(game);
}