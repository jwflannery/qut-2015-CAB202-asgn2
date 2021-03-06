#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>

#include "lcd.h"
#include "graphics.h"
#include "cpu_speed.h"
#include "sprite.h"

/**
 * Useful defines you can use in your system time calculations
 */
#define FREQUENCY 8000000.0
#define PRESCALER 1024.0
#define FACESIZE 16

static inline void debounce_left(void);
static inline void debounce_right(void);

typedef struct{
	float x;
	float y;
	float dx;
	float dy;
	int first_time;
	int spawned;
	int activated;
}Face;

/**
 * Global variables
 */
int press_count = 0;
int level = 1;
int rand_seed = 0;
int difficulty = 1;
volatile int first_time_ha = 1;
volatile int first_time_an = 1;
volatile int first_time_cr = 1;
volatile int lives = 4;
volatile int score = -2;
volatile uint8_t left_button_down = 0;
volatile uint8_t right_button_down = 0;
int RightPressed = 0;
int RightPressed_Confidence_Level = 0; //Measure button press cofidence
int RightReleased_Confidence_Level = 0;
int LeftPressed = 0;
int LeftPressed_Confidence_Level = 0; //Measure button press cofidence
int LeftReleased_Confidence_Level = 0;

/**
 * Definition of the smiley face sprite and it's x position
 */
#define BYTES_PER_FACE 32
unsigned char bm_player[16] = {
    0b00000111, 0b11100000,
    0b00011000, 0b00011000,
    0b00100000, 0b00000100,
    0b01000000, 0b00000010,
    0b01011000, 0b00011010,
    0b10011000, 0b00011001,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
};
float player_x = 36;
float player_y = 0;

unsigned char bm_happy[BYTES_PER_FACE] = {
    0b00000111, 0b11100000,
    0b00011000, 0b00011000,
    0b00100000, 0b00000100,
    0b01000000, 0b00000010,
    0b01011000, 0b00011010,
    0b10011000, 0b00011001,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b01000111, 0b11100010,
    0b01000000, 0b00000010,
    0b00100000, 0b00000100,
    0b00011000, 0b00011000,
    0b00000111, 0b11100000
};
// float happy_x = -40;
// float happy_y = -40;
// float happy_dx = 0;
// float happy_dy = 0.9;

unsigned char bm_angry[BYTES_PER_FACE] = {
    0b00000111, 0b11100000,
    0b00011000, 0b00011000,
    0b00100000, 0b00000100,
    0b01000000, 0b00000010,
    0b01011000, 0b00011010,
    0b10001100, 0b00110001,
    0b10000011, 0b11000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b01000111, 0b11100010,
    0b01000000, 0b00000010,
    0b00100000, 0b00000100,
    0b00011000, 0b00011000,
    0b00000111, 0b11100000
};
// float angry_x = -40;
// float angry_y = -40;
// float angry_dx = 0;
// float angry_dy = 0.9;

unsigned char bm_crazy[BYTES_PER_FACE] = {
    0b00000111, 0b11100000,
    0b00011000, 0b00011000,
    0b00100110, 0b01100100,
    0b01000000, 0b00000010,
    0b01011000, 0b00011010,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b01000111, 0b11100010,
    0b01000000, 0b00000010,
    0b00100000, 0b00000100,
    0b00011000, 0b00011000,
    0b00000111, 0b11100000
};
// float crazy_x = -40;
// float crazy_y = -40;
// float crazy_dx = 0;
// float crazy_dy = 0.9;

//volatile float dx = 0; 

/**
 * Function implementations
 */
void init_hardware(void);
void set_happy_position(void);
void set_angry_position(void);
void set_crazy_position(void);
void set_position(Face * face1, Face * face2, Face * face3);
int check_collision(Face * face1, Face * face2);
int check_happy_collision(void);
int check_angry_collision(void);
int check_crazy_collision(void);
int right_button_pressed(void);
int left_button_pressed(void);
void init_faces(Face * face);

void init_faces(Face * face){
	face->x = 0;
	face->y = 0;
	face->dx = 1;
	face->dy = 1;
	face->first_time = 1;
	face->spawned = 1;
	face->activated = 0;
}

/**
 * Function implementations
 */
