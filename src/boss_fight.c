#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "vec2.h"
#include "menu.h"
#include "level.h"
#include "global_stats.h"
#include "boss_fight.h"

struct boss_state {
	Vector objects;			// περιέχει στοιχεία Object (αστεροειδείς, σφαίρες)
	struct state_info info;	// Γενικές πληροφορίες για την κατάσταση του παιχνιδιού
    struct boss_info b_info;
	int next_bullet;		// Αριθμός frames μέχρι να επιτραπεί ξανά σφαίρα
	float speed_factor;		// Πολλαπλασιαστής ταχύτητς (1 = κανονική ταχύτητα, 2 = διπλάσια, κλπ)

    int boss_hp;
    int spaceship_hp;

	Levels level;
	GlobalStats stats;
};


BossState boss_state_create(GlobalStats stats) {

    BossState boss_state = malloc(sizeof(*boss_state));

    boss_state->info.win = false;
	boss_state->b_info.lost = false;
	boss_state->speed_factor = 1;
	boss_state->stats = stats;
	boss_state->next_bullet = 0;

    boss_state->b_info.phase = 0;
	boss_state->b_info.hits = 0;
	boss_state->b_info.next_bullet = 0;
	boss_state->b_info.left_movement = false;
	boss_state->b_info.right_movement = true;
	boss_state->b_info.phase_speed = 0.04f;

	boss_state->b_info.phase_two_moving = false;
	boss_state->b_info.phase_two_started = false;
	boss_state->b_info.phase_two_active = false;
	boss_state->b_info.phase_two_stage = 0;

		
	boss_state->b_info.phase_three_moving = false;
	boss_state->b_info.phase_three_started = false;
	boss_state->b_info.phase_three_active = false;
	
	boss_state->info.paused = false;
	boss_state->info.drawCoinsReward = false;

	boss_state->info.rewardMessages = list_create(NULL);
	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	boss_state->objects = vector_create(0, NULL);

    boss_state->b_info.spaceship = create_object(
        BOSS_SPACESHIP,
		(Vector2){450, 100},
        (Vector2){0, 0},   
        (Vector2){0, 1},    
        SPACESHIP_SIZE,     
        500      
    );

	boss_state->info.spaceship  = create_object(
		SPACESHIP,
		(Vector2){450, 450},
		(Vector2){0, 0},
		(Vector2){0, 1},	
		SPACESHIP_SIZE,
		gs_player_info(stats)->spaceship_hp
	);

	//Δημιουργώ ένα άσχετο αντικείμενο γιατί αλλιώς είχε πρόβλημα η remove null nodes
	Object bullet = create_object(
		BULLET,
		(Vector2){-50, -50},
		(Vector2){0, 0},
		(Vector2){27, 0},								
		BULLET_SIZE,
		0
	);
	vector_insert_last(boss_state->objects, bullet);

    return boss_state;
}


StateInfo boss_state_info(BossState state) {
	return &state->info;
}

BossInfo boss_state_binfo(BossState state) {
	return &state->b_info;
}

List boss_state_objects(BossState state, Vector2 top_left, Vector2 bottom_right) {

	List result = list_create(NULL);

	for (int i = 0; i < vector_size(state->objects); i++) {
		Object obj = vector_get_at(state->objects, i);
		if (obj != NULL) {
			if(
				obj->position.x >= top_left.x && obj->position.x <= bottom_right.x &&
				obj->position.y <= top_left.y && obj->position.y >= bottom_right.y	
			) {
				list_insert_next(result, LIST_BOF, obj);
			}
		}
			
	}
	
	return result;
}

void remove_null_nodes(BossState boss_state) {
	//Διαγραφή NULL κόμβων
	for (int i = 0; i < vector_size(boss_state->objects); i++) {
		Object obj = vector_get_at(boss_state->objects, i);
		Object last_obj = vector_get_at(boss_state->objects, vector_size(boss_state->objects)-1);
		while (last_obj == NULL) {
			vector_remove_last(boss_state->objects);
			last_obj = vector_get_at(boss_state->objects, vector_size(boss_state->objects)-1);
		}
		if (obj == NULL) {
			vector_swap(boss_state->objects, i, vector_size(boss_state->objects)-1);
			vector_remove_last(boss_state->objects);
			i--;
		}
	}
}

