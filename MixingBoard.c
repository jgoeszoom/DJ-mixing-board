/* Joseph Gozum
 * Email: jgozu001@ucr.edu
 *
 * UCR CS120B - Final Lab Project
 * Description: A simplified version of a DJ Mixing board,
 * this takes in input from a 10K potentiometer and 4x4 keypad
 * to navigate the menu system as well as manipulate the
 * properties of the included and user created songs.
*/

#include "avr/io.h"			
#include "avr/interrupt.h"
#include "avr/eeprom.h"
#include "ucr/keypad.h"		//Header used to take input from keypad
#include "ucr/io.c"			//C file used for the LCD functions
#include "ucr/io.h"			//Header file used for the LCD functions
#include "ucr/PWM.h"		//Header file for PWM functions
#include "ucr/scheduler.h"	//Header for the task struct
#include "ucr/timer.h"		//Header for the timer functions
#include "ucr/ADC.h"		//Header file for Analog-2-Digital functions
#include "ucr/shift.h"		//Header file for using the shift register

//==========Function Periods=================
unsigned char MasterPeriod				= 100; 
unsigned char GetButtonsPeriod			= 100;
unsigned char GetPotentiometerPeriod	= 100;
unsigned char MainMenuPeriod			= 100;
unsigned char CreateMenuPeriod			= 100; 
unsigned char ModifyMenuPeriod			= 100; 
unsigned char PlayMenuPeriod			= 100;
unsigned char BuzzerPeriod				= 100;  
unsigned char WelcomePeriod				= 100; 
//===========================================

//==========GetButtons=======================
//Shared variables from GetKeyPad function
//Represents the button that has been pressed
unsigned char uchar_keypadInput		= 0x00;
//Flag set by to determine if key has been 
//pressed
unsigned char IsKeyPressed			= 0;
//===========================================

//==========GetPotentionmeter================
//Shared variable from GetPotentiometer
unsigned char uchar_Potentiometer	= 0x00;
//===========================================

//==========Buzzer/PWM=======================
//Predetermined length for every song array
#define SONG_SIZE 30
//Defined values of note frequencies
#define c4	261
#define d4	293
#define e4	329
#define f4	349
#define g4	392
#define a4	440

//Flag for whether the Buzzer should be turned
//on or not 
unsigned char IsBuzzOn	= 0;	//0:Off 1:On
//Iterator to go through array of frequencies
unsigned char Buzzer_iter = 0; 
 unsigned short newSongAddr = 1; 
//===========================================

//==========LED Display======================
//Counter to display different patterns at different times
unsigned char LED_cntr = 0;
//Just a flag
unsigned char LEDsOn = 0; 
//Patterns to display on LED Bar
unsigned char LEDPattern1 = 0xAA;
unsigned char LEDPattern2 = 0x55;
unsigned char LEDPattern3 = 0x83;
unsigned char LEDPattern4 = 0x17;
unsigned char NULLPattern = 0x00;
//===========================================

//==========DisplaySMs=======================
//General Variables: 
//Flags for the different Menus
unsigned char Welcome_flag		= 1;		//Determines if Welcome message should be displayed
unsigned char MainMenu_flag		= 0;		//Determines if Main Menu should be displayed
unsigned char CreateMenu_flag	= 0;		//"			  " Create Menu should be displayed
unsigned char ModifyMenu_flag	= 0;		//"           " Modify Menu should be displayed
unsigned char PlayMenu_flag		= 0;		//"           " Play Menu should be displayed
//Wait times for instructions, this gives users time to read prompt
//As of now I have determined 4 seconds is a good length of time
#define wait_to_read 40	//Waits for 4 secs equivalently

#define Back_Button '*' 

//Welcome Menu Variables: 
char Welcome_Message[33] = "    DJ Board     Press Start(#) ";
unsigned char Welcome_cntr = 0; 

//Main Menu Variables:  
char MainMenu_Prompt[33] = "1:Create 3:Play 2:Modify (*)EXIT";

