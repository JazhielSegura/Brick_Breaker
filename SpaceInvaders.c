// SpaceInvaders.c (Brick Breaker Driver)
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the ECE319K Lab 10

// Last Modified: 1/2/2023 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php

// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// buttons connected to PE0-PE3
// 32*R resistor DAC bit 0 on PB0 (least significant bit)
// 16*R resistor DAC bit 1 on PB1
// 8*R resistor DAC bit 2 on PB2 
// 4*R resistor DAC bit 3 on PB3
// 2*R resistor DAC bit 4 on PB4
// 1*R resistor DAC bit 5 on PB5 (most significant bit)
// LED on PD1
// LED on PD0




#define C    2389   // 1046.5 Hz
#define B    2531   // 987.8 Hz
#define BF   2681   // 932.3 Hz
#define A    2841   // 880 Hz
#define AF   3010   // 830.6 Hz
#define G    3189   // 784 Hz
#define GF  3378   // 740 Hz
#define F   3579   // 698.5 Hz
#define E   3792   // 659.3 Hz
#define EF  4018   // 622.3 Hz
#define D   4257   // 587.3 Hz
#define DF  4510   // 554.4 Hz
#define C1		4778
#define T			10000
#define T2		20000




#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"
#include "Random.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "../inc/DAC.h"
#include "Images.h"
#include "../inc/wave.h"
#include "Timer1.h"
#include "Timer0.h"
//#include "Sound.c"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

int mode = 0;

void Timer1A_Handler(void){ // can be used to perform tasks in background
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
   // execute user task
	// use this for sound
}



































// Initialize Port F so PF1, PF2 and PF3 are heartbeats
void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;      // activate port F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 (built-in LED)
  GPIO_PORTF_PUR_R |= 0x10;
  GPIO_PORTF_DEN_R |=  0x1E;   // enable digital I/O on PF
}


void Timer3A_Init(uint32_t period, uint32_t priority){
  volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate TIMER3
  delay = SYSCTL_RCGCTIMER_R;         // user function
  TIMER3_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|(priority<<29); // priority  
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN1_R = 1<<(35-32);      // 9) enable IRQ 35 in NVIC
  TIMER3_CTL_R = 0x00000001;    // 10) enable timer3A
}

void Timer3A_Stop(void){
  NVIC_DIS1_R = 1<<(35-32);   // 9) disable interrupt 35 in NVIC
  TIMER3_CTL_R = 0x00000000;  // 10) disable timer3
}


//********************************************************************************************************


typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
typedef enum {DIED, LEVEL, LANGUAGE} phrase_t;
const char Died_English[] ="YOU DIED";
const char Died_Spanish[] ="USTED MURIO";
const char Level_English[]="LEVEL: ";
const char Level_Spanish[]="NIVEL: ";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char *Phrases[3][2]={
  {Died_English,Died_Spanish},
  {Level_English,Level_Spanish},
  {Language_English,Language_Spanish}
};




void Key_Init(void){ 
  // used in Lab 6 
	uint32_t volatile delay;
	
	SYSCTL_RCGCGPIO_R |= 0x10; // activate port E  _ _ _ 1    _ _ _ _ 
	
	delay = SYSCTL_RCGCGPIO_R;
	delay = SYSCTL_RCGCGPIO_R;

	GPIO_PORTE_DIR_R &= ~0x06;  // make PE1,2 input 	 _ _ _ _    _ 1 1 _ 
  GPIO_PORTE_DEN_R |= 0x06;  // enable digital I/O
	
	GPIO_PORTA_DIR_R &= ~0x10;  // make PA4 input 	 _ _ _ _    _ 1 1 _ 
  GPIO_PORTA_DEN_R |= 0x10;  // enable digital I/O
	
}


volatile uint32_t FallingEdges = 0;

void PausePlay_Init(void){

  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F

    DisableInterrupts();

    FallingEdges = 0;             // (b) initialize count and wait for clock
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  //GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event - commented because we want rising edge event
    GPIO_PORTF_IEV_R |= 0x10;    //     PF4 rising edge event 
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC


}

void GPIOPortF_Handler(){
	DisableInterrupts();

  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4
  FallingEdges = FallingEdges + 1;
	
			if (mode == 1){
			ST7735_FillScreen(0x0000);            // set screen to black
			ST7735_SetCursor(2, 1);
			ST7735_OutString("PRESS AGAIN");
			ST7735_SetCursor(2, 2);
			ST7735_OutString("TO CONTINUE PLAYING");
			while ((GPIO_PORTF_DATA_R & 0x10) == 0x10){}
			 // GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4??
			EnableInterrupts();				
			//print pause, then one contact again enable interrupts again	
			}
		
			if (mode == 2){
					
			ST7735_FillScreen(0x0000);            // set screen to black
			ST7735_SetCursor(2, 1);
			ST7735_OutString("PRESIONE DE NUEVO");
			ST7735_SetCursor(2, 2);
			ST7735_OutString("PARA CONTINUAR");
			while ((GPIO_PORTF_DATA_R & 0x10) == 0x10){}
			//GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4??
				EnableInterrupts();
			//print pause, then one contact again enable interrupts again	
			}
		}
	

		
		
		


