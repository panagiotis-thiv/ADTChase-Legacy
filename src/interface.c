#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"
#include "state.h"
#include "interface.h"
#include "vec2.h"
#include "menu.h"
#include "global_stats.h"
#include "boss_fight.h"


// Assets
Texture spaceship_img;
Texture background;
Texture coin;
Texture heart;

Music background_music;

void interface_init() {
	// Αρχικοποίηση του παραθύρου
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "game");
	SetTargetFPS(60);
    InitAudioDevice();


	// Φόρτωση εικόνων και ήχων
	spaceship_img = LoadTextureFromImage(LoadImage("assets/spaceship.png"));
	background = LoadTextureFromImage(LoadImage("assets/background.png"));
	coin = LoadTextureFromImage(LoadImage("assets/coin.png"));
	heart = LoadTextureFromImage(LoadImage("assets/heart.png"));

	background_music = LoadMusicStream("assets/background_music.mp3");

	PlayMusicStream(background_music);
}

void interface_close() {
	CloseAudioDevice();
	CloseWindow();
}

//Μετατροπή καρτισιανές συντεταγμένες σε συντεταγμένες για την raylib
Vector2 cartToRay(State state, Vector2 vec) {

	Vector2 rayCenter = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
	Vector2 spaceshipPos = state_info(state)->spaceship->position;
	
	return (Vector2){rayCenter.x - spaceshipPos.x + vec.x, rayCenter.y - (spaceshipPos.y * (-1)) + (vec.y * (-1))};

}

