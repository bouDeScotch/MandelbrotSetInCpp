#include "SDL2/SDL.h"
#include "SDL2/SDL_main.h"
#include <SDL2/SDL_video.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <omp.h>
#include <string>
#include <thread>
#include <vector>

#define HEX2RGBA(hex)                                                          \
  ((hex >> 24) & 0xFF), ((hex >> 16) & 0xFF), ((hex >> 8) & 0xFF), (hex & 0xFF)

static SDL_bool sdl_quit = SDL_FALSE;
static SDL_Event sdl_event;
static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;
static SDL_Texture *sdl_texture;

int width = 800;
int height = 600;
constexpr uint32_t BG = 0x332222FF;
constexpr uint32_t FG = 0xFF5555FF;
float realx_min = -2.5;
float realx_max = 1.0;
float realy_min = -3.5 / 2;
float realy_max = 3.5 / 2;
bool is_mouse_down = false;
bool is_mouse_just_released = false;
int mouse_down_x = 0;
int mouse_down_y = 0;
int mouse_x = 0;
int mouse_y = 0;

float scale = realx_max - realx_min;
int maxIter = std::clamp((int)(50 + 25 * std::log2(3.5f / scale)), 50, 2000);

bool frame_computed = false;

std::vector<float> real_coords;
std::vector<float> imag_coords;

void compute_vectors(int width, int height, float min_x, float max_x,
                     float min_y, float max_y) {
  real_coords.clear();
  imag_coords.clear();

  real_coords.resize(width);
  imag_coords.resize(height);
  for (int x = 0; x < width; x++) {
    real_coords[x] = (float)x / (float)width * (max_x - min_x) + min_x;
  }
  for (int y = 0; y < height; y++) {
    imag_coords[y] = (float)y / (float)height * (max_y - min_y) + min_y;
  }

  frame_computed = false;
  scale = realx_max - realx_min;
  maxIter = std::clamp((int)(50 + 25 * std::log2(3.5f / scale)), 50, 2000);
}

static void sdl_process_event(SDL_Event *event) {
  switch (event->type) {
  case SDL_QUIT:
    sdl_quit = SDL_TRUE;
    break;
  // Detect when mouse is clicked
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button != SDL_BUTTON_LEFT)
      break;
    is_mouse_down = true;
    mouse_down_x = event->button.x;
    mouse_down_y = event->button.y;
    SDL_Log("Click");
    break;
  // Detect when mouse is released
  case SDL_MOUSEBUTTONUP:
    if (event->button.button != SDL_BUTTON_LEFT)
      break;
    is_mouse_down = false;
    is_mouse_just_released = true;
    SDL_Log("Release");
    break;
  // Detect when mouse is moving
  case SDL_MOUSEMOTION:
    mouse_x = event->motion.x;
    mouse_y = event->motion.y;
    break;
  // Detect scrolling
  case SDL_MOUSEWHEEL:
    if (event->wheel.y > 0) {
      // Scroll up to zoom in
      float zoom_factor = 0.9f;
      float center_x = (realx_min + realx_max) / 2.0f;
      float center_y = (realy_min + realy_max) / 2.0f;
      float width_range = (realx_max - realx_min) * zoom_factor;
      float height_range = (realy_max - realy_min) * zoom_factor;
      realx_min = center_x - width_range / 2.0f;
      realx_max = center_x + width_range / 2.0f;
      realy_min = center_y - height_range / 2.0f;
      realy_max = center_y + height_range / 2.0f;
    } else if (event->wheel.y < 0) {
      // Scroll down to zoom out
      float zoom_factor = 1.1f;
      float center_x = (realx_min + realx_max) / 2.0f;
      float center_y = (realy_min + realy_max) / 2.0f;
      float width_range = (realx_max - realx_min) * zoom_factor;
      float height_range = (realy_max - realy_min) * zoom_factor;
      realx_min = center_x - width_range / 2.0f;
      realx_max = center_x + width_range / 2.0f;
      realy_min = center_y - height_range / 2.0f;
      realy_max = center_y + height_range / 2.0f;
    }
    compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);
    break;
  case SDL_WINDOWEVENT:
    switch (event->window.event) {
    case SDL_WINDOWEVENT_RESIZED:
      width = event->window.data1;
      height = event->window.data2;
      SDL_Log("Window %d resized to %dx%d", event->window.windowID, width,
              height);
      compute_vectors(width, height, realx_min, realx_max, realy_min,
                      realy_max);
      break;
    default:;
    }
    break;
  default:;
  }
}