//Create Menu Variables: 
char CreateMenu_Prompt1[33]	= "Create: Press key to insert note";
char CreateMenu_Prompt2[33] = "1:C4 2:D4 3:E4  4:F4 5:G4 6:A4 ";
char CreateMenu_Prompt3[33] = "No more space;  I'm full.";
unsigned char CM_cntr	= 0;		//Used to determine if wait_to_read time has been met 
unsigned numNotes		= 0;		//Keeps track of # of notes inserted to EEPROM
//User populated array of note frequencies
double NewSong[SONG_SIZE]; 
//Predefined array of frequencies					
double DEMO[SONG_SIZE] = {c4, c4, d4, c4, g4, e4, c4, g4, 
						  g4, g4, a4, g4, g4, e4, d4, g4, 
						  c4, e4, a4, f4, a4, e4, g4, f4, 
						  c4, d4, d4, g4, c4, d4}; 

//Modify Menu Variables:  
char ModifyMenu_Prompt[33] = "Change:         1:Pitch 3:Length";
char ModifyMenu_Prompt2[33] = "Pitch +/- x100Hz                "; 
char ModifyMenu_Prompt3[33] = "A:125 B:150 C:200 D:300 #:100 ms";
#define four	0x34
#define three	0x33
#define two		0x32
#define one		0x31
#define zero	0x30
//Used to modulate the pitch depending on the potentiometer
double Pitch_Modulation = 0.0; 
short PeriodDiff = 0; 
unsigned int PeriodChangeAddr = 0; 

//Play Menu Variables: 
char PlayMenu_Prompt1[33] = "Select song:    1:DEMO 2:NEW";
//Flag for which song to play
unsigned char WhichSong = 0; 
//===========================================

//State machine to take in keypad input
enum Keypad_States {Pressed, Unpressed};
int GetButtons(int state) {
	unsigned char Key = GetKeypadKey();
	switch(state) {
		case Unpressed:
			if (Key != '\0') {
				uchar_keypadInput = Key;
				IsKeyPressed = 1;
				state = Pressed;
			} else {
				state = Unpressed;
			}
			break;
		case Pressed:
			if (Key == '\0') {
				IsKeyPressed = 0;
				state = Unpressed;
			} else {
				state = Pressed;
			}
			break;
		default:
			state = Unpressed;
			break;
	}
	return state;
}

//State machine to take in potentiometer input
enum Potentiometer_States {GetValue};
int GetPotentiometer(int state) {
	unsigned int Potentiometer = ADC;
	switch(state) {
		case GetValue:
			uchar_Potentiometer = (char) Potentiometer; 
			state = GetValue;
			break;
		default: state = GetValue; break;
	}
	return state;
} 

//State machine to control the Buzzer output
enum Buzzer_States {Buzzer_Wait, Buzzer};
int PlayBuzzer(int state) {
	switch(state) {
		//Unless the Buzzer has been activated 
		//It will stay in this wait state, 
		//if turned on it will enable the PWM and go 
		//on to the state to set the frequencies 
		case Buzzer_Wait:
			if ( IsBuzzOn == 1 ) {
				TimerSet(MasterPeriod + eeprom_read_word((uint16_t*)0) ); 
				PWM_on(); 
				Buzzer_iter = 0; 
				state = Buzzer; 
			} else {
				PWM_off(); 
				state = Buzzer_Wait; 	
			}
			break;
		//Iterates through the song array containing 
		//the various frequencies to play a melody.
		//Once it has reached the end of the array it will reset
		//everything to before it began playing. 
		case Buzzer:
			if ( WhichSong == 1 ) {
				if ( Buzzer_iter < SONG_SIZE ) { 
					set_PWM(DEMO[Buzzer_iter] + Pitch_Modulation);
					Buzzer_iter = Buzzer_iter + 1;  
					state = Buzzer; 
				} else if ( Buzzer_iter >= SONG_SIZE ) {
					PWM_off(); 
					TimerSet(MasterPeriod); 
					Buzzer_iter = 0; 
					IsBuzzOn = 0;  
					WhichSong = 0; 
					state = Buzzer_Wait; 
				}
			} else if ( WhichSong == 2 ) {
				if ( Buzzer_iter < SONG_SIZE ) {
					set_PWM(eeprom_read_word((uint16_t*)newSongAddr));
					Buzzer_iter = Buzzer_iter + 1; 
					state = Buzzer;
				} else if ( Buzzer_iter >= SONG_SIZE ) {
					PWM_off();
					TimerSet(MasterPeriod); 
					Buzzer_iter = 0; 
					IsBuzzOn = 0;
					WhichSong = 0; 
					state = Buzzer_Wait;
				}	
			} else {
				Buzzer_iter	= 0; 
				IsBuzzOn = 0; 
				WhichSong = 0; 
				state = Buzzer_Wait; 
			}
			break;	
		default: state = Buzzer_Wait; break; 
	}	
	return state; 
}

