#include "app.h"
// agent: codex | 2026-09-06 | restore cli exit prompt | 6838bf
#include <raylib.h>
#include <stdbool.h>
#include <string.h>

typedef enum CliMode { CLI_MODE_NAV, CLI_MODE_EXIT } CliMode;
static bool active;
static CliMode mode;
static int selected;
static char input[128];

/** Set the current CLI text. @param text replacement text @return void */
static void cli_set_input(const char *text) {
  strncpy(input, text, sizeof(input) - 1);
  input[sizeof(input) - 1] = '\0';
}

/** Initialize CLI state. @return void */
void cli_init(void) {
  active = false;
  mode = CLI_MODE_NAV;
  selected = 0;
  cli_set_input("");
}

/** Process CLI keyboard input, including the exit prompt. @return whether CLI consumed input */
bool cli_input(void) {
  if (IsKeyPressed(KEY_ESCAPE) && !active) {
    active = true;
    mode = CLI_MODE_EXIT;
    selected = 0;
    cli_set_input("Y");
    return true;
  }
  if (IsKeyPressed(KEY_TAB) && mode == CLI_MODE_NAV) active = !active;
  if (!active) return false;

  if (IsKeyPressed(KEY_ESCAPE)) {
    mode = CLI_MODE_EXIT;
    selected = 0;
    cli_set_input("Y");
    return true;
  }
  if (mode == CLI_MODE_EXIT) {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
      selected = !selected;
      cli_set_input(selected ? "N" : "Y");
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (selected == 0) CloseWindow();
      else {
        mode = CLI_MODE_NAV;
        cli_set_input("");
      }
    }
    return true;
  }

  size_t length = strlen(input);
  int key = GetCharPressed();
  while (key > 0) {
    if (key >= 32 && key <= 126 && length < sizeof(input) - 1) input[length++] = (char)key;
    key = GetCharPressed();
  }
  input[length] = '\0';
  if (IsKeyPressed(KEY_BACKSPACE) && length > 0) input[length - 1] = '\0';
  return true;
}

/** Draw the CLI overlay and exit confirmation. @return void */
void cli_draw(void) {
  if (!active) return;
  DrawRectangle(0, 0, GetScreenWidth(), 88, (Color){0, 0, 0, 220});
  DrawRectangleLines(0, 0, GetScreenWidth(), 88, GREEN);
  if (mode == CLI_MODE_EXIT) {
    DrawText("exit?", 8, 8, 18, WHITE);
    DrawText(selected == 0 ? "[Y]  N" : "Y  [N]", 8, 40, 22, WHITE);
    DrawText("Left/Right select, Enter confirm", 8, 66, 14, LIGHTGRAY);
    return;
  }
  DrawText("cli (Tab toggles, Esc prompts exit)", 8, 8, 16, WHITE);
  DrawText(TextFormat("> %s", input), 8, 40, 20, WHITE);
}

/** Release CLI state. @return void */
void cli_despose(void) { active = false; }
