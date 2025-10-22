#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "vec2.h"
#include "menu.h"
#include "level.h"

struct levels {

    struct level_stats stats;
    struct core_info core;
};

Levels level_create(int asteroid_hp, int reward, int level_no, float speed_factor, int core_hp, int core_reward, int core_speed) {

    Levels level = malloc(sizeof(*level));

    level->stats.asteroid_hp = asteroid_hp;
    level->stats.reward = reward;
    level->stats.level_no = level_no;
    level->stats.speed_factor = speed_factor;

    level->core.hp = core_hp;
    level->core.reward = core_reward;
    level->core.speed = core_speed;

    return level;
}

LevelStats level_stats(Levels level) {
    return &level->stats;
}


CoreInfo level_core_info(Levels level) {
	return &level->core;
}