void init_hardware(void) {
    // Initialising the LCD screen
    LCDInitialise(LCD_DEFAULT_CONTRAST);

    // Initalising the buttons as inputs
    DDRF &= ~((1 << PF5) | (1 << PF6));

    // Initialising the LEDs as outputs
    DDRB |= ((1 << PB2) | (1 << PB3));
	
	// //Setup for Pin Interrupts
	// DDRB &= ~(1 << PB7);
	// PORTF = 0x00;
	
	// PCMSK0 |= (1<<PCINT0);
	// PCMSK0 |= (1<<PCINT1);
	
	// //Enable Pin Interrupts
	// PCICR |= (1<<PCIE0);

    // Setup all necessary timers, and interrupts
    // TODO
	
	//Set Global Interrupts
	sei();
}
void set_position(Face * face1, Face * face2, Face * face3){
	int i = 0;
	// if (first_time == 1 && level != 3){
		// happy_y = 10;
	//}
	face1->spawned = 1;	
	face1->activated = 1;
	if (level == 3){
		 do{
			 face1->y = rand() % 31;
		 }while(face1->y < 11);
		//face1->dx = face1->dx * ((rand() % 2 * 2) - 1);
		//face1->dy = face1->dy * ((rand() % 2 * 2) - 1);
	 }
	face1->x = ((rand()) % 68);
	while((((face1->x > face2->x - (FACESIZE+5)) && (face1->x < face2->x + FACESIZE+5)) && (face1->y > face2->y - (FACESIZE+5)) && (face1->y < face2->y + FACESIZE+5) 
		|| ((face1->x > face3->x - (FACESIZE+5)) && (face1->x < face3->x + FACESIZE+5)) && (face1->y > face3->y - (FACESIZE+5)) && (face1->y < face3->y + FACESIZE+5)
		|| ((face1->x > player_x - (FACESIZE+5)) && (face1->x < player_x + FACESIZE+5)) && (face1->y > player_y - (FACESIZE+5)) && (face1->y < player_y + FACESIZE+5)
		|| face1->x < 0 || face1->x > 84 || face1->y < 0 || face1->y > 48)  && i < 10)
	{
		face1->x = (rand() % 68);
		do{
			face1->y = rand() % 31;
		}while(face1->y < 11);
		i++;
		if (i >= 10 && !face1->first_time){
			face1->x = -50;
			face1->y = -50;
			face1->spawned = 0;
			face1->activated = 0;
			break;
		}
	}	
	//score += 2;

	face1->first_time = 0;

}

int check_collision(Face * face1, Face * face2){
	if ((face1->x > player_x - FACESIZE) && (face1->x < player_x + 8) && (face1->y > player_y - FACESIZE) && (face1->y < player_y - 8)){
		return 1;
	}
	if (level == 3){
		if (face1->x + FACESIZE > 84){
			face1->dx = -face1->dx;
			face1->x--;
		}
		if ( face1->x < 0){
			face1->dx = -face1->dx;
			face1->x++;
		}
		if (face1->y + FACESIZE > 48){
			face1->dy = -face1->dy;
			face1->y--;
		}
		 if (face1->y < 12){
			 face1->dy = -face1->dy;
			 face1->y++;
		 }
		
		if ((face1->x > face2->x - FACESIZE) && (face1->x < face2->x + FACESIZE) &&(face1->y > face2->y - FACESIZE) && (face1->y < face2->y + FACESIZE) ){
			if (abs(face2->x - face1->x) > abs(face2->y - face1->y)){
				if (face1->x > face2->x){
					//face1->x++;
				}else{
					//face1->x--;
				}
				face1->dx = -face1->dx;
				face2->dx = -face2->dx;
				return 0;
			}
			else{
				if (face1->y > face2->y){
					//face1->y++;
				}else{
					//face1->y--;
				}
				face1->dy = -face1->dy;
				face2->dy = -face2->dy;
				return 0;
			}
			return 2;
		}		
	}	
	return 0;
}

int checkButtonRight(){   
    if(bit_is_clear(PINF, 5)){
		RightPressed_Confidence_Level++;
		if (RightPressed_Confidence_Level > 500){
			if (RightPressed == 0){
				RightPressed = 1;
				return 1;
			}
		}
	}
	else{
		RightReleased_Confidence_Level++;
		RightPressed = 0;
	}
	return 0;	
}

