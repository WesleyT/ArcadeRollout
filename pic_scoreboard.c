// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF      // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable Bit (4x PLL Enabled)
#pragma config STVREN = ON     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = ON        // Low-Voltage Programming Enable (Low-voltage programming enabled)

#include <xc.h>
#include <string.h>
#include <stdint.h>
#include "i2c_arduino.h"       // Custom header for I2C communication with Arduino

#define _XTAL_FREQ 32000000     // Define clock frequency of 32MHz for delay calculations
#define STATE_NEW_GAME 0        // Define state for a new game initialization
#define STATE_ACTIVE_GAME 1     // Define state for active gameplay
#define STATE_END_GAME 2        // Define state for game over
#define STATE_WIN_GAME 3        // Define state when the player wins the game
#define STATE_REVERSE_GAME 4    // Define state for reverse play
#define TMR0_PRELOAD (256 - 250) // Preload for 1ms overflow in Timer0
#define SCORE_LOCKOUT_TIME 1000   // Lockout period in ms after scoring to prevent score spam
#define DEBOUNCE_CHECKS 5       // Number of checks during debouncing of switch
#define DEBOUNCE_REQUIRED 3    // Number of consistent reads required for a debounced state
#define EVENT_RB7 0x01  // Event flag for RB7 interrupt
#define EVENT_RA0 0x02  // Event flag for RA0 interrupt
#define EVENT_RA1 0x04  // Event flag for RA1 interrupt
#define EVENT_RA4 0x08  // Event flag for RA4 interrupt


volatile uint8_t eventFlag = 0;  // Global variable to store event flags

volatile int16_t score = 0; // Current game score
volatile int8_t balls = 10; // Balls remaining in the game
volatile int currentState = STATE_NEW_GAME; // Current state of the game
volatile unsigned long millisCounter = 0; // Millisecond counter for timing
volatile unsigned long lastScoreTime[4] = {0, 0, 0, 0}; // Time of last score per switch
volatile uint8_t lastSwitchState[4] = {0, 0, 0, 0}; // Last debounced state of each switch

// Function to safely read the millisecond counter
unsigned long millis(void) {
    unsigned long m;
    INTCONbits.GIE = 0;            // Disable global interrupts
    m = millisCounter;
    INTCONbits.GIE = 1;            // Re-enable global interrupts
    return m;
}

// Setup Timer0 for generating millisecond timebase
void setupTimer0() {
    OPTION_REGbits.T0CS = 0;   // Use the internal instruction cycle clock (FOSC/4)
    OPTION_REGbits.PSA = 0;    // Enable the prescaler
    OPTION_REGbits.PS = 0b100; // Set the prescaler to 1:32

    TMR0 = TMR0_PRELOAD;       // Preload for 1ms overflow
    INTCONbits.TMR0IE = 1;     // Enable Timer0 interrupt
    INTCONbits.GIE = 1;        // Enable global interrupt
    INTCONbits.PEIE = 1;       // Enable peripheral interrupt

    INTCONbits.TMR0IF = 0;     // Clear the interrupt flag
}


void initialSetup(){
    // Setup oscillator and clock at 32MHz with PLL on
    OSCCON = 0b11110000;

    // Setup global interrupts
    INTCON = 0b11011000;
    
    // Setup Timer0 for millis function
    setupTimer0();
    
    // Initialize I2C communication
    i2c_Init();
    i2c_clearScreen();  // Clear any previous data on the screen
    
    // Enable internal weak pull-ups
    OPTION_REGbits.nWPUEN = 0;

    // Configure RA5 as an output for debugging 
    TRISAbits.TRISA5 = 0;  // Set RA5 as an output pin
    LATAbits.LATA5 = 1;    // Turn off LED initially
    
    // Configure RC7 as input for the start button
    TRISCbits.TRISC7 = 1;
    WPUCbits.WPUC7 = 1;
    ANSELCbits.ANSC7 = 0;
   
}