//State machine to control LEDs to accompany music
enum LED_States {LED_Wait, LED_ON};
int LED_Display(int state) {
	switch(state) {
		case LED_Wait:
			if ( LEDsOn == 1 ) {
				state = LED_ON;
			} else {
				state = LED_Wait;
			}
			break;
		case LED_ON:
			if ( LEDsOn == 1 ) {
				if ( (LED_cntr % 2 == 0) && (LED_cntr % 3 == 0) ) {
					transmit_data(LEDPattern1); 
					LED_cntr = LED_cntr + 1; 
					state = LED_ON; 
				} else if ( (LED_cntr % 2 == 0) && !(LED_cntr % 3 == 0) ) {
					transmit_data(LEDPattern2);
					LED_cntr = LED_cntr + 1;
					state = LED_ON;
				} else if ( !(LED_cntr % 2 == 0)  && (LED_cntr % 3 == 0) ) {
					transmit_data(LEDPattern3);
					LED_cntr = LED_cntr + 1;
					state = LED_ON;
				} else if ( !(LED_cntr % 2 == 0) && !(LED_cntr % 3 == 0) ){
					transmit_data(LEDPattern4);
					LED_cntr = LED_cntr + 1; 
					state = LED_ON;
				} else {
					transmit_data(NULLPattern); 
					state = LED_ON; 	
				}
			} else if ( LEDsOn != 1 ) {
				PWM_off();
				transmit_data(NULLPattern); 
				state = LED_Wait;
			}
			break;	
		default: state = LED_Wait; break;
	}
	return state;
}

enum Welcomes_States {Welcome_Wait, Welcome_Displaying};
int Display_Welcome(int state) {
	switch(state) {
		case Welcome_Wait:
			if ( Welcome_flag == 1 ) {
				LCD_DisplayString(1, Welcome_Message);
				LCD_Cursor(1);
				LCD_WriteData(0x00);
				LCD_Cursor(16);
				LCD_WriteData(0x00);
				LEDsOn = 1; 
				state = Welcome_Displaying; 	
			}
			else if ( !(Welcome_flag == 1) ) {
				state = Welcome_Wait; 	
			}
			break; 
			
		//Instead of welcome counter try using button press
		case Welcome_Displaying: 
			if ( uchar_keypadInput != '#' ) {
				state = Welcome_Displaying;	
			}
			else if ( uchar_keypadInput == '#' ) {
				Welcome_flag = 0; 
				MainMenu_flag = 1; 
				Welcome_cntr = 0; 
				LEDsOn = 0; 
				state = Welcome_Wait; 
			}
			break; 
	}
	return state; 
}

//State machine to display the options available in the main menu
//and raise flags to navigate to the sub-menus
enum MainMenu_States {MainMenu_Wait, Main_Menu};
int Display_MainMenu(int state) {
	switch(state) {
		case MainMenu_Wait:
			if ( MainMenu_flag == 1 ) {
				LCD_DisplayString(1, MainMenu_Prompt); 
				LCD_Cursor(16); 
				LCD_WriteData(0x00); 
				state = Main_Menu;
			} else {
				state = MainMenu_Wait; 
			}
			break; 		
		case Main_Menu:
			if (IsKeyPressed == 1 && uchar_keypadInput == '1') {
				MainMenu_flag = 0; 
				CreateMenu_flag = 1; 
				state = MainMenu_Wait; 
			} else if (IsKeyPressed == 1 && uchar_keypadInput == '2') {
				MainMenu_flag = 0; 
				ModifyMenu_flag = 1; 
				state = MainMenu_Wait;
			} else if (IsKeyPressed == 1 && uchar_keypadInput == '3') {
				MainMenu_flag = 0; 
				PlayMenu_flag = 1; 
				state = MainMenu_Wait;
			} else if (IsKeyPressed == 1 && uchar_keypadInput == Back_Button) {
				MainMenu_flag = 0; 
				Welcome_flag = 1; 
				state = MainMenu_Wait; 
			}
			else {
				state = Main_Menu; 
			}
			break;		
		default: state = MainMenu_Wait; break; 	
	}
	return state; 
}
 
