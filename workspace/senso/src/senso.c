//Includes
#include "encoding.h"
#include "platform.h"
#include "wrap.h"
#include "startup.h"
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

#define T_SHORT 1000
#define T_LONG 2000
#define T_VERY_LONG 4000

#define SYS_SPEED 5//~60 for the real hardware, 5-10 on simulator, choose higher the faster the processor


static uint32_t level = 1;
static int n = 3;
static int t = T_LONG;
static int state = 0;

static const int leds[] = {GREEN_LED, BLUE_LED, YELLOW_LED, RED_LED};
static const int buttons[] = {GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON, RED_BUTTON};

void setupLEDs(const int pLeds[], int size);
void setupButtons(const int pButtons[], int size);
void lightLEDs(const int leds[], int size, int duration);
int pollButtons(const int pButtons[], int size);
void delay(int mod);

void bereitschaftsPhase(void);
void vorfuehrPhase(void);
void nachahmPhase(int sequence[]);
void verlorenSequenz(void);
void zwischenSequenz(void);
void endModus(void);

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
            break;
        }
    }
}

//Setting up LEDs as Output
void setupLEDs(const int pLeds[], int size){
    for(int i = 0; i < size; i++){
        //Disable IOF
        REG(GPIO_BASE + GPIO_IOF_EN) &= ~(1u << ((uint32_t)pLeds[i]));
        //Disable Input
	    REG(GPIO_BASE + GPIO_INPUT_EN) &= ~(1u << ((uint32_t)pLeds[i]));
        //Enable Output
	    REG(GPIO_BASE + GPIO_OUTPUT_EN) |= (1u << ((uint32_t)pLeds[i]));
        //Set Initial Output 0
	    REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1u << ((uint32_t)pLeds[i]));
    }
}

//Setting up Buttons as Input
void setupButtons(const int pButtons[], int size){
    for(int i = 0; i < size; i++){
        //Disable IOF
        REG(GPIO_BASE + GPIO_IOF_EN) &= ~(1u << ((uint32_t)pButtons[i]));
        //Enable Pull-Up
	    REG(GPIO_BASE + GPIO_PUE) |= (1u << ((uint32_t)pButtons[i]));
        //Enable Input
	    REG(GPIO_BASE + GPIO_INPUT_EN) |= (1u << ((uint32_t)pButtons[i]));
        //Disable Output
	    REG(GPIO_BASE + GPIO_OUTPUT_EN) &= ~(1u << ((uint32_t)pButtons[i]));
        //Set Initial Output to 1
	    REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1u << ((uint32_t)pButtons[i]));
    }
}

//Lights given amount of LEDs for the given time
void lightLEDs(const int leds[], int size, int duration){
    //LEDs on
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) |= (1u << ((uint32_t)leds[i]));
    }
    //Wait
    delay(duration);
    //LEDs off
    for(int i = 0; i < size; i++){
        REG(GPIO_BASE + GPIO_OUTPUT_VAL) &= ~(1u << ((uint32_t)leds[i]));
    }
}

//Multiple Buttons pressed count as -2, No Button as -1
int pollButtons(const int pButtons[], int size){
	int buttonsPressed = 0;
	int firstButton = -1;
	for(int i = 0; i < size; i++){
		if ((REG(GPIO_BASE + GPIO_INPUT_VAL) & (1u << ((uint32_t)pButtons[i]))) == 0u){
			firstButton = i;
			buttonsPressed += 1;
		}
	}
	return ((buttonsPressed > 1) ? -2 : firstButton);
}

void delay(int mod)
{
	for (int i = 0; i < (((int)SYS_SPEED)*15*mod); i++){}
}

void bereitschaftsPhase(void){
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
        led = ((led >= 3) ? 0 : (led + 1));
    }
    //->Vorführphase
    state = 1;
}

void vorfuehrPhase(void){
    //Pre-Sequence
    const int ledsToLight[2] = {BLUE_LED, YELLOW_LED};
    lightLEDs(ledsToLight, 2, T_SHORT);
    delay(T_SHORT);
    //Generating main sequence
    //Init with size 12(Max n) as n doesnt work for some reason
    int sequence[12] = {0};
    for(int i = 0; i < n; i++){
        sequence[i] = rand() % 4; //Random int between 0 and 3
    }
    //Showing sequence for t each with T_SHORT pause in between
    for(int i = 0; i < n; i++){
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
                break;
            }
        }
        if(j != sequence[h]){
            lost = true;
            break;
        }
        else{
            int ledToLight[1] = {leds[sequence[h]]};
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

void verlorenSequenz(void){
    //Lost-Sequence/End-Flash
    for(int i = 0; i < 2; i++){
        const int ledsToLight[2] = {RED_LED, GREEN_LED};
        lightLEDs(ledsToLight, 2, T_SHORT);
    }
    
    level -= 1u;
    bool bin[4] = {false, false, false, false};
    for(int i = 0; i < 4; i++){
        if((level & (1u << ((uint32_t)i))) > 0u){
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

    lightLEDs(ledsToLight, count, T_VERY_LONG);

    //->Bereitschaftsmodus
    state = 0;
}

void zwischenSequenz(void){
    //Mid-Sequence
    for(int i = 0; i < 2; i++){
        const int ledsToLight[2] = {GREEN_LED, YELLOW_LED};
        lightLEDs(ledsToLight, 2, T_SHORT);
        const int ledsToLight2[2] = {BLUE_LED, RED_LED};
        lightLEDs(ledsToLight2, 2, T_SHORT);
    }

    if(level > 10u){
        //-> Endmodus
        state = 4;
    }
    else{
        level++;
        n++;
        if((level % 2u) == 1u){
            t = t * 0.9;
        }
        //-> Vorfuehrphase
        state = 1;
    }
}

void endModus(void){
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
