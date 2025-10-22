#pragma once

#include "raylib.h"
#include "menu.h"
#include "ADTList.h"
#include "level.h"
#include "global_stats.h"
#include "state.h"

#define BOSS_SPEED 0.05f
#define BOSS_CIRCLE_SPEED 0.1f
#define BOSS_CIRCLE_RADIUS 200.0f

typedef struct boss_info {

    Object spaceship;

    bool lost;
    
    int phase;

    int next_bullet;
    int hits;

    bool left_movement;
    bool right_movement;

    float phase_speed;

    bool phase_two_started;
    bool phase_two_moving;
    int phase_two_stage;    //Keeps track of the stage of the second phase
    bool phase_two_active;

    bool phase_three_started;
    bool phase_three_moving;
    bool phase_three_active;

}* BossInfo;

typedef struct boss_state* BossState;

BossState boss_state_create(GlobalStats stats);

StateInfo boss_state_info(BossState state);

BossInfo boss_state_binfo(BossState state);

List boss_state_objects(BossState state, Vector2 top_left, Vector2 bottom_right);

void boss_state_update(BossState boss_state, KeyState keys, Menu menu);

void boss_state_destroy(BossState state);