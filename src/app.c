#define XML_H_IMPLEMENTATION
#include "vendor/xml.h"
// agent: codex | 2026-09-06 | add spawn inventory selection | 5712f9
#include "app.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEM_COUNT 24
#define STATION_COUNT 8
#define RECIPE_COUNT 16
#define RECIPE_INPUT_MAX 4
#define RECIPE_OUTPUT_MAX 2
#define ITEM_NAME_MAX 64
#define LABEL_SIZE 20
#define LABEL_PAD_X 8
#define LABEL_PAD_Y 5

static char item_names[ITEM_COUNT][ITEM_NAME_MAX];
static char item_labels[ITEM_COUNT][ITEM_NAME_MAX];
static char item_groups[ITEM_COUNT][ITEM_NAME_MAX];
static int item_ids[ITEM_COUNT];
static int item_count;
static char station_names[STATION_COUNT][ITEM_NAME_MAX];
static char station_labels[STATION_COUNT][ITEM_NAME_MAX];
static int station_ids[STATION_COUNT];
static int station_count;
static int inventory[ITEM_COUNT];
static int surroundings[STATION_COUNT];
typedef struct Recipe {
  int id;
  char name[ITEM_NAME_MAX];
  char label[ITEM_NAME_MAX];
  int input_ids[RECIPE_INPUT_MAX];
  char input_groups[RECIPE_INPUT_MAX][ITEM_NAME_MAX];
  bool input_is_group[RECIPE_INPUT_MAX];
  int input_amounts[RECIPE_INPUT_MAX];
  int input_count;
  int station_ids[STATION_COUNT];
  int recipe_station_count;
  int output_ids[RECIPE_OUTPUT_MAX];
  int output_amounts[RECIPE_OUTPUT_MAX];
  int output_count;
} Recipe;
static Recipe recipes[RECIPE_COUNT];
static int recipe_count;

//==============================================================================
// [SIMPLECRAFT] public inventory and section operations
//==============================================================================

/** Return an array entry count. @param values inventory array @param count array length @param index entry index @return count or 0 */
int simplecraft_array_has(const int *values, int count, int index) {
  return values && index >= 0 && index < count ? values[index] : 0;
}

/** Find the first non-empty array entry. @param values inventory array @param count array length @return entry index or -1 */
int simplecraft_array_find(const int *values, int count) {
  if (!values) return -1;
  for (int i = 0; i < count; i++) if (values[i] > 0) return i;
  return -1;
}

/** Add to an array entry. @param values inventory array @param count array length @param index entry index @param amount positive amount @return success */
bool simplecraft_array_add(int *values, int count, int index, int amount) {
  if (!values || index < 0 || index >= count || amount <= 0) return false;
  values[index] += amount;
  return true;
}

/** Remove from an array entry. @param values inventory array @param count array length @param index entry index @param amount positive amount @return success */
bool simplecraft_array_remove(int *values, int count, int index, int amount) {
  if (!values || index < 0 || index >= count || amount <= 0 || values[index] < amount) return false;
  values[index] -= amount;
  return true;
}

/** Find an item by name. @param name item name @return item id or -1 */
int simplecraft_item_find(const char *name) {
  if (!name) return -1;
  for (int i = 0; i < item_count; i++) if (strcmp(item_names[i], name) == 0) return item_ids[i];
  return -1;
}

/** Resolve an item id to an array index. @param item_id database id @return index or -1 */
static int item_index(int item_id) {
  for (int i = 0; i < item_count; i++) if (item_ids[i] == item_id) return i;
  return -1;
}

/** Return an inventory count. @param item_id item id @return count or 0 */
int simplecraft_inventory_has(int item_id) {
  return simplecraft_array_has(inventory, item_count, item_index(item_id));
}

/** Add inventory items. @param item_id item id @param amount positive amount @return success */
bool simplecraft_inventory_add(int item_id, int amount) {
  return simplecraft_array_add(inventory, item_count, item_index(item_id), amount);
}

