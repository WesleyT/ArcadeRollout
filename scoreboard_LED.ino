#include <FastLED.h>
#include <Wire.h>
#include <avr/pgmspace.h>

#define NUM_LEDS 256
#define WIDTH 32
#define HEIGHT 8
#define DATA_PIN 2
#define SLAVE_ADDRESS 0x04
#define BUFFER_SIZE 32  

volatile byte i2cBuffer[BUFFER_SIZE];
volatile int i2cBufferLength = 0;
volatile bool dataReady = false;

CRGB leds[NUM_LEDS];

// Mapping of LEDs for each character position
// created an array for easier itteration
// ONLY 8 CHARACTERS CAN BE DISPLAYED AT A TIME
const int size = 15;
// The top characters are mapped left adj
int top1[size] = {0, 1, 2, 3, 4, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
int top2[size] = {32, 33, 34, 35, 36, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52};
int top3[size] = {64, 65, 66, 67, 68, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84};
int top4[size] = {96, 97, 98, 99, 100, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116};
int top5[size] = {128, 129, 130, 131, 132, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148};
int top6[size] = {160, 161, 162, 163, 164, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180};
int top7[size] = {192, 193, 194, 195, 196, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212};
int top8[size] = {224, 225, 226, 227, 228, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244};
int* topMappings[8] = {top1, top2, top3, top4, top5, top6, top7, top8};

// the bottom characters are mapped righ adj
int bot1[size] = {12,11,10,9,8,23,22,21,20,19,28,27,26,25,24};
int bot2[size] = {44,43,42,41,40,55,54,53,52,51,60,59,58,57,56};
int bot3[size] = {76,75,74,73,72,87,86,85,84,83,92,91,90,89,88};
int bot4[size] = {108,107,106,105,104,119,118,117,116,115,124,123,122,121,120};
int bot5[size] = {140,139,138,137,136,151,150,149,148,147,156,155,154,153,152};
int bot6[size] = {172,171,170,169,168,183,182,181,180,179,188,187,186,185,184};
int bot7[size] = {204,203,202,201,200,215,214,213,212,211,220,219,218,217,216};
int bot8[size] = {236,235,234,233,232,247,246,245,244,243,252,251,250,249,248};
int* botMappings[8] = {bot1, bot2, bot3, bot4, bot5, bot6, bot7, bot8};

// certain leds indicate balls remaining
int ballLeds[] = {9, 25, 41, 57, 73, 89, 105, 121, 137, 153};

// Characters LED Font
// characters stored in progmem to free up ram
// This tells the arduino what each character looks like
const int ctA PROGMEM = 12;
const int A[ctA] PROGMEM = {0,1,2,3,4,7,9,10,11,12,13,14};
const int ctB PROGMEM = 12;
const int B[ctB] PROGMEM = {0,1,2,3,4,5,7,9,10,11,13,14};
const int ctC PROGMEM = 9;
const int C[ctC] PROGMEM = {0,1,2,3,4,5,9,10,14};
const int ctD PROGMEM = 10;
const int D[ctD] PROGMEM = {0,1,2,3,4,5,9,11,12,13};
const int ctE PROGMEM = 11;
const int E[ctE] PROGMEM = {0,1,2,3,4,5,7,9,10,12,14};
const int ctF PROGMEM = 9;
const int F[ctF] PROGMEM = {0,1,2,3,4,7,9,10,12};
const int ctG PROGMEM = 11;
const int G[ctG] PROGMEM = {0,1,2,3,4,5,9,10,12,13,14};
const int ctH PROGMEM = 11;
const int H[ctH] PROGMEM = {0,1,2,3,4,7,10,11,12,13,14};
const int ctI PROGMEM = 9;
const int I[ctI] PROGMEM = {0,4,5,6,7,8,9,10,14};
const int ctJ PROGMEM = 8;
const int J[ctJ] PROGMEM = {3,4,5,10,11,12,13,14};
const int ctK PROGMEM = 10;
const int K[ctK] PROGMEM = {0,1,2,3,4,7,10,11,13,14};
const int ctL PROGMEM = 7;
const int L[ctL] PROGMEM = {0,1,2,3,4,5,14};
const int ctM PROGMEM = 12;
const int M[ctM] PROGMEM = {0,1,2,3,4,7,8,10,11,12,13,14};
const int ctN PROGMEM = 10;
const int N[ctN] PROGMEM = {0,1,2,3,4,9,11,12,13,14};
const int ctO PROGMEM = 12;
const int O[ctO] PROGMEM = {0,1,2,3,4,5,9,10,11,12,13,14};
const int ctP PROGMEM = 10;
const int P[ctP] PROGMEM = {0,1,2,3,4,7,9,10,11,12};
const int ctQ PROGMEM = 11;
const int Q[ctQ] PROGMEM = {0,1,2,3,6,9,10,11,12,13,14};
const int ctR PROGMEM = 11;
const int R[ctR] PROGMEM = {0,1,2,3,4,7,9,10,11,13,14};
const int ctS PROGMEM = 11;
const int S[ctS] PROGMEM = {0, 1, 2, 4, 5, 7, 9, 10, 12, 13, 14};
const int ctT PROGMEM = 7;
const int T[ctT] PROGMEM = {0,5,6,7,8,9,10};
const int ctU PROGMEM = 11;
const int U[ctU] PROGMEM = {0,1,2,3,4,5,10,11,12,13,14};
const int ctV PROGMEM = 9;
const int V[ctV] PROGMEM = {0,1,2,3,5,10,11,12,13};
const int ctW PROGMEM = 12;
const int W[ctW] PROGMEM = {0,1,2,3,4,6,7,10,11,12,13,14};
const int ctX PROGMEM = 9;
const int X[ctX] PROGMEM = {0,1,3,4,7,10,11,13,14};
const int ctY PROGMEM = 9;
const int Y[ctY] PROGMEM = {0,1,2,5,6,7,10,11,12};
const int ctZ PROGMEM = 9;
const int Z[ctZ] PROGMEM = {0,3,4,5,7,9,10,11,14};
//Numbers
const int ct0 PROGMEM = 12;
const int n0[ct0] PROGMEM = {0,1,2,3,4,5,9,10,11,12,13,14};
const int ct1 PROGMEM = 8;
const int n1[ct1] PROGMEM = {0,4,5,6,7,8,9,14};
const int ct2 PROGMEM = 11;
const int n2[ct2] PROGMEM = {0,2,3,4,5,7,9,10,11,12,14};
const int ct3 PROGMEM = 10;
const int n3[ct3] PROGMEM = {0,4,5,7,9,10,11,12,13,14};
const int ct4 PROGMEM = 9;
const int n4[ct4] PROGMEM = {0,1,2,7,10,11,12,13,14};
const int ct5 PROGMEM = 11;
const int n5[ct5] PROGMEM = {0,1,2,4,5,7,9,10,12,13,14};
const int ct6 PROGMEM = 12;
const int n6[ct6] PROGMEM = {0,1,2,3,4,5,7,9,10,12,13,14};
const int ct7 PROGMEM = 7;
const int n7[ct7] PROGMEM = {0,5,6,7,9,10,11};
const int ct8 PROGMEM = 13;
const int n8[ct8] PROGMEM = {0,1,2,3,4,5,7,9,10,11,12,13,14};
const int ct9 PROGMEM = 10;
const int n9[ct9] PROGMEM = {0,1,2,7,9,10,11,12,13,14};
// this is for "?"
const int ctqq PROGMEM = 9;
const int qq[ctqq] PROGMEM = {0,5,7,9,10,11,12};


// this function takes the char from I2C and calls the correct Font variables
void displayCharacterFromChar(char character, int *ledMapping, CRGB color) {
    //this is setup so that a space will erase the previous character
    // doing an ___ will not erase. this is good for overlaying top and bottom text
    if (character == ' ') {
        clearLedMapping(ledMapping);
        return; // Skip the rest of the function
    }  else if (character == '_') {
        return; // Skip the rest of the function
    }  
    clearLedMapping(ledMapping);
    int patternSize = 0;
    const int *pattern = NULL;
    
    //this switch case maps each character to its pattern
    switch(character) {
        case 'A':
            patternSize = pgm_read_word_near(&ctA);
            pattern = A;
            break;
        case 'B':
            patternSize = pgm_read_word_near(&ctB);
            pattern = B;
            break;
        case 'C':
            patternSize = pgm_read_word_near(&ctC);
            pattern = C;
            break;
        case 'D':
            patternSize = pgm_read_word_near(&ctD);
            pattern = D;
            break;
        case 'E':
            patternSize = pgm_read_word_near(&ctE);
            pattern = E;
            break;
        case 'F':
            patternSize = pgm_read_word_near(&ctF);
            pattern = F;
            break;
        case 'G':
            patternSize = pgm_read_word_near(&ctG);
            pattern = G;
            break;
        case 'H':
            patternSize = pgm_read_word_near(&ctH);
            pattern = H;
            break;
        case 'I':
            patternSize = pgm_read_word_near(&ctI);
            pattern = I;
            break;
        case 'J':
            patternSize = pgm_read_word_near(&ctJ);
            pattern = J;
            break;
        case 'K':
            patternSize = pgm_read_word_near(&ctK);
            pattern = K;
            break;
        case 'L':
            patternSize = pgm_read_word_near(&ctL);
            pattern = L;
            break;
        case 'M':
            patternSize = pgm_read_word_near(&ctM);
            pattern = M;
            break;
        case 'N':
            patternSize = pgm_read_word_near(&ctN);
            pattern = N;
            break;
        case 'O':
            patternSize = pgm_read_word_near(&ctO);
            pattern = O;
            break;
        case 'P':
            patternSize = pgm_read_word_near(&ctP);
            pattern = P;
            break;
        case 'Q':
            patternSize = pgm_read_word_near(&ctQ);
            pattern = Q;
            break;
        case 'R':
            patternSize = pgm_read_word_near(&ctR);
            pattern = R;
            break;
        case 'S':
            patternSize = pgm_read_word_near(&ctS);
            pattern = S;
            break;
        case 'T':
            patternSize = pgm_read_word_near(&ctT);
            pattern = T;
            break;
        case 'U':
            patternSize = pgm_read_word_near(&ctU);
            pattern = U;
            break;
        case 'V':
            patternSize = pgm_read_word_near(&ctV);
            pattern = V;
            break;
        case 'W':
            patternSize = pgm_read_word_near(&ctW);
            pattern = W;
            break;
        case 'X':
            patternSize = pgm_read_word_near(&ctX);
            pattern = X;
            break;
        case 'Y':
            patternSize = pgm_read_word_near(&ctY);
            pattern = Y;
            break;
        case 'Z':
            patternSize = pgm_read_word_near(&ctZ);
            pattern = Z;
            break;
        case '0':
            patternSize = pgm_read_word_near(&ct0);
            pattern = n0;
            break;
        case '1':
            patternSize = pgm_read_word_near(&ct1);
            pattern = n1;
            break;
        case '2':
            patternSize = pgm_read_word_near(&ct2);
            pattern = n2;
            break;
        case '3':
            patternSize = pgm_read_word_near(&ct3);
            pattern = n3;
            break;
        case '4':
            patternSize = pgm_read_word_near(&ct4);
            pattern = n4;
            break;
        case '5':
            patternSize = pgm_read_word_near(&ct5);
            pattern = n5;
            break;
        case '6':
            patternSize = pgm_read_word_near(&ct6);
            pattern = n6;
            break;
        case '7':
            patternSize = pgm_read_word_near(&ct7);
            pattern = n7;
            break;
        case '8':
            patternSize = pgm_read_word_near(&ct8);
            pattern = n8;
            break;
        case '9':
            patternSize = pgm_read_word_near(&ct9);
            pattern = n9;
            break;
        case '?':
            patternSize = pgm_read_word_near(&ctqq);
            pattern = qq;
            break;
    }


        //Debugging code 
        /*
        Serial.print("Displaying character: ");
        Serial.println(character);
        Serial.print("Pattern Size: ");
        Serial.println(patternSize);
        Serial.print("Using LED mapping starting at index: ");
        Serial.println(ledMapping[0]);  // Assuming this prints the first LED index used for this character
        */

    // Maps leds
    for (int i = 0; i < patternSize; i++) {
        int ledIndex = pgm_read_word_near(&pattern[i]);
        if (ledIndex < size) {
            leds[ledMapping[ledIndex]] = color;
            // print for debugging
            //Serial.print("Activating LED at index: ");
            //Serial.println(ledMapping[ledIndex]);
        }
    }
}

//setup function
void setup() {
  //setup FastLED
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(050);
  //setup Wire
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  //setup Serial
  Serial.begin(9600);
  Serial.println("Waiting for data...");
}

// Function to assign a cgrb color to integers 0-9
CRGB colorSelect(int color) {
  switch (color) {
    case 1:
      return CRGB::Green;
    case 2:
      return CRGB::Blue;
    case 3:
      return CRGB::Red;
    case 4:
      return CRGB::Yellow;
    case 5:
      return CRGB::Purple;
    case 6:
      return CRGB::Orange;
    case 7:
      return CRGB::White;
    default:
      return CRGB::Black;  // Default color if no valid color selected
  }
}

// wait for i2c event
void receiveEvent(int howMany) {
    int index = 0;
    while (Wire.available() && index < BUFFER_SIZE) {
        i2cBuffer[index++] = Wire.read();
    }
    i2cBufferLength = index;  // Store the number of bytes read
    dataReady = true;         // Set flag to indicate data is ready to be processed
}
void loop() {
    if (dataReady) {
        processI2CData(i2cBuffer, i2cBufferLength);
        dataReady = false;  // Reset the flag after processing
    }
}
void processI2CData(const byte* buffer, int length) {
    if (length == 0) return;  // No data to process

    int command = buffer[0];  // First byte is the command
    switch (command) {
        case 0x01: {
            if (length < 3) return;  // Ensure minimum length for colorIndex and string length bytes

            int colorIndex = buffer[1];
            CRGB color = colorSelect(colorIndex);
            int stringLength = buffer[2];

            if (length < stringLength + 3) return;  // Ensure full string is in buffer

            char text[9] = {0}; // Array to hold up to 8 characters + null terminator
            int maxChars = min(stringLength, 8);  // Limit the characters processed to 8

            for (int i = 0; i < maxChars; i++) {
                text[i] = buffer[3 + i];
            }
            text[maxChars] = '\0';  // Ensure string is null-terminated

            for (int i = 0; i < maxChars; i++) {
                if (i < 8) {  // Ensure i does not exceed the number of defined mappings
                    displayCharacterFromChar(text[i], topMappings[i], color);
                }
            }
            FastLED.show();
            break;
        }
        case 0x02: {
            // Check for minimum length of command type, color index, and string length byte
            if (length < 3) return;  // Ensure there are at least 3 bytes: command, colorIndex, stringLength

            int colorIndex = buffer[1];
            CRGB color = colorSelect(colorIndex); 
            int stringLength = buffer[2];

            // Ensure there is enough buffer for the entire string specified by stringLength
            if (length < stringLength + 3) return;  // Checks the full command length including header

            char text[9] = {0};  // Buffer to hold text, space for null terminator
            int maxChars = min(stringLength, 8);  // Limit characters to 8 or less

            // Read characters from the buffer
            for (int i = 0; i < maxChars; i++) {
                text[i] = buffer[3 + i];  // Characters start after command, colorIndex, and stringLength
            }
            text[maxChars] = '\0';  // Null-terminate the string

            // Display characters on the bottom line
            for (int i = 0; i < maxChars; i++) {
                // Ensure i does not exceed the number of defined mappings
                displayCharacterFromChar(text[i], botMappings[i % 8], color);
            }
            FastLED.show();
            break;
        }
        case 0x03:
        {
            if (length < 3) return;  // Ensure there are enough bytes to read the high and low byte

            int highByte = buffer[1];   // Read high byte
            int lowByte = buffer[2];    // Read low byte

            // Combine the high and low bytes to form a 16-bit integer
            int number = (highByte << 8) | lowByte;

            displayNumber(number);  // Display the number
            FastLED.show();
            break;
        }
        case 0x04:
        {
            if (length < 2) return;  // Ensure there is at least one byte to read the ball count

            int ballCount = buffer[1];  // Read the ball count from buffer
            displayBalls(ballCount);   // Update the ball count display
            FastLED.show();
            break;
        }        
        case 0x05:
        {
            FastLED.clear();
            FastLED.show();  // Ensure to update the display after clearing
            break;
        }
        default: {
            Serial.print("Unknown command received: ");
            Serial.println(command);
            break;
        }
    }
}


//this function turns on 10 specific pixels to represent balls left
void displayBalls(int count) {
  // Turn off all ball LEDs first
  for (int i = 0; i < 10; i++) {
    leds[ballLeds[i]] = CRGB::Black;
  }

  // Now turn on the LEDs up to the count
  for (int i = 0; i < count; i++) {
    // Ensure the index is within the array bounds
    if (i < 10) {
      leds[ballLeds[i]] = CRGB::Orange; // Or any other color you wish to use
    }
  }
}

//simply displays a number when provided an int
void displayNumber(int number) {
    char numStr[5];  // Assuming a maximum of four digits plus a terminating null character
    snprintf(numStr, sizeof(numStr), "%d", number);  // Convert the number to a string

    // Clear only the last three character mappings before displaying new number
    for (int i = 5; i < 8; i++) {  // Assuming mappings 5, 6, and 7 are the bottom right three positions
        clearLedMapping(botMappings[i]);
    }

    int len = strlen(numStr);  // Get the length of the number string
    // Display the number starting from the rightmost available character mapping set
    for (int i = 0; i < len; i++) {
        // Calculate which mapping to use (counting backwards from the last mapping)
        int* currentMapping = botMappings[7 - (len - 1 - i)];
        // Convert char to uppercase, necessary if using alphanumeric character mappings
        char currentChar = numStr[i];
        // Display the character using a specific color, e.g., CRGB::Red
        displayCharacterFromChar(currentChar, currentMapping, CRGB::Red);
    }
}

// Function to clear all LEDs in a given mapping
void clearLedMapping(int *ledMapping) {
    for (int i = 0; i < size; i++) {
        leds[ledMapping[i]] = CRGB::Black;
    }
}

//displays a character. must pass through displaycharacterfromchar for easier access
void displayCharacter(int pattern[], int patternSize, int ledMapping[], int mappingSize, CRGB color) {
  
  for (int i = 0; i < patternSize; i++) {
    if (pattern[i] < mappingSize) {  // Ensure the pattern index is within the mapping array size
      leds[ledMapping[pattern[i]]] = color;  // Turn on the LED at the mapped index
    }
  }
}