//State machine to display the options available in the create menu
enum CreateMenu_States {CreateMenu_Wait, Create_Options, Create_Menu, Create_Full};
int Display_CreateMenu(int state) {
	switch(state) {
		case CreateMenu_Wait:
			if ( CreateMenu_flag == 1 && numNotes < SONG_SIZE ) {
				LCD_DisplayString(1, CreateMenu_Prompt1);
				state = Create_Options;
			} else if ( CreateMenu_flag == 1 && numNotes >= SONG_SIZE ) {
				LCD_DisplayString(1, CreateMenu_Prompt3); 
				state = Create_Full;
			} else {
				state = CreateMenu_Wait; 
			}
			break;		
			
		case Create_Options:
			if ( CM_cntr < wait_to_read ) {
				CM_cntr = CM_cntr + 1; 
				state = Create_Options; 
			} else if ( CM_cntr >= wait_to_read ) {
				LCD_DisplayString(1, CreateMenu_Prompt2); 
				LCD_Cursor(32); 
				CM_cntr = 0;
				state = Create_Menu; 
			} 
			break; 		
			
		case Create_Full:
			if ( CM_cntr < wait_to_read ) {
				CM_cntr = CM_cntr + 1;
				state = Create_Full;
			} else if ( CM_cntr >= wait_to_read ) {
				MainMenu_flag = 1;
				CreateMenu_flag = 0;
				CM_cntr = 0;
				state = CreateMenu_Wait;
			}
			break;
			
		case Create_Menu: 
			if ( newSongAddr >= SONG_SIZE ) {
				LCD_DisplayString(1, CreateMenu_Prompt3); 
				state = Create_Full; 
				newSongAddr = 1; 
				break; 
			}
			if ( uchar_keypadInput == Back_Button ) {
				CreateMenu_flag = 0;
				MainMenu_flag = 1;
				state = CreateMenu_Wait;
			} else if ( uchar_keypadInput == '1' ) {
				LCD_Cursor(1); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)c4);
				newSongAddr = newSongAddr + 1; 
				state = Create_Menu;  
			} else if ( uchar_keypadInput == '2' ) {
				LCD_Cursor(6); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)d4);
				newSongAddr = newSongAddr + 1;
				state = Create_Menu; 
			} else if ( uchar_keypadInput == '3' ) {
				LCD_Cursor(11); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)e4);
				newSongAddr = newSongAddr + 1;
				state = Create_Menu; 	
			} else if ( uchar_keypadInput == '4' ) {
				LCD_Cursor(17); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)f4);
				newSongAddr = newSongAddr + 1;
				state = Create_Menu; 
			} else if ( uchar_keypadInput == '5' ) {
				LCD_Cursor(22); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)g4);
				newSongAddr = newSongAddr + 1;
				state = Create_Menu; 
			} else if ( uchar_keypadInput == '6' ) {
				LCD_Cursor(27); 
				eeprom_write_word((uint16_t*)newSongAddr, (uint16_t)a4);
				newSongAddr = newSongAddr + 1;
				state = Create_Menu; 
			} else {
				LCD_Cursor(32); 
				state = Create_Menu; 
			}
			break;
		default: state = CreateMenu_Wait; break;
	}
	return state;
}