/** Remove inventory items. @param item_id item id @param amount positive amount @return success */
bool simplecraft_inventory_remove(int item_id, int amount) {
  return simplecraft_array_remove(inventory, item_count, item_index(item_id), amount);
}

/** Find a station by name. @param name station name @return station id or -1 */
int simplecraft_station_find(const char *name) {
  if (!name) return -1;
  for (int i = 0; i < station_count; i++) if (strcmp(station_names[i], name) == 0) return station_ids[i];
  return -1;
}

/** Resolve a station id to an array index. @param station_id database id @return index or -1 */
static int station_index(int station_id) {
  for (int i = 0; i < station_count; i++) if (station_ids[i] == station_id) return i;
  return -1;
}

/** Return whether a station is in surroundings. @param station_id station id @return true when enabled */
bool simplecraft_surroundings_has(int station_id) {
  return simplecraft_array_has(surroundings, station_count, station_index(station_id)) > 0;
}

/** Add a station to surroundings. @param station_id station id @return success */
bool simplecraft_surroundings_add(int station_id) {
  return simplecraft_array_add(surroundings, station_count, station_index(station_id), 1);
}

/** Remove a station from surroundings. @param station_id station id @return success */
bool simplecraft_surroundings_remove(int station_id) {
  return simplecraft_array_remove(surroundings, station_count, station_index(station_id), 1);
}

/** Sum inventory for an item group. @param group group identity @return total count */
int simplecraft_group_has(const char *group) {
  int total = 0;
  if (!group) return 0;
  for (int i = 0; i < item_count; i++) if (strcmp(item_groups[i], group) == 0) total += simplecraft_inventory_has(item_ids[i]);
  return total;
}

/** Remove inventory from any members of an item group. @param group group identity @param amount positive amount @return success */
bool simplecraft_group_remove(const char *group, int amount) {
  if (!group || amount <= 0 || simplecraft_group_has(group) < amount) return false;
  for (int i = 0; i < item_count && amount > 0; i++) {
    if (strcmp(item_groups[i], group) != 0) continue;
    int available = simplecraft_inventory_has(item_ids[i]);
    int removed = available < amount ? available : amount;
    if (removed > 0) {
      simplecraft_inventory_remove(item_ids[i], removed);
      amount -= removed;
    }
  }
  return true;
}

/** Find a recipe by name. @param name recipe identity @return recipe id or -1 */
int simplecraft_recipe_find(const char *name) {
  if (!name) return -1;
  for (int i = 0; i < recipe_count; i++) if (strcmp(recipes[i].name, name) == 0) return recipes[i].id;
  return -1;
}

/** Resolve a recipe id to an array index. @param recipe_id database id @return index or -1 */
static int recipe_index(int recipe_id) {
  for (int i = 0; i < recipe_count; i++) if (recipes[i].id == recipe_id) return i;
  return -1;
}

/** Return whether a recipe can be crafted. @param recipe_id database id @return true when craftable */
bool simplecraft_recipe_can_craft(int recipe_id) {
  int index = recipe_index(recipe_id);
  if (index < 0) return false;
  Recipe *recipe = &recipes[index];
  for (int i = 0; i < recipe->input_count; i++) {
    int available = recipe->input_is_group[i] ? simplecraft_group_has(recipe->input_groups[i]) : simplecraft_inventory_has(recipe->input_ids[i]);
    if (available < recipe->input_amounts[i]) return false;
  }
  for (int i = 0; i < recipe->recipe_station_count; i++) if (!simplecraft_surroundings_has(recipe->station_ids[i])) return false;
  return recipe->output_count > 0;
}

