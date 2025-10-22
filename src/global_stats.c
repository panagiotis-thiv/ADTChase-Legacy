#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "state.h"
#include "menu.h"
#include "level.h"
#include "global_stats.h"

//Global stats means stats that aren't exclusive to each state. Which means:
//1. Spaceship hp
//2. Coins
//3. Guns upgrades & bullets
//4. Levels won + when each level should be unlocked etc. 

struct global_stats {
    
    struct player_info player;
    struct guns_info guns;
    struct levels_info levels; 
    struct store_info store;
    struct user_input input;
};

Gun gs_gun_create(int bullets, int delay, int damage) {

	Gun gun = malloc(sizeof(*gun));

    gun->bullets = bullets;
    gun->delay = delay;
    gun->damage = damage;

    return gun;
}

GlobalStats gs_create() {

	GlobalStats stats = malloc(sizeof(*stats));

    stats->player.coins = 50;
    stats->player.spaceship_hp = 50;

    Gun pistol = gs_gun_create(30, 70, 20);
    stats->guns.pistol = pistol;

    Gun rifle = gs_gun_create(65, 5, 10);
    stats->guns.rifle = rifle;

    Gun sniper = gs_gun_create(8, 110, 50);
    stats->guns.sniper = sniper;

    stats->guns.selected_gun = stats->guns.pistol;
    stats->guns.prev_gun = stats->guns.pistol;

    stats->levels.level1 = 1;
    stats->levels.level2 = 0;
    stats->levels.level3 = 0;
    stats->levels.level4 = 0;
    stats->levels.level5 = 0;

    stats->store.spaceship_hp = 50;

    stats->store.rifle = false;
    stats->store.sniper = false;

    stats->store.slot1 = stats->guns.rifle;
    stats->store.slot2 = stats->guns.sniper;

    stats->input.coinsEntered = 0;
    stats->input.isEnteringInput = false;
    stats->input.inputBuffer[0] = '\0';
    stats->input.charCount = 0;


	return stats;
}

GunsInfo gs_guns_info(GlobalStats stats) {
	return &stats->guns;
}

LevelsInfo gs_levels_info(GlobalStats stats) {
	return &stats->levels;
}

PlayerInfo gs_player_info(GlobalStats stats) {
    return &stats->player;
}

StoreInfo gs_store_info(GlobalStats stats) {
    return &stats->store;
}

UserInput gs_user_input(GlobalStats stats) {
    return &stats->input;
}