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
            //snake_update_direction(game->snake, SnakeDirection_DOWN);
            break;
        case SDLK_RIGHT: case SDLK_d:
            game->io->type = KeyType_RIGHT;
            //snake_update_direction(game->snake, SnakeDirection_RIGHT);
            break;
        case SDLK_UP: case SDLK_w:
            game->io->type = KeyType_UP;
            //snake_update_direction(game->snake, SnakeDirection_UP);
            break;

        /// pause.
        case SDLK_SPACE:
            game->state = game->state == GameState_PAUSE ? GameState_PLAY : GameState_PAUSE;
            break;

        /// test invertting direction.
        case SDLK_k:
            //snake_invert_direction(game->snake);
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
#endif

static void poll_sdl2(game_t * game)
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

static void poll_allegro(game_t * game)
{

}

void snake_poll(game_t * game)
{
    #ifdef ALLEGRO
        poll_allegro(game);
    #elif SDL2
        poll_sdl2(game);
    #endif
}