/** Craft a recipe by consuming inputs and adding outputs. @param recipe_id database id @return success */
bool simplecraft_recipe_craft(int recipe_id) {
  int index = recipe_index(recipe_id);
  if (index < 0 || !simplecraft_recipe_can_craft(recipe_id)) return false;
  Recipe *recipe = &recipes[index];
  for (int i = 0; i < recipe->input_count; i++) {
    if (recipe->input_is_group[i]) simplecraft_group_remove(recipe->input_groups[i], recipe->input_amounts[i]);
    else simplecraft_inventory_remove(recipe->input_ids[i], recipe->input_amounts[i]);
  }
  for (int i = 0; i < recipe->output_count; i++) simplecraft_inventory_add(recipe->output_ids[i], recipe->output_amounts[i]);
  return true;
}

//==============================================================================
// [BACKEND] database and persistent game state
//==============================================================================

static void load_recipes(XMLNode *section);

/** Load crafting item names into game state. @return void */
void app_init(void) {
  XMLNode *document_root = xml_parse_file("res/items.xml");
  if (!document_root) {
    TraceLog(LOG_ERROR, "simplecraft: failed to load res/items.xml");
    return;
  }
  for (int section_index = 0; section_index < 2; section_index++) {
    bool is_items = section_index == 0;
    XMLNode *section = xml_node_find_tag(document_root, is_items ? "items" : "stations", true);
    if (!section) continue;
    for (size_t i = 0; i < section->children->len; i++) {
      XMLNode *entry = xml_node_child_at(section, i);
      const char *name = entry ? xml_node_attr(entry, "name") : NULL;
      const char *label = entry ? xml_node_attr(entry, "label") : NULL;
      const char *group = entry ? xml_node_attr(entry, "group") : NULL;
      const char *id_text = entry ? xml_node_attr(entry, "id") : NULL;
      if (!name || !label || !id_text) continue;
      if (is_items && simplecraft_item_find(name) >= 0) continue;
      if (!is_items && simplecraft_station_find(name) >= 0) continue;
      char (*names)[ITEM_NAME_MAX] = is_items ? item_names : station_names;
      int *count = is_items ? &item_count : &station_count;
      int limit = is_items ? ITEM_COUNT : STATION_COUNT;
      if (*count >= limit) continue;
      strncpy(names[*count], name, ITEM_NAME_MAX - 1);
      names[(*count)++][ITEM_NAME_MAX - 1] = '\0';
      if (is_items) {
        int index = *count - 1;
        item_ids[index] = atoi(id_text);
        strncpy(item_groups[index], group ? group : "", ITEM_NAME_MAX - 1);
        item_groups[index][ITEM_NAME_MAX - 1] = '\0';
        strncpy(item_labels[index], label, ITEM_NAME_MAX - 1);
        item_labels[index][ITEM_NAME_MAX - 1] = '\0';
      } else {
        int index = *count - 1;
        station_ids[index] = atoi(id_text);
        strncpy(station_labels[index], label, ITEM_NAME_MAX - 1);
        station_labels[index][ITEM_NAME_MAX - 1] = '\0';
      }
      printf("%s: %s\n", is_items ? "item" : "station", name);
    }
  }
  load_recipes(xml_node_find_tag(document_root, "recipes", true));
  if (item_count == 0 || station_count == 0) {
    TraceLog(LOG_WARNING, "simplecraft: item or station database section is empty");
  }
  xml_node_free(document_root);
}