// Setup  4 micro switches for interrupts
void setupSwitches(){
    // Configure RB7, RA0, RA1, and RA4 as inputs with pull-ups and interrupt-on-change
    // RB7 Switch
    TRISBbits.TRISB7 = 1;
    WPUBbits.WPUB7 = 1;
    IOCBPbits.IOCBP7 = 1;
    IOCBNbits.IOCBN7 = 0;
    IOCBFbits.IOCBF7 = 0;
    
    // RA0 Switch
    TRISAbits.TRISA0 = 1;
    ANSELAbits.ANSA0 = 0;
    INLVLAbits.INLVLA0 = 1;
    WPUAbits.WPUA0 = 1;
    IOCAPbits.IOCAP0 = 1;
    IOCANbits.IOCAN0 = 0;
    IOCAFbits.IOCAF0 = 0;
    
    // RA1 Switch
    TRISAbits.TRISA1 = 1;
    ANSELAbits.ANSA1 = 0;
    WPUAbits.WPUA1 = 1;
    IOCAPbits.IOCAP1 = 1;
    IOCANbits.IOCAN1 = 0;
    IOCAFbits.IOCAF1 = 0;
    
    // RA4 Switch
    TRISAbits.TRISA4 = 1;
    WPUAbits.WPUA4 = 1;
    ANSELAbits.ANSA4 = 0;
    IOCAPbits.IOCAP4 = 1;
    IOCANbits.IOCAN4 = 0;
    IOCAFbits.IOCAF4 = 0;
}

void clearEvents(){
   eventFlag &= ~EVENT_RB7;
   eventFlag &= ~EVENT_RA0;
   eventFlag &= ~EVENT_RA1;
   eventFlag &= ~EVENT_RA4;
}
// Improved Debounce Logic
int debounceSwitch(int pin, int switchIndex) {
    int readValue = (pin == 1) ? 1 : 0;  // Direct pin read replaced by a hypothetical read function if needed
    int stableCount = 0;
    int lastState = lastSwitchState[switchIndex];

    for (int i = 0; i < DEBOUNCE_CHECKS; i++) {
        __delay_ms(1);  // Delay for debounce check
        int currentState = (pin == 1) ? 1 : 0;
        
        if (currentState == lastState) {
            stableCount++;
            if (stableCount >= DEBOUNCE_REQUIRED) {
                lastSwitchState[switchIndex] = currentState;
                return currentState;
            }
        } else {
            stableCount = 0; // Reset count if state changes
            lastState = currentState;  // Update last state for next comparison
        }
    }
    lastSwitchState[switchIndex] = lastState;  // Persist the last checked state
    return lastState; // Return the last stable state
}

// function for when a ball scores. takes integer for point value and adds it
void ballIn(int points){
    if (balls > 0){
        score += points;
        balls -= 1;
    }
    
}

// reverse for when a ball scores. takes integer for point value and removes it
void ballInReverse(int points){
    if (balls < 10){
        score -= points;
        if (score < 0){
            score = 0;
        }
        balls += 1;
    }
}

// returns current state of the start button
int startButtonPressed() {
    
    return !PORTCbits.RC7; // Check RB7 as the start button
}

// Check if sufficient time has passed to score again
// Call this within the interrupt service routine for each switch
int canScoreAgain(int switchIndex) {
    unsigned long currentTime = millis(); 
    if (currentTime - lastScoreTime[switchIndex] > SCORE_LOCKOUT_TIME) {
        lastScoreTime[switchIndex] = currentTime;
        return 1;
    }
    return 0;
}
void handleSwitches(){
    if (eventFlag & EVENT_RB7) {  // Check if RB7 event occurred
            if (debounceSwitch(PORTBbits.RB7, 0)) {  // Perform debouncing
                if (canScoreAgain(0)) {  // Check scoring conditions
                    if (currentState == STATE_ACTIVE_GAME){
                        ballIn(5);
                    } else if (currentState == STATE_REVERSE_GAME) {
                        ballInReverse(5);
                    } else if (currentState == STATE_END_GAME || currentState == STATE_WIN_GAME) {
                        currentState = STATE_REVERSE_GAME;
                        ballInReverse(5);
                    }
                }
            }
            eventFlag &= ~EVENT_RB7;
    }
    //RA0
    if (eventFlag & EVENT_RA0) {  // Check if RA0 event occurred
            if (debounceSwitch(PORTAbits.RA0, 1)) {  // Perform debouncing
                if (canScoreAgain(1)) {  // Check scoring conditions
                    if (currentState == STATE_ACTIVE_GAME){
                        ballIn(10);
                    } else if (currentState == STATE_REVERSE_GAME) {
                        ballInReverse(10);
                    } else if (currentState == STATE_END_GAME || currentState == STATE_WIN_GAME) {
                        currentState = STATE_REVERSE_GAME;
                        ballInReverse(10);
                    }
                }
            }
            eventFlag &= ~EVENT_RA0;
    }
    //RA1
    if (eventFlag & EVENT_RA1) {  // Check if RA1 event occurred
            if (debounceSwitch(PORTAbits.RA1, 2)) {  // Perform debouncing
                if (canScoreAgain(2)) {  // Check scoring conditions
                    if (currentState == STATE_ACTIVE_GAME){
                        ballIn(50);
                    } else if (currentState == STATE_REVERSE_GAME) {
                        ballInReverse(50);
                    } else if (currentState == STATE_END_GAME || currentState == STATE_WIN_GAME) {
                        currentState = STATE_REVERSE_GAME;
                        ballInReverse(50);
                    }
                }
            }
            eventFlag &= ~EVENT_RA1;
    }
    //RA4
    if (eventFlag & EVENT_RA4) {  // Check if RA4 event occurred
            if (debounceSwitch(PORTAbits.RA4, 3)) {  // Perform debouncing
                if (canScoreAgain(3)) {  // Check scoring conditions
                    if (currentState == STATE_ACTIVE_GAME){
                        ballIn(100);
                    } else if (currentState == STATE_REVERSE_GAME) {
                        ballInReverse(100);
                    } else if (currentState == STATE_END_GAME || currentState == STATE_WIN_GAME) {
                        currentState = STATE_REVERSE_GAME;
                        ballInReverse(100);
                    }
                }
            }
            eventFlag &= ~EVENT_RA4;
    }
}

