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


// Οι ολοκληρωμένες πληροφορίες της κατάστασης του παιχνιδιού.
// Ο τύπος State είναι pointer σε αυτό το struct, αλλά το ίδιο το struct
// δεν είναι ορατό στον χρήστη.


struct state {
	Vector objects;			// περιέχει στοιχεία Object (αστεροειδείς, σφαίρες)
	struct state_info info;	// Γενικές πληροφορίες για την κατάσταση του παιχνιδιού
	int next_bullet;		// Αριθμός frames μέχρι να επιτραπεί ξανά σφαίρα
	float speed_factor;		// Πολλαπλασιαστής ταχύτητς (1 = κανονική ταχύτητα, 2 = διπλάσια, κλπ)

	Levels level;
	GlobalStats stats;
};

// Δημιουργεί και επιστρέφει ένα αντικείμενο

Object create_object(ObjectType type, Vector2 position, Vector2 speed, Vector2 orientation, double size, int health) {
	Object obj = malloc(sizeof(*obj));
	obj->type = type;
	obj->position = position;
	obj->speed = speed;
	obj->orientation = orientation;
	obj->size = size;
	obj->health = health;
	return obj;
}

// Επιστρέφει έναν τυχαίο πραγματικό αριθμό στο διάστημα [min,max]

static double randf(double min, double max) {
	return min + (double)rand() / RAND_MAX * (max - min);
}

// Προσθέτει num αστεροειδείς στην πίστα (η οποία μπορεί να περιέχει ήδη αντικείμενα).
//
// ΠΡΟΣΟΧΗ: όλα τα αντικείμενα έχουν συντεταγμένες x,y σε ένα καρτεσιανό επίπεδο.
// - Η αρχή των αξόνων είναι η θέση του διαστημόπλοιου στην αρχή του παιχνιδιού
// - Στο άξονα x οι συντεταγμένες μεγαλώνουν προς τα δεξιά.
// - Στον άξονα y οι συντεταγμένες μεγαλώνουν προς τα πάνω.

static void add_asteroids(State state, Levels level, int num) { 	
	for (int i = 0; i < num; i++) {
		// Τυχαία θέση σε απόσταση [ASTEROID_MIN_DIST, ASTEROID_MAX_DIST]
		// από το διστημόπλοιο.
		//
		Vector2 position = vec2_add(
			state->info.spaceship->position,
			vec2_from_polar(
				randf(ASTEROID_MIN_DIST, ASTEROID_MAX_DIST),	// απόσταση
				randf(0, 2*PI)									// κατεύθυνση
			)
		);

		// Τυχαία ταχύτητα στο διάστημα [ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED]
		// με τυχαία κατεύθυνση.
		//
		Vector2 speed = vec2_from_polar(
			randf(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED) * state->speed_factor,
			randf(0, 2*PI)
		);
		
		float object_size = randf(ASTEROID_MIN_SIZE, ASTEROID_MAX_SIZE);
		float sizePercentage = ((float)object_size / (float)ASTEROID_MAX_SIZE);

		int asteroidHP = sizePercentage * level_stats(level)->asteroid_hp;

		if (asteroidHP == 0)
			asteroidHP = 1;
		
		Object asteroid = create_object(
			ASTEROID,
			position,
			speed,
			(Vector2){0, 0},								// δεν χρησιμοποιείται για αστεροειδείς
			object_size,		// τυχαίο μέγεθος
			asteroidHP
		);
		vector_insert_last(state->objects, asteroid);
	}
}

// Δημιουργεί και επιστρέφει την αρχική κατάσταση του παιχνιδιού

State state_create(Levels level, GlobalStats stats) {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	state->level = level;
	state->stats = stats;
	// Γενικές πληροφορίες
	state->info.paused = false;				// Το παιχνίδι ξεκινάει χωρίς να είναι paused.
	state->next_bullet = 0;					// Σφαίρα επιτρέπεται αμέσως
	state->info.win = false;
	state->info.won = false;
	state->speed_factor = level_stats(level)->speed_factor;
	
	state->info.drawCoinsReward = false;
	state->info.coinsReward = 0;
	state->info.coinsPos = (Vector2){0,0};

	state->info.spawn_core = false;
	state->info.tp_core = false;
	state->info.hide_core = false;
	state->info.core = false;
	
	state->info.coreSpawnTimer = 0.0f;
	state->info.coreTPTimer = 0.0f;
	state->info.coreHideTimer = 0.0f;

	state->info.rewardMessages = list_create(NULL);
	state->info.eliminate = false;
	state->info.hol = false;


	state->info.level_number = level_stats(level)->level_no;

	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	state->objects = vector_create(0, NULL);

	// Δημιουργούμε το διαστημόπλοιο
	state->info.spaceship = create_object(
		SPACESHIP,
		(Vector2){0, 0},			// αρχική θέση στην αρχή των αξόνων
		(Vector2){0, 0},			// μηδενική αρχική ταχύτητα
		(Vector2){0, 1},			// κοιτάει προς τα πάνω
		SPACESHIP_SIZE,				// μέγεθος
		gs_player_info(stats)->spaceship_hp
	);

	// Προσθήκη αρχικών αστεροειδών
	add_asteroids(state, level, ASTEROID_NUM);

	return state;
}