/** Load recipe children from the XML database. @param section recipe XML section @return void */
static void load_recipes(XMLNode *section) {
  if (!section) return;
  for (size_t i = 0; i < section->children->len && recipe_count < RECIPE_COUNT; i++) {
    XMLNode *node = xml_node_child_at(section, i);
    const char *id_text = node ? xml_node_attr(node, "id") : NULL;
    const char *name = node ? xml_node_attr(node, "name") : NULL;
    const char *label = node ? xml_node_attr(node, "label") : NULL;
    if (!id_text || !name || !label) continue;
    Recipe *recipe = &recipes[recipe_count++];
    recipe->id = atoi(id_text);
    strncpy(recipe->name, name, ITEM_NAME_MAX - 1);
    strncpy(recipe->label, label, ITEM_NAME_MAX - 1);
    for (size_t child_index = 0; child_index < node->children->len; child_index++) {
      XMLNode *child = xml_node_child_at(node, child_index);
      if (!child || !child->tag) continue;
      const char *group = xml_node_attr(child, "group");
      const char *ref = group ? group : xml_node_attr(child, child->tag[0] == 's' ? "name" : "item");
      const char *amount_text = xml_node_attr(child, "amount");
      int amount = amount_text ? atoi(amount_text) : 1;
      if (!ref || amount <= 0) continue;
      if (strcmp(child->tag, "input") == 0 && recipe->input_count < RECIPE_INPUT_MAX) {
        int input = recipe->input_count++;
        recipe->input_is_group[input] = group != NULL;
        if (group) strncpy(recipe->input_groups[input], ref, ITEM_NAME_MAX - 1);
        else recipe->input_ids[input] = simplecraft_item_find(ref);
        recipe->input_amounts[input] = amount;
      } else if (strcmp(child->tag, "station") == 0 && recipe->recipe_station_count < STATION_COUNT) {
        recipe->station_ids[recipe->recipe_station_count++] = simplecraft_station_find(ref);
      } else if (strcmp(child->tag, "output") == 0 && recipe->output_count < RECIPE_OUTPUT_MAX) {
        recipe->output_ids[recipe->output_count] = simplecraft_item_find(ref);
        recipe->output_amounts[recipe->output_count++] = amount;
      }
    }
  }
}

/** Return the number of loaded items. @return item count */
int app_item_count(void) { return item_count; }
/** Return an item name by index. @param index item index @return name or NULL */
const char *app_item_name(int index) { return index >= 0 && index < item_count ? item_names[index] : NULL; }
/** Release game state. @return void */
void app_dispose(void) { item_count = 0; }

//==============================================================================
// [FRONTEND] layout, navigation, input, and drawing
//==============================================================================

static Rectangle spawn_rects[ITEM_COUNT];
static Rectangle inventory_rects[ITEM_COUNT];
static int inventory_rect_count;
static int inventory_title_y;
static int selected_item;
static bool inventory_selected;
static Rectangle spawn_section;
static Rectangle inventory_section;
static Rectangle station_rects[STATION_COUNT];
static Rectangle station_section;
static int station_title_y;
static bool station_selected;
static int selected_station;
static bool surroundings_selected;
static Rectangle recipe_rects[RECIPE_COUNT];
static int recipe_visible[RECIPE_COUNT];
static int recipe_visible_count;
static int recipe_title_y;
static Rectangle recipe_section;
static int selected_recipe;
typedef enum SelectionKind {
  SELECTION_SPAWN,
  SELECTION_INVENTORY,
  SELECTION_STATION,
  SELECTION_SURROUNDINGS,
  SELECTION_RECIPE
} SelectionKind;
static SelectionKind selection_kind = SELECTION_SPAWN;
static bool has_surroundings(void);
static void select_first_surrounding(void);
static void select_label(SelectionKind kind, int index);

/** Return whether a recipe uses an item. @param recipe recipe data @param item_id database id @return true when used */
static bool recipe_uses_item(const Recipe *recipe, int item_id) {
  for (int i = 0; i < recipe->input_count; i++) if (recipe->input_ids[i] == item_id) return true;
  int index = item_index(item_id);
  for (int i = 0; i < recipe->input_count; i++) if (index >= 0 && recipe->input_is_group[i] && strcmp(item_groups[index], recipe->input_groups[i]) == 0) return true;
  for (int i = 0; i < recipe->output_count; i++) if (recipe->output_ids[i] == item_id) return true;
  return false;
}

/** Return required input amount for an item. @param recipe recipe data @param item_id database id @return amount or 0 */
static int recipe_input_amount(const Recipe *recipe, int item_id) {
  int index = item_index(item_id);
  for (int i = 0; i < recipe->input_count; i++) {
    if (recipe->input_ids[i] == item_id) return recipe->input_amounts[i];
    if (index >= 0 && recipe->input_is_group[i] && strcmp(item_groups[index], recipe->input_groups[i]) == 0) return recipe->input_amounts[i];
  }
  return 0;
}

