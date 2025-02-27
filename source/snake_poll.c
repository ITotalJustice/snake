#include "snake.h"

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
            game->io->type = KeyType_LEFT;
            break;
        case SDLK_DOWN: case SDLK_s:
            game->io->type = KeyType_DOWN;
            break;
        case SDLK_RIGHT: case SDLK_d:
            game->io->type = KeyType_RIGHT;
            break;
        case SDLK_UP: case SDLK_w:
            game->io->type = KeyType_UP;
            break;

        /// pause.
        case SDLK_SPACE:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
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
            //snake_update_direction(game->snake, SnakeDirection_LEFT);
            break;
        case SDL_HAT_DOWN:
            //snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDL_HAT_RIGHT:
            //snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;
        case SDL_HAT_UP:
            //snake_update_direction(game->snake, SnakeDirection_UP);
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
            //snake_update_direction(game->snake, SnakeDirection_UP);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            //snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            //snake_update_direction(game->snake, SnakeDirection_LEFT);
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            //snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;

        case SDL_CONTROLLER_BUTTON_START:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;
    }
}

static void poll_sdl2(game_t * game)
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
#endif

#ifdef ALLEGRO
static void keyboard_update(game_t * game, ALLEGRO_KEYBOARD_EVENT * e)
{
    assert(game); assert(e);

    switch (e->keycode)
    {
        case ALLEGRO_KEY_UP: case ALLEGRO_KEY_W:
            game->io->type = KeyType_UP;
            break;
        case ALLEGRO_KEY_DOWN: case ALLEGRO_KEY_S:
            game->io->type = KeyType_DOWN;
            break;
        case ALLEGRO_KEY_LEFT: case ALLEGRO_KEY_A:
            game->io->type = KeyType_LEFT;
            break;
        case ALLEGRO_KEY_RIGHT: case ALLEGRO_KEY_D:
            game->io->type = KeyType_RIGHT;
            break;

        case ALLEGRO_KEY_SPACE:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;
            
        case ALLEGRO_KEY_ESCAPE:
            game->state = GameState_QUIT;
            break;
        break;
    }
}

static void jbutton_update(game_t * game, ALLEGRO_JOYSTICK_EVENT * e)
{
    assert(game); assert(e);

    switch (e->type)
    {
        default:
            break;
    }
}

static void poll_allegro(game_t * game)
{
    assert(game);

    ALLEGRO_EVENT event;
    while (al_wait_for_event_timed(game->renderer->queue, &event, 0))
    {
        switch (event.type)
        {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                game->state = GameState_QUIT;
                break;

            case ALLEGRO_EVENT_DISPLAY_RESIZE:
                game->renderer->clip.w = event.display.width; game->renderer->clip.h = event.display.height;
                game->renderer->scale = (game->renderer->clip.w < game->renderer->clip.h ? game->renderer->clip.w : game->renderer->clip.h) / 20;
                game->renderer->clip.x = (game->renderer->clip.w - (game->renderer->scale * 20)) / 2;
                game->renderer->clip.y = (game->renderer->clip.h - (game->renderer->scale * 20)) / 2;
                al_acknowledge_resize(event.display.source);
                break;

            case ALLEGRO_EVENT_KEY_DOWN:
                keyboard_update(game, &event.keyboard);
                break;

            case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
                jbutton_update(game, &event.joystick);
                break;

            default:
                break;
        }
    }
}
#endif

void snake_poll(game_t * game)
{
    #ifdef ALLEGRO
        poll_allegro(game);
    #elif SDL2
        poll_sdl2(game);
    #endif
}