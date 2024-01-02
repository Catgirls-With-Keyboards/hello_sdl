#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_TITLE "SDL Example"
#define EXAMPLE_TEXT "Example text"
#define EXAMPLE_TEXT_SIZE 30
#define EXAMPLE_TEXT_COLOR (SDL_Color) { 255, 0, 0, 255 }
#define FONT_PATH "lemon.ttf"

#define MUSIC_PATH "Secret Melody.mp3"
#define MUSIC_LOOPS -1 // infinite loop
#define AUDIO_CHANNEL_COUNT 16

#define IMAGE_PATH "bg.jpg"

#define SDL_ERROR(asst, str)                                                   \
  (!(asst)                                                                     \
   ? SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, str ": %s", SDL_GetError()),   \
   exit(1) : 0)

int main(int argc, char *argv[]) {

  // Initialize SDL
  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
  Mix_Init(MIX_INIT_MP3);
  TTF_Init();
  puts("SDL initialized");

  // Create window
  SDL_Window *window = SDL_CreateWindow(
      WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_ERROR(window, "Failed to create window");
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_ERROR(renderer, "Failed to create renderer");
  puts("Window created");

  // Load image
  SDL_Surface *imageSurface = IMG_Load(IMAGE_PATH);
  SDL_ERROR(imageSurface, "Failed to load image");
  SDL_Texture *imageTexture =
      SDL_CreateTextureFromSurface(renderer, imageSurface);
  SDL_ERROR(imageTexture, "Failed to create texture");
  SDL_FreeSurface(imageSurface);
  puts("Image loaded");

  // Load font
  TTF_Font *example_text_font = TTF_OpenFont(FONT_PATH, EXAMPLE_TEXT_SIZE);
  SDL_ERROR(example_text_font, "Failed to load font");
  puts("Font loaded");

  // Load music
  int audio_failed = Mix_OpenAudio(44100, AUDIO_S32, 2, 4096);
  SDL_ERROR(!audio_failed, "Failed to open audio channel");
  audio_failed = Mix_AllocateChannels(AUDIO_CHANNEL_COUNT);
  SDL_ERROR(audio_failed == AUDIO_CHANNEL_COUNT, "Failed to allocate channels");
  Mix_Music *music = Mix_LoadMUS(MUSIC_PATH);
  SDL_ERROR(music, "Failed to load music");
  puts("Music loaded");

  // Play music
  Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
  audio_failed = Mix_PlayMusic(music, MUSIC_LOOPS);
  SDL_ERROR(!audio_failed, "Failed to play music");
  puts("Music playing");

  // Render text
  SDL_Color textColor = EXAMPLE_TEXT_COLOR;
  SDL_Surface *textSurface =
      TTF_RenderText_Solid(example_text_font, EXAMPLE_TEXT, textColor);
  SDL_ERROR(textSurface, "Failed to render text");
  SDL_Texture *textTexture =
      SDL_CreateTextureFromSurface(renderer, textSurface);
  SDL_ERROR(textTexture, "Failed to create texture");
  SDL_Rect textRect = {50, 50, textSurface->w, textSurface->h};
  SDL_FreeSurface(textSurface);
  puts("Text rendered");

  // Main loop
  SDL_Event e;
  int screen_dirty = 1;
  int quit = 0;
  while (1) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        goto exit_main_loop;
      else if (e.type == SDL_WINDOWEVENT)
        if (e.window.event != SDL_WINDOWEVENT_MOVED)
          screen_dirty = 1;

      printf("Event: %" PRIu32 "\n", e.type);
    }

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
  TTF_CloseFont(example_text_font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  puts("SDL cleaned up");

  return 0;
}