State state_create_eliminate(GlobalStats stats) {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	state->stats = stats;
	// Γενικές πληροφορίες
	state->info.paused = false;				// Το παιχνίδι ξεκινάει χωρίς να είναι paused.
	state->next_bullet = 0;					// Σφαίρα επιτρέπεται αμέσως
	state->info.win = false;
	state->info.won = false;

	state->info.rewardMessages = list_create(NULL);
	state->info.eliminate = true;
	state->info.hol = false;

	state->info.level_number = 6;

	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	state->objects = vector_create(0, NULL);

	// Δημιουργούμε το διαστημόπλοιο
	state->info.spaceship = create_object(
		SPACESHIP,
		(Vector2){0, 0},			// αρχική θέση στην αρχή των αξόνων
		(Vector2){0, 0},			// μηδενική αρχική ταχύτητα
		(Vector2){0, 1},			// κοιτάει προς τα πάνω
		SPACESHIP_SIZE,				// μέγεθος
		gs_player_info(stats)->spaceship_hp
	);

	// Προσθήκη αρχικών αστεροειδών
	const int num_asteroids = 10;
	const float radius = 250.0f;  
	const float angle_increment = 2 * PI / num_asteroids;

	float rewards[10] = {0.1f, 0.1f, 0.1f, 0.4f, 0.4f, 0.8f, 1.0f, 1.2f, 1.4f, 2.0f};
	int good_status[10] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1};

	for (int i = num_asteroids -1; i > 0; i--) {

		int j = rand() % (i + 1);
		float temp_reward = rewards[i];

		rewards[i] = rewards[j];
		rewards[j] = temp_reward;

		int temp_status = good_status[i];
		good_status[i] = good_status[j];
		good_status[j] = temp_status;

	}


	for (int i = 0; i < num_asteroids; i++) {
		float angle = i * angle_increment;
		Vector2 position = {
			state->info.spaceship->position.x + radius * cos(angle),
			state->info.spaceship->position.y + radius * sin(angle)
		};

		Object asteroid = create_object(
			ASTEROID,
			position,
			(Vector2){rewards[i], 0}, 		//Για να μην δημιούργησω καινούργιες μεταβλήτες χρησιμοποιώ αυτές
			(Vector2){good_status[i], 0},   //που ήδη υπάρχουν αφού δεν χρησιμοποιούνται
			100,       
			10
		);

		vector_insert_last(state->objects, asteroid);
	}

	return state;
}