struct brick	{
	int32_t brickx;
	int32_t bricky;
	int32_t length;
	int32_t height;

};

	struct brick red;
	//struct brick blue;
	struct brick green;
	struct brick yellow;





uint32_t userx = 50;
uint32_t usery = 100;
uint32_t yrate = -5; // change starting rate to 2
uint32_t xrate = 0;
uint32_t level = 1;
uint32_t counter = 0;
uint32_t flagegg = 1;
void render_sprites(void) {
	

	ST7735_DrawBitmap(red.brickx, red.bricky, Egg, red.length, red.height);
//	ST7735_DrawBitmap(blue.brickx, blue.bricky, Egg, blue.length, blue.height);
	ST7735_DrawBitmap(green.brickx, green.bricky, Egg, green.length, green.height);
	ST7735_DrawBitmap(yellow.brickx, yellow.bricky, Egg, yellow.length, yellow.height);


}


uint32_t platformX;
int Data;
uint32_t flag = 0;


// write an adc handler
void Timer3A_Handler (void) {
	
	NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value
  NVIC_ST_CURRENT_R = 0;    // any write to current clears it
  NVIC_ST_CTRL_R = 5;
	TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER2A timeout
	Data = ADC_In(); // sample the ADC
	Data = Data/37;
	

	ST7735_FillScreen(0x0000);            // set screen to black
	render_sprites();

	ST7735_DrawBitmap(Data, 159, Platform, 24,8);	// render platform
	
	if (flagegg ==1){
	ST7735_DrawBitmap(userx, usery, User, 12,12); // render user sprite
	}
	if (flagegg == -1){
	ST7735_DrawBitmap(userx, usery, User1, 12,12); // render blue user sprite
	}
	usery -= (yrate * level); // decrement/increment the y position to give illusion of movement
	userx -= (xrate * level);
	
	// add method to pass in array to a function (method intakes type array, depending on what array is passed in, populate new array)
		
	if (usery == 10){ // bounding against the upper wall
		yrate *= -1;
	}
	if ((userx + 12 >= Data) && (userx <= Data + 24) && usery == 150){ // bounding against the platform
		yrate *= -1;
	}
	if (userx <= 10) {// bounding against the left wall
		xrate = -1;
	}
	if (userx >= 120) {// bounding against the right wall
		xrate = 1;
	}
	if ((userx + 6) < (Data + 12) && usery == 150){ // bounding against left side of platform
		int rate = userx -(Data + 12) ;
		if (rate < 0 ) {
			rate *= -1;
		}
		xrate = (rate/5); 
	}
	if ((userx + 6) > (Data + 12)&& usery == 150){ // bounding against right side of platform
		int ratey = Data - userx;
		xrate = (ratey/5);
	}
	if(usery >= 160) { 	// bounding against the floor (raise a flag? display end screen)
		// raise flag
		flag = 1;
		Wave_Shoot();
	}
	
	// removing bricks
	if (((userx > red.brickx) && (userx < (red.brickx + red.length))&& ((usery - 12) <= red.height)) // if user hits bottom of brick 
		) { // if user hits top of brick
			red.brickx = 300;
			red.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	if (
		(((userx == red.brickx + red.length) || (userx == red.brickx)) && (red.bricky <= usery <= red.bricky + red.height)) || // if user hits side of brick
	((userx > red.brickx) && (userx < (red.brickx + red.length))&& ((usery) <= red.bricky - red.height))) { // if user hits top of brick
			red.brickx = 300;
			red.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	
		if (((userx > green.brickx) && (userx < (green.brickx + green.length))&& ((usery - 12) <= green.height))) { // if user hits top of brick
			green.brickx = 300;
			green.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	
	
			if ((((userx == green.brickx + green.length) || (userx == green.brickx)) && (green.bricky <= usery <= green.bricky + green.height)) || // if user hits side of brick
	((userx > green.brickx) && (userx < (green.brickx + green.length))&& ((usery) <= green.bricky - green.height))) { // if user hits top of brick
			green.brickx = 300;
			green.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	
			if (((userx > yellow.brickx) && (userx < (yellow.brickx + yellow.length))&& ((usery - 12) <= yellow.height))// if user hits bottom of brick 
		) { // if user hits top of brick
			yellow.brickx = 300;
			yellow.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	
		if (
		//(((userx == yellow.brickx + yellow.length) || (userx == yellow.brickx)) && (yellow.bricky <= usery <= yellow.bricky + yellow.height)) || // if user hits side of brick
	((userx > yellow.brickx) && (userx < (yellow.brickx + yellow.length))&& ((usery) <= yellow.bricky - yellow.height))) { // if user hits top of brick
			yellow.brickx = 300;
			yellow.bricky = 300;
			xrate = -1;
			yrate *= -1;
			counter++;
	}
	
	
	if (counter == 3){
		counter = 0;
		//ST7735_DrawBitmap(50, 100, User, 35,35); // render user sprite
		level++;
		userx = 50;
		usery = 80;
		red.brickx = 5;
		red.bricky = 30;
	//	blue.brickx = 35;
	//	blue.bricky = 50;
		green.brickx = 70;
		green.bricky = 50;
		yellow.brickx = 100;
		yellow.bricky = 30;

		
		// clear the board and re-render (reset brickx and bricky)
	}
	

}









//*********************************************************************************************************
int main(void){ char l;
  DisableInterrupts();
  TExaS_Init(NONE);       // Bus clock is 80 MHz 
           // your lab 6 solution
	ST7735_InitR(INITR_REDTAB);
	
	
	NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value
  NVIC_ST_CURRENT_R = 0;    // any write to current clears it
  NVIC_ST_CTRL_R = 5;
	Output_Init();
	Key_Init();
	Wave_Init();
	PausePlay_Init();
	DAC_Init();
	ADC_Init();
  PortF_Init();
	Timer3A_Init(8000000,3); //10 Hz

	// Home screen: offer play or language select
	ST7735_FillScreen(0x0000);            // set screen to black
	ST7735_SetCursor(2, 1);
	ST7735_OutString("CHICKEN + EGG");
	ST7735_SetCursor(2, 2);
  ST7735_OutString("SELECT LANGUAGE");
	ST7735_SetCursor(2, 3);
  ST7735_OutString("PE1 for ENGLISH"); // switch 1 = PE1
  ST7735_SetCursor(2, 4);
  ST7735_OutString("PE2 for SPANISH");
	ST7735_DrawBitmap(50, 100, Chicken, 35,35); // render user sprite

	
	
	
	GPIO_PORTE_DATA_R = 0;
	while (mode == 0){
	if (GPIO_PORTE_DATA_R == 0x02){
		mode = 1;
	}
	if (GPIO_PORTE_DATA_R == 0x04){
		mode = 2;
	}	
}
	


	

	
//GAME 
	//set screen to black 
	ST7735_FillScreen(0x0000);            // set screen to black

	red.brickx = 5;
	red.bricky = 30;
	red.length = 20;
	red.height = 20;

//	blue.brickx = 35;
//	blue.bricky = 50;
//	blue.length = 20;
//	blue.height = 20;

	green.brickx = 70;
	green.bricky = 50;
	green.length = 20;
	green.height = 20;

	yellow.brickx = 100;
	yellow.bricky = 30;
	yellow.length = 20;
	yellow.height = 20;

	EnableInterrupts();	//enable interrupts
	

// ****************************************************************************************************
while(1){
				Wave_Sample();
        if ((GPIO_PORTA_DATA_R & 0x10) == 0x10){
        volatile uint32_t delay;
        delay = SYSCTL_RCGCTIMER_R;         // user function
        delay = SYSCTL_RCGCTIMER_R;         // user function

            flagegg *=-1;
            delay = SYSCTL_RCGCTIMER_R;         // user function
        delay = SYSCTL_RCGCTIMER_R;         // user function

        }
		
		
		if ( flag == 1 && mode == 1) {
			EnableInterrupts();
			for (int i = 0; i <100; i++) {
				Wave_Shoot();
			}
			DisableInterrupts();
			ST7735_FillScreen(0x0000);            // set screen to black
			ST7735_SetCursor(1, 3);
			//ST7735_OutString("YOU LOST");
			ST7735_OutString((char *)Phrases[DIED][0]);

			ST7735_SetCursor(1, 4);
			ST7735_OutString((char *)Phrases[LEVEL][0]);
			ST7735_SetCursor(8, 4);
			ST7735_OutUDec(level);
			ST7735_DrawBitmap(50, 100, Chicken, 35,35); // render user sprite
			while(1){}
		}
		
		if ( flag == 1 && mode == 2) {
			EnableInterrupts();
			for (int i = 0; i < 100; i++) {
				Wave_Shoot();
			}
			DisableInterrupts();
			ST7735_FillScreen(0x0000);            // set screen to black
			ST7735_SetCursor(1, 3);
			ST7735_OutString((char *)Phrases[DIED][1]);
			ST7735_SetCursor(1, 4);
			ST7735_OutString((char *)Phrases[LEVEL][1]);
			ST7735_SetCursor(8, 4);
			ST7735_OutUDec(level);
			ST7735_DrawBitmap(50, 100, Chicken, 35,35); // render user sprite
			while(1){}
		}

	}
	// Sounds:
	// wave_start will set the index to 0, iterate through the array of sounds, and then disable interrupts (or set back to 0 for continuous loop). 
   
}

