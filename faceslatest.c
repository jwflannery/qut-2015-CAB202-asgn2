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
#include "usb_serial.h"


/**
 * Useful defines you can use in your system time calculations
 */
#define FREQUENCY 8000000.0
#define PRESCALER 1024.0
#define FACESIZE 16

#define SLOW 0.25//0.5//0.25
#define MEDIUM 0.5
#define FAST 1

#define LTHRES 500
#define RTHRES 500

#define USE_SERIAL 1
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
volatile int press_count = 0;
volatile int level = 1;
volatile int rand_seed = 0;
volatile int difficulty = 1;
volatile int lives = 3;
volatile int score = 0;
int first_loop = 1;
int test = 0;
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
    0b11111111, 0b11111111,
    0b10011000, 0b00011001,
    0b10100000, 0b00000101,
    0b11000000, 0b00000011,
    0b11011000, 0b00011011,
    0b10011000, 0b00011001,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b11000111, 0b11100011,
    0b11000000, 0b00000011,
    0b10100000, 0b00000101,
    0b10011000, 0b00011001,
    0b11111111, 0b11111111
};
// float happy_x = -40;
// float happy_y = -40;
// float happy_dx = 0;
// float happy_dy = 0.9;

unsigned char bm_angry[BYTES_PER_FACE] = {
    0b11111111, 0b11111111,
    0b10011000, 0b00011001,
    0b10100000, 0b00000101,
    0b11000000, 0b00000011,
    0b11011000, 0b00011011,
    0b10001100, 0b00110001,
    0b10000011, 0b11000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b11000111, 0b11100011,
    0b11000000, 0b00000011,
    0b10100000, 0b00000101,
    0b10011000, 0b00011001,
    0b11111111, 0b11111111
};
// float angry_x = -40;
// float angry_y = -40;
// float angry_dx = 0;
// float angry_dy = 0.9;

unsigned char bm_crazy[BYTES_PER_FACE] = {
    0b11111111, 0b11111111,
    0b10011000, 0b00011001,
    0b10100110, 0b01100101,
    0b11000000, 0b00000011,
    0b11011000, 0b00011011,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10000000, 0b00000001,
    0b10010000, 0b00001001,
    0b10010000, 0b00001001,
    0b10001000, 0b00010001,
    0b11000111, 0b11100011,
    0b11000000, 0b00000011,
    0b10100000, 0b00000101,
    0b10011000, 0b00011001,
    0b11111111, 0b11111111
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
int checksign(float num);
int check_happy_collision(void);
int check_angry_collision(void);
int check_crazy_collision(void);
int rightPressed(void);
int leftPressed(void);
void init_faces(Face * face);
void init_vars(void);
uint16_t adc_read(uint8_t ch);

uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with '7' will always keep the value
    // of 'ch' between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing
 
    // start single conversion
    // write '1' to ADSC
    ADCSRA |= (1<<ADSC);
 
    // wait for conversion to complete
    // ADSC becomes '0' again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));
 
    return (ADC);
}

void init_faces(Face * face){
	face->x = 0;
	face->y = 0;
	face->dx = SLOW;
	face->dy = SLOW;
	face->first_time = 1;
	face->spawned = 1;
	face->activated = 0;
}