State state_create_hol(GlobalStats stats) {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	state->stats = stats;
	// Γενικές πληροφορίες
	state->info.paused = false;				// Το παιχνίδι ξεκινάει χωρίς να είναι paused.
	state->next_bullet = 0;					// Σφαίρα επιτρέπεται αμέσως
	state->info.win = false;

	state->info.rewardMessages = list_create(NULL);
	state->info.eliminate = false;
	state->info.hol = true;
	
	state->info.hol_reward = gs_user_input(stats)->coinsEntered;

	state->info.level_number = 7;

	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	state->objects = vector_create(0, NULL);

	// Δημιουργούμε το διαστημόπλοιο
	state->info.spaceship = create_object(
		SPACESHIP,
		(Vector2){0, 0},			// αρχική θέση στην αρχή των αξόνων
		(Vector2){0, 0},			// μηδενική αρχική ταχύτητα
		(Vector2){0, 1},			// κοιτάει προς τα πάνω
		SPACESHIP_SIZE,				// μέγεθος
		gs_player_info(stats)->spaceship_hp
	);

	// Προσθήκη αρχικών αστεροειδών
    const float distance = 250.0f; 

    //Μπροστινός αστεροειδής
    Vector2 front_pos = {
        state->info.spaceship->position.x,
        state->info.spaceship->position.y + distance
    };

    //Δεξιός αστεροιεδής (όχι δεν ψήφισε ΝΔ)
    Vector2 right_pos = {
        state->info.spaceship->position.x + distance,
        state->info.spaceship->position.y
    };

    //Αριστερός αστεροειδής (όχι δεν ψήφισε ΚΚΕ)
    Vector2 left_pos = {
        state->info.spaceship->position.x - distance,
        state->info.spaceship->position.y
    };

    //Πίσω αστεροειδής
    Vector2 back_pos = {
        state->info.spaceship->position.x,
        state->info.spaceship->position.y - distance
    };

    float initial_size = randf(50, 120);

    Object front_asteroid = create_object(ASTEROID, front_pos, (Vector2){0, 0}, (Vector2){0, 0}, initial_size, 10);
    Object right_asteroid = create_object(ASTEROID, right_pos, (Vector2){1, 0}, (Vector2){0, 0}, 120, 10); 
    Object left_asteroid = create_object(ASTEROID, left_pos, (Vector2){2, 0}, (Vector2){0, 0}, 120, 10);   
    Object back_asteroid = create_object(ASTEROID, back_pos, (Vector2){3, 0}, (Vector2){0, 0}, 120, 10);   

    vector_insert_last(state->objects, front_asteroid);
    vector_insert_last(state->objects, right_asteroid);
    vector_insert_last(state->objects, left_asteroid);
    vector_insert_last(state->objects, back_asteroid);

    state->info.hol_multiplier = 1.0f;  
    state->info.hol_round = 1;
    state->info.hol_reward = 0.0f;

	return state;

}


// Επιστρέφει τις βασικές πληροφορίες του παιχνιδιού στην κατάσταση state

StateInfo state_info(State state) {
	return &state->info;
}

// Επιστρέφει μια λίστα με όλα τα αντικείμενα του παιχνιδιού στην κατάσταση state,
// των οποίων η θέση position βρίσκεται εντός του παραλληλογράμμου με πάνω αριστερή
// γωνία top_left και κάτω δεξιά bottom_right.