// Draw game (one frame)
void interface_draw_frame(State state, GlobalStats stats) {

	UpdateMusicStream(background_music);
	BeginDrawing();
	ClearBackground(BLACK);
	
	int current_SCREEN_WIDTH = GetScreenWidth();
	int current_SCREEN_HEIGHT = GetScreenHeight();

	DrawTexturePro(background, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Vector2){0,0}, 0, DARKGRAY);

	Rectangle spaceshipRectangle = {0,0,spaceship_img.width,spaceship_img.height};
	Vector2 spaceshipCenter = {spaceship_img.width / 2, spaceship_img.height/2};

	float rotation = atan2(state_info(state)->spaceship->orientation.y, state_info(state)->spaceship->orientation.x *(-1)) * RAD2DEG;

	DrawTexturePro(spaceship_img,spaceshipRectangle,(Rectangle){SCREEN_WIDTH/2, SCREEN_HEIGHT/2, spaceshipRectangle.width,spaceshipRectangle.height},
				   spaceshipCenter,rotation,WHITE);

	Vector2 topleft = {state_info(state)->spaceship->position.x + ((3*SCREEN_WIDTH)*(-1)), state_info(state)->spaceship->position.y + (3*SCREEN_HEIGHT)};
	Vector2 bottomright = {state_info(state)->spaceship->position.x + (3*SCREEN_WIDTH), state_info(state)->spaceship->position.y + ((3*SCREEN_HEIGHT)*(-1))};

	List objects = state_objects(state, topleft, bottomright);

	for (ListNode node = list_first(objects); node != LIST_EOF; node = list_next(objects, node)) {
		Object obj = list_node_value(objects, node);

		if (obj->type == ASTEROID) {
			Vector2 objPos = cartToRay(state, obj->position);
			DrawCircleLines(objPos.x, objPos.y, obj->size/2, LIGHTGRAY);
		} 
		else if (obj->type == BULLET) {
			Vector2 objPos = cartToRay(state, obj->position);
			DrawCircle(objPos.x, objPos.y, obj->size, LIME);
		}
		else if (obj->type == CORE){
			Vector2 objPos = cartToRay(state, obj->position);
			DrawCircleLines(objPos.x, objPos.y, obj->size/2, RED);
		}

	}
	
	Rectangle coinRectangle = {0,0,coin.width,coin.height};
	Vector2 coinCenter = {coin.width / 2, coin.height/2};
	
	if (gs_player_info(stats)->coins < 10)
		DrawTexturePro(coin, coinRectangle, (Rectangle){60, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	else if (gs_player_info(stats)->coins < 100)
		DrawTexturePro(coin, coinRectangle, (Rectangle){80, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	else if (gs_player_info(stats)->coins < 1000)
		DrawTexturePro(coin, coinRectangle, (Rectangle){100, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	else
		DrawTexturePro(coin, coinRectangle, (Rectangle){130, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);

	DrawText(TextFormat("%d", gs_player_info(stats)->coins), 10, 10, 40, GRAY);
	DrawFPS(SCREEN_WIDTH - 80, 0);

	//Είχα ένα περίεργο bug με το παρακάτω στο eliminate state που δεν μπορώ να βρω / να το ξανακάνω επίτηδες 
	//όποτε έβαλα τον δεύτερο όρο μήπως το φτιάξει.
	if (state_info(state)->drawCoinsReward && !state_info(state)->eliminate && !state_info(state)->hol) {
		Vector2 coinPos = cartToRay(state, state_info(state)->coinsPos);
		DrawText(TextFormat("+ %d", state_info(state)->coinsReward), coinPos.x, coinPos.y, 20, GOLD);
	}

	//Eliminate minigame
	if (state_info(state)->eliminate) {
		for (ListNode node = list_first(state_info(state)->rewardMessages); node != LIST_EOF; node = list_next(state_info(state)->rewardMessages, node)) {
			RewardMessage rewardMessage = list_node_value(state_info(state)->rewardMessages, node);
			Vector2 rewardPos = cartToRay(state, rewardMessage->position);
			if (list_first(state_info(state)->rewardMessages) == node && list_size(state_info(state)->rewardMessages) > 9) 
				DrawText(TextFormat("Reward: %.1fx", rewardMessage->rewardValue), rewardPos.x-10, rewardPos.y, 20, GOLD);
			else
				DrawText(TextFormat("Reward: %.1fx", rewardMessage->rewardValue), rewardPos.x-10, rewardPos.y, 20, WHITE);
		}

		
		DrawText(TextFormat("Current bet: %d", gs_user_input(stats)->coinsEntered), SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 345, 25, GOLD);
		if (!state_info(state)->win)
			DrawText(TextFormat("You get back: -"), SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 305, 25, GOLD);
		
		

		if (state_info(state)->win) 
			DrawText(TextFormat("You get back: %d", (int)state_info(state)->eliminate_reward), SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 300, 25, GOLD);
	}

	//Higher or lower minigame
	if (state_info(state)->hol) {

		DrawText(TextFormat("Current bet: %d (Multiplier: %.2fx)", gs_user_input(stats)->coinsEntered, state_info(state)->hol_multiplier), SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 345, 25, GOLD);
		if (list_size(state_info(state)->rewardMessages) == 0) {
			DrawText(TextFormat("Higher!"), 670, 420, 25, WHITE);
			DrawText(TextFormat("Lower!"), 160, 420, 25, WHITE);
		}
		DrawText(TextFormat("Cash out: %d", (int)state_info(state)->hol_reward), 385, 665, 25, GOLD);


		for (ListNode node = list_first(state_info(state)->rewardMessages); node != LIST_EOF; node = list_next(state_info(state)->rewardMessages, node)) {
			RewardMessage rewardMessage = list_node_value(state_info(state)->rewardMessages, node);

			if (list_size(state_info(state)->rewardMessages) > 0) {

				if (rewardMessage->asteroid == 1) {
					DrawText(TextFormat("Higher!"), 670, 420, 25, GREEN);
					DrawText(TextFormat("Lower!"), 160, 420, 25, WHITE);
				} else if (rewardMessage->asteroid == 2) {
					DrawText(TextFormat("Higher!"), 670, 420, 25, WHITE);
					DrawText(TextFormat("Lower!"), 160, 420, 25, GREEN);
				}
			}
		}


	}

	Rectangle heartRectangle = {0,0,heart.width,heart.height};
	Vector2 heartCenter = {heart.width / 2, heart.height/2};	

	DrawTexturePro(heart, heartRectangle, (Rectangle){SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, heartRectangle.width,heartRectangle.height}, heartCenter, 0, WHITE);

	if (gs_store_info(stats)->spaceship_hp >= 100)
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 138, SCREEN_HEIGHT - 30, 20, GRAY);
	else if (gs_store_info(stats)->spaceship_hp >= 500)
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 155, SCREEN_HEIGHT - 30, 20, GRAY);
	else
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 120, SCREEN_HEIGHT - 30, 20, GRAY);

	if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->pistol) {
		DrawText(TextFormat("Pistol: %d / 50 BULLETS", gs_guns_info(stats)->pistol->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->rifle) {
		DrawText(TextFormat("Rifle: %d / 100 BULLETS", gs_guns_info(stats)->rifle->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->sniper) {
		DrawText(TextFormat("Sniper: %d / 25 BULLETS", gs_guns_info(stats)->sniper->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	}

	
	if (state_info(state)->paused) {
		PauseMusicStream(background_music);
		if (!state_info(state)->win) {
			DrawText(
				"PRESS [P] TO PLAY AGAIN OR [ALT] TO EXIT",
				GetScreenWidth() / 2 - MeasureText("PRESS [P] TO PLAY AGAIN OR [ALT] TO EXIT", 20) / 2,
				GetScreenHeight() / 2 - 50, 20, GRAY
			);
		}
		else {
			DrawText(
				"LETS GO YOU DESTROYED THE CORE!",
				GetScreenWidth() / 2 - MeasureText("PRESS [P] TO PLAY AGAIN OR [ALT] TO EXIT", 20) / 2,
				GetScreenHeight() / 2 - 50, 20, GRAY
			);
		}
	} 
	else 
		ResumeMusicStream(background_music);

	EndDrawing();
}

void draw_main_menu(Menu menu) {

	DrawText("Version: 0.9.5 | Playable 80%", SCREEN_WIDTH/2 - 230, SCREEN_HEIGHT/2 + 300, 33, DARKGREEN);

	DrawText("ADTChase", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 300, 70, DARKGREEN);

	if (selected_menu(menu) == 1)
		DrawText("> Play <", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, 40, BLUE);
	else 
		DrawText("  Play  ", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, 40, BLUE);

	if (selected_menu(menu) == 2)
		DrawText("> Store <", SCREEN_WIDTH/2 - 100, ((SCREEN_HEIGHT/2)) - 40, 40, BLUE);
	else
		DrawText("  Store  ", SCREEN_WIDTH/2 - 100, ((SCREEN_HEIGHT/2)) - 40, 40, BLUE);
	
	if (selected_menu(menu) == 3) 
		DrawText("> Help <", SCREEN_WIDTH/2 - 100, ((SCREEN_HEIGHT/2)) + 20, 40, BLUE);
	else
		DrawText("  Help  ", SCREEN_WIDTH/2 - 100, ((SCREEN_HEIGHT/2)) + 20, 40, BLUE);

}

void draw_help_menu(Menu menu) {
	
	DrawText("ADTChase", 10, 10, 30, DARKGREEN);

	DrawText("Help", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT/2 - 300, 50, DARKGREEN);

	if (get_page(menu) == 1) {
		DrawText("  1/2 > ", SCREEN_WIDTH - 140, SCREEN_HEIGHT - 50, 40, DARKGREEN);

		DrawText("ADTChase is a simple game based on the\nclassic asteroid game.\nThere are currently 5 levels\nyou have to beat", SCREEN_WIDTH/2 - 400 , SCREEN_HEIGHT/2 - 200, 35, BLUE);
		DrawText("To beat each level you have to destroy\nthe \"core\" of that level, which is a uni-\nque asteroid. It appears after a while\nand it looks different.", SCREEN_WIDTH/2 - 400 , SCREEN_HEIGHT/2 + 50, 35, BLUE);
	}

	if (get_page(menu) == 2) {
		DrawText("< 2/2   ", SCREEN_WIDTH - 140, SCREEN_HEIGHT - 50, 40, DARKGREEN);

		DrawText("To destroy each core you will need to\nupgrade your stats (health, type of\ngun) which you can purchase through\nthe store using coins.", SCREEN_WIDTH/2 - 400 , SCREEN_HEIGHT/2 - 200, 35, BLUE);
		DrawText("To get coins you will need to destroy\nasteroids! If you collide with\nasteroids you lose HP and if you die you\nlose some of your coins (depending on the\nlevel) !", SCREEN_WIDTH/2 - 400 , SCREEN_HEIGHT/2 + 50, 35, BLUE);
	}

	DrawText("Press [ALT] to go back.", 10, SCREEN_HEIGHT - 40, 20, DARKGREEN);

}


void draw_level_menu(Menu menu, GlobalStats stats) {

	DrawText("ADTChase", SCREEN_WIDTH/2 - 180, SCREEN_HEIGHT/2 - 300, 70, DARKGREEN);

	if (get_page(menu) != 7 && get_page(menu) != 8 && get_page(menu) != 9) {

		//Level 1
		if (gs_levels_info(stats)->level1 == 1) {
			DrawCircleLines(200, 300, 55, RED);
			DrawCircleLines(200, 300, 56, RED);
			DrawCircleLines(200, 300, 57, RED);
			DrawCircleLines(200, 300, 58, RED);

			//Line 1->2
			DrawLine(259, 299, 292, 299, RED);
			DrawLine(259, 300, 292, 300, RED);
			DrawLine(259, 301, 292, 301, RED);


			//Line below 1 connecting to line below 1 & 2
			DrawLine(199, 358, 199, 385, RED);
			DrawLine(200, 358, 200, 385, RED);
			DrawLine(201, 358, 201, 385, RED);

		} else {
			DrawCircleLines(200, 300, 55, GREEN);
			DrawCircleLines(200, 300, 56, GREEN);
			DrawCircleLines(200, 300, 57, GREEN);
			DrawCircleLines(200, 300, 58, GREEN);

			//Line 1->2
			DrawLine(259, 299, 292, 299, GREEN);
			DrawLine(259, 300, 292, 300, GREEN);
			DrawLine(259, 301, 292, 301, GREEN);


			//Line below level 1 & 2
			DrawLine(200, 384, 425, 384, GREEN);
			DrawLine(200, 385, 425, 385, GREEN);
			DrawLine(200, 386, 425, 386, GREEN);

			//Line below 1 connecting to line below 1 & 2
			DrawLine(199, 358, 199, 385, GREEN);
			DrawLine(200, 358, 200, 385, GREEN);
			DrawLine(201, 358, 201, 385, GREEN);
		}

		if (get_page(menu) == 1)
			DrawText("> Level 1 <", 150, 290, 20, WHITE);
		else
			DrawText("  Level 1  ", 150, 290, 20, WHITE);
		
		//Level 2

		if (gs_levels_info(stats)->level2 == 0) {

			DrawCircleLines(350, 300, 55, GRAY);
			DrawCircleLines(350, 300, 56, GRAY);
			DrawCircleLines(350, 300, 57, GRAY);
			DrawCircleLines(350, 300, 58, GRAY);

			//Line below level 1 & 2
			DrawLine(200, 384, 425, 384, LIGHTGRAY);
			DrawLine(200, 385, 425, 385, LIGHTGRAY);
			DrawLine(200, 386, 425, 386, LIGHTGRAY);

			//Line below 2 connecting to line below 1 & 2
			DrawLine(349, 358, 349, 385, LIGHTGRAY);
			DrawLine(350, 358, 350, 385, LIGHTGRAY);
			DrawLine(351, 358, 351, 385, LIGHTGRAY);

			//Line 2->3
			DrawLine(408, 299, 445, 299, LIGHTGRAY);
			DrawLine(408, 300, 445, 300, LIGHTGRAY);
			DrawLine(408, 301, 445, 301, LIGHTGRAY);

		} else if (gs_levels_info(stats)->level2 == 1) {

			DrawCircleLines(350, 300, 55, RED);
			DrawCircleLines(350, 300, 56, RED);
			DrawCircleLines(350, 300, 57, RED);
			DrawCircleLines(350, 300, 58, RED);

			//Line below level 1 & 2
			DrawLine(200, 384, 425, 384, RED);
			DrawLine(200, 385, 425, 385, RED);
			DrawLine(200, 386, 425, 386, RED);

			//Line below 2 connecting to line below 1 & 2
			DrawLine(349, 358, 349, 385, RED);
			DrawLine(350, 358, 350, 385, RED);
			DrawLine(351, 358, 351, 385, RED);

			//Line 2->3
			DrawLine(408, 299, 445, 299, RED);
			DrawLine(408, 300, 445, 300, RED);
			DrawLine(408, 301, 445, 301, RED);
		} else if (gs_levels_info(stats)->level2 == 2) {

			DrawCircleLines(350, 300, 55, GREEN);
			DrawCircleLines(350, 300, 56, GREEN);
			DrawCircleLines(350, 300, 57, GREEN);
			DrawCircleLines(350, 300, 58, GREEN);

			//Line below level 1 & 2
			DrawLine(200, 384, 425, 384, GREEN);
			DrawLine(200, 385, 425, 385, GREEN);
			DrawLine(200, 386, 425, 386, GREEN);

			//Line below 2 connecting to line below 1 & 2
			DrawLine(349, 358, 349, 385, GREEN);
			DrawLine(350, 358, 350, 385, GREEN);
			DrawLine(351, 358, 351, 385, GREEN);

			//Line 2->3
			DrawLine(408, 299, 445, 299, GREEN);
			DrawLine(408, 300, 445, 300, GREEN);
			DrawLine(408, 301, 445, 301, GREEN);
		}

		if (get_page(menu) == 2)
			DrawText("> Level 2 <", 300, 290, 20, WHITE);
		else
			DrawText("  Level 2  ", 300, 290, 20, WHITE);

		//Level 3

		if (gs_levels_info(stats)->level3 == 0) {
			DrawCircleLines(500, 300, 55, GRAY);
			DrawCircleLines(500, 300, 56, GRAY);
			DrawCircleLines(500, 300, 57, GRAY);
			DrawCircleLines(500, 300, 58, GRAY);

			//Line 3->4
			DrawLine(558, 299, 592, 299, LIGHTGRAY);
			DrawLine(558, 300, 592, 300, LIGHTGRAY);
			DrawLine(558, 301, 592, 301, LIGHTGRAY);
			
			//Line below 3 connecting to line below 3 & 4
			DrawLine(499, 358, 499, 385, LIGHTGRAY);
			DrawLine(500, 358, 500, 385, LIGHTGRAY);
			DrawLine(501, 358, 501, 385, LIGHTGRAY);

		} else if (gs_levels_info(stats)->level3 == 1) {

			DrawCircleLines(500, 300, 55, RED);
			DrawCircleLines(500, 300, 56, RED);
			DrawCircleLines(500, 300, 57, RED);
			DrawCircleLines(500, 300, 58, RED);

			//Line 3->4
			DrawLine(558, 299, 592, 299, RED);
			DrawLine(558, 300, 592, 300, RED);
			DrawLine(558, 301, 592, 301, RED);
			
			//Line below 3 connecting to line below 3 & 4
			DrawLine(499, 358, 499, 385, RED);
			DrawLine(500, 358, 500, 385, RED);
			DrawLine(501, 358, 501, 385, RED);

		} else {
			DrawCircleLines(500, 300, 55, GREEN);
			DrawCircleLines(500, 300, 56, GREEN);
			DrawCircleLines(500, 300, 57, GREEN);
			DrawCircleLines(500, 300, 58, GREEN);

			//Line 3->4
			DrawLine(558, 299, 592, 299, GREEN);
			DrawLine(558, 300, 592, 300, GREEN);
			DrawLine(558, 301, 592, 301, GREEN);
			
			//Line below 3 connecting to line below 3 & 4
			DrawLine(499, 358, 499, 385, GREEN);
			DrawLine(500, 358, 500, 385, GREEN);
			DrawLine(501, 358, 501, 385, GREEN);

		}

		if (get_page(menu) == 3)
			DrawText("> Level 3 <", 450, 290, 20, WHITE);
		else
			DrawText("  Level 3  ", 450, 290, 20, WHITE);

		//Level 4

		if (gs_levels_info(stats)->level4 == 0) {

			DrawCircleLines(650, 300, 55, GRAY);
			DrawCircleLines(650, 300, 56, GRAY);
			DrawCircleLines(650, 300, 57, GRAY);
			DrawCircleLines(650, 300, 58, GRAY);

			//Line below level 3 & 4
			DrawLine(425, 384, 650, 384, LIGHTGRAY);
			DrawLine(425, 385, 650, 385, LIGHTGRAY);
			DrawLine(425, 386, 650, 386, LIGHTGRAY);

			//Line below 4 connecting to line below 3 & 4
			DrawLine(649, 358, 649, 385, LIGHTGRAY);
			DrawLine(650, 358, 650, 385, LIGHTGRAY);
			DrawLine(651, 358, 651, 385, LIGHTGRAY);

		} else if (gs_levels_info(stats)->level4 == 1) {
			DrawCircleLines(650, 300, 55, RED);
			DrawCircleLines(650, 300, 56, RED);
			DrawCircleLines(650, 300, 57, RED);
			DrawCircleLines(650, 300, 58, RED);

			//Line below level 3 & 4
			DrawLine(425, 384, 650, 384, RED);
			DrawLine(425, 385, 650, 385, RED);
			DrawLine(425, 386, 650, 386, RED);

			//Line below 4 connecting to line below 3 & 4
			DrawLine(649, 358, 649, 385, RED);
			DrawLine(650, 358, 650, 385, RED);
			DrawLine(651, 358, 651, 385, RED);
		} else {
			DrawCircleLines(650, 300, 55, GREEN);
			DrawCircleLines(650, 300, 56, GREEN);
			DrawCircleLines(650, 300, 57, GREEN);
			DrawCircleLines(650, 300, 58, GREEN);

			//Line below level 3 & 4
			DrawLine(425, 384, 650, 384, GREEN);
			DrawLine(425, 385, 650, 385, GREEN);
			DrawLine(425, 386, 650, 386, GREEN);

			//Line below 4 connecting to line below 3 & 4
			DrawLine(649, 358, 649, 385, GREEN);
			DrawLine(650, 358, 650, 385, GREEN);
			DrawLine(651, 358, 651, 385, GREEN);
		}

		if (get_page(menu) == 4)
			DrawText("> Level 4 <", 600, 290, 20, WHITE);
		else
			DrawText("  Level 4  ", 600, 290, 20, WHITE);

		//Level 5
		
		
		if (gs_levels_info(stats)->level5 == 0) {

			DrawCircleLines(425, 480, 55, DARKPURPLE);
			DrawCircleLines(425, 480, 56, DARKPURPLE);
			DrawCircleLines(425, 480, 57, DARKPURPLE);
			DrawCircleLines(425, 480, 58, DARKPURPLE);
			DrawCircleLines(425, 480, 59, DARKPURPLE);
			DrawCircleLines(425, 480, 60, DARKPURPLE);
			DrawCircleLines(425, 480, 61, DARKPURPLE);
			DrawCircleLines(425, 480, 62, DARKPURPLE);


			//Line above level 5
			DrawLine(425, 385, 424, 418, LIGHTGRAY);
			DrawLine(426, 385, 425, 418, LIGHTGRAY);
			DrawLine(427, 385, 426, 418, LIGHTGRAY);

		} else if (gs_levels_info(stats)->level5 == 1) {
			DrawCircleLines(425, 480, 55, DARKPURPLE);
			DrawCircleLines(425, 480, 56, DARKPURPLE);
			DrawCircleLines(425, 480, 57, DARKPURPLE);
			DrawCircleLines(425, 480, 58, DARKPURPLE);
			DrawCircleLines(425, 480, 59, DARKPURPLE);
			DrawCircleLines(425, 480, 60, DARKPURPLE);
			DrawCircleLines(425, 480, 61, DARKPURPLE);
			DrawCircleLines(425, 480, 62, DARKPURPLE);


			//Line above level 5
			DrawLine(425, 385, 424, 418, RED);
			DrawLine(426, 385, 425, 418, RED);
			DrawLine(427, 385, 426, 418, RED);
		} else {
			DrawCircleLines(425, 480, 55, DARKBLUE);
			DrawCircleLines(425, 480, 56, DARKBLUE);
			DrawCircleLines(425, 480, 57, DARKBLUE);
			DrawCircleLines(425, 480, 58, DARKBLUE);
			DrawCircleLines(425, 480, 59, DARKBLUE);
			DrawCircleLines(425, 480, 60, DARKBLUE);
			DrawCircleLines(425, 480, 61, DARKBLUE);
			DrawCircleLines(425, 480, 62, DARKBLUE);

			//Line above level 5
			DrawLine(425, 385, 424, 418, GREEN);
			DrawLine(426, 385, 425, 418, GREEN);
			DrawLine(427, 385, 426, 418, GREEN);
		}
		
		if (get_page(menu) == 5)
			DrawText("> Level 5 <", 375, 470, 20, WHITE);
		else
			DrawText("  Level 5  ", 375, 470, 20, WHITE);


		//GAMBLE Level

		if (get_page(menu) == 6) {

			if (gs_levels_info(stats)->level2 != 2) {
				DrawText("> ? ? ? <", 800, 650, 20, WHITE);
				DrawText("Beat level 2\n to find out \nwhat this is!", 770, 550, 20, WHITE);
			} else
				DrawText("> Gamble <", 800, 650, 20, WHITE);

		} else {

			if (gs_levels_info(stats)->level2 != 2)
				DrawText("  ? ? ?  ", 800, 650, 20, WHITE);
			else
				DrawText("  Gamble  ", 800, 650, 20, WHITE);
		}
	} else {

		DrawText("Test your luck! You will win!", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 200, 30, WHITE);
	    DrawText(TextFormat("Current bet: %d", gs_user_input(stats)->coinsEntered), SCREEN_WIDTH/2 - 125, SCREEN_HEIGHT/2 - 150, 30, GOLD);

		if (get_page(menu) == 7)
			DrawText("> Eliminate! <", 190, 340, 20, WHITE);
		else
			DrawText("  Eliminate!  ", 190, 340, 20, WHITE);

		DrawCircleLines(250, 350, 64, GOLD);
		DrawCircleLines(250, 350, 65, GOLD);
		DrawCircleLines(250, 350, 66, GOLD);
		DrawCircleLines(250, 350, 67, GOLD);

		if (get_page(menu) == 9)
			DrawText("> Guess! <", 600, 340, 20, WHITE);
		else
			DrawText("  Guess!  ", 600, 340, 20, WHITE);

		DrawCircleLines(650, 350, 64, GOLD);
		DrawCircleLines(650, 350, 65, GOLD);
		DrawCircleLines(650, 350, 66, GOLD);
		DrawCircleLines(650, 350, 67, GOLD);

		if (get_page(menu) == 8)
			DrawText("> Enter amount: <", 360, 320, 20, WHITE);
		else
			DrawText("  Enter amount:  ", 360, 320, 20, WHITE);

	// Draw input prompt for coins
	if (gs_user_input(stats)->isEnteringInput) {
		DrawText(">>>:",  400, 350, 25, WHITE);
		DrawText(gs_user_input(stats)->inputBuffer, 445, 350, 25, WHITE);
	}

	Rectangle coinRectangle = {0,0,coin.width,coin.height};
	Vector2 coinCenter = {coin.width / 2, coin.height/2};

	if (gs_player_info(stats)->coins >= 100) {
		DrawTexturePro(coin, coinRectangle, (Rectangle){870, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 740, 10, 40, GRAY);
	}
	else if (gs_player_info(stats)->coins >= 20) {
		DrawTexturePro(coin, coinRectangle, (Rectangle){870, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 800, 10, 40, GRAY);
	}
	else {
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 750, 10, 40, GRAY);
		DrawTexturePro(coin, coinRectangle, (Rectangle){870, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	}

	}


	DrawText("Press [ALT] to go back.", 10, SCREEN_HEIGHT - 40, 20, DARKGREEN);
}

void draw_store_menu(Menu menu, State state, GlobalStats stats) {
	DrawText("ADTChase", 10, 10, 30, DARKGREEN);

	DrawText("Store", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 300, 50, DARKGREEN);
	
	Rectangle coinRectangle = {0,0,coin.width,coin.height};
	Vector2 coinCenter = {coin.width / 2, coin.height/2};

	DrawTexturePro(coin, coinRectangle, (Rectangle){870, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);

	if (gs_player_info(stats)->coins < 10) 
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 820, 10, 40, GRAY);
	else if (gs_player_info(stats)->coins < 100) 
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 800, 10, 40, GRAY);
	else if (gs_player_info(stats)->coins < 1000)
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 770, 10, 40, GRAY);
	else 
		DrawText(TextFormat("%d", gs_player_info(stats)->coins), 750, 10, 40, GRAY);
	
	if (get_page(menu) % 2 == 1 && get_page(menu) < 9) {

		DrawText("Spaceship Upgrades", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 200, 30, BLUE);
		DrawText("> > Gun Upgrades", SCREEN_WIDTH/2 + 180, SCREEN_HEIGHT/2 - 195, 25, GRAY);

		if (gs_store_info(stats)->spaceship_hp < 500) 

			switch (get_page(menu))
			{
			case 1:
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 50, 25, GRAY);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, GRAY);
				break;	
			
			case 3:
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 50, 25, SKYBLUE);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, GRAY);
				break;
			
			case 5:
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 50, 25, GRAY);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, SKYBLUE);
				break;

			default:
				break;
			}

		else
			switch (get_page(menu))
			{
			case 1:
				DrawText("--> ", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 - 50, 25, GRAY);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, GRAY);
				break;	
			
			case 3:
				DrawText("--> ", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 - 50, 25, SKYBLUE);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, GRAY);
				break;
			
			case 5:
				DrawText("--> ", SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT/2 - 50, 25, GRAY);
				DrawText("--> ", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 10, 25, SKYBLUE);
				break;

			default:
				break;
			}


		switch (gs_store_info(stats)->spaceship_hp) {
		case 50:
			DrawText("50HP > > 70HP | ", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("-30 Coins", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP /  %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 230, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 70:
			DrawText("70HP > > 100HP | ", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("-70 Coins", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP /  %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 230, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 100:
			DrawText("100HP > > 160HP | ", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("-140 Coins", SCREEN_WIDTH/2 + 60, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP /  %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 240, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 160:
			DrawText("160HP > > 250HP | ", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("-190 Coins", SCREEN_WIDTH/2 + 70, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP /  %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 240, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 250:
			DrawText("250HP > > 500HP | ", SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("-413 Coins", SCREEN_WIDTH/2 + 70, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP /  %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 255, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 500:
			DrawText("500HP | ", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText("MAX HP", SCREEN_WIDTH/2 + 5, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP / %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 255, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		case 1000:
			DrawText("1000HP ? ", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 25, ORANGE);
			DrawText(" NO WAY", SCREEN_WIDTH/2 + 15, SCREEN_HEIGHT/2 - 50, 25, MAROON);

			DrawText(TextFormat("%dHP / %dHP | REGEN 10 HP | ", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 10, 25, ORANGE);
			DrawText("-15 Coins", SCREEN_WIDTH/2 + 270, SCREEN_HEIGHT/2 - 10, 25, MAROON);
			break;

		default:
			break;
		}

	}

	if (get_page(menu) % 2 == 0 && get_page(menu) < 10) {
		
		DrawText("Gun Upgrades", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 200, 30, BLUE);
		DrawText("Spaceship Upgrades < <", SCREEN_WIDTH/2 - 400, SCREEN_HEIGHT/2 - 195, 25, GRAY);
		DrawText("> > Perks", SCREEN_WIDTH/2 + 130, SCREEN_HEIGHT/2 - 195, 25, GRAY);

		if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->pistol) {

			DrawText("Selected Gun: Pistol", SCREEN_WIDTH/2 - 125, SCREEN_HEIGHT/2 - 140, 25, ORANGE);
			DrawText("Rate of fire: slow | Bullets: 50 | Damage: 15", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 110, 20, ORANGE);
			DrawText(TextFormat("%d / 50 BULLETS | +8 |", gs_guns_info(stats)->pistol->bullets), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 30, 25, LIGHTGRAY);
			DrawText("-10 Coins", SCREEN_WIDTH/2 + 225, SCREEN_HEIGHT/2 + 30, 25, MAROON);

		} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->rifle) {

			DrawText("Selected Gun: Rifle", SCREEN_WIDTH/2 - 125, SCREEN_HEIGHT/2 - 140, 25, ORANGE);
			DrawText("Rate of fire: really high | Bullets: 100 | Damage: 9", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 110, 20, ORANGE);
			DrawText(TextFormat("%d / 100 BULLETS | +30 |", gs_guns_info(stats)->rifle->bullets), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 30, 25, LIGHTGRAY);
			DrawText("-20 Coins", SCREEN_WIDTH/2 + 235, SCREEN_HEIGHT/2 + 30, 25, MAROON);

		} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->sniper) {

			DrawText("Selected Gun: Sniper", SCREEN_WIDTH/2 - 125, SCREEN_HEIGHT/2 - 140, 25, ORANGE);
			DrawText("Rate of fire: really slow | Bullets: 25 | Damage: 50", SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 110, 20, ORANGE);
			DrawText(TextFormat("%d / 25 BULLETS | +8 |", gs_guns_info(stats)->sniper->bullets), SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 30, 25, LIGHTGRAY);
			DrawText("-35 Coins", SCREEN_WIDTH/2 + 225, SCREEN_HEIGHT/2 + 30, 25, MAROON);

		}

		if (!gs_store_info(stats)->rifle) {
			DrawText("Rifle      |", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 50, 25, LIGHTGRAY);
			DrawText("-100 Coins", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, MAROON);
		} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->pistol) {
			DrawText("Pistol    | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 50, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, LIME);
		} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->rifle) {
			DrawText("Rifle      | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 50, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, LIME);
		} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->sniper) {
			DrawText("Sniper | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 50, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 50, 25, LIME);
		}

		if (!gs_store_info(stats)->sniper) {
			DrawText("Sniper |", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, 25, LIGHTGRAY);
			DrawText("-287 Coins", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 10, 25, MAROON);
		} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->pistol) {
			DrawText("Pistol    | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 10, 25, LIME);
		} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->rifle) {
			DrawText("Rifle      | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 10, 25, LIME);
		} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->sniper) {
			DrawText("Sniper | ", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, 25, LIGHTGRAY);
			DrawText("BOUGHT", SCREEN_WIDTH/2 + 50, SCREEN_HEIGHT/2 - 10, 25, LIME);
		}


		switch (get_page(menu)) {
		case 2:
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 50, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 10, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 30, 25, GRAY);
			break;	
		
		case 4:
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 50, 25, SKYBLUE);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 10, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 30, 25, GRAY);
			
			if (!gs_store_info(stats)->rifle) {
				DrawText("    Rate of fire: high\nBullets: 100 | Damage: 9", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->pistol) {
				DrawText("    Rate of fire: slow\nBullets: 50 | Damage: 15", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->rifle) {
				DrawText("    Rate of fire: high\nBullets: 100 | Damage: 9", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot1 == gs_guns_info(stats)->sniper) {
				DrawText("Rate of fire: really slow\nBullets: 30 | Damage: 50", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			}

			break;
		case 6:
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 50, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 10, 25, SKYBLUE);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 30, 25, GRAY);

			if (!gs_store_info(stats)->sniper) {
				DrawText("Rate of fire: really slow\nBullets: 30 | Damage: 50", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->pistol) {
				DrawText("    Rate of fire: slow\nBullets: 50 | Damage: 15", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->rifle) {
				DrawText("    Rate of fire: high\nBullets: 100 | Damage: 9", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			} else if (gs_store_info(stats)->slot2 == gs_guns_info(stats)->sniper) {
				DrawText("Rate of fire: really slow\nBullets: 30 | Damage: 50", SCREEN_WIDTH/2 + 190, SCREEN_HEIGHT/2 - 45, 20, BEIGE);
			}

			break;
		case 8:
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 50, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 10, 25, GRAY);
			DrawText("--> ", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 + 30, 25, SKYBLUE);
			break;

		default:
			break;
		}

	}

	if (get_page(menu) >= 9) {

		DrawText("Perks", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 - 200, 30, BLUE);
		DrawText("Gun Upgrades < <", SCREEN_WIDTH/2 - 280, SCREEN_HEIGHT/2 - 195, 25, GRAY);

		DrawText("Coming Soon!", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - 50, 40, GOLD);


	}

	DrawText("Press [ALT] to go back.", 10, SCREEN_HEIGHT - 40, 20, DARKGREEN);
}


void interface_draw_menu(Menu menu, State state, GlobalStats stats) {

	BeginDrawing();
	ClearBackground(BLACK);

	int current_SCREEN_WIDTH = GetScreenWidth();
	int current_SCREEN_HEIGHT = GetScreenHeight();

	DrawTexturePro(background, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Vector2){0,0}, 0, DARKGRAY);

	switch (active_menu(menu)) {
	case 0:
		draw_main_menu(menu);
		break;
	case 1:
		draw_level_menu(menu, stats);
		break;
	case 2:
		draw_store_menu(menu, state, stats);
		break;
	case 3:
		draw_help_menu(menu);
		break;

	}
	
	EndDrawing();

}


Vector2 boss_cartToRay(BossState state, Vector2 vec) {

	Vector2 rayCenter = {SCREEN_WIDTH/2, SCREEN_HEIGHT/2};
	//Vector2 spaceshipPos = boss_state_info(state)->spaceship->position;
	
	//return (Vector2){rayCenter.x - spaceshipPos.x + vec.x, rayCenter.y - (spaceshipPos.y * (-1)) + (vec.y * (-1))};
	return (Vector2){rayCenter.x + vec.x, rayCenter.y + (vec.y * (-1))};
}	


//Boss State Interface

void interface_draw_boss_frame(BossState state, GlobalStats stats) {

	UpdateMusicStream(background_music);
	BeginDrawing();
	ClearBackground(BLACK);

	int current_SCREEN_WIDTH = GetScreenWidth();
	int current_SCREEN_HEIGHT = GetScreenHeight();

	DrawTexturePro(background, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Rectangle){0,0,current_SCREEN_WIDTH,current_SCREEN_HEIGHT}, (Vector2){0,0}, 0, DARKGRAY);
	
	Rectangle spaceshipRectangle = {0,0,spaceship_img.width,spaceship_img.height};
	Vector2 spaceshipCenter = {spaceship_img.width / 2, spaceship_img.height/2};

	float rotation = atan2(boss_state_info(state)->spaceship->orientation.y, boss_state_info(state)->spaceship->orientation.x *(-1)) * RAD2DEG;

	DrawTexturePro(spaceship_img,spaceshipRectangle,(Rectangle){boss_state_info(state)->spaceship->position.x, boss_state_info(state)->spaceship->position.y, spaceshipRectangle.width,spaceshipRectangle.height},
				   spaceshipCenter,rotation,WHITE);

    float bossRotation = atan2(boss_state_binfo(state)->spaceship->orientation.y, boss_state_binfo(state)->spaceship->orientation.x * (-1)) * RAD2DEG;

    DrawTexturePro(spaceship_img, spaceshipRectangle, (Rectangle){ boss_state_binfo(state)->spaceship->position.x,  boss_state_binfo(state)->spaceship->position.y, spaceshipRectangle.width, spaceshipRectangle.height},
                   spaceshipCenter, bossRotation, RED);

	//Boss Health Bar
	float health_percentage = (float)boss_state_binfo(state)->spaceship->health / 500;
    
	float border = 2; 

	DrawRectangle(220 - border, 20- border, 
                  500 + 2 * border, 30 + 2 * border, RAYWHITE);

    DrawRectangle(220, 20, 500, 30, DARKGRAY);
    
	if (health_percentage > 0.7) 
		DrawRectangle(220, 20, 500 * health_percentage, 30, LIME);
	else if (health_percentage > 0.4) 
		DrawRectangle(220, 20, 500 * health_percentage, 30, ORANGE);
	else
		DrawRectangle(220, 20, 500 * health_percentage, 30, RED);


	Vector2 top_left = {boss_state_info(state)->spaceship->position.x + ((3*SCREEN_WIDTH)*(-1)), boss_state_info(state)->spaceship->position.y + (3*SCREEN_HEIGHT)};
	Vector2 bottom_right = {boss_state_info(state)->spaceship->position.x + (3*SCREEN_WIDTH), boss_state_info(state)->spaceship->position.y + ((3*SCREEN_HEIGHT)*(-1))};

	List objects = boss_state_objects(state, top_left, bottom_right);

	for (ListNode node = list_first(objects); node != LIST_EOF; node = list_next(objects, node)) {
		Object obj = list_node_value(objects, node);

		if (obj->type == ASTEROID) {
			DrawCircleLines(obj->position.x, obj->position.y, obj->size/2, LIGHTGRAY);
		} else if (obj->type == BULLET) {
			if (obj->orientation.x != 20)
				DrawCircle(obj->position.x, obj->position.y, obj->size, LIME);
			else
				DrawCircle(obj->position.x, obj->position.y, obj->size, RED);
		}

	}
	
	Rectangle coinRectangle = {0,0,coin.width,coin.height};
	Vector2 coinCenter = {coin.width / 2, coin.height/2};

	if (gs_player_info(stats)->coins >= 100)
		DrawTexturePro(coin, coinRectangle, (Rectangle){130, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	else if (gs_player_info(stats)->coins >= 20)
		DrawTexturePro(coin, coinRectangle, (Rectangle){80, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);
	else
		DrawTexturePro(coin, coinRectangle, (Rectangle){70, 28, coinRectangle.width,coinRectangle.height}, coinCenter, 0, WHITE);

	DrawText(TextFormat("%d", gs_player_info(stats)->coins), 10, 10, 40, GRAY);
	DrawFPS(SCREEN_WIDTH - 80, 0);

	if (boss_state_info(state)->drawCoinsReward) {
		Vector2 coinPos = boss_cartToRay(state, boss_state_info(state)->coinsPos);
		DrawText(TextFormat("+ %d", boss_state_info(state)->coinsReward), coinPos.x, coinPos.y, 20, GOLD);
	}

	Rectangle heartRectangle = {0,0,heart.width,heart.height};
	Vector2 heartCenter = {heart.width / 2, heart.height/2};	

	DrawTexturePro(heart, heartRectangle, (Rectangle){SCREEN_WIDTH - 20, SCREEN_HEIGHT - 20, heartRectangle.width,heartRectangle.height}, heartCenter, 0, WHITE);

	if (gs_store_info(stats)->spaceship_hp >= 100)
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 138, SCREEN_HEIGHT - 30, 20, GRAY);
	else if (gs_store_info(stats)->spaceship_hp >= 500)
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 155, SCREEN_HEIGHT - 30, 20, GRAY);
	else
		DrawText(TextFormat("%d / %d", gs_player_info(stats)->spaceship_hp, gs_store_info(stats)->spaceship_hp), SCREEN_WIDTH - 120, SCREEN_HEIGHT - 30, 20, GRAY);

	if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->pistol) {
		DrawText(TextFormat("Pistol: %d / 50 BULLETS", gs_guns_info(stats)->pistol->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->rifle) {
		DrawText(TextFormat("Rifle: %d / 100 BULLETS", gs_guns_info(stats)->rifle->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	} else if (gs_guns_info(stats)->selected_gun == gs_guns_info(stats)->sniper) {
		DrawText(TextFormat("Sniper: %d / 25 BULLETS", gs_guns_info(stats)->sniper->bullets), SCREEN_WIDTH - 580, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
	}

	
	if (boss_state_info(state)->paused) {
		PauseMusicStream(background_music);

		if (boss_state_info(state)->win) {
			DrawText(
				"Well, well, well... YOU WON GG!!!",
				GetScreenWidth() / 2 - MeasureText("Well, well, well... YOU WON GG!!!", 20) / 2,
				GetScreenHeight() / 2 - 50, 20, GRAY
			);
		} else if (boss_state_binfo(state)->lost) {
			DrawText(
				"YOU DIED!!!!!!!!!!! PRESS [ALT] TWICE TO EXIT",
				GetScreenWidth() / 2 - MeasureText("YOU LOST!!!!!!!!!!! PRESS [ALT] TWICE TO EXIT", 20) / 2,
				GetScreenHeight() / 2 - 50, 20, GRAY
			);
		} else {
			DrawText(
				"PRESS [P] TO CONTINUE OR [ALT] TWICE TO EXIT\n(EXIT MEANS YOU WILL LOSE ALL PROGRESS)",
				GetScreenWidth() / 2 - MeasureText("PRESS [P] TO CONTINUE OR [ALT] TWICE TO EXIT\n(EXIT MEANS YOU WILL LOSE ALL PROGRESS)", 20) / 2,
				GetScreenHeight() / 2 - 50, 20, GRAY
			);
		}
	} 
	else 
		ResumeMusicStream(background_music);

	EndDrawing();
}