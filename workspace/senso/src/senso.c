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
#define GPIO_PUE 0x10

#define GREEN_LED 18 //Pin 2 = GPIO 18
#define BLUE_LED 21  //Pin 5 = GPIO 21
#define YELLOW_LED 0 //Pin 8 = GPIO 0
#define RED_LED 3   //Pin 11 = GPIO 3

#define GREEN_BUTTON 19 //Pin 3 = GPIO 19
#define BLUE_BUTTON 20  //Pin 4 = GPIO 20
#define YELLOW_BUTTON 1 //Pin 9 = GPIO 1
#define RED_BUTTON 2  //Pin 10 = GPIO 2

#define T_SHORT 1600
#define T_LONG 3200
#define T_VERY_LONG 6400


int level = 1;
int n = 3;
int t = T_LONG;

int state = 0;

const int leds[] = {GREEN_LED, BLUE_LED, YELLOW_LED, RED_LED};
const int buttons[] = {GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON, RED_BUTTON};

//Function declarations
//Duplicate with empty.c
void setupLEDs(const int pLeds[], int size);
void setupButtons(const int pButtons[], int size);
void lightLEDs(const int leds[], int size, int duration);
int pollButtons(const int pButtons[], int size);
void delay(int mod);


void bereitschaftsPhase();
void vorfuehrPhase();
void nachahmPhase(int sequence[]);
void verlorenSequenz();
void zwischenSequenz();
void endModus();

int main(void){
    //Set up LEDs
    setupLEDs(leds, 4);
    //Set up Buttons
    setupButtons(buttons,4);
    //Main Loop/ State Machine
    while(1){
        switch(state){
            case 0:
            bereitschaftsPhase();
            break;

            case 1:
            //vorfuehrPhase and nachahmPhase as one state, so the main colour-sequence can be given as a parameter and not use a global variable
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

//Lights given amount of LEDs for the given time
void lightLEDs(const int leds[], int size, int duration){
    //LEDs on
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1 << leds[i]);
    }
    //Wait
    delay(duration);
    //LEDs off
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1 << leds[i]);
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

//Mit mod 1000 in etwa 0.8sec auf Simulator -> T_VERY_LONG etwa 3 sec
void delay(int mod)
{
    volatile uint32_t i = 0;
	for (i = 0; i < (150*mod); i++){}
}

void bereitschaftsPhase(){
    //TODO: Remove Debug print
    printf("Bereitschaft");
    level = 1;
    n = 3;
    t = T_LONG;
    bool pressed = false;
    int led = 0;
    while(!pressed){//High Poll Delay of T_SHORT(ideally ~0.8 sec), so better hold the Button a bit
        const int ledToLight[1] = {leds[led]};
        lightLEDs(ledToLight, 1, T_SHORT);
        int buttonToPoll[1] = {GREEN_BUTTON};
        int j = pollButtons(buttonToPoll, 1);
        if(j > -1){
            pressed = true;
        }
        led = (led >= 3 ? 0 : led + 1);
    }
    //->Vorführphase
    state = 1;
}

void vorfuehrPhase(){
    //TODO: Remove Debug print
    printf("Vorfuehr");
    //Pre-Sequence
    const int ledsToLight[2] = {BLUE_LED, YELLOW_LED};
    lightLEDs(ledsToLight, 2, T_SHORT);
    delay(T_SHORT);
    //Generating main sequence
    //Init with size 12(Max n) as n doesnt work for some reason
    int sequence[12] = {};
    for(int i = 0; i < n; i++){
        sequence[i] = rand() % 4; //Random int between 0 and 3
    }
    //Showing sequence for t each with T_SHORT pause in between
    for(int i = 0; i < n; i++){
        //TODO: Remove Debug print
        printf("Sequenz %d: LED %d", i, sequence[i]);
        const int ledToLight[1] = {leds[sequence[i]]};
        lightLEDs(ledToLight, 1, t);
        delay(T_SHORT);
    }
    //Post-Sequence
    lightLEDs(leds, 4, T_SHORT);
    //-> Nachahmphase
    nachahmPhase(sequence);
}

void nachahmPhase(int sequence[]){
    //TODO: Remove Debug print
    printf("Nachahm");

    int pollRate = 200; //Magic Number :D
    int iterations = t/pollRate; //Every iteration takes at least <pollrate>units due to poll delay -> all iterations should roughly take t
    int j = -1;
    bool lost = false;
    for(int h = 0; h < n; h++){//Sequence has size n
        j = -1;
        for(int i = 0; i < iterations; i++){
            delay(200);
            j = pollButtons(buttons, 4);
            if(j != -1){
                i = iterations;
                printf("Polled %d", j);
            }
        }
        if(j != sequence[h]){
            lost = true;
            h = n;
        }
        else{
            int ledToLight[1] = {leds[sequence[h]]};
            printf("COrrect, lighting %d", sequence[h]);
            lightLEDs(ledToLight, 1, T_SHORT);
        }
    }
    if(lost){
        //-> Verlorensequenz
        state = 3;
    }
    else{
        //-> Zwischensequenz
        state = 2;
    }
}

void verlorenSequenz(){
    //TODO: Remove Debug print
    printf("Verloren");
    //Lost-Sequence/End-Flash
    for(int i = 0; i < 2; i++){
        const int ledsToLight[2] = {RED_LED, GREEN_LED};
        lightLEDs(ledsToLight, 2, T_SHORT);
    }
    
    //TODO:Letztes Level als Binäre Zahl, LSB rechts(rot)
    lightLEDs(leds, 4, T_VERY_LONG);
    printf("Letztes geschafftes Level: %d", level-1);
    //->Bereitschaftsmodus
    state = 0;
}

void zwischenSequenz(){
    //TODO: Remove Debug print
    printf("Zwischen");
    //Mid-Sequence
    for(int i = 0; i < 2; i++){
        const int ledsToLight[2] = {GREEN_LED, YELLOW_LED};
        lightLEDs(ledsToLight, 2, T_SHORT);
        const int ledsToLight2[2] = {BLUE_LED, RED_LED};
        lightLEDs(ledsToLight2, 2, T_SHORT);
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
        printf("Next Level:%d, mit n=%d und t=%d\n", level, n, t);
        //-> Vorfuehrphase
        state = 1;
    }
}

void endModus(){
    //TODO: Remove Debug print
    printf("Ende");
    //End-Sequence
    lightLEDs(leds, 4, T_SHORT);
    delay(T_SHORT);
    lightLEDs(leds, 4, T_LONG);
    delay(T_SHORT);
    lightLEDs(leds, 4, T_SHORT);
    delay(T_SHORT);
    lightLEDs(leds, 4, T_LONG);

    //->Bereitschaftsmodus
    state = 0;
}
