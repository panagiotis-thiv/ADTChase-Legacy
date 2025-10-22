#pragma once

#include "raylib.h"
#include "menu.h"
#include "ADTList.h"
#include "level.h"
#include "global_stats.h"
#include "ADTVector.h"

// Χαρακτηριστικά αντικειμένων
#define ASTEROID_NUM 6
#define ASTEROID_MIN_SIZE 10
#define ASTEROID_MAX_SIZE 80
#define ASTEROID_MIN_SPEED 1
#define ASTEROID_MAX_SPEED 1.5
#define ASTEROID_MIN_DIST 300
#define ASTEROID_MAX_DIST 400
#define BULLET_SPEED 10
#define BULLET_SIZE 3
#define BULLET_DELAY 15
#define SPACESHIP_SIZE 40
#define SPACESHIP_ROTATION (PI/32)
#define SPACESHIP_ACCELERATION 0.1
#define SPACESHIP_SLOWDOWN 0.98

#define SCREEN_WIDTH 900	// Πλάτος της οθόνης
#define SCREEN_HEIGHT 700	// Υψος της οθόνης

typedef enum {
	SPACESHIP, ASTEROID, BULLET, CORE, HIDDEN, BOSS_SPACESHIP
} ObjectType;

typedef enum {
	IDLE, JUMPING, FALLING, MOVING_UP, MOVING_DOWN
} VerticalMovement;

// Πληροφορίες για κάθε αντικείμενο
typedef struct object {
	ObjectType type;			// Τύπος (Διαστημόπλοιο, Αστεροειδής, Σφαίρα)
	Vector2 position;			// Θέση
	Vector2 speed;				// Ταχύτητα (pixels/frame)
	double size;				// Μέγεθος (pixels)
	Vector2 orientation;		// Κατεύθυνση (μόνο για διαστημόπλοιο)
	int health;
	
}* Object;

Object create_object(ObjectType type, Vector2 position, Vector2 speed, Vector2 orientation, double size, int health);

typedef struct reward_message{
	//Eliminate
    float rewardValue;
    Vector2 position;
	//Higher or lower
	int asteroid;
}* RewardMessage;

// Γενικές πληροφορίες για την κατάσταση του παιχνιδιού
typedef struct state_info {
	Object spaceship;				// πληροφορίες για τη το διαστημόπλοιο
	bool paused;					// true αν το παιχνίδι είναι paused

	int coins;						// το τρέχον σκορ
	bool drawCoinsReward;
	int coinsReward;
	Vector2 coinsPos;
	
	bool spawn_core; 				//Άμα πρέπει να δημιουργηθεί το core
	bool tp_core; 					//Άμα πρέπει να ξανα εμφανιστεί κοντά στον παίχτη το core
	bool hide_core; 				//Άμα το core πρέπει να "κρυφτεί"

	float coreSpawnTimer;
	float coreHideTimer;
	float coreTPTimer;
	
	bool core; 						//Άμα υπάρχει core
	bool isCoreHidden; 				//Άμα το core είναι κρυμμένο.

	bool win; 						//Άμα το core έχει καταστραφεί τότε το state τελειώνει
	bool won; 						//Άμα έχει ήδη νικήσει το level

	int level_number;
	
	//Eliminate Message Info
	List rewardMessages;
	float eliminate_reward;
	bool eliminate;

	//Higher or Lower
	bool hol;
	float hol_multiplier;
	int hol_round;
	float hol_reward;

}* StateInfo;

// Πληροφορίες για το ποια πλήκτρα είναι πατημένα
typedef struct key_state {
	bool up;						// true αν το αντίστοιχο πλήκτρο είναι πατημένο
	bool left;
	bool right;
	bool enter;
	bool space;
	bool n;
	bool p;
	bool alt;
}* KeyState;

// Η κατάσταση του παιχνιδιού (handle)
typedef struct state* State;

// Δημιουργεί και επιστρέφει την αρχική κατάσταση του παιχνιδιού

State state_create(Levels level, GlobalStats stats);

// Επιστρέφει τις βασικές πληροφορίες του παιχνιδιού στην κατάσταση state

StateInfo state_info(State state);

// Επιστρέφει μια λίστα με όλα τα αντικείμενα του παιχνιδιού στην κατάσταση state,
// των οποίων η θέση position βρίσκεται εντός του παραλληλογράμμου με πάνω αριστερή
// γωνία top_left και κάτω δεξιά bottom_right.

List state_objects(State state, Vector2 top_left, Vector2 bottom_right);

void vector_swap(Vector vec, int pos1, int pos2);

// Ενημερώνει την κατάσταση state του παιχνιδιού μετά την πάροδο 1 frame.
// Το keys περιέχει τα πλήκτρα τα οποία ήταν πατημένα κατά το frame αυτό.

void state_update(State state, KeyState keys, Menu menu);

// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη.

void state_destroy(State state);

State state_create_eliminate(GlobalStats stats);

void state_update_eliminate(State state, KeyState keys, Menu menu);

State state_create_hol(GlobalStats stats);

void state_update_hol(State state, KeyState keys, Menu menu);