// Handle winning the game
void winGame() {
    unsigned long lastUpdate = 0;
    unsigned long updateInterval = 1000; // Update every 1000ms
    int displayState = 0; // State variable to control which message is displayed

    // Handle end game display and wait for user to acknowledge
    // Transition back to new game after some condition or timeout
    while (!startButtonPressed()) {
        unsigned long currentMillis = millis();
        handleSwitches();
        if (currentState != STATE_END_GAME) {
            break; // Exit the loop if the state has changed
        }
        // Check if it's time to update the display
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis; // Reset the last update time
            
            // Toggle display based on the current state
            if (displayState == 0) {
                //display MAX and the score
                i2c_clearScreen();
                __delay_ms(50);
                i2c_WriteToArduino(0x01,2, "MAX");
                __delay_ms(50);
                i2c_WriteScoreToArduino(score);
                displayState = 1; // Change state to display "GAME OVER" next
            } else {
                // Display "GAME OVER"
                i2c_clearScreen();
                __delay_ms(50);
                i2c_WriteToArduino(0x01,2, "GREAT");
                __delay_ms(50);
                i2c_WriteToArduino(0x02,4, "_____JOB");
                displayState = 0; // Reset to display score next
            }
        }

        // Check if start button is pressed to exit the loop and reset the game
        if (startButtonPressed()) {
            currentState = STATE_NEW_GAME;
            break; // Break the loop to ensure the game state is changed immediately
        }
    }
}

// Handle game over
void endGame() {
    unsigned long lastUpdate = 0;
    unsigned long updateInterval = 1000; // Update every 1000ms
    int displayState = 0; // State variable to control which message is displayed

    // Handle end game display and wait for user to acknowledge
    // Transition back to new game after some condition or timeout
    while (!startButtonPressed()) {
        unsigned long currentMillis = millis();
        handleSwitches();
        if (currentState != STATE_END_GAME) {
            break; // Exit the loop if the state has changed
        }
        // Check if it's time to update the display
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis; // Reset the last update time
            
            // Toggle display based on the current state
            if (displayState == 0) {
                // Display the score
                i2c_clearScreen();
                __delay_ms(50); // Short delay to ensure the screen is cleared
                i2c_WriteToArduino(0x01, 1, "SCORE");
                __delay_ms(50);
                i2c_WriteScoreToArduino(score);
                displayState = 1; // Change state to display "GAME OVER" next
            } else {
                // Display "GAME OVER"
                i2c_clearScreen();
                __delay_ms(50); // Short delay to ensure the screen is cleared
                i2c_WriteToArduino(0x01, 3, "GAME");
                __delay_ms(50);
                i2c_WriteToArduino(0x02, 6, "____OVER");
                displayState = 0; // Reset to display score next
            }
        }

        // Check if start button is pressed to exit the loop and reset the game
        if (startButtonPressed()) {
            currentState = STATE_NEW_GAME;
            break; // Break the loop to ensure the game state is changed immediately
        }
    }
}


