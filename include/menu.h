#pragma once

#include "raylib.h"
#include "ADTList.h"

typedef struct menu* Menu;

//For the options argument, it should be the number of options not including the main game. The game is considered option 0.

Menu menu_create(int options);

int selected_menu(Menu menu);

void set_selected(Menu menu, int selected);

void next_menu(Menu menu);

void prev_menu(Menu menu);

int active_menu(Menu menu);

void set_active_menu(Menu menu, int select);

void set_max_page(Menu menu, int page);

void set_page_next(Menu menu);

void set_page_prev(Menu menu);

int get_page(Menu menu);

void set_page(Menu menu, int page);