List state_objects(State state, Vector2 top_left, Vector2 bottom_right) {

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

//Αντιστρέφει δύο vectors
void vector_swap(Vector vec, int pos1, int pos2) {

	if (pos1 == pos2)
		return;

	Pointer obj1 = vector_get_at(vec, pos1);
	Pointer obj2 = vector_get_at(vec, pos2);

	vector_set_at(vec, pos1, obj2);
	vector_set_at(vec, pos2, obj1);
}


// Ενημερώνει την κατάσταση state του παιχνιδιού μετά την πάροδο 1 frame.
// Το keys περιέχει τα πλήκτρα τα οποία ήταν πατημένα κατά το frame αυτό.

void state_update(State state, KeyState keys, Menu menu) {

	//Ελέγχος άμα είναι πατημένο το p ώστε το παιχνίδι να σταματήσει ή όχι.
	if (keys->p) 
		state->info.paused = !state->info.paused;

	if (state->info.paused == false || keys->n) {
		
		//printf("you are playing level %d\n", state_info(state)->level_number);

		//printf("Hey, spaceship pos is %f and %f\n", state->info.spaceship->position.x, state->info.spaceship->position.y);

		Levels level = state->level;
		//Ανανέωση θέσης αντικειμένων και διαστημόπλοιου.
		//Δημιουργεί κιόλας, άμα χρειάζονται, αστεροειδείς "κοντά" στο διαστημόπλοιο.

		Vector2 top_left = vec2_add(state->info.spaceship->position, (Vector2){-ASTEROID_MAX_DIST,ASTEROID_MAX_DIST});
		Vector2 bottom_right = vec2_add(state->info.spaceship->position, (Vector2){ASTEROID_MAX_DIST,-ASTEROID_MAX_DIST});
		int countAsteroid = 0;

		Object core = NULL;

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			if (obj != NULL) {
				if (obj->type == CORE || obj->type == HIDDEN || obj->type == BULLET)
					obj->position = vec2_add(obj->position, obj->speed);
				else 
					obj->position = vec2_add(obj->position, vec2_scale(obj->speed, state->speed_factor));

				if(
					obj->position.x >= top_left.x && obj->position.x <= bottom_right.x &&
					obj->position.y <= top_left.y && obj->position.y >= bottom_right.y &&
					obj->type == ASTEROID
				)
				countAsteroid++;

				if (obj->type == CORE || obj->type == HIDDEN)
					core = obj;
			}
		}
		if (state_info(state)->level_number != 3) {
			if (countAsteroid < ASTEROID_NUM)
				add_asteroids(state, level, ASTEROID_NUM-countAsteroid);
		} else {
			if (countAsteroid < 20)
				add_asteroids(state, level, 20-countAsteroid);
		}
		
		//state->info.spaceship->position = vec2_add(state->info.spaceship->position, state->info.spaceship->speed);
		state->info.spaceship->position = vec2_add(state->info.spaceship->position, vec2_scale(state->info.spaceship->speed, state->speed_factor));


		//Υλοποίηση αστεροειδή core

		if (state_info(state)->spawn_core && !state_info(state)->core && !state_info(state)->win && !state_info(state)->won) {
			
			//printf("SPAWING CORE\n!!!");
			Vector2 position = vec2_add(
				state->info.spaceship->position,
				vec2_from_polar(
					randf(100, 300),
					randf(0, 2*PI)								
				)
			);

			Vector2 speed = vec2_from_polar(
				level_core_info(level)->speed,
				randf(0, 2*PI)
			);
			
			Object asteroid = create_object(
				CORE,
				position,
				speed,
				(Vector2){0, 0},				
				50,		
				level_core_info(level)->hp
			);
			vector_insert_last(state->objects, asteroid);

			state_info(state)->core = true;

			state_info(state)->isCoreHidden = false;
			state_info(state)->tp_core = false;

			state_info(state)->coreHideTimer = 0.0f;
			state_info(state)->coreTPTimer = 0.0f;
		}

		if (state_info(state)->hide_core && !state_info(state)->isCoreHidden && state_info(state)->core && !state_info(state)->win) {
			if (core != NULL) {
				//printf("HIDING CORE\n!!!");
				core->type = HIDDEN;
				state_info(state)->hide_core = false;
				state_info(state)->isCoreHidden = true;
			}
		}


		if (state_info(state)->tp_core && state_info(state)->core && !state_info(state)->win) {
			if (core != NULL) {
				//printf("TPING CORE\n!!!");

				Vector2 position = vec2_add(
					state->info.spaceship->position,
					vec2_from_polar(
						randf(100, 300),	// απόσταση
						randf(0, 2*PI)		// κατεύθυνση
					)
				);

				core->position = position;
				core->type = CORE;

				state_info(state)->isCoreHidden = false;
				state_info(state)->tp_core = false;
			}
		}

		//Έλεγχος για όταν κάποιο κουμπί είναι πατημένο.

		if (keys->right)
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, -SPACESHIP_ROTATION);
		if (keys->left) 
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, SPACESHIP_ROTATION);
		if (keys->up) {
			Vector2 accel = vec2_scale(state->info.spaceship->orientation, SPACESHIP_ACCELERATION);
			state->info.spaceship->speed = vec2_add(state->info.spaceship->speed, accel);
		}
		else 
			state->info.spaceship->speed = vec2_scale(state->info.spaceship->speed, SPACESHIP_SLOWDOWN);

		bool shoot = true;
		if (keys->space) {

			if (gs_guns_info(state->stats)->selected_gun->bullets <= 0)
				shoot = false;

			if (shoot) {
				if (state->next_bullet == 0) {
					Vector2 speed = vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, BULLET_SPEED));
					Vector2 position = state->info.spaceship->position;

					Object bullet = create_object(
						BULLET,
						position,
						speed,
						(Vector2){0, 0},								
						BULLET_SIZE,
						0
					);
					vector_insert_last(state->objects, bullet);

					state->next_bullet++;
					gs_guns_info(state->stats)->selected_gun->bullets--;

				}
				else if (state->next_bullet >= gs_guns_info(state->stats)->selected_gun->delay)
					state->next_bullet = 0;
				else
					state->next_bullet++;
			}
		}

		//Έλεγχος συγκρούσεων
		
		bool collisionAsteroidSpaceship; 
		bool collisionCoreSpaceship;
		bool collisionAsteroidBullet;
	
		//Αστεροειδής - Διαστημόπλοιο
		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			collisionAsteroidSpaceship = false;
			collisionCoreSpaceship = false;
			if (obj != NULL) {
				if(obj->type == ASTEROID)
					collisionAsteroidSpaceship = CheckCollisionCircles(obj->position, (obj->size)/2, state->info.spaceship->position, SPACESHIP_SIZE/2);
				if (collisionAsteroidSpaceship) {

					float sizePercentage = (obj->size / (float)ASTEROID_MAX_SIZE) * 10;
					
					gs_player_info(state->stats)->spaceship_hp -= (int)(2.5 * sizePercentage);

					if (gs_player_info(state->stats)->spaceship_hp <= 0) {

						switch (state_info(state)->level_number) {
						case 1:
							gs_player_info(state->stats)->coins -= 10;
							break;
						case 2:
							gs_player_info(state->stats)->coins -= 50;
							break;
						case 3:
							gs_player_info(state->stats)->coins -= 100;
							break;
						case 4:
							gs_player_info(state->stats)->coins -= 20;
							break;
						}
						gs_player_info(state->stats)->spaceship_hp = gs_store_info(state->stats)->spaceship_hp/2;

						if (gs_player_info(state->stats)->coins < 0)
							gs_player_info(state->stats)->coins = 0;
					}

					free(obj);
					vector_set_at(state->objects, i, NULL);
					break;
				}
				if (obj->type == CORE) {
					collisionCoreSpaceship = CheckCollisionCircles(obj->position, (obj->size)/2, state->info.spaceship->position, SPACESHIP_SIZE/2);
					if (collisionCoreSpaceship) {
						
						gs_player_info(state->stats)->spaceship_hp /= 2;

						if (gs_player_info(state->stats)->spaceship_hp <= 0) {
							gs_player_info(state->stats)->coins -= 500;

							if (gs_player_info(state->stats)->coins < 0)
								gs_player_info(state->stats)->coins = 0;
								
							gs_player_info(state->stats)->spaceship_hp = gs_store_info(state->stats)->spaceship_hp;
						}

						core->type = HIDDEN;
						state_info(state)->hide_core = false;
						state_info(state)->isCoreHidden = true;

						collisionCoreSpaceship = false;

						break;
					}
				}
			}
		}

		//Διαγραφή NULL κόμβων
		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			Object last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			while (last_obj == NULL) {
				vector_remove_last(state->objects);
				last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			}
			if (obj == NULL) {
				vector_swap(state->objects, i, vector_size(state->objects)-1);
				vector_remove_last(state->objects);
				i--;
			}
		}


		//Αστεροειδής - Σφαίρα

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			for (int j = 0; j < vector_size(state->objects); j++) {
				Object obj2 = vector_get_at(state->objects, j);
				collisionAsteroidBullet = false;
				if (obj != NULL && obj2 != NULL) {
					if ((obj->type == ASTEROID || obj->type == CORE) && obj2->type == BULLET ) 
						collisionAsteroidBullet = CheckCollisionCircles(obj->position, (obj->size)/2, obj2->position, BULLET_SIZE/2);					
					if (collisionAsteroidBullet) {
						if (obj->health > gs_guns_info(state->stats)->selected_gun->damage + 1) {
							obj->health = obj->health - gs_guns_info(state->stats)->selected_gun->damage;

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						}
					
						else {

							if (obj->type == CORE) {

								state_info(state)->hide_core = false;
								state_info(state)->core = false;
								state_info(state)->spawn_core = false;

								state_info(state)->win = true;
								state_info(state)->paused = true;

								state_info(state)->drawCoinsReward = true;
								state_info(state)->coinsReward = level_core_info(level)->reward;
								state_info(state)->coinsPos = obj->position;

								gs_player_info(state->stats)->coins += level_core_info(level)->reward;

								//Καταστρέφω core και σφαίρα
								free(obj);
								vector_set_at(state->objects, i, NULL);

								free(obj2);
								vector_set_at(state->objects, j, NULL);
								break;
							}

							//Δημιουργώ τους καινούργιους αστεροειδείς
							if (obj->size > ASTEROID_MIN_SIZE*2.5) {
								for (int k = 0; k < 2; k++) {
									// Τυχαία κατεύθυνση και μήκος 1,5 φορά μεγαλύτερο της ταχύτητας του αρχικού.
									Vector2 speed = vec2_from_polar(
										obj->speed.x * 1.5,
										randf(0, 2*PI)
									);

									float object_size = randf(ASTEROID_MIN_SIZE, obj->size/2);
									float sizePercentage = ((float)object_size / (float)obj->size/2);
									
									int asteroidHP = sizePercentage * level_stats(level)->asteroid_hp;

									if (asteroidHP == 0)
										asteroidHP = 1;

									Object asteroid = create_object(
										ASTEROID,
										obj->position,									//Η θέση τους θα είναι η θέση του αστεροειδή που συγκρούστηκε
										speed,
										(Vector2){0, 0},								//Δεν χρησιμοποιείται για αστεροειδείς
										object_size,		    						//Τυχαίο μέγεθος μέχρι το μέγεθος του αστεροειδή που συγκρούστηκε δία 2
										asteroidHP
									);
									vector_insert_last(state->objects, asteroid);
								}
							}
						
							//Coin system

							float sizePercentage = ((float)obj->size / (float)ASTEROID_MAX_SIZE) * 100;
								
							int minReward = 0, maxReward = 0;
							int threshold = 10;

							for (int i = 0; i < 10; i++) {
								if (sizePercentage <= threshold) {
									if (i == 0) {
										minReward = 0;
										maxReward = (threshold * level_stats(level)->reward) / 100;
									} else {
										minReward = ((threshold - 10) * level_stats(level)->reward) / 100;
										maxReward = (threshold * level_stats(level)->reward) / 100;
									}
									break;
								}
								threshold = threshold + 10;
							}							

							if (minReward < 1) 
								minReward = 1;
							
							int coinsReward = randf(minReward, maxReward);

							state_info(state)->drawCoinsReward = true;
							state_info(state)->coinsReward = coinsReward;
							state_info(state)->coinsPos = obj->position;

							gs_player_info(state->stats)->coins += coinsReward;

							//Καταστρέφω αστεροειδή και σφαίρα
							free(obj);
							vector_set_at(state->objects, i, NULL);

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						}
					}
				}
			}
		}
		

		//Διαγραφή NULL κόμβων
		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			Object last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			while (last_obj == NULL) {
				vector_remove_last(state->objects);
				last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			}
			if (obj == NULL) {
				vector_swap(state->objects, i, vector_size(state->objects)-1);
				vector_remove_last(state->objects);
				i--;
			}
		}
	}

}

