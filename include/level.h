#pragma once

typedef struct level_stats {

    int asteroid_hp;        
    int reward;                 //Max amount of coins obtained from destroying an asteroid
    int level_no;               
    float speed_factor;    
}* LevelStats;

typedef struct core_info {
	int hp;
    int speed;
    int reward;  
}* CoreInfo;

typedef struct levels* Levels;
 
Levels level_create(int asteroid_hp, int reward, int level_no, float speed_factor, int core_hp, int core_reward, int core_speed);

LevelStats level_stats(Levels level);
CoreInfo level_core_info(Levels level);