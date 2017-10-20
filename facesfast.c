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
	
	
    // Setup the hardware
    init_hardware();

	//Game Loop
	int i = 0;
	while(1){
		clear_screen();

			if(first_loop && USE_SERIAL){
			// Wait until the 'debugger' is attached...
				draw_string(17, 24, "Waiting for");
				draw_string(24, 24, "debugger...");
				show_screen();
				while(!usb_configured() || !usb_serial_get_control());
				usb_serial_flush_input();
			}
			clear_screen();
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

			draw_sprite(&player);
			// Draw and rest between steps
			show_screen();
			curr_char = 'l';
			first_loop = 0;
			_delay_ms(25);

		}

    // We'll never get here...
    return 0;
}