void spawn_asteroids_phase_2(BossState state, Vector2 spawn_point, Vector2 direction, int position) {

	Vector2 pos;
	int num_asteroids;

	switch (position) {
		//Left to Right or Right to Left
		case 1:
		case 3:
			num_asteroids = 5;
			break;
		//Top to Bottom or Bottom to Top
		case 2:
		case 4:
			num_asteroids = 7;
			break;
	}

	for (int i = 0; i < num_asteroids; i++) {
		
		Vector2 asteroid_speed;

		switch (position) {
		//Left to Right
		case 1:
			pos = (Vector2){0,125*i};
			asteroid_speed = (Vector2){1, 0};  
			break;
		// Right to Left
		case 3:
			pos = (Vector2){0,125*i};
			asteroid_speed = (Vector2){-1, 0};
			break;
		//Top to Bottom 
		case 2:
			pos = (Vector2){125*i,0};
			asteroid_speed = (Vector2){0, 1};
			break;
		//Bottom to Top
		case 4:
			pos = (Vector2){125*i,0};
			asteroid_speed = (Vector2){0, -1};
			break;
		}

		Vector2 asteroid_position = vec2_add(spawn_point, pos);
		
        // Normalize the direction and adjust speed
        asteroid_speed = vec2_normalize(asteroid_speed);
        asteroid_speed = vec2_scale(asteroid_speed, 1.4 + (0.6 * i));


		Object asteroid = create_object(
			ASTEROID,
			asteroid_position,
			asteroid_speed,
			(Vector2){27,0},
			95,
			99999
		);

		vector_insert_last(state->objects, asteroid);

	}
}