static int sdl_fillrect(int x, int y, int w, int h, int color) {
  SDL_Rect rect = {x, y, w, h};
  SDL_SetRenderDrawColor(sdl_renderer, HEX2RGBA(color));
  return SDL_RenderFillRect(sdl_renderer, &rect);
}

float screen_to_real(int n, int limit, float min, float max) {
  return (float)n / (float)limit * (max - min) + min;
}

void compute_mandelbrot(int startx, int endx, int height, int pitch,
                        uint32_t *pixel_buffer) {
  for (int x = startx; x < endx; x++) {
    for (int y = 0; y < height; ++y) {
      float a = real_coords[x];
      float b = imag_coords[y];
      float ca = a;
      float cb = b;
      int n = 0;
      float q = (a - 0.25f) * (a - 0.25f) + b * b;

      // Test for period-2 bulb
      if ((a + 1) * (a + 1) + b * b < 0.0625) {
        n = maxIter;
      }
      // Test for main cardioid
      if (q * (q + a - 0.25f) < 0.25f * b * b) {
        n = maxIter;
      }
      while (a * a + b * b < 4 && n < maxIter) {
        float aa = a * a - b * b;
        float bb = 2 * a * b;
        a = aa + ca;
        b = bb + cb;
        n++;
      }

      // Colorize the pixel based of the const FG and BG
      int bright = 255 - (int)((float)n / maxIter * 255);
      int R = (bright * ((FG >> 24) & 0xFF) +
               (255 - bright) * ((BG >> 24) & 0xFF)) /
              255;
      int G = (bright * ((FG >> 16) & 0xFF) +
               (255 - bright) * ((BG >> 16) & 0xFF)) /
              255;
      int B =
          (bright * ((FG >> 8) & 0xFF) + (255 - bright) * ((BG >> 8) & 0xFF)) /
          255;
      uint32_t color = (R << 24) | (G << 16) | (B << 8) | 0xFF;
      pixel_buffer[y * (pitch / 4) + x] = color;
    }
  }
}

int sign(int x) { return (x > 0) - (x < 0); }

void configure_vertices(GLuint &vao, GLuint &vbo) {
  float vertices[] = {// positions   // texcoords
                      -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,
                      -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f};
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

GLuint compileShader(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);

  int success;
  char infoLog[512];
  glGetShaderiv(s, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(s, 512, nullptr, infoLog);
    std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
  }

  return s;
}