//State machine to display the options available in the Modify menu
enum ModifyMenu_States {ModifyMenu_Wait, Modify_Menu, Change_Pitch, Change_Length};
int Display_ModifyMenu(int state) {
	switch(state) {
		case ModifyMenu_Wait:
			if ( ModifyMenu_flag == 1 ) {
				LCD_DisplayString(1, ModifyMenu_Prompt);
				state = Modify_Menu;
			} else {
				state = ModifyMenu_Wait;
			}
			break;				
		case Modify_Menu:
			if ( uchar_keypadInput == Back_Button ) {
				ModifyMenu_flag = 0;
				MainMenu_flag = 1;
				state = ModifyMenu_Wait; 
			} else if ( uchar_keypadInput == '1' ) {
				LCD_DisplayString(1, ModifyMenu_Prompt2); 
				state = Change_Pitch;
			} else if ( uchar_keypadInput == '3' ) {
				LCD_DisplayString(1, ModifyMenu_Prompt3); 
				state = Change_Length; 
			} else {
				state = Modify_Menu; 
			}
			break;			
		case Change_Length: 
			if ( uchar_keypadInput == Back_Button ) {
				state = Modify_Menu; 	
			} else if ( uchar_keypadInput == 'A' ) {
				//Subtracts from master period to 
				//decrease length of Change_Length
				PeriodDiff = 25;
				eeprom_write_word((uint16_t*)PeriodChangeAddr, (uint16_t)PeriodDiff); 
				LCD_Cursor(1); 
				state = Change_Length; 
			} else if ( uchar_keypadInput == 'B' ) {
				PeriodDiff = 50;	
				eeprom_write_word((uint16_t*)PeriodChangeAddr, (uint16_t)PeriodDiff); 
				LCD_Cursor(7);
				state = Change_Length; 
			} else if ( uchar_keypadInput == 'C' ) {
				//Adds to master period to 
				//increase length of Change_Length
				PeriodDiff = 100; 
				eeprom_write_word((uint16_t*)PeriodChangeAddr, (uint16_t)PeriodDiff); 
				LCD_Cursor(13); 
				state = Change_Length; 
			} else if ( uchar_keypadInput == 'D' ) {
				PeriodDiff = 200; 
				eeprom_write_word((uint16_t*)PeriodChangeAddr, (uint16_t)PeriodDiff); 
				LCD_Cursor(19); 
				state = Change_Length; 
			} else if ( uchar_keypadInput == '#' ) {
				PeriodDiff = 0; 
				eeprom_write_word((uint16_t*)PeriodChangeAddr, (uint16_t)PeriodDiff); 
				LCD_Cursor(25); 
				state = Change_Length; 
			}
			break;				
		case Change_Pitch:
			if ( uchar_keypadInput == Back_Button ) {
				state = Modify_Menu;
			} else if ( uchar_Potentiometer <= 28 ) {
				LCD_Cursor(17); 
				LCD_WriteData(four);
				LCD_Cursor(9);
				Pitch_Modulation = -400.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 28 && uchar_Potentiometer <= 56 ) {
				LCD_Cursor(17); 
				LCD_WriteData(three);
				LCD_Cursor(9); 
				Pitch_Modulation = -300.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 56 && uchar_Potentiometer <= 84 ) {
				LCD_Cursor(17); 
				LCD_WriteData(two);
				LCD_Cursor(9); 
				Pitch_Modulation = -200.0; 
				state = Change_Pitch;
			} else if ( uchar_Potentiometer > 84 && uchar_Potentiometer <= 112 ) {
				LCD_Cursor(17); 
				LCD_WriteData(one);
				LCD_Cursor(9); 
				Pitch_Modulation = -100.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 112 && uchar_Potentiometer <= 140 ) {
				LCD_Cursor(17); 
				LCD_WriteData(zero); 
				LCD_Cursor(8); 
				Pitch_Modulation = 0.0; 
				state = Change_Pitch;
			} else if ( uchar_Potentiometer > 140 && uchar_Potentiometer <= 168 ) {
				LCD_Cursor(17); 
				LCD_WriteData(one);
				LCD_Cursor(7); 
				Pitch_Modulation = 100.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 168 && uchar_Potentiometer <= 196 ) {
				LCD_Cursor(17); 
				LCD_WriteData(two);
				LCD_Cursor(7); 
				Pitch_Modulation = 200.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 224 && uchar_Potentiometer <= 252 ) {
				LCD_Cursor(17); 
				LCD_WriteData(three);
				LCD_Cursor(7); 
				Pitch_Modulation = 300.0; 
				state = Change_Pitch; 
			} else if ( uchar_Potentiometer > 252 ) {
				LCD_Cursor(17); 
				LCD_WriteData(four); 
				LCD_Cursor(7); 
				Pitch_Modulation = 400.0; 
				state = Change_Pitch; 
			} else {
				state = Change_Pitch; 
			}
			break; 
		default: state = ModifyMenu_Wait; break;
	}
	return state;
}

