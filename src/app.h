#ifndef APP_H
#define APP_H
// agent: codex | 2026-09-06 | restore runtime host scaffold | bec3b6
#include <stdbool.h>
int simplecraft_array_has(const int *values, int count, int index);
int simplecraft_array_find(const int *values, int count);
bool simplecraft_array_add(int *values, int count, int index, int amount);
bool simplecraft_array_remove(int *values, int count, int index, int amount);
int simplecraft_item_find(const char *name);
int simplecraft_inventory_has(int item_id);
bool simplecraft_inventory_add(int item_id, int amount);
bool simplecraft_inventory_remove(int item_id, int amount);
int simplecraft_station_find(const char *name);
bool simplecraft_surroundings_has(int station_id);
bool simplecraft_surroundings_add(int station_id);
bool simplecraft_surroundings_remove(int station_id);
int simplecraft_group_has(const char *group);
bool simplecraft_group_remove(const char *group, int amount);
int simplecraft_recipe_find(const char *name);
bool simplecraft_recipe_can_craft(int recipe_id);
bool simplecraft_recipe_craft(int recipe_id);
void app_init(void);
void app_inputs(void);
void app_draw(void);
void app_dispose(void);
int app_item_count(void);
const char *app_item_name(int index);
void cli_draw(void);
bool cli_input(void);
void cli_init(void);
void cli_despose(void);
#endif
