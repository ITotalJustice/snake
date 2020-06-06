#include "snake.h"

#ifdef ALLEGRO
static int allegro_init(renderer_t * renderer, const uint32_t w, const uint32_t h)
{
    assert(renderer); assert(w); assert(h);

    bool result;
    result = al_init(); assert(result);
    result = al_install_keyboard(); assert(result);
    result = al_install_mouse(); assert(result);
    result = al_install_joystick(); assert(result);
    result = al_init_primitives_addon(); assert(result);
    result = al_init_font_addon(); assert(result);
    result = al_init_ttf_addon(); assert(result);

    al_set_new_display_flags(ALLEGRO_RESIZABLE | ALLEGRO_OPENGL_3_0);
    renderer->display = al_create_display(renderer->clip.w = w, renderer->clip.h = h);
    assert(renderer->display);

    renderer->queue = al_create_event_queue();
    assert(renderer->queue);

    renderer->font = al_load_ttf_font("data/mplus-2p-regular.ttf", 64, 0);
    assert(renderer->font);

    al_register_event_source(renderer->queue, al_get_display_event_source(renderer->display));
    al_register_event_source(renderer->queue, al_get_keyboard_event_source());
    al_register_event_source(renderer->queue, al_get_mouse_event_source());
    al_register_event_source(renderer->queue, al_get_joystick_event_source());

    renderer->opengl = true;
    renderer->scale = 30;

    return 0;
}

static void allegro_exit(renderer_t * renderer)
{
    al_destroy_display(renderer->display);
    al_destroy_event_queue(renderer->queue);
    al_destroy_font(renderer->font);

    if (al_is_keyboard_installed()) al_uninstall_keyboard();
    if (al_is_mouse_installed()) al_uninstall_mouse();
    if (al_is_joystick_installed()) al_uninstall_joystick();
    if (al_is_primitives_addon_initialized()) al_shutdown_primitives_addon();
    if (al_is_ttf_addon_initialized()) al_shutdown_ttf_addon();
    if (al_is_font_addon_initialized()) al_shutdown_font_addon();
}
#endif

#ifdef SDL2
static int sdl2_init(renderer_t * renderer, const uint32_t w, const uint32_t h)
{
    assert(renderer); assert(w); assert(h);

    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    assert(result == 0);

    renderer->window = SDL_CreateWindow("snake",
        0, 0, renderer->clip.w = w, renderer->clip.h = h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    assert(renderer->window);

    renderer->renderer = SDL_CreateRenderer(renderer->window,
        -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    assert(renderer->renderer);

    renderer->texture = SDL_CreateTexture(renderer->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        w, h);
    assert(renderer->texture);

    renderer->opengl = false;
    renderer->scale = 30;

    return 0;
}

static void sdl2_exit(renderer_t * renderer)
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

    SDL_Quit();
}
#endif

int snake_render_init(renderer_t * renderer, const uint32_t w, const uint32_t h)
{
    #ifdef ALLEGRO
        return allegro_init(renderer, w, h);
    #elif SDL2
        return sdl2_init(renderer, w, h);
    #endif

    return -1;
}

void snake_render_exit(renderer_t * renderer)
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

static void draw_text(const renderer_t * renderer, const colour_t colour, float x, float y, int flags, char const * text, ...)
{
    #ifdef ALLEGRO
        al_draw_text(renderer->font, al_map_rgba(colour.r, colour.g, colour.b, colour.a), x, y, flags, text);
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

            draw_board_rect(renderer, map_rect(r * renderer->scale, c * renderer->scale, renderer->scale, renderer->scale), board->board[r][c]);
        }
    }
}

static void draw_osd(const renderer_t * renderer, const board_t * board)
{
    assert(renderer); assert(board);

    draw_text(renderer, map_rgb(80,80,80), renderer->clip.w / 2, 1 * renderer->scale, 1 /*ALLEGRO_ALIGN_CENTER*/, "Snake");
}

void snake_render(game_t * game)
{
    assert(game);

    render_clear(game->renderer, map_rgb(0,0,0));

    //draw_grid(game->renderer, game->board->rows, game->board->columns, game->renderer->clip, map_rgb(255,255,255));
    draw_board(game->renderer, game->board);
    draw_osd(game->renderer, game->board);

    render_update(game->renderer);
}