#include "snake.h"

#define WRAP(v,x,max) ((((uint16_t)(v + x)) % max))
#define DIRECTION_INVERT(x) (((x + 2) % 4))

static void snake_update_direction(snake_t * snake, const SnakeDirection new_direction)
{
    if (snake->body[snake->h_pos].direction != new_direction && \
        DIRECTION_INVERT(snake->body[snake->h_pos].direction) != new_direction)
    {
        snake->body[snake->h_pos].direction = new_direction;
    }
}

static void snake_invert_direction(snake_t * snake)
{
    assert(snake);

    /// swap the body emelments around.
    for (uint16_t i = 0, sz = snake->size / 2; i < sz; i++)
    {
        const uint16_t wrapped_h_pos = WRAP(snake->h_pos, i, snake->size_max);
        const uint16_t wrapped_t_pos = WRAP(snake->h_pos, -i, snake->size_max);

        snake_body_t temp_body = snake->body[wrapped_h_pos];
        temp_body.direction = DIRECTION_INVERT(temp_body.direction);

        snake->body[wrapped_h_pos] = snake->body[wrapped_t_pos];
        snake->body[wrapped_h_pos].direction = DIRECTION_INVERT(snake->body[wrapped_h_pos].direction);

        snake->body[wrapped_t_pos] = temp_body;
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

    /// sanity check to make sure we are still in bounds (should never fail).
    assert(new_head.x < game->board->columns); assert(new_head.y < game->board->rows);

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
                if (game->board->items[i].x == new_head.x && \
                    game->board->items[i].y == new_head.y && \
                    game->board->items[i].type != ItemType_NONE)
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

void snake_update(game_t * game)
{
    assert(game);

    /// update frame count.
    game->frame = (game->frame + 1) % 60;

    if (game->io->type != KeyType_NONE)
    {
        switch (game->io->type)
        {
            case KeyType_UP:
                snake_update_direction(game->snake, SnakeDirection_UP);
                break;

            case KeyType_DOWN:
                snake_update_direction(game->snake, SnakeDirection_DOWN);
                break;

            case KeyType_LEFT:
                snake_update_direction(game->snake, SnakeDirection_LEFT);
                break;

            case KeyType_RIGHT:
                snake_update_direction(game->snake, SnakeDirection_RIGHT);
                break;
            
            default:
                break;
        }

        game->io->type = KeyType_NONE;
    }

    if (game->state == GameState_PLAY)
    {
        update_board(game);
    }
}