void handle_resize(float windowRatio) {
  // 1) rectangle brut en pixels
  int x0 = std::min(mouse_down_x, mouse_x);
  int x1 = std::max(mouse_down_x, mouse_x);
  int y0 = std::min(mouse_down_y, mouse_y);
  int y1 = std::max(mouse_down_y, mouse_y);

  // 2) ajuster au ratio de la fenêtre
  float sel_w = (float)(x1 - x0);
  float sel_h = (float)(y1 - y0);
  if (sel_w <= 1 || sel_h <= 1) { // clic trop petit
    is_mouse_just_released = false;
    return;
  }

  float sel_ratio = sel_w / sel_h;
  if (sel_ratio > windowRatio) {
    // trop large → augmenter la hauteur
    float new_h = sel_w / windowRatio;
    float delta = new_h - sel_h;
    // étendre vers le bas/haut selon le sens du drag
    if (mouse_y >= mouse_down_y) {
      y1 = std::min(height, (int)std::lround(y0 + new_h));
    } else {
      y0 = std::max(0, (int)std::lround(y1 - new_h));
    }
  } else if (sel_ratio < windowRatio) {
    // trop haut → augmenter la largeur
    float new_w = sel_h * windowRatio;
    float delta = new_w - sel_w;
    if (mouse_x >= mouse_down_x) {
      x1 = std::min(width, (int)std::lround(x0 + new_w));
    } else {
      x0 = std::max(0, (int)std::lround(x1 - new_w));
    }
  }

  // 3) pixels → [0..1]
  float nx0 = x0 / (float)width;
  float nx1 = x1 / (float)width;

  // ATTENTION: Y écran 0=haut → UV 0=bas → on FLIPPE
  // le pixel y0 (haut) doit correspondre à uv_y1, et y1 (bas) à uv_y0
  float ny_top = 1.0f - (y0 / (float)height);    // uv en haut du rect
  float ny_bottom = 1.0f - (y1 / (float)height); // uv en bas du rect

  // 4) [0..1] → coords fractales
  float oldx_min = realx_min, oldx_max = realx_max;
  float oldy_min = realy_min, oldy_max = realy_max;

  float new_xmin = lerp(oldx_min, oldx_max, nx0);
  float new_xmax = lerp(oldx_min, oldx_max, nx1);
  float new_ymin = lerp(oldy_min, oldy_max, ny_bottom); // bas
  float new_ymax = lerp(oldy_min, oldy_max, ny_top);    // haut

  // 5) assignation (au cas où)
  if (new_xmin > new_xmax)
    std::swap(new_xmin, new_xmax);
  if (new_ymin > new_ymax)
    std::swap(new_ymin, new_ymax);

  realx_min = new_xmin;
  realx_max = new_xmax;
  realy_min = new_ymin;
  realy_max = new_ymax;

  compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);
  is_mouse_just_released = false;
}

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  sdl_window = SDL_CreateWindow("Hello World", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, 800, 600,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

  sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
  sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, width, height);

  SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
  SDL_GL_SetSwapInterval(0); // Enable/disable vsync

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  glViewport(0, 0, width, height);

  GLuint vao, vbo;
  configure_vertices(vao, vbo);

  compute_vectors(width, height, realx_min, realx_max, realy_min, realy_max);

  float total_time = 0.0f;
  int frame_count = 0;
  int num_threads = 64;

  // Get vertex and fragment shader source code from files
  std::string vertexShaderSource = "";
  std::string fragmentShaderSource = "";
  std::string vertexPath = "mandel.vert";
  std::string fragmentPath = "mandel.frag";

  std::ifstream vertexFile(vertexPath);
  if (vertexFile.is_open()) {
    std::string line;
    while (std::getline(vertexFile, line)) {
      vertexShaderSource += line + "\n";
    }
    vertexFile.close();
  } else {
    std::cerr << "Failed to open vertex shader file: " << vertexPath
              << std::endl;
  }

  std::ifstream fragmentFile(fragmentPath);
  if (fragmentFile.is_open()) {
    std::string line;
    while (std::getline(fragmentFile, line)) {
      fragmentShaderSource += line + "\n";
    }
    fragmentFile.close();
  } else {
    std::cerr << "Failed to open fragment shader file: " << fragmentPath
              << std::endl;
  }

  // For debugging: print the shader sources
  std::cout << "Vertex Shader Source:\n" << vertexShaderSource << std::endl;
  std::cout << "Fragment Shader Source:\n" << fragmentShaderSource << std::endl;

  GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource.c_str());
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource.c_str());
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vs);
  glAttachShader(shaderProgram, fs);
  glLinkProgram(shaderProgram);
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
  }
  glUseProgram(shaderProgram);

  GLint loc_u_min = glGetUniformLocation(shaderProgram, "u_min");
  GLint loc_u_max = glGetUniformLocation(shaderProgram, "u_max");
  GLint loc_u_maxIter = glGetUniformLocation(shaderProgram, "u_maxIter");

  while (!sdl_quit) {
    glViewport(0, 0, width, height);
    // Event processing
    while (SDL_PollEvent(&sdl_event)) {
      sdl_process_event(&sdl_event);
    }

    SDL_SetRenderDrawColor(sdl_renderer, HEX2RGBA(0x000000FF));
    SDL_RenderClear(sdl_renderer);

    void *pixels = nullptr;
    int pitch = 0;
    SDL_LockTexture(sdl_texture, nullptr, &pixels, &pitch);

    // GPU Version
    auto start = std::chrono::high_resolution_clock::now();

    glUseProgram(shaderProgram);
    glUniform2f(loc_u_min, realx_min, realy_min);
    glUniform2f(loc_u_max, realx_max, realy_max);
    glUniform1i(loc_u_maxIter, maxIter);

    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SDL_GL_SwapWindow(sdl_window);
    float deltaTime = std::chrono::duration<float>(
                          std::chrono::high_resolution_clock::now() - start)
                          .count();
    total_time += deltaTime;
    frame_count++;

    float windowRatio = (float)width / height;
    if (is_mouse_down) {
      // Draw a rectangle showing the area selected by the user
      int x = mouse_down_x;
      int y = mouse_down_y;
      int w = mouse_x - mouse_down_x;
      int h = mouse_y - mouse_down_y;
      int side = std::max(std::abs(w), std::abs(h));
    }

    windowRatio = (float)width / (float)height;

    if (is_mouse_just_released) {
      handle_resize(windowRatio);
    }

    // SDL_RenderPresent(sdl_renderer);
  }

  printf("Average frame time: %.3f ms\n", (total_time / frame_count) * 1000.0f);
  printf("FPS: %.2f\n", (float)frame_count / total_time);
  // printf("Number of threads: %d\n", num_threads);
  return 0;
}