void newGame() {
    //print "new game?"
    i2c_clearScreen();
    __delay_ms(50);
    i2c_WriteToArduino(0x01,1, "NEW");
    __delay_ms(50);
    i2c_WriteToArduino(0x02,2, "___GAME?");
    __delay_ms(50);
    // Wait for start button press to initialize a new game
    while (!startButtonPressed()) {
        
    }
    // once button is pressed initialize game variables
    balls = 10;
    score = 0;
    clearEvents();
    //begin game
    if (startButtonPressed()) {
        currentState = STATE_ACTIVE_GAME;
    } 
}

void activeGame() {
    unsigned long lastUpdate = 0;
    unsigned long updateInterval = 200; // Update every 200ms
    
    //clear screen before starting
    i2c_clearScreen();
    __delay_ms(50);
    
    //game stays active as long as at least 1 ball remains
    while (balls > 0) {
        unsigned long currentMillis = millis();
        handleSwitches();
        // Only update the display if at least 200ms have passed
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis; // Update the last update time
            i2c_WriteToArduino(0x01,1, "SCORE");
            __delay_ms(50);
            i2c_WriteScoreToArduino(score);
            __delay_ms(50);
            i2c_SendBallCount(balls);
            __delay_ms(50);
        }
    }
    
    // After exiting the loop, determine game state based on score
    if (score >= 1000) {
        currentState = STATE_WIN_GAME;
    } else {
        currentState = STATE_END_GAME;
    }
    //clear events before changing states
    clearEvents();
}

void reverseGame() {
    unsigned long lastUpdate = 0;
    unsigned long updateInterval = 200; // Update every 200ms
    
    //clear screen before starting
    i2c_clearScreen();
    __delay_ms(50);
    
    //game stays active as long as at least 1 ball remains
    while (balls < 10 && score > 0) {
        unsigned long currentMillis = millis();
        handleSwitches();
        // Only update the display if at least 200ms have passed
        if (currentMillis - lastUpdate >= updateInterval) {
            lastUpdate = currentMillis; // Update the last update time
            i2c_WriteToArduino(0x01,2, "SCORE");
            __delay_ms(50);
            i2c_WriteScoreToArduino(score);
            __delay_ms(50);
            i2c_SendBallCount(balls);
            __delay_ms(50);
        }
    }
    if (score == 0) {
        i2c_WriteScoreToArduino(score);
        __delay_ms(50);
        i2c_WriteToArduino(0x01,1, "SUCCESS");
        __delay_ms(50);
        i2c_SendBallCount(balls);
        __delay_ms(50);
    } else {
        i2c_WriteToArduino(0x01,3, "FAIL ");
        __delay_ms(50);
        i2c_WriteScoreToArduino(score);
        __delay_ms(50);
        i2c_SendBallCount(balls);
        __delay_ms(50);
    }
    while (!startButtonPressed()) {
        
    }
    //begin game
    if (startButtonPressed()) {
        currentState = STATE_ACTIVE_GAME;
    } 
    // once button is pressed initialize game variables
    balls = 10;
    score = 0;
    clearEvents();
  
}

// these interrupts for the micro switches are simple functions that call ballIn to adjust the score
void __interrupt() isr() {
    //interrupt for RB7
    if (IOCBFbits.IOCBF7) {
        IOCBFbits.IOCBF7 = 0;
        eventFlag |= EVENT_RB7;
    }
    //interrupt for RA0
    if (IOCAFbits.IOCAF0) {
        IOCAFbits.IOCAF0 = 0;
        eventFlag |= EVENT_RA0;
    }
    //interrupt for RA1
    if (IOCAFbits.IOCAF1) {
        IOCAFbits.IOCAF1 = 0;
        eventFlag |= EVENT_RA1;
    }
    //interrupt for RA4
    if (IOCAFbits.IOCAF4) {
        IOCAFbits.IOCAF4 = 0;
        eventFlag |= EVENT_RA4;
    }
    //timer 0 interrupt for millis      
    if (INTCONbits.TMR0IF) {   // Check if Timer0 interrupt
        TMR0 = TMR0_PRELOAD;   // Reload the Timer0 preload value
        millisCounter++;       // Increment the milliseconds counter
        INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag
    }
}

// Main loop
void main(void) {
    initialSetup();
    setupSwitches();

    // Active loop with state switch
    while (1) {
        switch (currentState) {
            case STATE_NEW_GAME:
                newGame();
                break;
            case STATE_ACTIVE_GAME:
                activeGame();
                break;
            case STATE_END_GAME:
                endGame();
                break;
            case STATE_WIN_GAME:
                winGame();
                break;
            case STATE_REVERSE_GAME:
                reverseGame();
                break;
        } 
    }
}