/** Return whether an item's recipe requirement is satisfied. @param recipe recipe data @param item_id database id @return true when sufficient */
static bool recipe_input_sufficient(const Recipe *recipe, int item_id) {
  int index = item_index(item_id);
  for (int i = 0; i < recipe->input_count; i++) {
    if (recipe->input_ids[i] == item_id) return simplecraft_inventory_has(item_id) >= recipe->input_amounts[i];
    if (index >= 0 && recipe->input_is_group[i] && strcmp(item_groups[index], recipe->input_groups[i]) == 0) {
      return simplecraft_group_has(recipe->input_groups[i]) >= recipe->input_amounts[i];
    }
  }
  return true;
}

/** Return output amount for an item. @param recipe recipe data @param item_id database id @return amount or 0 */
static int recipe_output_amount(const Recipe *recipe, int item_id) {
  for (int i = 0; i < recipe->output_count; i++) if (recipe->output_ids[i] == item_id) return recipe->output_amounts[i];
  return 0;
}

/** Return whether a recipe uses a station. @param recipe recipe data @param station_id database id @return true when used */
static bool recipe_uses_station(const Recipe *recipe, int station_id) {
  for (int i = 0; i < recipe->recipe_station_count; i++) if (recipe->station_ids[i] == station_id) return true;
  return false;
}

/** Place craftable recipes in wrapped label rectangles. @param y top position @return bottom edge */
static int layout_recipes(int y) {
  y += LABEL_SIZE + 8;
  int x = 24;
  int row_height = LABEL_SIZE + LABEL_PAD_Y * 2 + 6;
  int width = GetScreenWidth() - 24;
  recipe_visible_count = 0;
  for (int i = 0; i < recipe_count; i++) {
    int label_width = MeasureText(recipes[i].label, LABEL_SIZE) + LABEL_PAD_X * 2;
    if (x != 24 && x + label_width > width) {
      x = 24;
      y += row_height;
    }
    recipe_rects[i] = (Rectangle){x, y, label_width, row_height};
    recipe_visible[recipe_visible_count++] = i;
    x += label_width + 8;
  }
  return recipe_visible_count ? y + row_height : y + LABEL_SIZE;
}

/** Place wrapped labels in a section and return its bottom edge. @param title section title @param rects output rectangles @param counts label count @param y top position @param inventory_labels whether to show counts @return bottom edge */
static int layout_labels(char names[][ITEM_NAME_MAX], const int *ids, int count, Rectangle *rects, int y, bool inventory_labels) {
  y += LABEL_SIZE + 8;
  int x = 24;
  int row_height = LABEL_SIZE + LABEL_PAD_Y * 2 + 6;
  int width = GetScreenWidth() - 24;
  int visible = 0;
  for (int i = 0; i < count; i++) {
    int amount = inventory_labels ? simplecraft_inventory_has(ids[i]) : 0;
    if (inventory_labels && amount <= 0) continue;
    const char *label = inventory_labels ? TextFormat("%s (x%d)", names[i], amount) : names[i];
    int label_width = MeasureText(label, LABEL_SIZE) + LABEL_PAD_X * 2;
    if (x != 24 && x + label_width > width) {
      x = 24;
      y += row_height;
    }
    rects[i] = (Rectangle){x, y, label_width, row_height};
    x += label_width + 8;
    visible++;
  }
  return visible ? y + row_height : y + LABEL_SIZE;
}