//State machine to display the options available in the Play menu
enum PlayMenu_States {PlayMenu_Wait, Play_Menu};
int Display_PlayMenu(int state) {
	switch(state) {
		case PlayMenu_Wait:
			if ( PlayMenu_flag == 1 ) {
				LCD_DisplayString(1, PlayMenu_Prompt1);
				LCD_Cursor(32); 
				LCD_WriteData(0x00); 
				state = Play_Menu;
			} else {
				state = PlayMenu_Wait;
			}
			break;		
		case Play_Menu:
			if ( uchar_keypadInput == 'A' ) {
				IsBuzzOn = 0; 
				LEDsOn = 0; 
				WhichSong = 0; 
				TimerSet(MasterPeriod); 
				state = Play_Menu; 
			}
			else if ( uchar_keypadInput == '1' && IsBuzzOn == 0) {
				LCD_Cursor(17);
				IsBuzzOn = 1; 
				LEDsOn = 1; 
				WhichSong = 1; 
				state = Play_Menu;
			} else if ( uchar_keypadInput == '2' && IsBuzzOn == 0 ) {
				LCD_Cursor(24);
				IsBuzzOn = 1;  
				WhichSong = 2; 
				LEDsOn = 1; 
				state = Play_Menu; 	
			} else if ( uchar_keypadInput == Back_Button ) {
				PlayMenu_flag = 0; 
				MainMenu_flag = 1; 
				IsBuzzOn = 0; 
				LEDsOn = 0; 
				state = PlayMenu_Wait; 
			}
			else {
				state = Play_Menu; 
			}
			break;			
		default: state = PlayMenu_Wait; break;
	}
	return state;
}

int main (void) {
	
	//Initializes the PORT registers
	//for input or output
	DDRA = 0x06; PORTA = 0xF9;	//Potentiometer and LCD control lines on this port
	DDRB = 0xFF; PORTB = 0xFF;	//Buzzer and output on this port
	DDRC = 0xFF; PORTC = 0x00;	//LCD Data Bus on this port
	DDRD = 0xF0; PORTD = 0x0F;	//Keypad is on this port
	
    unsigned short task_iter = 0;	
	const unsigned short tasksNum = 9;	//No. of tasks in total
	task tasks[tasksNum];				//An array of task structs
	
	//Sets up the various task structs
    tasks[task_iter].state = Unpressed;
	tasks[task_iter].period = GetButtonsPeriod;	
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &GetButtons;
	++task_iter;
	tasks[task_iter].state = GetValue;
	tasks[task_iter].period = GetPotentiometerPeriod;	
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &GetPotentiometer;
	++task_iter;
	tasks[task_iter].state = Buzzer_Wait; 
	tasks[task_iter].period = BuzzerPeriod; 
	tasks[task_iter].elapsedTime = 0; 
	tasks[task_iter].TickFct = &PlayBuzzer; 
	++task_iter; 
	tasks[task_iter].state = LED_Wait; 
	tasks[task_iter].period = 100; 
	tasks[task_iter].elapsedTime = 0; 
	tasks[task_iter].TickFct = &LED_Display;
	++task_iter;
	tasks[task_iter].state = Welcome_Wait;
	tasks[task_iter].period = WelcomePeriod;
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &Display_Welcome;
	++task_iter; 
	tasks[task_iter].state = MainMenu_Wait;
	tasks[task_iter].period = MainMenuPeriod; 
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &Display_MainMenu;
	++task_iter; 
	tasks[task_iter].state = CreateMenu_Wait;
	tasks[task_iter].period = CreateMenuPeriod; 
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &Display_CreateMenu;
	++task_iter;
	tasks[task_iter].state = ModifyMenu_Wait;
	tasks[task_iter].period = ModifyMenuPeriod; 
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &Display_ModifyMenu;
	++task_iter;
	tasks[task_iter].state = PlayMenu_Wait;
	tasks[task_iter].period = PlayMenuPeriod; 
	tasks[task_iter].elapsedTime = 0;
	tasks[task_iter].TickFct = &Display_PlayMenu;

	//Sets up the global period
	//for all the functions 
	TimerSet(MasterPeriod); //The task period
    TimerOn();
	//Initializes the ACD for use
	ADC_init();
	//Initializes the LCD for use
	LCD_init();
	LCD_InitCustomChar(); 

	while(1) {
		for ( task_iter = 0; task_iter < tasksNum; ++task_iter ) {
			if ( tasks[task_iter].elapsedTime >= tasks[task_iter].period ) {
				tasks[task_iter].state = tasks[task_iter].TickFct(tasks[task_iter].state);
				tasks[task_iter].elapsedTime = 0;
            }
            tasks[task_iter].elapsedTime += MasterPeriod;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return 0;
}
