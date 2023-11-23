#include "wrap.h"
#include "startup.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define REG(P) (*(volatile uint32_t *) (P))

#define GPIO_BASE 0x10012000
#define GPIO_INPUT_EN 0x4
#define GPIO_INPUT_VAL 0x0
#define GPIO_PUE 0x10
#define GPIO_OUTPUT_EN 0x8
#define GPIO_OUTPUT_VAL 0xc
#define GPIO_IOF_EN 0x38

#define GREEN_LED 18 //Pin 2 = GPIO 18
#define BLUE_LED 21  //Pin 5 = GPIO 21
#define YELLOW_LED 0 //Pin 8 = GPIO 0
#define RED_LED 3   //Pin 11 = GPIO 3

#define GREEN_BUTTON 19 //Pin 3 = GPIO 19
#define BLUE_BUTTON 20  //Pin 4 = GPIO 20
#define YELLOW_BUTTON 1 //Pin 9 = GPIO 1
#define RED_BUTTON 2  //Pin 10 = GPIO 2

#define T_SHORT 1000
#define T_LONG 2000
#define T_VERY_LONG 4000

#define SYS_SPEED 5

const int leds[] = {GREEN_LED, BLUE_LED, YELLOW_LED, RED_LED};
const int buttons[] = {GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON, RED_BUTTON};

void setupLEDs(const int pLeds[] , int size);
void setupButtons(const int pButtons[], int size);
int pollButtons(const int pButtons[], int size);
void delay(int mod);
void lightLEDs(const int pLeds[], int size, int duration);
void testDelays();
void testLEDs();
void testButtons(int pDelay);
void testLevelBin(int level);

int main (void)
{
	setupLEDs(leds, 4);
	setupButtons(buttons, 4);

	while(1)
	{
		//testDelays();
		//testLEDs();
		//testButtons(200);
		for(int i = 0; i < 16; i++){
			printf("%d", i);
			testLevelBin(i);
		}
	}
}

void testLevelBin(int level){
	//Lost-Sequence/End-Flash
    for(int i = 0; i < 2; i++){
        const int ledsToLight[2] = {RED_LED, GREEN_LED};
        lightLEDs(ledsToLight, 2, T_SHORT);
    }
    
    //TODO:Letztes Level als Binäre Zahl, LSB rechts(rot)
    bool bin[4] = {false};
    for(int i = 0; i < 4; i++){
        if((level & (1 << i)) > 0){
            bin[3-i] = true;
        }
    }
    
    int count = 0;
    int ledsToLight[4] = {0};
    for(int i = 0; i < 4; i++){
        if(bin[i]){
            ledsToLight[count] = leds[i];
            count += 1;
        }
    }

    lightLEDs(ledsToLight, count, T_LONG);
}

//Testing Buttons, toggling respective LED on each press
void testButtons(int pDelay){
	delay(pDelay);
	int j = -1;
	j = pollButtons(buttons, 4);
	if(j > -1){
		REG(GPIO_BASE + GPIO_OUTPUT_VAL) ^= (1 << leds[j]);
	}

}

//Testing LEDs and Delays
void testLEDs(){
	const int ledsToLight[2] = {BLUE_LED, RED_LED};
	delay(T_SHORT);
	lightLEDs(ledsToLight, 2, T_SHORT);
	delay(T_LONG);
	const int ledsToLight2[2] = {GREEN_LED, YELLOW_LED};
	lightLEDs(ledsToLight2, 2, T_LONG);
	delay(T_VERY_LONG);
	lightLEDs(leds, 4, T_VERY_LONG);
}

void testDelays(){
	lightLEDs(leds, 4, T_SHORT*10);
	delay(T_SHORT*10);
}

//Setting up LEDs as Output
void setupLEDs(const int pLeds[], int size){
    for(int i = 0; i < size; i++){
        //Disable IOF
        REG(GPIO_BASE + GPIO_IOF_EN) &= ~(1 << pLeds[i]);
        //Disable Input
	    REG(GPIO_BASE + GPIO_INPUT_EN) &= ~(1 << pLeds[i]);
        //Enable Output
	    REG(GPIO_BASE + GPIO_OUTPUT_EN) |= (1 << pLeds[i]);
        //Set Initial Output 0
	    REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1 << pLeds[i]);
    }
}

//Setting up Buttons as Input
void setupButtons(const int pButtons[], int size){
    for(int i = 0; i < size; i++){
        //Disable IOF
        REG(GPIO_BASE + GPIO_IOF_EN) &= ~(1 << pButtons[i]);
        //Enable Pull-Up
	    REG(GPIO_BASE + GPIO_PUE) |= (1 << pButtons[i]);
        //Enable Input
	    REG(GPIO_BASE + GPIO_INPUT_EN) |= (1 << pButtons[i]);
        //Disable Output
	    REG(GPIO_BASE + GPIO_OUTPUT_EN) &= ~(1 << pButtons[i]);
        //Set Initial Output to 1
	    REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1 << pButtons[i]);
    }
}

//Multiple Buttons pressed count as -2, No Button as -1
int pollButtons(const int pButtons[], int size){
	int buttonsPressed = 0;
	int firstButton = -1;
	for(int i = 0; i < size; i++){
		if ((REG(GPIO_BASE + GPIO_INPUT_VAL) & (1 << pButtons[i])) == 0){
			firstButton = i;
			buttonsPressed += 1;
		}
	}
	return buttonsPressed > 1 ? -2 : firstButton;
}

void delay(int mod)
{
    volatile uint32_t i = 0;
	for (i = 0; i < (15*SYS_SPEED*mod); i++){}
}

//Lights given amount of LEDs for the given time
void lightLEDs(const int pLeds[], int size, int duration){
    //LEDs on
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1 << pLeds[i]);
    }
    //Wait
    delay(duration);
    //LEDs off
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1 << pLeds[i]);
    }
}
