//Includes
#include "encoding.h"
#include "platform.h"
#include "wrap.h"
#include "startup.h"
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

//Defines
#define REG(P) (*(volatile uint32_t *) (P))

#define GPIO_BASE 0x10012000
#define GPIO_INPUT_VAL 0x00
#define GPIO_INPUT_EN 0x4
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


int level = 1;
int n = 3;
int t = T_LONG;

int state = 0;

const int leds[] = {GREEN_LED, BLUE_LED, YELLOW_LED, RED_LED};
const int buttons[] = {GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON, RED_BUTTON};


int main(void){
    //Set up LEDs
    setupLEDs(leds, 4);
    //Set up Buttons
    setupButtons(buttons,4);
    //Main Loop/ State Machine
    while(1){
        switch(state):
            case 0:
            bereitschaftsPhase();
            break;

            case 1:
            //vorfuehrPhase and nachahmPhase as one state, so the main colour-sequence can be given as a parameter
            vorfuehrPhase();
            break;

            case 2:
            zwischenSequenz();
            break;

            case 3:
            verlorenSequenz();
            break;

            case 4:
            endModus();
            break;

            default:
            //This shouldnt happen
            printf("Something bad happened!");
    }
}

//Setting up LEDs as Output
void setupLEDs(int[] pLeds, int size){
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
void setupButtons(int[] pButtons, int size){
    for(int i = 0; i < size; i++){
        //Disable IOF
        REG(GPIO_BASE + GPIO_IOF_EN) &= ~(1 << pButtons[i]);
        //Enable Pull-Up
	    REG(GPIO_BASE + GPIO_PUE) |= (1 << pButtons[i]);
        //Enable Input
	    REG(GPIO_BASE + GPIO_INPUT_EN) |= (1 << pButtons[i]);
        //Disable Output
	    REG(GPIO_BASE + GPIO_OUTPUT_EN) &= ~(1 << pButtons[i]);
        //TODO:Brauch ich das? (Set Initial Output to 0)
	    REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1 << pButtons[i]);
    }
}

//Lights given amount of LEDs for the given time
void lightLEDs(int[] leds, int size, int duration){
    //LEDs on
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1 << leds[i]);
    }
    //Wait
    sleep(duration);
    //LEDs off
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1 << leds[i]);
    }
}

void bereitschaftsPhase(){
    bool pressed = false
    int led = 0;
    while(!pressed){ 
        lightLEDs({leds[led]}, 1, T_SHORT);
        //TODO: button pollen und softwareseitig "entprellen"
        led = (led >= 3 ? 0 : led + 1);
    }
    //->Vorführphase
    state = 1;
}

void vorfuehrPhase(){
    //Pre-Sequence
    lightLEDs({BLUE_LED, YELLOW_LED}, 2, T_SHORT);
    sleep(T_SHORT);
    //Generating main sequence
    int[n] sequence = {};
    for(int i = 0; i < n; i++){
        sequence[i] = rand() % 4; //Random int between 0 and 3
    }
    //Showing sequence for t each with T_SHORT pause in between
    for(int i = 0; i < n; i++){
        lightLEDs({leds[sequence[i]]}, 1, t);
        sleep(T_SHORT);
    }
    //Post-Sequence
    lightLEDs(leds, 4, T_SHORT);
    //-> Nachahmphase
    nachahmPhase(sequence);
}

void nachahmPhase(int[] sequence){


    //pollen -> Array
    //Array iterieren, firstPressed und Counter

    //-> Zwischensequenz
    state = 2;

    //-> Verlorensequenz
    state = 3;
}

void verlorenSequenz(){
    //Lost-Sequence/End-Flash
    for(int i = 0; i < 2; i++){
        lightLEDs({RED_LED, GREEN_LED}, 2, T_SHORT);
    }
    
    //TODO:Letztes Level als Binäre Zahl, LSB rechts(rot)

    //->Bereitschaftsmodus
    state = 0;
}

void zwischenSequenz(){
    //Mid-Sequence
    for(int i = 0; i < 2; i++){
        lightLEDs({GREEN_LED, YELLOW_LED}, 2, T_SHORT);
        lightLEDs({BLUE_LED, RED_LED}, 2, T_SHORT);
    }

    if(level > 10){
        //-> Endmodus
        state = 4;
    }
    else{
        level++;
        n++;
        if(level % 2 == 1){
            t = t * 0.9;
        }
        //-> Vorfuehrphase
        state = 1;
    }
}

void endModus(){
    //End-Sequence
    lightLEDs(leds, 4, T_SHORT);
    sleep(T_SHORT);
    lightLEDs(leds, 4, T_LONG);
    sleep(T_SHORT);
    lightLEDs(leds, 4, T_SHORT);
    sleep(T_SHORT);
    lightLEDs(leds, 4, T_LONG);

    //->Bereitschaftsmodus
    state = 0;
}