#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} NumberFactory;

SDL_AppResult SDL_AppInit(void **state, int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    *state = NULL;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    NumberFactory *game = SDL_calloc(1, sizeof(*game));
    if (!game) {
        SDL_Log("Failed to allocate game state");
        return SDL_APP_FAILURE;
    }
    *state = game;

    if (!SDL_CreateWindowAndRenderer("jam", 800, 600, 0, &game->window,
                                     &game->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *state) {
    NumberFactory *game = state;

    SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
    SDL_RenderClear(game->renderer);
    SDL_RenderPresent(game->renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *state, SDL_Event *event) {
    (void)state;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *state, SDL_AppResult result) {
    (void)result;
    NumberFactory *game = state;
    if (game->renderer)
        SDL_DestroyRenderer(game->renderer);
    if (game->window)
        SDL_DestroyWindow(game->window);
    SDL_free(game);
}
