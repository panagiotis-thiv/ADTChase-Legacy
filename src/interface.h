#pragma once

#include "state.h"
#include "menu.h"
#include "global_stats.h"
#include "boss_fight.h"

// Αρχικοποιεί το interface του παιχνιδιού
void interface_init();

// Κλείνει το interface του παιχνιδιού
void interface_close();

// Σχεδιάζει ένα frame με την τωρινή κατάσταση του παιχνδιού
void interface_draw_frame(State state, GlobalStats stats);

void interface_draw_boss_frame(BossState state, GlobalStats stats);

void interface_draw_menu(Menu menu, State state, GlobalStats stats);

void draw_coinsReward(int coinsReward, Vector2 pos);