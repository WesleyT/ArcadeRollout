#include <string.h>
#include <stdint.h>

#define I2C_WRITE 0
#define I2C_READ 1

void i2c_Init(void) {
    // Initialize I2C MSSP 1 module in Master mode at 100KHz
    TRISB4 = 1;              // Set SCL pin as input
    TRISB6 = 1;              // Set SDA pin as input
    SSP1CON1 = 0b00101000;   // Enable I2C, set Master mode
    SSP1CON2 = 0x00;         // Clear SSP1CON2 register
    SSP1ADD = 79;            // Set clock at 100KHz with 4MHz Fosc
    SSP1STAT = 0b11000000;   // Disable slew rate
}

void i2c_Wait(void) {
    // Wait for I2C transfer to finish
    while ((SSP1CON2 & 0x1F) || (SSP1STAT & 0x04));
}

void i2c_Start(void) {
    // Start I2C communication
    i2c_Wait();
    SSP1CON2bits.SEN = 1;
}

void i2c_Stop(void) {
    // Stop I2C communication
    i2c_Wait();
    SSP1CON2bits.PEN = 1;
}

void i2c_Write(unsigned char data) {
    // Send one byte of data
    i2c_Wait();
    SSP1BUF = data;
}

void i2c_Address(unsigned char address, unsigned char mode) {
    // Send slave address and read/write mode (I2C_WRITE or I2C_READ)
    unsigned char l_address;
    l_address = address << 1;
    l_address += mode;
    i2c_Wait();
    SSP1BUF = l_address;
}

// prints a message on the led matrix
// first pass 0x01 or 0x02 for top or bottom row
// second pass a color 0-9
// third pass the message
void i2c_WriteToArduino(char command,int color, const char* message) {
    i2c_Start();                    // Start I2C communication
    i2c_Address(0x04, I2C_WRITE);   // Address the Arduino configured to listen at 0x04
    i2c_Write(command);             // Send the command byte first
    i2c_Write(color);
    i2c_Write(strlen(message));     // Send the length of the message
    
    // Send each character of the message
    for (int i = 0; message[i] != '\0'; i++) {
        i2c_Write(message[i]); 
    }
    i2c_Stop();
}

// Function to send a number to the Arduino for display
// This one is different from write because it can take an Int
// the display from this is right adjudicated on the bottom row in red text
void i2c_WriteScoreToArduino(int16_t number) {
    i2c_Start();                        // Start I2C communication
    i2c_Address(0x04, I2C_WRITE);       // Address the Arduino configured to listen at 0x04
    i2c_Write(0x03);                    // Command byte for sending a number
    // Send the high byte first, then the low byte 
    i2c_Write((number >> 8) & 0xFF);     // Send the high byte
    i2c_Write(number & 0xFF);            // Send the low byte
    i2c_Stop();                         // Stop I2C communication
}

// function that takes an int and displays that many pixels to represent balls
void i2c_SendBallCount(uint8_t ballCount) {
    i2c_Start();                           // Start I2C communication
    i2c_Address(0x04, I2C_WRITE);          // Address the Arduino
    i2c_Write(0x04);                       // Command byte for sending ball count 
    i2c_Write(ballCount);                  // Send the ball count
    i2c_Stop();                            // Stop I2C communication
}

// aimple clear screen function. this wipes the entire led matrix
void i2c_clearScreen() {
    i2c_Start();                           // Start I2C communication
    i2c_Address(0x04, I2C_WRITE);           // Address the Arduino
    i2c_Write(0x05);                       // Command byte clearing screen
    i2c_Stop();                            // Stop I2C communication
}