void init_vars(void){
	press_count = 0;
	difficulty = 1;
	lives = 3;
	score = 0;
	
	if (level == 3){
		difficulty = 0;
		lives = 4;
		score = -2;
	}
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
	
	   // AREF = AVcc
    ADMUX = (1<<REFS0);
 
    // ADC Enable and pre-scaler of 128
    // 8000000/128 = 62500
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	
	
	// //Setup for Pin Interrupts
	// DDRB &= ~(1 << PB7);
	// PORTF = 0x00;
	
	// PCMSK0 |= (1<<PCINT0);
	// PCMSK0 |= (1<<PCINT1);
	
	// //Enable Pin Interrupts
	// PCICR |= (1<<PCIE0);

    // Setup all necessary timers, and interrupts
    // TODO
	usb_init();
	
	//Set Global Interrupts
	sei();
}
void set_position(Face * face1, Face * face2, Face * face3){
	int i = 0;
	face1->spawned = 1;	
	face1->activated = 1;
	if (level != 3){	
		if (level != 3){
			face1->y = 10;
			face1->dx = 0;
			face1->x = ((rand()) % 68);
		}
		
	}
	if (level == 3){
		do{
			 face1->y = rand() % 31;
		}while(face1->y < 11);
			face1->dx = face1->dx * ((rand() % 2 * 2) - 1);
			face1->dy = face1->dy * ((rand() % 2 * 2) - 1);
			face1->x = ((rand()) % 68);
	}
	
	
		while((((face1->x > face2->x - (FACESIZE+5)) && (face1->x < face2->x + FACESIZE+5)) && (face1->y > face2->y - (FACESIZE+5)) && (face1->y < face2->y + FACESIZE+5) 
			|| ((face1->x > face3->x - (FACESIZE+5)) && (face1->x < face3->x + FACESIZE+5)) && (face1->y > face3->y - (FACESIZE+5)) && (face1->y < face3->y + FACESIZE+5)
			|| ((face1->x > player_x - (FACESIZE+5)) && (face1->x < player_x + FACESIZE+5)) && (face1->y > player_y - (FACESIZE+5)) && (face1->y < player_y + FACESIZE+5)
			|| face1->x < 0 || face1->x > 84 || face1->y < 0 || face1->y > 48)  && i < 10)
		{
		if (level == 3){
				face1->x = (rand() % 68);
				do{
					face1->y = rand() % 31;
				}while(face1->y < 11);
				i++;
				//if (i >= 10 && !face1->first_time){
					if (i >= 10){
					face1->x = -50;
					face1->y = -50;
					face1->spawned = 0;
					face1->activated = 0;
					break;
				}
			}else{
				face1->x = (rand() % 68);
				i++;
				//if (i >= 10 && !face1->first_time){
					if(i >= 10){
					face1->x = -50;
					face1->y = -50;
					face1->spawned = 0;
					face1->activated = 0;
				break;
				}
		}
	}		
		//score += 2;
	face1->first_time = 0;

}

int check_collision(Face * face1, Face * face2){
	if ((face1->x > player_x - FACESIZE) && (face1->x < player_x + 8) && (face1->y > player_y - FACESIZE) && (face1->y < player_y + 8)){
		return 1;
	}
	if (level == 3){
		if (face1->x + FACESIZE > 83){
			face1->dx = -fabsf(face1->dx);
		}
		if ( face1->x < 1){
			face1->dx = fabsf(face1->dx);
		}
		if (face1->y + FACESIZE > 47){
			face1->dy = -fabsf(face1->dy);
		}
		 if (face1->y < 11){
			 face1->dy = fabsf(face1->dy);
		}
		
		if ((face1->x > face2->x - (FACESIZE + 0)) && (face1->x < face2->x + (FACESIZE + 0)) &&(face1->y > face2->y - (FACESIZE + 0)) && (face1->y < face2->y + (FACESIZE + 0)) ){
			if (fabsf(face2->x - face1->x) > fabsf(face2->y - face1->y)){
				if (face1->x > face2->x){
					//face1->x++;
					face1->dx = fabsf(face1->dx);
					face2->dx = -fabsf(face2->dx);
				}else{
					//face1->x--;
					face1->dx = -fabsf(face1->dx);
					face2->dx = fabsf(face2->dx);
				}


				// face1->x += face1->dx;
				// face1->y += face1->dy;
				// face2->x += face2->dx;
				// face2->y += face2->dy;
				return 2;
			}
			else{
				if (face1->y > face2->y){
					face1->dy = fabsf(face1->dy);
					face2->dy = -fabsf(face2->dy);					
					//face1->y++;
				}else{
					//face1->y--;
					face1->dy = -fabsf(face1->dy);
					face2->dy = fabsf(face2->dy);
				}
				// face1->dy = -face1->dy;
				// face2->dy = -face2->dy;
				// face1->x += face1->dx;
				// face1->y += face1->dy;
				// face2->x += face2->dx;
				// face2->y += face2->dy;
				return 2;
			}
			return 2;
		}		
	}	
	return 0;
}