//Eliminate Minigame state update
void state_update_eliminate(State state, KeyState keys, Menu menu) {

	//Ελέγχος άμα είναι πατημένο το p ώστε το παιχνίδι να σταματήσει ή όχι.
	if (keys->p) 
		state->info.paused = !state->info.paused;

	if (state->info.paused == false || keys->n) {
		
		int count = 0;

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			if (obj != NULL) {
				if (obj->type == BULLET)
					obj->position = vec2_add(obj->position, obj->speed);
				if (obj->type == ASTEROID) {
					count++;
				}
			}
		}
	
		if (count == 0) {
			RewardMessage rewardMessage = list_node_value(state_info(state)->rewardMessages, list_first(state_info(state)->rewardMessages));
			
			state_info(state)->win = true;
			int reward = (int)(rewardMessage->rewardValue * gs_user_input(state->stats)->coinsEntered);
			state_info(state)->eliminate_reward = reward;
		}

		//Έλεγχος για όταν κάποιο κουμπί είναι πατημένο.

		if (keys->right)
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, -SPACESHIP_ROTATION);
		if (keys->left) 
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, SPACESHIP_ROTATION);
	
		bool shoot = true;
		if (keys->space) {

			if (gs_guns_info(state->stats)->selected_gun->bullets <= 0)
				shoot = false;

			if (shoot) {
				if (state->next_bullet == 0) {
					Vector2 speed = vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, BULLET_SPEED));
					Vector2 position = state->info.spaceship->position;

					Object bullet = create_object(
						BULLET,
						position,
						speed,
						(Vector2){0, 0},								
						BULLET_SIZE,
						0
					);
					vector_insert_last(state->objects, bullet);

					state->next_bullet++;
					gs_guns_info(state->stats)->selected_gun->bullets--;

				}
				else if (state->next_bullet >= gs_guns_info(state->stats)->selected_gun->delay)
					state->next_bullet = 0;
				else
					state->next_bullet++;
			}
		}

		//Έλεγχος συγκρούσεων
		
		bool collisionAsteroidBullet;

		//Αστεροειδής - Σφαίρα

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			for (int j = 0; j < vector_size(state->objects); j++) {
				Object obj2 = vector_get_at(state->objects, j);
				collisionAsteroidBullet = false;
				if (obj != NULL && obj2 != NULL) {
					if ((obj->type == ASTEROID || obj->type == CORE) && obj2->type == BULLET ) 
						collisionAsteroidBullet = CheckCollisionCircles(obj->position, (obj->size)/2, obj2->position, BULLET_SIZE/2);					
					if (collisionAsteroidBullet) {
						if (obj->health > gs_guns_info(state->stats)->selected_gun->damage + 1) {
							obj->health = obj->health - gs_guns_info(state->stats)->selected_gun->damage;

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						}
					
						else {
							
							RewardMessage rewardMessage = malloc(sizeof(RewardMessage));
							rewardMessage->rewardValue = obj->speed.x; 
							rewardMessage->position = obj->position;    
    
							list_insert_next(state->info.rewardMessages, LIST_BOF, rewardMessage);

							//Καταστρέφω αστεροειδή και σφαίρα
							free(obj);
							vector_set_at(state->objects, i, NULL);

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						}
					}
				}
			}
		}
		

		//Διαγραφή NULL κόμβων
		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			Object last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			while (last_obj == NULL) {
				vector_remove_last(state->objects);
				last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			}
			if (obj == NULL) {
				vector_swap(state->objects, i, vector_size(state->objects)-1);
				vector_remove_last(state->objects);
				i--;
			}
		}
	}

}