/** Rebuild spawn and inventory hit rectangles. @return void */
static void layout(void) {
  int bottom = layout_labels(item_labels, item_ids, item_count, spawn_rects, 58, false);
  spawn_section = (Rectangle){16, 50, GetScreenWidth() - 32, bottom - 42};
  inventory_title_y = bottom + 18;
  int inventory_bottom = layout_labels(item_labels, item_ids, item_count, inventory_rects, inventory_title_y, true);
  inventory_section = (Rectangle){16, inventory_title_y - 8, GetScreenWidth() - 32, inventory_bottom - inventory_title_y + 24};
  inventory_rect_count = 0;
  for (int i = 0; i < item_count; i++) inventory_rect_count += simplecraft_inventory_has(item_ids[i]) > 0;
  station_title_y = inventory_bottom + 18;
  int station_bottom = layout_labels(station_labels, station_ids, station_count, station_rects, station_title_y, false);
  station_section = (Rectangle){16, station_title_y - 8, GetScreenWidth() - 32, station_bottom - station_title_y + 24};
  recipe_title_y = station_bottom + 18;
  int recipe_bottom = layout_recipes(recipe_title_y);
  recipe_section = (Rectangle){16, recipe_title_y - 8, GetScreenWidth() - 32, recipe_bottom - recipe_title_y + 24};
}

/** Activate the currently selected item. @return void */
static void activate_selected(void) {
  if (selection_kind == SELECTION_RECIPE) {
    simplecraft_recipe_craft(recipes[selected_recipe].id);
    return;
  }
  if (selection_kind == SELECTION_SURROUNDINGS) {
    simplecraft_surroundings_remove(station_ids[selected_station]);
    select_label(SELECTION_STATION, selected_station);
    return;
  }
  if (selection_kind == SELECTION_STATION) {
    if (simplecraft_surroundings_has(station_ids[selected_station])) simplecraft_surroundings_remove(station_ids[selected_station]);
    else simplecraft_surroundings_add(station_ids[selected_station]);
    return;
  }
  if (selection_kind == SELECTION_SPAWN) {
    simplecraft_inventory_add(item_ids[selected_item], 1);
    return;
  }
  if (selection_kind == SELECTION_INVENTORY && simplecraft_inventory_has(item_ids[selected_item]) > 0) {
    int removed = selected_item;
    simplecraft_inventory_remove(item_ids[selected_item], 1);
    if (simplecraft_inventory_has(item_ids[selected_item]) > 0) return;
    for (int step = 1; step <= item_count; step++) {
      int next = (removed + step) % item_count;
      if (simplecraft_inventory_has(item_ids[next]) > 0) {
        selected_item = next;
        return;
      }
    }
    select_label(SELECTION_SPAWN, removed);
  }
}

/** Set the one active global selection. @param kind label group @param index group index @return void */
static void select_label(SelectionKind kind, int index) {
  selection_kind = kind;
  inventory_selected = kind == SELECTION_INVENTORY;
  station_selected = kind == SELECTION_STATION;
  surroundings_selected = kind == SELECTION_SURROUNDINGS;
  if (kind == SELECTION_SPAWN || kind == SELECTION_INVENTORY) selected_item = index;
  else if (kind == SELECTION_STATION || kind == SELECTION_SURROUNDINGS) selected_station = index;
  else selected_recipe = index;
}

/** Return the rectangle for the active global selection. @return selection rectangle */
static Rectangle selected_rect(void) {
  if (selection_kind == SELECTION_SPAWN) return spawn_rects[selected_item];
  if (selection_kind == SELECTION_INVENTORY) return inventory_rects[selected_item];
  if (selection_kind == SELECTION_RECIPE) return recipe_rects[selected_recipe];
  return station_rects[selected_station];
}