int rightPressed(){   
	if ((PINF >> PF5 & 1)){
		_delay_ms(2);
		if ((PINF >> PF5) & 1){
			return 1;
		}
	}
	return 0;
}

int leftPressed (){
	if ((PINF >> PF6 & 1)){
		_delay_ms(5);
		if ((PINF >> PF6) & 1){
			return 1;
		}
	}
	return 0;
}

int checksign(float num){
	if (num > 0)
		return 1;
	if (num == 0)
		return 0;
	if (num < 0)
		return -1;
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
	
	uint16_t adc_result0, adc_result1;
	
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
			if (PINF >> PF6 & 1){
				srand(rand_seed);
				state = level + 1;
				init_vars();
				//Set initial random face positions
				set_position(&happy, &angry, &crazy);
				set_position(&angry, &happy, &crazy);
				set_position(&crazy, &angry, &happy);
			}
						
			if ((PINF >> PF5 & 1) && !triggered){
				_delay_ms(5);
				if ((PINF >> PF5) & 1){
					level++;
					if (level > 3){
						level = 1;
					}
					triggered = 1;
				}
			}
			if (!(PINF >> PF5 & 1)){
				triggered = 0;
			}
						
			rand_seed++;

		}
		
		// Run the game loop
		while (state == 2) {
			if (!happy.spawned)
				set_position(&happy, &angry, &crazy);
			if (!angry.spawned)
				set_position(&angry, &happy, &crazy);
			if (!crazy.spawned)
				set_position(&crazy, &angry, &happy);
			
			// debounce_left();
			// debounce_right();
			clear_screen();
			
			// Draw Status Board
			// Draw Status Board
			char buff[80];
			//buff = level;
			clear_screen();
			//draw_string(0, 0, "Level");
			//draw_string(0, 0, "Level: ");
			sprintf(buff, "X: %u", level);
			draw_string(0, 0, buff);
			
			sprintf(buff, "S: %u", score);
			draw_string(30, 0, buff);
			
			draw_line(0, 9, 84, 9);
			
			
			// Update the player position
			//smiley_x += dx;
			player.x = player_x;
			player.y = player_y;
			
			//Update the face positions
			happy.x += happy.dx;
			happy.y += happy.dy;
			happysprite.x = happy.x;
			happysprite.y = happy.y;
			
			angry.x += angry.dx;
			angry.y += angry.dy;
			angrysprite.x = angry.x;
			angrysprite.y = angry.y;
			
			crazy.x += crazy.dx;
			crazy.y += crazy.dy;
			crazysprite.x = crazy.x;
			crazysprite.y = crazy.y;
			
			//Update dy for difficulty
				if (difficulty == 1){
					happy.dy = SLOW;
					angry.dy = SLOW;
					crazy.dy = SLOW;
				}
				if (difficulty == 2){
					happy.dy = MEDIUM;
					angry.dy = MEDIUM;
					crazy.dy = MEDIUM;
				}
				if (difficulty == 3){
					happy.dy = FAST;
					angry.dy = FAST;
					crazy.dy = FAST;
				}
						
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
			
			//Reset faces when they hit the bottom of the screen
			if (happy.y > 48){
				// happy_x = rand() % 84;
				set_position(&happy, &angry, &crazy);
			}
			if (angry.y > 48){
				// angry_x = rand() % 84;
				set_position(&angry, &happy, &crazy);
			}
			if (crazy.y > 48){
				// crazy_x = rand() % 84;
				set_position(&crazy, &angry, &happy);
			}
						
			//Move on button press
			// if ((PINF >> PF5) & 1){
				// _delay_ms(4);
					// if ((PINF >> PF5) & 1){
						// if (player_x + 8 < 84){
							// player_x +=2;
						// }
					// }
			// }
				if ((PINF >> PF5) & 1){
					if (player_x + 8 < 84){
						player_x +=2;
					}
				}
			// return 0;
			// if (rightPressed()){

			// }
			
			 if (leftPressed()){
				 if (player_x > 0){
				 player_x += -2;
				 }				 
			 }
			
			// if (PINF>>PF6 & 1){
				
				// set_happy_position();
				// set_angry_position();
				// set_crazy_position();

			// }
			//if 

			//Draw sprites
			draw_sprite(&player);
			draw_sprite(&happysprite);
			draw_sprite(&angrysprite);
			draw_sprite(&crazysprite);
			
			// Draw and rest between steps
			show_screen();
			_delay_ms(10);
			
			if (score == 20 || lives == 0){
				state=1;
				press_count = 0;
				level = 1;
				lives = 3;
				score = 0;
			}
		}
		while (state == 3) {
			
			if (!happy.spawned)
				set_position(&happy, &angry, &crazy);
			if (!angry.spawned)
				set_position(&angry, &happy, &crazy);
			if (!crazy.spawned)
				set_position(&crazy, &angry, &happy);
			
			adc_result0 = adc_read(0);      // read adc value at PA0
			adc_result1 = adc_read(1);      // read adc value at PA1
			
			float max_adc = 1023.0;
			long max_lcd_adc = (adc_result0*(long)(LCD_X-5)) / max_adc;
			
			
			// debounce_left();
			// debounce_right();
			clear_screen();
			
			// Draw Status Board
			// Draw Status Board
			char buff[80];
			//buff = level;
			clear_screen();
			//draw_string(0, 0, "Level");
			//draw_string(0, 0, "Level: ");
			sprintf(buff, "X: %u", level);
			draw_string(0, 0, buff);
			
			sprintf(buff, "S: %u", score);
			draw_string(30, 0, buff);
			
			draw_line(0, 9, 84, 9);
			
			
			// Update the player position
			//smiley_x += dx;
			player.x = player_x;
			player.y = player_y;
			
			//Update the face positions
			happy.x += happy.dx;
			happy.y += happy.dy;
			happysprite.x = happy.x;
			happysprite.y = happy.y;
			
			angry.x += angry.dx;
			angry.y += angry.dy;
			angrysprite.x = angry.x;
			angrysprite.y = angry.y;
			
			crazy.x += crazy.dx;
			crazy.y += crazy.dy;
			crazysprite.x = crazy.x;
			crazysprite.y = crazy.y;
			
			//Update dy for difficulty
				if (difficulty == 1){
					happy.dy = SLOW;
					angry.dy = SLOW;
					crazy.dy = SLOW;
				}
				if (difficulty == 2){
					happy.dy = MEDIUM;
					angry.dy = MEDIUM;
					crazy.dy = MEDIUM;
				}
				if (difficulty == 3){
					happy.dy = FAST;
					angry.dy = FAST;
					crazy.dy = FAST;
				}
			
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
			
			//Reset faces when they hit the bottom of the screen
			if (happy.y > 48){
				// happy_x = rand() % 84;
				set_position(&happy, &angry, &crazy);
			}
			if (angry.y > 48){
				// angry_x = rand() % 84;
				set_position(&angry, &happy, &crazy);
			}
			if (crazy.y > 48){
				// crazy_x = rand() % 84;
				set_position(&crazy, &angry, &happy);
			}
			
			//Draw sprites
			draw_sprite(&player);
			draw_sprite(&happysprite);
			draw_sprite(&angrysprite);
			draw_sprite(&crazysprite);
			
			//Move on ADC
			player_x = max_lcd_adc;

			// Draw and rest between steps
			show_screen();
			_delay_ms(10);
			
			if (score == 20 || lives == 0){
				state=1;
				press_count = 0;
				level = 1;
				lives = 3;
				score = 0;
			}
			
		}	
		while (state == 4) {
			if(first_loop && USE_SERIAL){
			// Wait until the 'debugger' is attached...
				draw_string(17, 24, "Waiting for");
				draw_string(24, 24, "debugger...");
				show_screen();
				while(!usb_configured() || !usb_serial_get_control());
				usb_serial_flush_input();
			}
			int16_t curr_char;
			while(usb_serial_available()){
				curr_char = usb_serial_getchar();
				test++;
				//usb_serial_flush_input();
			}
			if (USE_SERIAL){
				if (curr_char == 'w'){
					player_y -= 2;
				}
				if (curr_char == 's'){
					player_y += 2;
				}
				if (curr_char == 'a'){
					player_x -= 2;
				}
				if (curr_char == 'd'){
					player_x += 2;
				}
			}
			
			
			clear_screen();
			
			if (!happy.spawned)
				set_position(&happy, &angry, &crazy);
			if (!angry.spawned)
				set_position(&angry, &happy, &crazy);
			if (!crazy.spawned)
				set_position(&crazy, &angry, &happy);
							
			//Check for collisions
			if (check_collision(&happy, &angry) == 1){
				set_position(&happy, &angry, &crazy);
			}
			//if (check_collision(&happy, &angry) == 2){
				 happy.x += happy.dx;
				 happy.y += happy.dy;
			//}
			
			if (check_collision(&angry, &crazy) == 1){
				set_position(&angry, &happy, &crazy);
			}
			//if (check_collision(&angry, &crazy) == 1){
				 angry.x += angry.dx;
				 angry.y += angry.dy;
			//}
			
			if(check_collision(&crazy, &happy) == 1){
				set_position(&crazy, &angry, &happy);
				
			}

			//if(check_collision(&crazy, &happy) == 1){
				 crazy.x += crazy.dx;
				 crazy.y += crazy.dy;

			//}
				happysprite.x = happy.x;
				happysprite.y = happy.y;
				angrysprite.x = angry.x;
				angrysprite.y = angry.y;
				crazysprite.x = crazy.x;
				crazysprite.y = crazy.y;

			
			// Draw Status Board
			char buff[80];
			//buff = level;
			clear_screen();
			//draw_string(0, 0, "Level");
			//draw_string(0, 0, "Level: ");
			sprintf(buff, "X: %u", test);
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
			if(crazy.activated){
				if(difficulty < 3){
					 difficulty++;
				}
				// //Update dy for difficulty
				if (difficulty == 1){
					 happy.dy = SLOW * (checksign(happy.dy));
					 angry.dy = SLOW * (checksign(angry.dy));
					 crazy.dy = SLOW * (checksign(crazy.dy));
					 
					 happy.dx = SLOW * (checksign(happy.dx));
					 angry.dx = SLOW * (checksign(angry.dx));
					 crazy.dx = SLOW * (checksign(crazy.dx));
				}
			if (difficulty == 2){
					 happy.dy = MEDIUM * (checksign(happy.dy));
					 angry.dy = MEDIUM * (checksign(angry.dy));
					 crazy.dy = MEDIUM * (checksign(crazy.dy));
					 
					 happy.dx = MEDIUM * (checksign(happy.dx));
					 angry.dx = MEDIUM * (checksign(angry.dx));
					 crazy.dx = MEDIUM * (checksign(crazy.dx));
			 }
			 if (difficulty == 3){
					 happy.dy = FAST * (checksign(happy.dy));
					 angry.dy = FAST * (checksign(angry.dy));
					 crazy.dy = FAST * (checksign(crazy.dy));
					 
					 happy.dx = FAST * (checksign(happy.dx));
					 angry.dx = FAST * (checksign(angry.dx));
					 crazy.dx = FAST * (checksign(crazy.dx));
			 }	
				crazy.activated = 0;
			}
			
			if (!USE_SERIAL){
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
			}
			// if (USE_SERIAL){
				// if (curr_char == 'w'){
					// player_y -= 2;
				// }
				// if (curr_char == 's'){
					// player_y += 2;
				// }
				// if (curr_char == 'a'){
					// player_x -= 2;
				// }
				// if (curr_char == 'd'){
					// player_x += 2;
				// }
			// }
			

			
			// if (PINF>>PF6 & 1){
				
				// set_happy_position();
				// set_angry_position();
				// set_crazy_position();

			// }
			//if 
			//Draw sprites
			draw_sprite(&player);
			draw_sprite(&happysprite);
			draw_sprite(&angrysprite);
			draw_sprite(&crazysprite);

			// Draw and rest between steps
			show_screen();
			_delay_ms(5);//25
			

			
			// if (score == 200 || lives == -50){
				// state=1;
				// press_count = 0;
				// level = 1;
				// lives = 4;
				// score = -2;
				// happy.dy = 0.3;
				// angry.dy = 0.3;
				// crazy.dy = 0.3;
			// }
			//_delay_ms(250);
			
			first_loop = 0;
			curr_char = 'l';
		}

	}

    // We'll never get here...
    return 0;
}