int main() {
    set_clock_speed(CPU_8MHz);
	int state = 1;
	int triggered = 0;
    // Create and initialise the player sprite
    Sprite player;
    init_sprite(&player, 42, 40, 8, 8, bm_player);
	player_x = 42;
	player_y = 40;
	
	//Setup Happy sprite
	Sprite happysprite;
	init_sprite(&happysprite, 20, 0, FACESIZE, FACESIZE, bm_happy);
		
	//Setup angry sprite
	Sprite angrysprite;
	init_sprite(&angrysprite, 40, 0, FACESIZE, FACESIZE, bm_angry);

	
	//Setup crazy sprite
	Sprite crazysprite;
	init_sprite(&crazysprite, 60, 0, FACESIZE, FACESIZE, bm_crazy);
	
	Face happy;
	Face angry;
	Face crazy;
	
	init_faces(&happy);
	init_faces(&angry);
	init_faces(&crazy);
	
    // Setup the hardware
    init_hardware();

	//Game Loop
	while(1){
		int i = 0;
		//Run the level select loop
		while (state == 0){
			clear_screen();
			draw_string(0,10, "James Flannery");
			draw_string(0, 30, "n8326631");
			show_screen();
			
			i++;
			if (i == 100){
				state = 1;
			}
		}
		while (state == 1){
			char buff[80];
			clear_screen();
			sprintf(buff, "Level: %u", level);
			draw_string(0, 0, buff);
			show_screen();
			if (PINF >> PF5 & 1){
				first_time_ha = 1;
				first_time_an = 1;
				first_time_cr = 1;
				srand(rand_seed);
				state = level + 1;
				//Set initial random face positions
				set_position(&happy, &angry, &crazy);
				set_position(&angry, &happy, &crazy);
				set_position(&crazy, &angry, &happy);
			}
			
			if(PINF >> PF6 & 1 && triggered == 0){
				triggered = 1;
				level++;
				if (level > 3){
					level = 1;
				}
			}
			if (!(PINF >> PF6 & 1)){
				triggered = 0;
			}
			
			rand_seed++;

		}
		
		// Run the game loop
		while (state == 2) {
			// debounce_left();
			// debounce_right();
			clear_screen();
			
			// Draw Status Board
			char buff[80];
			//buff = level;
			clear_screen();
			draw_string(0, 0, "Level");
			draw_string(0, 0, "Level: ");
			sprintf(buff, "L: %u", lives);
			draw_string(0, 0, buff);
			
			sprintf(buff, "S: %u", score);
			draw_string(30, 0, buff);
			
			draw_line(0, 9, 84, 9);
			
			
			// Update the player position
			//smiley_x += dx;
			player.x = player_x;
			player.y = player_y;
			
			//Update the face positions
			// happy_x += happy_dx;
			// happy_y += happy_dy;
			// happysprite.x = happy.x;
			// happysprite.y = happy.y;
			
			// angry_x += angry_dx;
			// angry_y += angry_dy;
			// angry.x = angry_x;
			// angry.y = angry_y;
			
			// crazy_x += crazy_dx;
			// crazy_y += crazy_dy;
			// crazy.x = crazy_x;
			// crazy.y = crazy_y;
			
			//Update dy for difficulty
			// happy_dy = 0.3 * difficulty;
			// angry_dy = 0.3 * difficulty;
			// crazy_dy = 0.3 * difficulty;
			
			//Check for collisions with player
			if (check_collision(&happy, &angry) == 1){
				set_position(&happy, &angry, &crazy);
				score += 2;
			}
			if (check_collision(&angry, &happy) == 1){
				set_position(&angry, &happy, &crazy);
				lives--;
			}
			if(check_collision(&crazy, &angry) == 1){
				set_position(&crazy, &angry, &happy);

				if(difficulty < 3){
					difficulty++;
				}
			}
			
			// //Reset faces when they hit the bottom of the screen
			// if (happy_y + 16 > 48){
				// // happy_x = rand() % 84;
				// set_position(&happy, &angry, &crazy);
			// }
			// if (angry_y + 16 > 48){
				// // angry_x = rand() % 84;
				// set_position(&angry, &happy, &crazy);
			// }
			// if (crazy_y + 16 > 48){
				// // crazy_x = rand() % 84;
				// set_position(&crazy, &angry, &happy);
			// }
			
			//Draw sprites
			draw_sprite(&player);
			draw_sprite(&happysprite);
			draw_sprite(&angrysprite);
			draw_sprite(&crazysprite);
			
			//Move on button press
			if ((PINF >> PF5) & 1){
				_delay_ms(4);
					if ((PINF >> PF5) & 1){
						player_x +=2;
					}
			}
			
			 if (PINF >>PF6 & 1){
				 player_x += -2;
			 }
			
			// if (PINF>>PF6 & 1){
				
				// set_happy_position();
				// set_angry_position();
				// set_crazy_position();

			// }
			//if 

			// Draw and rest between steps
			show_screen();
			_delay_ms(10);
			
			if (score == 20 || lives == 0){
				state=1;
				press_count = 0;
				level = 1;
				lives = 3;
				score = 0;
				// happy_dy = 0.3;
				// angry_dy = 0.3;
				// crazy_dy = 0.3;
			}
			
		}
		while (state == 4) {
			
			if (!happy.spawned)
				set_position(&happy, &angry, &crazy);
			if (!angry.spawned)
				set_position(&angry, &happy, &crazy);
			if (!crazy.spawned)
				set_position(&crazy, &angry, &happy);
			
			clear_screen();
			
			//Check for collisions
			if (check_collision(&happy, &angry) == 1){
				set_position(&happy, &angry, &crazy);
			}
			happy.x += happy.dx;
			happy.y += happy.dy;
			happysprite.x = happy.x;
			happysprite.y = happy.y;
			
			if (check_collision(&angry, &crazy) == 1){
				set_position(&angry, &happy, &crazy);
			}
			angry.x += angry.dx;
			angry.y += angry.dy;
			angrysprite.x = angry.x;
			angrysprite.y = angry.y;
			
			if(check_collision(&crazy, &happy) == 1){
				set_position(&crazy, &angry, &happy);
			}
			crazy.x += crazy.dx;
			crazy.y += crazy.dy;
			crazysprite.x = crazy.x;
			crazysprite.y = crazy.y;

				// if(difficulty < 3){
					// difficulty++;
				// }
				// //Update dy for difficulty
				// if (abs(happy.dy)<0.9)
					// happy.dy = happy.dy * difficulty;
				// if (abs(angry.dy)<0.9)
					// angry.dy = angry.dy * difficulty;
				// if (abs(crazy.dy)<0.9)
					// crazy.dy = crazy.dy * difficulty;
				// if (abs(happy.dx)<0.9)
					// happy.dx = happy.dx * difficulty;
				// if (abs(angry.dx)<0.9)
					// angry.dx = angry.dx * difficulty;
				// if(abs(crazy.dx)<0.9)
					// crazy.dx = crazy.dx * difficulty;
			// }
			
			// Draw Status Board
			char buff[80];
			//buff = level;
			clear_screen();
			//draw_string(0, 0, "Level");
			//draw_string(0, 0, "Level: ");
			sprintf(buff, "L: %u", lives);
			draw_string(0, 0, buff);
			
			sprintf(buff, "S: %u", score);
			draw_string(30, 0, buff);
			
			draw_line(0, 9, 84, 9);
			
			
			// Update the player position
			//smiley_x += dx;
			player.x = player_x;
			player.y = player_y;
			
			//Update the face positions
			// happy_x += happy_dx;
			// happy_y += happy_dy;
			// happy.x = happy_x;
			// happy.y = happy_y;

			
			if (happy.activated){
				score+=2;
				happy.activated = 0;
			}
			if(angry.activated){
				lives--;
				angry.activated = 0;
			}
			if(crazy.spawned){
				crazy.activated = 0;
			}
			

							
			//Reset faces when they hit the bottom of the screen
			// if (happy_y + 16 > 48){
				// // happy_x = rand() % 84;
				// set_happy_position();
			// }
			// if (angry_y + 16 > 48){
				// // angry_x = rand() % 84;
				// set_angry_position();
			// }
			// if (crazy_y + 16 > 48){
				// // crazy_x = rand() % 84;
				// set_crazy_position();
			//}
			
			//Draw sprites
			draw_sprite(&player);
			draw_sprite(&happysprite);
			draw_sprite(&angrysprite);
			draw_sprite(&crazysprite);
			
			//Move on button press
			if ((PINF >> PF5) & 1){
				_delay_ms(4);
					if ((PINF >> PF5) & 1){
						player_x +=2;
					}
			}
			
			 if (PINF >>PF6 & 1){
				 player_x += -2;
			 }
			
			// if (PINF>>PF6 & 1){
				
				// set_happy_position();
				// set_angry_position();
				// set_crazy_position();

			// }
			//if 

			// Draw and rest between steps
			show_screen();
			_delay_ms(10);
			
			if (score == 200 || lives == -50){
				state=1;
				press_count = 0;
				level = 1;
				lives = 4;
				score = -2;
				happy.dy = 0.3;
				angry.dy = 0.3;
				crazy.dy = 0.3;
			}
			//_delay_ms(250);
		}

	}

    // We'll never get here...
    return 0;
}

