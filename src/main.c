#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *win = SDL_CreateWindow(
    "jam",
    800, 600,
    0
  );
  if (!win) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Delay(3000);

  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
