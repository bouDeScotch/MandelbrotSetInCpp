#include <iostream>
#include "SDL2/SDL_main.h"
#include "SDL2/SDL.h"
#include <string>
#include <vector>
#include <thread>

#define HEX2RGBA(hex) ((hex >> 24) & 0xFF), ((hex >> 16) & 0xFF), ((hex >> 8) & 0xFF), (hex & 0xFF)


static SDL_bool sdl_quit = SDL_FALSE;
static SDL_Event sdl_event;
static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture *sdl_texture;

int width = 800;
int height = 600;
constexpr int MAX_ITERATIONS = 100;
constexpr uint32_t BG = 0x332222FF;
constexpr uint32_t FG = 0xFF5555FF;
double realx_min = -2.5;
double realx_max = 1.0;
double realy_min = -3.5/2;
double realy_max = 3.5/2;
bool is_mouse_down = false;
bool is_mouse_just_released = false;
int mouse_down_x = 0;
int mouse_down_y = 0;
int mouse_x = 0;
int mouse_y = 0;

std::vector<double> real_coords;
std::vector<double> imag_coords;

void compute_vectors(int width, int height, double min_x, double max_x, double min_y, double max_y) {
     real_coords.clear();
     imag_coords.clear();

     real_coords.resize(width);
     imag_coords.resize(height);
     for (int x = 0; x < width; x++) {
          real_coords[x] = (double)x / (double)width * (max_x - min_x) + min_x;
     }
     for (int y = 0; y < height; y++) {
          imag_coords[y] = (double)y / (double)height * (max_y - min_y) + min_y;
     }
}

static void sdl_process_event(SDL_Event *event) {
     switch (event->type) {
          case SDL_QUIT:
               sdl_quit = SDL_TRUE;
               break;
          // Detect when mouse is clicked
          case SDL_MOUSEBUTTONDOWN:
               is_mouse_down = true;
               mouse_down_x = event->button.x;
               mouse_down_y = event->button.y;
               SDL_Log("Click");
               break;
          // Detect when mouse is released
          case SDL_MOUSEBUTTONUP:
               is_mouse_down = false;
               is_mouse_just_released = true;
               SDL_Log("Release");
               break;
          // Detect when mouse is moving
          case SDL_MOUSEMOTION:
               mouse_x = event->motion.x;
               mouse_y = event->motion.y;
               break;
          case SDL_WINDOWEVENT:
               switch (event->window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                         width = event->window.data1;
                         height = event->window.data2;
                         SDL_Log("Window %d resized to %dx%d", event->window.windowID, width, height);
                         SDL_DestroyTexture(sdl_texture);
                         sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
                         compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);
                         break;
                    default: ;
               }
               break;
          default: ;
     }
}

static int sdl_fillrect(int x, int y, int w, int h, int color) {
     SDL_Rect rect = {x, y, w, h};
     SDL_SetRenderDrawColor(sdl_renderer, HEX2RGBA(color));
     return SDL_RenderFillRect(sdl_renderer, &rect);
}

double screen_to_real(int n, int limit, double min, double max) {
     return (double)n / (double)limit * (max - min) + min;
}

void compute_mandelbrot(int startx, int endx, int height, int pitch, uint32_t *pixel_buffer) {
     for (int x = startx; x < endx; x++) {
          for (int y = 0; y < height; y++) {
               double a = real_coords[x];
               double b = imag_coords[y];
               double ca = a;
               double cb = b;
               int n = 0;

               if ((a + 1) * (a + 1) + b * b < 0.0625) {
                    n = MAX_ITERATIONS;
               }
               while (a*a + b*b < 4 && n < MAX_ITERATIONS) {
                    double aa = a*a - b*b;
                    double bb = 2 * a * b;
                    a = aa + ca;
                    b = bb + cb;
                    n++;
               }

               // Colorize the pixel based of the const FG and BG
               int bright = 255 - (int)((double)n / (double)MAX_ITERATIONS * 255);
               int R = (bright * ((FG >> 24) & 0xFF) + (255 - bright) * ((BG >> 24) & 0xFF)) / 255;
               int G = (bright * ((FG >> 16) & 0xFF) + (255 - bright) * ((BG >> 16) & 0xFF)) / 255;
               int B = (bright * ((FG >> 8) & 0xFF) + (255 - bright) * ((BG >> 8) & 0xFF)) / 255;
               uint32_t color = (R << 24) | (G << 16) | (B << 8) | 0xFF;
               pixel_buffer[y * (pitch / 4) + x] = color;
          }
     }
}

int sign(int x) {
     return (x > 0) - (x < 0);
}

int main(int argc, char* argv[]) {
     SDL_Init(SDL_INIT_VIDEO);
     sdl_window = SDL_CreateWindow("Hello World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE);
     sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
     sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

     compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);

     while (!sdl_quit) {
          while (SDL_PollEvent(&sdl_event)) {
               sdl_process_event(&sdl_event);
          }

          SDL_SetRenderDrawColor(sdl_renderer, HEX2RGBA(0x000000FF));
          SDL_RenderClear(sdl_renderer);

          void* pixels = nullptr;
          int pitch = 0;
          SDL_LockTexture(sdl_texture, nullptr, &pixels, &pitch);


          // Time the process to compare number of threads which works the best
          auto start = std::chrono::high_resolution_clock::now();
          auto* pixel_buffer = static_cast<uint32_t*>(pixels);
          int num_threads = 16;
          std::vector<std::thread> threads;
          for (int i = 0; i < num_threads; i++) {
               int startx = i * width / num_threads;
               int endx = (i + 1) * width / num_threads;
               threads.push_back(std::thread(compute_mandelbrot, startx, endx, height, pitch, pixel_buffer));
          }

          for (auto& thread : threads) {
               thread.join();
          }

          auto end = std::chrono::high_resolution_clock::now();
          std::chrono::duration<double> elapsed = end - start;
          SDL_Log("Elapsed time: %f", elapsed.count() * 1000);

          SDL_UnlockTexture(sdl_texture);
          SDL_RenderClear(sdl_renderer);
          SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);

          if (is_mouse_down) {
               // Draw a rectangle showing the area selected by the user
               int x = mouse_down_x;
               int y = mouse_down_y;
               int w = mouse_x - mouse_down_x;
               int h = mouse_y - mouse_down_y;
               int side = std::max(std::abs(w), std::abs(h));
               sdl_fillrect(x, y, side * (sign(w)), side * (sign(h)), 0x4444DD22);
          }

          if (is_mouse_just_released) {
               int w = mouse_x - mouse_down_x;
               int h = mouse_y - mouse_down_y;
               int side = std::max(std::abs(w), std::abs(h));
               double oldx_min = realx_min;
               double oldx_max = realx_max;
               realx_min = screen_to_real(mouse_down_x, width, oldx_min, oldx_max);
               realx_max = screen_to_real(mouse_down_x + side * sign(w), width, oldx_min, oldx_max);
               if (realx_min > realx_max) {
                    std::swap(realx_min, realx_max);
               }
               double oldy_min = realy_min;
               double oldy_max = realy_max;
               realy_min = screen_to_real(mouse_down_y, height, oldy_min, oldy_max);
               realy_max = screen_to_real(mouse_down_y + side * sign(h), height, oldy_min, oldy_max);
               if (realy_min > realy_max) {
                    std::swap(realy_min, realy_max);
               }

               compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);
               is_mouse_just_released = false;
          }

          SDL_RenderPresent(sdl_renderer);
     }



     return 0;
}
