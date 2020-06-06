#include "snake.h"

#define ROWS    20
#define COLUMNS 20
#define SCALE   30

#define WIN_W    ROWS * SCALE
#define WIN_H    COLUMNS * SCALE

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

void snake_exit(game_t * game)
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

int snake_new_game(game_t * game)
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

static inline void snake_run(game_t * game)
{
    snake_poll(game);
    snake_update(game);
    snake_render(game);
}

void snake_play()
{
    srand(time(NULL));

    game_t *game = snake_init();
    snake_render_init(game->renderer, WIN_W, WIN_H);

    snake_new_game(game);
    //game->state = GameState_PAUSE;

    while (game->state != GameState_QUIT)
    {
        snake_run(game);
    }

    snake_render_exit(game->renderer);
    snake_exit(game);
}