void state_update_hol(State state, KeyState keys, Menu menu) {

	//Ελέγχος άμα είναι πατημένο το p ώστε το παιχνίδι να σταματήσει ή όχι.
	if (keys->p) 
		state->info.paused = !state->info.paused;

	if (state->info.paused == false || keys->n) {

		int reward = (int)(state_info(state)->hol_multiplier * gs_user_input(state->stats)->coinsEntered);
		state_info(state)->hol_reward = reward;

		int count_right = 0, count_left = 0;

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			if (obj != NULL) {
				if (obj->type == BULLET)
					obj->position = vec2_add(obj->position, obj->speed);
				if (obj->type == ASTEROID) {
					if (obj->speed.x == 1)
						count_right++;
					else if (obj->speed.x == 2)
						count_left++;
				}
			}
		}		

		//Έλεγχος για όταν κάποιο κουμπί είναι πατημένο.

		if (keys->right)
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, -SPACESHIP_ROTATION);
		if (keys->left) 
			state->info.spaceship->orientation = vec2_rotate(state->info.spaceship->orientation, SPACESHIP_ROTATION);
	
		bool shoot = true;
		if (keys->space && !state_info(state)->win) {

			if (gs_guns_info(state->stats)->selected_gun->bullets <= 0)
				shoot = false;

			if (shoot) {
				if (state->next_bullet == 0) {
					Vector2 speed = vec2_add(state->info.spaceship->speed, vec2_scale(state->info.spaceship->orientation, BULLET_SPEED));
					Vector2 position = state->info.spaceship->position;

					Object bullet = create_object(
						BULLET,
						position,
						speed,
						(Vector2){0, 0},								
						BULLET_SIZE,
						0
					);
					vector_insert_last(state->objects, bullet);

					state->next_bullet++;
					gs_guns_info(state->stats)->selected_gun->bullets--;

				}
				else if (state->next_bullet >= gs_guns_info(state->stats)->selected_gun->delay)
					state->next_bullet = 0;
				else
					state->next_bullet++;
			}
		}

		//Έλεγχος συγκρούσεων
		
		bool collisionAsteroidBullet;

		//Αστεροειδής - Σφαίρα

		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			for (int j = 0; j < vector_size(state->objects); j++) {
				Object obj2 = vector_get_at(state->objects, j);
				collisionAsteroidBullet = false;
				if (obj != NULL && obj2 != NULL) {
					if ((obj->type == ASTEROID || obj->type == CORE) && obj2->type == BULLET ) 
						collisionAsteroidBullet = CheckCollisionCircles(obj->position, (obj->size)/2, obj2->position, BULLET_SIZE/2);					
					if (collisionAsteroidBullet) {
						if (obj->health > gs_guns_info(state->stats)->selected_gun->damage + 1) {
							obj->health = obj->health - gs_guns_info(state->stats)->selected_gun->damage;

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						} else {
							const float distance = 250.0f; 
							
							if (obj->speed.x == 3) {
								state_info(state)->win = true;
								int reward = (int)(state_info(state)->hol_multiplier * gs_user_input(state->stats)->coinsEntered);
								state_info(state)->hol_reward = reward;	

								free(obj);
								vector_set_at(state->objects, i, NULL);

								free(obj2);
								vector_set_at(state->objects, j, NULL);
								break;
							}

							if (obj->speed.x == 0) {
								
								if (count_right == 0 || count_left == 0) {

									Vector2 front_pos = {
										state->info.spaceship->position.x,
										state->info.spaceship->position.y + distance
									};

									float new_size = randf(50, 120);

									if (count_right == 0) { 	//Έχει επιλέξει μεγαλύτερο

										if (new_size >= obj->size) {
											switch (state_info(state)->hol_round) {
											case 1: 
												state_info(state)->hol_multiplier += 0.10;
												break;
											case 2:
												state_info(state)->hol_multiplier += 0.15;
												break;
											case 3:
												state_info(state)->hol_multiplier += 0.20;
												break;
											default:
												if (state_info(state)->hol_round >= 4)
													state_info(state)->hol_multiplier += 0.25;
												break;
											}					
											int reward = (int)(state_info(state)->hol_multiplier * gs_user_input(state->stats)->coinsEntered);
											state_info(state)->hol_reward = reward;											
											state_info(state)->hol_round++;
										} else {
											state_info(state)->win = true;
											state_info(state)->hol_multiplier = 0.0f;
											state_info(state)->hol_reward = 0;
										}

									}
								
									if (count_left == 0) { 	//Έχει επιλέξει μικρότερο

										if (new_size < obj->size) {
											switch (state_info(state)->hol_round) {
											case 1: 
												state_info(state)->hol_multiplier += 0.10;
												break;
											case 2:
												state_info(state)->hol_multiplier += 0.15;
												break;
											case 3:
												state_info(state)->hol_multiplier += 0.20;
												break;
											default:
												if (state_info(state)->hol_round >= 4)
													state_info(state)->hol_multiplier += 0.25;
												break;
											}
											int reward = (int)(state_info(state)->hol_multiplier * gs_user_input(state->stats)->coinsEntered);
											state_info(state)->hol_reward = reward;
											state_info(state)->hol_round++;
										} else {
											state_info(state)->hol_multiplier = 0.0f;
											state_info(state)->win = true;
											state_info(state)->hol_reward = 0;
										}

									}

									Object front_asteroid = create_object(ASTEROID, front_pos, (Vector2){0, 0}, (Vector2){0, 0}, new_size, 10);

									vector_insert_last(state->objects, front_asteroid);

									//Καταστρέφω αστεροειδή και σφαίρα
									free(obj);
									vector_set_at(state->objects, i, NULL);
								}

								free(obj2);
								vector_set_at(state->objects, j, NULL);
								break;
							}

							if (obj->speed.x == 1 && count_left == 0) {

								Vector2 left_pos = {
									state->info.spaceship->position.x - distance,
									state->info.spaceship->position.y
								};
    							
								Object left_asteroid = create_object(ASTEROID, left_pos, (Vector2){2, 0}, (Vector2){0, 0}, 120, 10);   
								vector_insert_last(state->objects, left_asteroid);

								list_remove_next(state->info.rewardMessages, LIST_BOF);
							}
							
							if (obj->speed.x == 2 && count_right == 0) {
								Vector2 right_pos = {
									state->info.spaceship->position.x + distance,
									state->info.spaceship->position.y
								};

								Object right_asteroid = create_object(ASTEROID, right_pos, (Vector2){1, 0}, (Vector2){0, 0}, 120, 10); 

    							vector_insert_last(state->objects, right_asteroid);

								list_remove_next(state->info.rewardMessages, LIST_BOF);
							}


							RewardMessage rewardMessage = malloc(sizeof(RewardMessage));
							rewardMessage->asteroid = obj->speed.x; 

							list_insert_next(state->info.rewardMessages, LIST_BOF, rewardMessage);


							//Καταστρέφω αστεροειδή και σφαίρα
							free(obj);
							vector_set_at(state->objects, i, NULL);

							free(obj2);
							vector_set_at(state->objects, j, NULL);
							break;
						}
					}
				}
			}
		}
		

		//Διαγραφή NULL κόμβων
		for (int i = 0; i < vector_size(state->objects); i++) {
			Object obj = vector_get_at(state->objects, i);
			Object last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			while (last_obj == NULL) {
				vector_remove_last(state->objects);
				last_obj = vector_get_at(state->objects, vector_size(state->objects)-1);
			}
			if (obj == NULL) {
				vector_swap(state->objects, i, vector_size(state->objects)-1);
				vector_remove_last(state->objects);
				i--;
			}
		}
	}

}


// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη.

void state_destroy(State state) {

	for (int i = 0; i < vector_size(state->objects); i++) {
		free(vector_get_at(state->objects, i));
	}

	vector_destroy(state->objects);
	free(state->info.spaceship);

	List rewardMessages = state_info(state)->rewardMessages;
    for (ListNode node = list_first(rewardMessages); node != LIST_EOF; node = list_next(rewardMessages, node)) {
        free(list_node_value(rewardMessages, node));
    }
    list_destroy(rewardMessages);

	free(state);
}