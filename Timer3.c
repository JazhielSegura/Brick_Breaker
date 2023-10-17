// Lab9.c
// Runs on LM4F120 or TM4C123
// Student names: Keira Tran, Jazhiel Segura
// Last modification date: change this to the last modification date or look very silly
// Last Modified: 1/2/2023

// Analog Input connected to PD2=ADC1
// Labs 8 and 9 specify PD2
// displays on Sitronox ST7735
// PF3, PF2, PF1 are heartbeats
// UART1 on PC4-5
// * Start with where you left off in Lab8. 
// * Get Lab8 code working in this project.
// * Understand what parts of your main have to move into the UART1_Handler ISR
// * Rewrite the Timer3A_Handler
// * Implement the s/w Fifo on the receiver end 
//    (we suggest implementing and testing this first)

#include <stdint.h>

#include "../inc/ST7735.h"
#include "TExaS.h"
#include "../inc/ADC.h"
#include "../inc/tm4c123gh6pm.h"
#include "UART1.h"
#include "Fifo.h"
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PC54      (*((volatile uint32_t *)0x400060C0)) // bits 5-4
#define PF321     (*((volatile uint32_t *)0x40025038)) // bits 3-1
// TExaSdisplay logic analyzer shows 7 bits 0,PC5,PC4,PF3,PF2,PF1,0 
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PC54; // sends at 10kHz
}

//*****the main2 is for debugging *****
// main2 test your fifo


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

uint32_t Data;      // 12-bit ADC
uint32_t Position;  // 32-bit fixed-point 0.001 cm
int32_t TxCounter = 0;

uint32_t startTime,stopTime;
uint32_t ADCtime,Converttime,OutDectime,OutFixtime; // in usec

// Initialize Port F so PF1, PF2 and PF3 are heartbeats
void PortF_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x20;      // activate port F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 (built-in LED)
  GPIO_PORTF_PUR_R |= 0x10;
  GPIO_PORTF_DEN_R |=  0x1E;   // enable digital I/O on PF
}
// ***************** Timer3A_Init ****************
// Activate Timer3 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (1/clockfreq)
//          priority 0 (highest) to 7 (lowest)
// Outputs: none
// if samping at 10 Hz, the period is 1/10 second. The bus clock is 12.5 ns
// 0.1 seconds / 12.5 ns = 100000000
// 10^9 x 0.1 /12.5 = 8000000
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




// Get fit from excel and code the convert routine with the constants
// from the curve-fit
uint32_t Convert(uint32_t x){
  // copy this from Lab 8 
	return 1910*x/4096+8; // 
}


// final main program for bidirectional communication
// Sender sends using Timer3 interrupt
// Receiver receives using RX interrupt

//initializes the TExaS, Timer3, ADC, UART1, heartbeat, and enables interrupts. 
	//After initialization, this transmitter test main (foreground) performs a do-nothing loop.


int mainreal(void){  
	  //PLL_Init();
  TExaS_Init(&LogicAnalyzerTask);
	
  ST7735_InitR(INITR_REDTAB);
	Timer3A_Init (2000, 2000000); // not sure what these values should be
	
  ADC_Init();    // initialize to sample ADC1
	
  PortF_Init();
	
  UART1_Init();       // initialize UART
	
  EnableInterrupts();
	
  while(1){ // runs every 10ms
  // write this
// Calls your InChar (FIFO get) waiting until new data arrives.
//    wait until you see the ‘<’byte
// Calls your InChar (FIFO get)  waiting 5 more times
//    The next five characters after the ‘<’ should be the ASCII representation of the distance
//  Output the fixed-point number (same format as Lab 8) with units on the LCD. 
    
  }
}


