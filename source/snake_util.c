#include "snake.h"

SnakeDirection snake_gen_rand_direction(void)
{
    return (SnakeDirection)(rand() % 4);
}

void board_gen_rand_item_pos(board_t * board, const ItemType type)
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