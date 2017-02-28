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
unsigned short ushort_Potentiometer = 0;

enum Keypad_States {GetInput};
int GetKeypad (int state) {
	unsigned char  Key = GetKeypadKey();
	switch(state) {
		case GetOutput:
			switch (Key) {
				case '\0': uchar_keypadInput = 0x1F; break;
				case '0': uchar_keypadInput = 0x00; break;
				case '1': uchar_keypadInput = 0x01; break;
				case '2': uchar_keypadInput = 0x02; break;
				case '3':  uchar_keypadInput = 0x03; break;
				case '4': uchar_keypadInput = 0x04; break;
				case '5': uchar_keypadInput = 0x05; break;
				case '6': uchar_keypadInput = 0x06; break;
				case '7': uchar_keypadInput = 0x07;  break;
				case '8': uchar_keypadInput = 0x08; break;
				case '9': uchar_keypadInput = 0x09; break;
				case 'A': uchar_keypadInput = 0x0A; break;
				case 'B': uchar_keypadInput = 0x0B; break;
				case 'C': uchar_keypadInput = 0x0C; break;
				case 'D': uchar_keypadInput = 0x0D; break;
				case '*': uchar_keypadInput = 0x0E; break;
				case '#': uchar_keypadInput = 0x0F; break;
				default: uchar_keypadInput = 0x1B; break;
			}
			state = GetInput; break;
		default: state = GetInput; break;
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

int main () {
      unsigned short iter = 0;
      task[iter].state = GetOutput;
	task[iter].period = 1;
	task[iter].elapsedTime = 0;
	task[iter].TickFct = &GetKey;

      unsigned short taskPeriod = 1;

      TimerSet(taskPeriod);
      TimerOn();

	ADC_init();

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