/** Choose the closest visible label in one arrow direction. @param dx horizontal direction @param dy vertical direction @return void */
static void move_global(int dx, int dy) {
  Rectangle current = selected_rect();
  float current_x = current.x + current.width * 0.5f;
  float current_y = current.y + current.height * 0.5f;
  float best_score = 1000000.0f;
  SelectionKind best_kind = selection_kind;
  int best_index = -1;
  for (int group = 0; group < 5; group++) {
    int count = group < 2 ? item_count : (group == 4 ? recipe_visible_count : station_count);
    for (int i = 0; i < count; i++) {
      if (group == 1 && simplecraft_inventory_has(item_ids[i]) <= 0) continue;
      if (group == 3 && !simplecraft_surroundings_has(station_ids[i])) continue;
      int index = group == 4 ? recipe_visible[i] : i;
      Rectangle rect = group < 2 ? (group == 0 ? spawn_rects[index] : inventory_rects[index]) : (group == 4 ? recipe_rects[index] : station_rects[index]);
      float x = rect.x + rect.width * 0.5f;
      float y = rect.y + rect.height * 0.5f;
      float primary = dx * (x - current_x) + dy * (y - current_y);
      float orthogonal = fabsf(-dy * (x - current_x) + dx * (y - current_y));
      if (primary <= 0.0f || (primary + orthogonal) < 1.0f) continue;
      if (dx != 0 && orthogonal > current.height * 0.75f) continue;
      float score = primary + orthogonal * 0.7f;
      if (score < best_score) {
        best_score = score;
        best_kind = (SelectionKind)group;
        best_index = index;
      }
    }
  }
  if (best_index >= 0) {
    select_label(best_kind, best_index);
    return;
  }
  if (selection_kind == SELECTION_STATION && dy > 0 && has_surroundings()) {
    select_first_surrounding();
    select_label(SELECTION_SURROUNDINGS, selected_station);
  } else if (selection_kind == SELECTION_SURROUNDINGS && dy < 0) {
    select_label(SELECTION_STATION, selected_station);
  }
}

/** Return whether one station is in the invisible surroundings list. @return true when non-empty */
static bool has_surroundings(void) {
  return simplecraft_array_find(surroundings, station_count) >= 0;
}

/** Select the first enabled station in surroundings. @return void */
static void select_first_surrounding(void) {
  int first = simplecraft_array_find(surroundings, station_count);
  if (first >= 0) selected_station = first;
}

/** Process keyboard and mouse game input without managing the host window. @return void */
void app_inputs(void) {
  if (cli_input()) return;
  layout();
  if (IsKeyPressed(KEY_UP)) move_global(0, -1);
  if (IsKeyPressed(KEY_DOWN)) move_global(0, 1);
  if (IsKeyPressed(KEY_LEFT)) move_global(-1, 0);
  if (IsKeyPressed(KEY_RIGHT)) move_global(1, 0);
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) activate_selected();

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < item_count; i++) {
    if (CheckCollisionPointRec(mouse, spawn_rects[i])) {
      select_label(SELECTION_SPAWN, i);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activate_selected();
      return;
    }
    if (simplecraft_inventory_has(item_ids[i]) > 0 && CheckCollisionPointRec(mouse, inventory_rects[i])) {
      select_label(SELECTION_INVENTORY, i);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activate_selected();
      return;
    }
  }
  for (int i = 0; i < station_count; i++) {
    if (!CheckCollisionPointRec(mouse, station_rects[i])) continue;
    SelectionKind kind = simplecraft_surroundings_has(station_ids[i]) && surroundings_selected
                             ? SELECTION_SURROUNDINGS
                             : SELECTION_STATION;
    select_label(kind, i);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activate_selected();
    return;
  }
  for (int i = 0; i < recipe_count; i++) {
    if (!CheckCollisionPointRec(mouse, recipe_rects[i])) continue;
    select_label(SELECTION_RECIPE, i);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activate_selected();
    return;
  }
}

/** Draw a section frame with its title embedded in the top border. @param section frame bounds @param title section label @return void */
static void draw_section(Rectangle section, const char *title) {
  DrawRectangleLinesEx(section, 2, DARKGRAY);
  int title_width = MeasureText(title, LABEL_SIZE) + 12;
  DrawRectangle((int)section.x + 16, (int)section.y - 8, title_width, 18, BLACK);
  DrawText(title, section.x + 22, section.y - 7, LABEL_SIZE, RAYWHITE);
}

