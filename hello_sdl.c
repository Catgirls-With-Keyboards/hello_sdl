#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define AUDIO_CHANNEL_COUNT 16

#define SDLE(asst, str)                                                        \
  (!(asst)                                                                     \
   ? SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, str ": %s", SDL_GetError()),   \
   exit(1) : 0)
#define MUSE(asst, str)                                                        \
  (!(asst)                                                                     \
   ? SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, str ": %s", Mix_GetError()),   \
   exit(1) : 0)

#define MUSIC_PATH "Secret Melody.mp3"
#define IMAGE_PATH "bg.jpg"
#define FONT_PATH "lemon.ttf"

int main(int argc, char *argv[]) {

  // Initialize SDL
  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  Mix_Init(MIX_INIT_MP3);
  TTF_Init();
  puts("SDL initialized");

  // Create window
  SDL_Window *window = SDL_CreateWindow(
      "SDL Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDLE(window, "Failed to create window");
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDLE(renderer, "Failed to create renderer");
  puts("Window created");

  // Load image
  SDL_Surface *imageSurface = IMG_Load(IMAGE_PATH);
  SDLE(imageSurface, "Failed to load image");
  SDL_Texture *imageTexture =
      SDL_CreateTextureFromSurface(renderer, imageSurface);
  SDLE(imageTexture, "Failed to create texture");
  SDL_FreeSurface(imageSurface);
  puts("Image loaded");

  // Load font
  TTF_Font *font = TTF_OpenFont(FONT_PATH, 30);
  SDLE(font, "Failed to load font");
  puts("Font loaded");

  // Load music
  int audio_failed = Mix_OpenAudio(44100, AUDIO_S32, 2, 4096);
  MUSE(!audio_failed, "Failed to open audio channel");
  audio_failed = Mix_AllocateChannels(AUDIO_CHANNEL_COUNT);
  MUSE(audio_failed == AUDIO_CHANNEL_COUNT, "Failed to allocate channels");
  Mix_Music *music = Mix_LoadMUS(MUSIC_PATH);
  MUSE(music, "Failed to load music");
  puts("Music loaded");

  // Play music
  Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
  audio_failed = Mix_PlayMusic(music, -1);
  MUSE(!audio_failed, "Failed to play music");
  puts("Music playing");

  // Render text
  SDL_Color textColor = {255, 0, 0, 255}; // Red color
  SDL_Surface *textSurface =
      TTF_RenderText_Solid(font, "Example Text", textColor);
  SDLE(textSurface, "Failed to render text");
  SDL_Texture *textTexture =
      SDL_CreateTextureFromSurface(renderer, textSurface);
  SDLE(textTexture, "Failed to create texture");
  SDL_Rect textRect = {50, 50, textSurface->w, textSurface->h};
  SDL_FreeSurface(textSurface);
  puts("Text rendered");

  // Main loop
  SDL_Event e;
  int screen_dirty = 1;
  int quit = 0;
  while (1) {
    while (SDL_PollEvent(&e))
      if (e.type == SDL_QUIT)
        goto exit_main_loop;

    if (screen_dirty) {
      screen_dirty = 0;
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, imageTexture, NULL, NULL);
      SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
      SDL_RenderPresent(renderer);
      puts("Screen rendered");
    }
  }

// Cleanup
exit_main_loop:
  Mix_CloseAudio();
  Mix_FreeMusic(music);
  SDL_DestroyTexture(textTexture);
  SDL_DestroyTexture(imageTexture);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  puts("SDL cleaned up");

  return 0;
}