void Timer3A_Handler(void){
	uint32_t	digit = 0;
	uint32_t digit1 = 0;
	volatile uint32_t delay;// 
	uint32_t time=0;
  volatile uint32_t elapsedTime;
	uint32_t sampleStartTime;
 
  NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value
  NVIC_ST_CURRENT_R = 0;    // any write to current clears it
  NVIC_ST_CTRL_R = 5;
	
	TIMER3_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER2A timeout

		
// Toggle just once if you are using TExaS logic analyzer, toggle three times when using the real scope:
//1) toggle a heartbeat , 
        //GPIO_PORTF_DATA_R ^= 0x04; //toggle PF2
//2) sample the ADC
//4) convert to distance and create the 8-byte message
//5) send the 8-byte message to the other computer (calls UART1_OutChar 8 times)
				
	
		//while(1){
    //PF2 ^= 0x04;      // Heartbeat

   // Data = ADC_In();  // sample 12-bit channel 5
   // Position = Convert(Data); 
			ST7735_PlotClear(0,2000); 

		//ST7735_SetCursor(0,0);
    //ST7735_OutUDec(Data); 
		//ST7735_OutString("    "); 
    //ST7735_SetCursor(6,0);
    //ST7735_OutFix(Position);
		//ST7735_OutString (" cm ");
		
		
			
	GPIO_PORTF_DATA_R ^= 0x04; //toggle PF2

	UART1_OutChar('<');
	Data=ADC_In();
	Data=Convert(Data);
		
	digit=Data/1000;
	digit1=digit+0x30;
	UART1_OutChar(digit1);
		
	UART1_OutChar('.'); // output '.'
		
	Data=Data-(digit*1000);
	digit=Data/100;
	digit1=digit+0x30;
	UART1_OutChar(digit1);
	
	Data=Data-(digit*100);
	digit=Data/10;
	digit1=digit+0x30;
	UART1_OutChar(digit1);
	
	Data=Data-(digit*10);
	digit1=digit+0x30;
	UART1_OutChar(digit1);
	
	UART1_OutChar('>');
				
	UART1_OutChar(0x0A); // next line
	
	
	
//6) increment a TxCounter, used as debugging monitor of the number of ADC samples collected 
        TxCounter++;
// return from interrupt
}


uint32_t M;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}  
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}  
void MyFifo_Init(uint32_t size);
uint32_t MyFifo_Put(char data);
char MyFifo_Get(void);
uint32_t FifoError;
int main2(void){ // Make this main to test Fifo
  char me,you;
  char data;
  PLL_Init();
  PortF_Init();
  // your FIFO must be larger than 8 and less than 63
  Fifo_Init();     // your FIFO
  M = 4; // seed
  FifoError = 0;
  // size =17 means FIFO can store up to 16 elements
  MyFifo_Init(16); // change 17 to match your FIFO size
  for(uint32_t i = 0; i<10000; i++){
    uint32_t k = Random(4);
    for(uint32_t l=0; l<k ;l++){
      data = Random(256);
      you = Fifo_Put(data);
      me = MyFifo_Put(data);
      if(me != you){
        FifoError++; // put a breakpoint here
      }
    } 
    uint32_t j = Random(40);
    for(uint32_t l=0; l<j ;l++){
      data = Random(256);
      you = Fifo_Put(data);
      me = MyFifo_Put(data);
      if(me != you){
        FifoError++; // put a breakpoint here
      }
    }
    for(uint32_t l=0; l<j ;l++){
      you = Fifo_Get();
      me = MyFifo_Get();
      if(me != you){
        FifoError++; // put a breakpoint here
      }
    } 
    for(uint32_t l=0; l<k ;l++){
      you = Fifo_Get();
      me = MyFifo_Get();
      if(me != you){
        FifoError++; // put a breakpoint here
      }
    }  
  }    
  if(FifoError == 0){
    for(;;){ // success
      GPIO_PORTF_DATA_R ^= 0x08; // green
      for(int i=0; i<100000; i++){
      }
    }
  }else{
    for(;;){ // failure
      GPIO_PORTF_DATA_R ^= 0x02; // red
      for(int i=0; i<50000; i++){
      }
    }
  }
}



int main(void){ 
	DisableInterrupts();
	TExaS_Init(&LogicAnalyzerTask);
  //PLL_Init();
	ST7735_InitR(INITR_REDTAB);
	ST7735_PlotClear(0,2000); 
	ST7735_SetCursor(0,0);

  NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value
  NVIC_ST_CURRENT_R = 0;    // any write to current clears it
  NVIC_ST_CTRL_R = 5;

	ADC_Init();
	UART1_Init();
  PortF_Init();
	Timer3A_Init(8000000,3); //10 Hz
	EnableInterrupts();

		 while(1){ // runs every 10ms
		// Calls your InChar (FIFO get) waiting until new data arrives.
			ST7735_SetCursor(0,0);

			//    wait until you see the ‘<’byte
		if (UART1_InChar() != ('<')) {
			continue;
		}
			// Calls your InChar (FIFO get)  waiting 5 more times
			for (int i = 0; i < 5; i++){
			//    The next five characters after the ‘<’ should be the ASCII representation of the distance
			//  Output the fixed-point number (same format as Lab 8) with units on the LCD. 
			ST7735_OutChar(UART1_InChar());
		}
		ST7735_OutString (" cm ");

		    
  }
}