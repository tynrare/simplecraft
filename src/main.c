#include "app.h"
// agent: codex | 2026-09-06 | restore runtime host scaffold | c38f53
#include <raylib.h>
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

static const int VIEWPORT_W = 800;
static const int VIEWPORT_H = 600;
static int viewport_w = VIEWPORT_W;
static int viewport_h = VIEWPORT_H;
static double resize_timestamp = -1.0;
static const double resize_threshold = 0.3;
static Vector2 requested_viewport = {VIEWPORT_W, VIEWPORT_H};

/** Reconcile the logical viewport after a resize settles. @return void */
static void equilizer(void) {
  const int width = GetScreenWidth();
  const int height = GetScreenHeight();
  const double now = GetTime();
  if (requested_viewport.x != width || requested_viewport.y != height) {
    requested_viewport = (Vector2){width, height};
    resize_timestamp = now;
    return;
  }
  const bool resized = requested_viewport.x != viewport_w || requested_viewport.y != viewport_h;
  if (resized && now - resize_timestamp > resize_threshold) {
    viewport_w = width;
    viewport_h = height;
    resize_timestamp = now;
  }
}

/** Run one host frame around the game hooks. @return void */
static void step(void) {
  equilizer();
  app_inputs();
  if (WindowShouldClose()) return;
  BeginDrawing();
  app_draw();
  EndDrawing();
}

/** Run the native or web host loop. @return void */
static void loop(void) {
#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(step, 0, 1);
#else
  while (!WindowShouldClose()) step();
#endif
}

int main(void) {
  ChangeDirectory(GetApplicationDirectory());
  InitWindow(viewport_w, viewport_h, "simplecraft");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  SetRandomSeed(2);
  SetExitKey(0);
  app_init();
  cli_init();
  loop();
  cli_despose();
  app_dispose();
  if (IsWindowReady()) CloseWindow();
  return 0;
}
