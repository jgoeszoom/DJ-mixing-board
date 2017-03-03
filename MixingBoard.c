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
#include "keypad.h"
#include "io.c"
#include "io.h"
#include "PWM.h"
#include "scheduler.h"
#include "timer.h"
#include "ADC.h"

const unsigned short tasksNum = 1;
task tasks[tasksNum];

unsigned char uchar_keypadInput = 0x00;
unsigned char uchar_prevkeypadInput = 0x01;

//The following are flags set by the various SMs
//This flag comes from GetKeypad
unsigned char uchar_keyPressFlag = 0;
//These flags come from MenuDisplay
unsigned char uchar_CreateMenuFlag = 0;
unsigned char uchar_ManipulateMenuFlag = 0;
unsigned char uchar_PlayMenuFlag = 0;
unsigned char uchar_SongSelectedFlag = 0;

unsigned short ushort_Potentiometer = 0x0000;

enum Keypad_States {Pressed, Unpressed};
int GetKeypad(int state) {
	unsigned char Key = GetKeypadKey();
	switch(state) {
		case unpressed:
			if (x != '\0') {
				uchar_keypadInput = key;
				uchar_keyPressFlag = 1;
				state = Pressed;
			} else {
				state = Unpressed;
			}
			break;
		case pressed:
			if (x == '\0') {
				uchar_keyPressFlag = 0;
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

enum Potentiometer_States {GetValue}
int GetPotentiometer(int state) {
	unsigned short Potentiometer = ADC;
	switch(state) {
		case GetValue:
			ushort_Potentiometer = Potentiometer;
			state = GetValue;
			break;
		default: state = GetValue; break;
	}
	return state;
}

enum Display_States {Main_Menu, Create_Menu, Manipulate_Menu, Play_Menu}
int MenuDisplay (int state) {
	case Main_Menu:
		if(uchar_keypadInput == '1') {
			uchar_CreateMenuFlag = 1;
			state = Create_Menu;
		} else if (uchar_keypadInput == '2') {
			uchar_ManipulateMenuFlag = 1;
			state = Manipulate_Menu;
		} else if (uchar_keypadInput == '3') {
			uchar_PlayMenuFlag = 1;
			state = Play_Menu;
		}
		else {
			state = Main_Menu;
		}

	case Create_Menu:
		break;

	case Manipulate_Menu:
		break;

	case Play_Menu:
		break;

	default:
		break;
	return state;
}

int main () {
      unsigned short iter = 0;
      task[iter].state = GetInput;
	task[iter].period = 100;
	task[iter].elapsedTime = 0;
	task[iter].TickFct = &GetKeypad;
	++iter;
	task[iter].state = GetValue;
	task[iter].period = 50;
	task[iter].elapsedTime = 0;
	task[iter].TickFct = &GetPotentiometer;
	++iter;
	task[iter].state = Main_Menu;
	task[iter].period = 300;
	task[iter].elapsedTime = 0;
	task[iter].TickFct = &MenuDisplay;

      unsigned short taskPeriod = 50;

      TimerSet(taskPeriod);
      TimerOn();

	ADC_init();

	LCD_init();
	LCD_ClearScreen();

      while(1) {
                  for ( iter = 0; iter < tasksNum; ++iter ) {
                        if ( tasks[iter].elapsedTime >= tasks[iter].period ) {
                             tasks[iter].state = tasks[iter].TickFct(tasks[iter].state);
                             tasks[iter].elapsedTime = 0;
                       }
                       tasks[iter].elapsedTime += taskPeriod;
              }
              while(!TimerFlag);
              TimerFlag = 0;
      }
      return 0;
}
