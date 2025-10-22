#pragma once

typedef struct user_input {

    int coinsEntered;       
    bool isEnteringInput;    
    char inputBuffer[6];     
    int charCount;           

}* UserInput;


typedef struct gun {

	int bullets;
    int delay;
    int damage;
	
}* Gun;

typedef struct guns_info {

	Gun selected_gun;
    Gun prev_gun;

    Gun pistol;
    Gun rifle;
    Gun sniper;

}* GunsInfo;

typedef struct levels_info {
    int level1;
    int level2;
    int level3;
    int level4;
    int level5;
}* LevelsInfo;

typedef struct player_info {

    int spaceship_hp;
    int coins;

}* PlayerInfo;

typedef struct store_info {

    int spaceship_hp;
    
    bool rifle;
    bool sniper;

    Gun slot1;
    Gun slot2;

}* StoreInfo;

typedef struct global_stats* GlobalStats;

GlobalStats gs_create();

Gun gs_gun_create(int bullets, int delay, int damage);

GunsInfo gs_guns_info(GlobalStats stats);

LevelsInfo gs_levels_info(GlobalStats stats);

PlayerInfo gs_player_info(GlobalStats stats);

StoreInfo gs_store_info(GlobalStats stats);

UserInput gs_user_input(GlobalStats stats);