void boss_state_update(BossState boss_state, KeyState keys, Menu menu) {

	if (keys->p && !boss_state_info(boss_state)->win && !boss_state_binfo(boss_state)->lost) 
		boss_state->info.paused = !boss_state->info.paused;

	if (boss_state->info.paused == false) {

		Vector2 top_left = (Vector2){40,625};
		Vector2 bottom_right = (Vector2){850,70};

		for (int i = 0; i < vector_size(boss_state->objects); i++) {
			Object obj = vector_get_at(boss_state->objects, i);
			if (obj != NULL) {
                if (obj->type == BULLET || obj->type == ASTEROID)
					obj->position = vec2_add(obj->position, obj->speed);
				if(
					obj->position.x < top_left.x || obj->position.x > bottom_right.x ||
					obj->position.y > top_left.y || obj->position.y < bottom_right.y 
				) { 
					//Δεν ξέρω γιατί αλλά δεν με αφήνε να το βάλω μέσα στην προηγούμενη if με &&, οπότε το έβαλα εδώ.
					if (obj->orientation.x != 27) {
						free(obj);
						vector_set_at(boss_state->objects, i, NULL);
					}
				}
			}
		}

		void remove_null_nodes(BossState boss_state);

		//float health_precentage = (float)boss_state->b_info.spaceship->health / 500;
		//printf("Hey, spaceship pos is %f and %f\n", boss_state->info.spaceship->position.x, boss_state->info.spaceship->position.y);
		//printf("Hey, boss pos is %f and %f with speed.x %f and hp perc: %f \n", boss_state->b_info.spaceship->position.x, boss_state->b_info.spaceship->position.y, boss_state->b_info.spaceship->speed.x, health_precentage);

		//Level Borders

		Vector2 top_left_border = (Vector2){40,625};
		Vector2 bottom_right_border = (Vector2){850,70};

		Vector2 spaceship_pos = vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->info.spaceship->speed, boss_state->speed_factor));

		if (spaceship_pos.x < top_left_border.x) {
			spaceship_pos.x = top_left_border.x;
		    boss_state->info.spaceship->speed.x = 0;
		}
		else if (spaceship_pos.x > bottom_right_border.x) {
			spaceship_pos.x = bottom_right_border.x;
			boss_state->info.spaceship->speed.x = 0;  
		}
		if (spaceship_pos.y < bottom_right_border.y) {
			spaceship_pos.y = bottom_right_border.y;
		    boss_state->info.spaceship->speed.y = 0; 
		}

		else if (spaceship_pos.y > top_left_border.y) {
			spaceship_pos.y = top_left_border.y;
			boss_state->info.spaceship->speed.y = 0; 
		}

		boss_state->info.spaceship->position = spaceship_pos;

		boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->speed, boss_state->speed_factor));
		//Έλεγχος για όταν κάποιο κουμπί είναι πατημένο.

		if (keys->right)
			boss_state->info.spaceship->orientation = vec2_rotate(boss_state->info.spaceship->orientation, -SPACESHIP_ROTATION);
		if (keys->left) 
			boss_state->info.spaceship->orientation = vec2_rotate(boss_state->info.spaceship->orientation, SPACESHIP_ROTATION);
		if (keys->up && !boss_state->b_info.phase_two_moving && !boss_state->b_info.phase_three_moving) {
			Vector2 accel = vec2_scale(boss_state->info.spaceship->orientation, SPACESHIP_ACCELERATION);
			accel.y *= -1;
			boss_state->info.spaceship->speed = vec2_add(boss_state->info.spaceship->speed, accel);
		} else 
			boss_state->info.spaceship->speed = vec2_scale(boss_state->info.spaceship->speed, SPACESHIP_SLOWDOWN);

        //Boss Orientation
		Vector2 distance = vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->position, -1));

		boss_state->b_info.spaceship->orientation = vec2_normalize(distance);
		boss_state->b_info.spaceship->orientation.y *= -1;
	
		if (boss_state_binfo(boss_state)->phase == 1) {
			
			if (boss_state->b_info.right_movement)
				boss_state->b_info.spaceship->speed.x += boss_state_binfo(boss_state)->phase_speed;
			else if (boss_state->b_info.left_movement)
				boss_state->b_info.spaceship->speed.x -= boss_state_binfo(boss_state)->phase_speed;

			if (boss_state->b_info.spaceship->position.x <= top_left_border.x) {
				boss_state->b_info.spaceship->speed.x = boss_state_binfo(boss_state)->phase_speed;
				boss_state->b_info.right_movement = true;
				boss_state->b_info.left_movement = false;
				boss_state->b_info.spaceship->position.x += 1;
			}

			if (boss_state->b_info.spaceship->position.x >= bottom_right_border.x) {
				boss_state->b_info.spaceship->speed.x = -boss_state_binfo(boss_state)->phase_speed;
				boss_state->b_info.right_movement = false;
				boss_state->b_info.left_movement = true;
				boss_state->b_info.spaceship->position.x -= 1;
			}

			if (boss_state->b_info.next_bullet == 0) {
				Vector2 target = vec2_normalize(vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->position, -1)));
				Vector2 speed = vec2_scale(target, 4);				
				Vector2 position = boss_state->b_info.spaceship->position;

				Object bullet = create_object(
					BULLET,
					position,
					speed,
					(Vector2){20, 0},								
					BULLET_SIZE,
					0
				);
				vector_insert_last(boss_state->objects, bullet);

				boss_state->b_info.next_bullet++;
			} else if (boss_state->b_info.next_bullet >= 80)
				boss_state->b_info.next_bullet = 0;
			else
				boss_state->b_info.next_bullet++;

		} else if (boss_state_binfo(boss_state)->phase == 2) {
			
			if (!boss_state->b_info.phase_two_started) {
				boss_state->b_info.phase_two_moving = true;
				boss_state->b_info.phase_two_started = true;
			}

			if (boss_state->b_info.phase_two_moving) {
				
				Vector2 top_left = (Vector2){40,625};
				Vector2 bottom_right = (Vector2){850,70};

				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if(
							obj->position.x >= top_left.x && obj->position.x <= bottom_right.x &&
							obj->position.y <= top_left.y && obj->position.y >= bottom_right.y	
						) {
							free(obj);
							vector_set_at(boss_state->objects, i, NULL);
						}
					}
				}

				boss_state->b_info.spaceship->speed.x = 0;

				Vector2 boss_destination = (Vector2){40,70};
				Vector2 spaceship_destination = (Vector2){850,625};

				float move_speed = 4.0f; 

				//Spaceship "Animation" Movement
				Vector2 direction_to_destination = vec2_normalize(vec2_add(spaceship_destination, vec2_scale(boss_state->info.spaceship->position, -1)));
				boss_state->info.spaceship->position = vec2_add(boss_state->info.spaceship->position, vec2_scale(direction_to_destination, move_speed));

				if (vec2_distance(boss_state->info.spaceship->position, spaceship_destination) < 5.0f)
					boss_state->info.spaceship->position = spaceship_destination;

				//Boss "Animation" Movement
    			Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
    			boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

				if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
					boss_state->b_info.spaceship->position = boss_destination;

				//Stop "Animation"
				if (vec2_distance(boss_state->info.spaceship->position, spaceship_destination) < 5.0f && vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
					boss_state->b_info.phase_two_moving = false;
					boss_state->b_info.phase_two_stage = 1;
				}

			}

			//Boss Shooting
			if (boss_state->b_info.next_bullet == 0) {
				Vector2 target = vec2_normalize(vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->position, -1)));
				Vector2 speed = vec2_scale(target, 7);				
				Vector2 position = boss_state->b_info.spaceship->position;

				Object bullet = create_object(
					BULLET,
					position,
					speed,
					(Vector2){20, 0},								
					BULLET_SIZE,
					0
				);
				vector_insert_last(boss_state->objects, bullet);

				boss_state->b_info.next_bullet++;
			} else if (boss_state->b_info.next_bullet >= 200)
				boss_state->b_info.next_bullet = 0;
			else
				boss_state->b_info.next_bullet++;
			
			//And here the magic starts... Good luck to me! 
			//Phase 2 Implementation
			//Όλα στην ουσία θα λειτουργούν το ίδιο, απλά θα αλλάζουν κάτι αριθμοί κάθε φορά. Θα το εξηγήσω βήμα βήμα για να καταλαβαίνω και εγώ τι κάνω.
			//Η βασική ιδέα είναι η εξής: 4 αστεροειδής θα εμφανίζονται κάθε φορά από κάποια πλευρά και θα προχωράνε στην απένταντι πλευρά, ταυτόχρονα θα 
			//εμφανίζεται κάτι σαν "laser" λίγο πιο κάτω από το boss που δεν θα αφήνει να περάσουν σφαίρες και θα πρέπει ο παίχτης να περάσει απέναντι από αυτό
			//την επόμενη φορά που αλλάξει το "mini phase". Αυτό θα γίνεται κάθε φορά απλά θα αλλάζει η κατεύθυνση των αστεροειδών και του laser. 
			//Το πως ακριβώς θα δουλεύει το gameplay θα φανεί στο τέλος. 
			switch (boss_state->b_info.phase_two_stage) {
			case 1:
				
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){40,70};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;
					
					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){0, 125}, (Vector2){900, 125}, 1);
						boss_state->b_info.phase_two_active = true;
					}
				}
				//Laser implementation TODO (I don't think so, it's too much)
				// Vector2 top_left_laser_1 = (Vector2){100,220};
				// Vector2 bottom_right_laser_1 = (Vector2){820,270};

				int count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x >= 900 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 2;
				}

				break;
			case 2:
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){450,100};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;
					
					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){80, -100}, (Vector2){80, 700}, 2);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x == 80 && obj->position.y >= 700) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 3;
				}

				break;
			case 3:
				//Removing Asteroids
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}

					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){825,100};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;

					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){950, 125}, (Vector2){0, 125}, 3);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x <= 0 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 4;
				}


				break;
			case 4:
				//Removing Asteroids
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}

					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){820,340};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;

					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){950, 125}, (Vector2){0, 125}, 3);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x <= 0 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 5;
				}
				break;
			case 5:
				//Removing Asteroids
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}

					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){800,585};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;

					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){950, 125}, (Vector2){0, 125}, 3);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x <= 0 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 6;
				}
				break;
			case 6:
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){450,590};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;
					
					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){80, 700}, (Vector2){80, -100}, 4);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x == 80 && obj->position.y <= 0) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 7;
				}

				break;
			case 7:
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){55,570};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;
					
					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){0, 125}, (Vector2){900, 125}, 1);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x >= 900 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 8;
				}

				break;
			case 8:
				if (!boss_state->b_info.phase_two_active) {
					for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
					remove_null_nodes(boss_state);

					Vector2 boss_destination = (Vector2){50,340};
					float move_speed = 3.0f; 

					//Boss "Animation" Movement
					Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) 
						boss_state->b_info.spaceship->position = boss_destination;
					
					//Asteroids Spawning
					if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 5.0f) {
						spawn_asteroids_phase_2(boss_state, (Vector2){0, 125}, (Vector2){900, 125}, 1);
						boss_state->b_info.phase_two_active = true;
					}
				}

				//Finding last asteroid of stage
				count = 0;
				for (int i = 0; i < vector_size(boss_state->objects); i++) {
					Object obj = vector_get_at(boss_state->objects, i);
					if (obj != NULL) {
						if (obj->type == ASTEROID && obj->orientation.x == 27 && obj->position.x >= 900 && obj->position.y == 125) {
							count++;
						}
					}
				}

				if (count == 1) {
					boss_state->b_info.phase_two_active = false;
					boss_state->b_info.phase_two_stage = 1;
				}
				break;
			}
			

		} else if (boss_state_binfo(boss_state)->phase == 3) {

			if (!boss_state->b_info.phase_three_started) {
				boss_state->b_info.phase_three_moving = true;
				boss_state->b_info.phase_three_started = true;
			}

			if (boss_state->b_info.phase_three_moving) {

				for (int i = 0; i < vector_size(boss_state->objects); i++) {
						Object obj = vector_get_at(boss_state->objects, i);
						if (obj != NULL) {
							if (obj->type == ASTEROID && obj->orientation.x == 27) {
								free(obj);
								vector_set_at(boss_state->objects, i, NULL);
							}
						}
					}
				
				remove_null_nodes(boss_state);

				Vector2 boss_destination = (Vector2){450,250};
				Vector2 spaceship_destination = (Vector2){450,590};

				float move_speed = 1.5f; 

				//Spaceship "Animation" Movement
				Vector2 direction_to_destination = vec2_normalize(vec2_add(spaceship_destination, vec2_scale(boss_state->info.spaceship->position, -1)));
				boss_state->info.spaceship->position = vec2_add(boss_state->info.spaceship->position, vec2_scale(direction_to_destination, move_speed));

				if (vec2_distance(boss_state->info.spaceship->position, spaceship_destination) < 1.0f)
					boss_state->info.spaceship->position = spaceship_destination;

				//Boss "Animation" Movement
				Vector2 boss_direction_to_destination = vec2_normalize(vec2_add(boss_destination, vec2_scale(boss_state->b_info.spaceship->position, -1)));
				boss_state->b_info.spaceship->position = vec2_add(boss_state->b_info.spaceship->position, vec2_scale(boss_direction_to_destination, move_speed));

				if (vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 1.0f) 
					boss_state->b_info.spaceship->position = boss_destination;

				//Stop "Animation"
				if (vec2_distance(boss_state->info.spaceship->position, spaceship_destination) < 1.0f && vec2_distance(boss_state->b_info.spaceship->position, boss_destination) < 1.0f) {
					boss_state->b_info.phase_three_moving = false;
				}
			}

			if (!boss_state->b_info.phase_three_moving) {
				//Boss Shooting
				if (boss_state->b_info.next_bullet == 0) {
					Vector2 target = vec2_normalize(vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->position, -1)));
					Vector2 speed = vec2_scale(target, BULLET_SPEED);				
					Vector2 position = boss_state->b_info.spaceship->position;

					Object bullet = create_object(
						BULLET,
						position,
						speed,
						(Vector2){20, 0},								
						BULLET_SIZE,
						0
					);
					vector_insert_last(boss_state->objects, bullet);

					boss_state->b_info.next_bullet++;
				} else if (boss_state->b_info.next_bullet >= 50)
					boss_state->b_info.next_bullet = 0;
				else
					boss_state->b_info.next_bullet++;

				//Boss Movement
				boss_state->b_info.spaceship->speed = vec2_normalize(distance);
			}
		    


		}
		
		remove_null_nodes(boss_state);

		bool shoot = true;
		if (keys->space && !boss_state->b_info.phase_two_moving && !boss_state->b_info.phase_three_moving) {

			if (gs_guns_info(boss_state->stats)->selected_gun->bullets <= 0)
				shoot = false;
			               
			if (shoot) {
				if (boss_state->next_bullet == 0) {
					Vector2 speed = vec2_add(boss_state->info.spaceship->speed, vec2_scale(boss_state->info.spaceship->orientation, BULLET_SPEED));
					speed.y *= -1;
					Vector2 position = boss_state->info.spaceship->position;

					Object bullet = create_object(
						BULLET,
						position,
						speed,
						(Vector2){10, 0},								
						BULLET_SIZE,
						0
					);
					vector_insert_last(boss_state->objects, bullet);

					boss_state->next_bullet++;
					gs_guns_info(boss_state->stats)->selected_gun->bullets--;

				}
				else if (boss_state->next_bullet >= gs_guns_info(boss_state->stats)->selected_gun->delay)
					boss_state->next_bullet = 0;
				else
					boss_state->next_bullet++;
			}
		}

		//Έλεγχος συγκρούσεων
		
		bool collisionAsteroidSpaceship; 
		bool collisionAsteroidBullet;

		bool collisionBossBullet;
		bool collisionSpaceshipBullet;

		bool collisionBossSpaceship;
	
		//Αστεροειδής - Διαστημόπλοιο
		for (int i = 0; i < vector_size(boss_state->objects); i++) {
			Object obj = vector_get_at(boss_state->objects, i);
			collisionAsteroidSpaceship = false;
			if (obj != NULL) {
				if(obj->type == ASTEROID)
					collisionAsteroidSpaceship = CheckCollisionCircles(obj->position, (obj->size)/2, boss_state->info.spaceship->position, SPACESHIP_SIZE/2);
				if (collisionAsteroidSpaceship) {
					
					gs_player_info(boss_state->stats)->spaceship_hp -= 5;

					if (gs_player_info(boss_state->stats)->spaceship_hp <= 0) {
						boss_state->b_info.lost = true;
						boss_state->info.paused = true;
						gs_player_info(boss_state->stats)->spaceship_hp = gs_store_info(boss_state->stats)->spaceship_hp/3;
					}
				}
			}
		}

		void remove_null_nodes(BossState boss_state);

		//Αστεροειδής - Σφαίρα
		for (int i = 0; i < vector_size(boss_state->objects); i++) {
			Object obj = vector_get_at(boss_state->objects, i);
			for (int j = 0; j < vector_size(boss_state->objects); j++) {
				Object obj2 = vector_get_at(boss_state->objects, j);
				collisionAsteroidBullet = false;
				if (obj != NULL && obj2 != NULL) {
					if ((obj->type == ASTEROID) && obj2->type == BULLET && obj2->orientation.x != 20) 
						collisionAsteroidBullet = CheckCollisionCircles(obj->position, (obj->size)/2, obj2->position, BULLET_SIZE/2);					
					if (collisionAsteroidBullet) {
						if (obj->health > gs_guns_info(boss_state->stats)->selected_gun->damage + 1) {
							obj->health = obj->health - gs_guns_info(boss_state->stats)->selected_gun->damage;

							free(obj2);
							vector_set_at(boss_state->objects, j, NULL);
							break;
						}
					
						else {

							//Καταστρέφω αστεροειδή και σφαίρα
							free(obj);
							vector_set_at(boss_state->objects, i, NULL);

							free(obj2);
							vector_set_at(boss_state->objects, j, NULL);
							break;
						}
					}
				}
			}
		}
		
		void remove_null_nodes(BossState boss_state);

		//Boss - Σφαίρα
		for (int i = 0; i < vector_size(boss_state->objects); i++) {
			Object obj = vector_get_at(boss_state->objects, i);
			collisionBossBullet = false;
			if (obj != NULL) {
				if (obj->type == BULLET && obj->orientation.x != 20)
					collisionBossBullet = CheckCollisionCircles(obj->position, (obj->size)/2, boss_state->b_info.spaceship->position, SPACESHIP_SIZE/2);
				if (collisionBossBullet) {
					
					boss_state->b_info.spaceship->health -= gs_guns_info(boss_state->stats)->selected_gun->damage;
					boss_state->b_info.hits++;

					if (boss_state->b_info.hits == 1) 
						boss_state->b_info.phase = 1;

					float health_precentage = (float)boss_state->b_info.spaceship->health / 500;

					if (health_precentage > 0.7 && health_precentage < 0.85)
						boss_state->b_info.phase_speed = 0.1;

					if (health_precentage > 0.4 && health_precentage <= 0.7)
						boss_state->b_info.phase = 2;
	
					if (health_precentage <= 0.4)
						boss_state->b_info.phase = 3;

					if (boss_state->b_info.spaceship->health <= 0) {
						gs_player_info(boss_state->stats)->coins += 5000;
						boss_state->info.win = true;
						boss_state->info.paused = true;
					}

					free(obj);
					vector_set_at(boss_state->objects, i, NULL);
					break;
				}
			}
		}

		void remove_null_nodes(BossState boss_state);


		//Spaceship - Σφαίρα
		for (int i = 0; i < vector_size(boss_state->objects); i++) {
			Object obj = vector_get_at(boss_state->objects, i);
			collisionSpaceshipBullet = false;
			if (obj != NULL) {
				if (obj->type == BULLET && obj->orientation.x != 10)
					collisionSpaceshipBullet = CheckCollisionCircles(obj->position, (obj->size)/2, boss_state->info.spaceship->position, SPACESHIP_SIZE/2);
				if (collisionSpaceshipBullet) {
					
					gs_player_info(boss_state->stats)->spaceship_hp -= 50;

					if (gs_player_info(boss_state->stats)->spaceship_hp <= 0) {
						boss_state->b_info.lost = true;
						boss_state->info.paused = true;
						gs_player_info(boss_state->stats)->spaceship_hp = gs_store_info(boss_state->stats)->spaceship_hp/3;
					}


					free(obj);
					vector_set_at(boss_state->objects, i, NULL);
					break;
				}
			}
		}

		void remove_null_nodes(BossState boss_state);

		//Spaceship - Boss
		collisionBossSpaceship = CheckCollisionCircles(boss_state->b_info.spaceship->position, SPACESHIP_SIZE*0.75, boss_state->info.spaceship->position, SPACESHIP_SIZE/2);
		if (collisionBossSpaceship) {
			
			gs_player_info(boss_state->stats)->spaceship_hp -= 100;

			if (gs_player_info(boss_state->stats)->spaceship_hp <= 0) {
				boss_state->b_info.lost = true;
				boss_state->info.paused = true;
				gs_player_info(boss_state->stats)->spaceship_hp = gs_store_info(boss_state->stats)->spaceship_hp/3;
			}

			Vector2 push_direction = vec2_normalize(vec2_add(boss_state->info.spaceship->position, vec2_scale(boss_state->b_info.spaceship->position, -1)));
   		 	boss_state->info.spaceship->speed = vec2_add(boss_state->info.spaceship->speed, vec2_scale(push_direction, 5));

		}
	}
}

void boss_state_destroy(BossState state) {

	for (int i = 0; i < vector_size(state->objects); i++) {
		free(vector_get_at(state->objects, i));
	}

	vector_destroy(state->objects);
	free(state->info.spaceship);
	free(state->b_info.spaceship);

	List rewardMessages = boss_state_info(state)->rewardMessages;
    for (ListNode node = list_first(rewardMessages); node != LIST_EOF; node = list_next(rewardMessages, node)) {
        free(list_node_value(rewardMessages, node));
    }
    list_destroy(rewardMessages);

	free(state);
}