/** Draw wrapped spawn and inventory labels with selection outlines. @return void */
void app_draw(void) {
  ClearBackground(BLACK);
  layout();
  DrawText("simplecraft", 24, 20, 24, RAYWHITE);
  draw_section(spawn_section, "SPAWN");
  draw_section(inventory_section, "INVENTORY");
  draw_section(station_section, "STATIONS");
  draw_section(recipe_section, "RECIPES");
  const Recipe *selected = selection_kind == SELECTION_RECIPE ? &recipes[selected_recipe] : NULL;
  for (int i = 0; i < item_count; i++) {
    Color item_color = LIGHTGRAY;
    if (selected) {
      int required = recipe_input_amount(selected, item_ids[i]);
      int output = recipe_output_amount(selected, item_ids[i]);
      if (output > 0) {
        item_color = GREEN;
        DrawRectangleLinesEx(spawn_rects[i], 2, GREEN);
      } else if (required > 0 && !recipe_input_sufficient(selected, item_ids[i])) {
        item_color = RED;
        DrawRectangleLinesEx(spawn_rects[i], 2, RED);
      } else if (required > 0) {
        DrawRectangleLinesEx(spawn_rects[i], 2, BLUE);
      }
    }
    if (selection_kind == SELECTION_SPAWN && selected_item == i) DrawRectangleLinesEx(spawn_rects[i], 2, YELLOW);
    DrawText(item_labels[i], spawn_rects[i].x + LABEL_PAD_X, spawn_rects[i].y + LABEL_PAD_Y, LABEL_SIZE, item_color);
    if (simplecraft_inventory_has(item_ids[i]) <= 0) continue;
    if (selected) {
      int required = recipe_input_amount(selected, item_ids[i]);
      int output = recipe_output_amount(selected, item_ids[i]);
      if (output > 0) DrawRectangleLinesEx(inventory_rects[i], 2, GREEN);
      else if (required > 0 && !recipe_input_sufficient(selected, item_ids[i])) DrawRectangleLinesEx(inventory_rects[i], 2, RED);
      else if (required > 0) DrawRectangleLinesEx(inventory_rects[i], 2, BLUE);
    }
    if (selection_kind == SELECTION_INVENTORY && selected_item == i) DrawRectangleLinesEx(inventory_rects[i], 2, YELLOW);
    DrawText(TextFormat("%s (x%d)", item_labels[i], simplecraft_inventory_has(item_ids[i])), inventory_rects[i].x + LABEL_PAD_X, inventory_rects[i].y + LABEL_PAD_Y, LABEL_SIZE, item_color);
  }
  for (int i = 0; i < station_count; i++) {
    bool station_active = (station_selected || surroundings_selected) && selected_station == i;
    Color color = simplecraft_surroundings_has(station_ids[i]) ? WHITE : GRAY;
    if (station_active) DrawRectangleLinesEx(station_rects[i], 2, YELLOW);
    if (selected && recipe_uses_station(selected, station_ids[i])) DrawRectangleLinesEx(station_rects[i], 2, BLUE);
    DrawText(station_labels[i], station_rects[i].x + LABEL_PAD_X, station_rects[i].y + LABEL_PAD_Y, LABEL_SIZE, color);
  }
  if (inventory_rect_count == 0) DrawText("(empty)", 24, inventory_title_y + LABEL_SIZE + 8, LABEL_SIZE, DARKGRAY);
  if (recipe_visible_count == 0) DrawText("(none available)", 24, recipe_title_y + LABEL_SIZE + 8, LABEL_SIZE, DARKGRAY);
  for (int i = 0; i < recipe_visible_count; i++) {
    int recipe_index = recipe_visible[i];
    if (selection_kind == SELECTION_RECIPE && selected_recipe == recipe_index) DrawRectangleLinesEx(recipe_rects[recipe_index], 2, YELLOW);
    Color recipe_color = simplecraft_recipe_can_craft(recipes[recipe_index].id) ? LIGHTGRAY : DARKGRAY;
    DrawText(recipes[recipe_index].label, recipe_rects[recipe_index].x + LABEL_PAD_X, recipe_rects[recipe_index].y + LABEL_PAD_Y, LABEL_SIZE, recipe_color);
  }
  cli_draw();
}
