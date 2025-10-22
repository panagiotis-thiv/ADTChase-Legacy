#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "interface.h"
#include "menu.h"
#include "vec2.h"

struct menu {

    int m_selected;
    int m_active;
    int options;

    int page_selected;
    int page_max;

};

Menu menu_create(int options) {

    Menu menu = malloc(sizeof(*menu));

    menu->m_selected = 1;
    menu->m_active = 0;
    menu->options = options;

    menu->page_selected = 1;

    return menu;
}

int selected_menu(Menu menu) {
    return menu->m_selected;
}

void set_selected(Menu menu, int selected) {
    menu->m_selected = selected;
}

void next_menu(Menu menu) {
    menu->m_selected++;
    if (menu->m_selected > menu->options) 
        menu->m_selected = menu->options;
}  

void prev_menu(Menu menu) {
    menu->m_selected--;
    if (menu->m_selected < 1) 
        menu->m_selected = 1;
}

int active_menu(Menu menu) {
    return menu->m_active;
}

void set_active_menu(Menu menu, int select) {
    menu->m_active = select;
}

void set_max_page(Menu menu, int page) {
    menu->page_max = page;
}

void set_page_next(Menu menu) {
    menu->page_selected++;
    if (menu->page_selected > menu->page_max)
        menu->page_selected = menu->page_max;
}

void set_page_prev(Menu menu) {
    menu->page_selected--;
    if (menu->page_selected < 1)
        menu->page_selected = 1;
}

int get_page(Menu menu) {
    return menu->page_selected;
}

void set_page(Menu menu, int page) {
    if (page <= menu->page_max)
        menu->page_selected = page;
}