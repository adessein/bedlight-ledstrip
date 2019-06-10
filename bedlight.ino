/* bedlight-ledstrip
 * A simple LED strip controlled with a Pro Micro and a rotary encoder
 * Arnaud Dessein
 * https://github.com/adessein/bedlight-ledstrip
 * GNU GENERAL PUBLIC LICENSE v3
 */

// Rotary encoder
int CLK = 14;
int DT = 16;
int SW = 10;
int RotPosition = 0; 
int rotation;  
int value;
boolean LeftRight;
boolean ColorMode; // if False intensity mode

int hueVal = 0;
int satVal = 100;
int valVal = 100;

// Output
int redPin = 6;   // Red LED
int grnPin = 5;  // Green LED
int bluPin = 9;  // Blue LED

// Color arrays
int black[3]  = { 
  0, 0, 0 };
int white[3]  = { 
  100, 100, 100 };
int red[3]    = { 
  100, 0, 0 };
int green[3]  = { 
  0, 100, 0 };
int blue[3]   = { 
  0, 0, 100 };
int yellow[3] = { 
  40, 95, 0 };
int dimWhite[3] = { 
  30, 30, 30 };
// etc.

// Set initial color
int redVal = black[0];
int grnVal = black[1]; 
int bluVal = black[2];

int wait = 50;      // 10ms internal crossFade delay; increase for slower fades
int hold = 2000;       // Optional hold when a color is complete, before the next crossFade
int DEBUG = 0;      // DEBUG counter; if set to 1, will write values back via serial
int loopCount = 60; // How often should DEBUG report?
int repeat = 0;     // How many times should we loop before stopping? (0 for no stop)
int j = 0;          // Loop counter for repeat
int mode = 0;        // hue sat val

// Initialize color variables
int prevR = redVal;
int prevG = grnVal;
int prevB = bluVal;

boolean oldButton = false,
        newButton = false;

// Set up the LED outputs
void setup()
{
  pinMode(redPin, OUTPUT);   // sets the pins as output
  pinMode(grnPin, OUTPUT);   
  pinMode(bluPin, OUTPUT); 

  pinMode (CLK,INPUT);
  pinMode (DT,INPUT);
  pinMode (SW,INPUT);
  
  digitalWrite(SW, HIGH); //turn pullup resistor on
}

// Main program: list the order of crossfades
void loop()
{
  value = digitalRead(CLK);
  newButton = digitalRead(SW);
  if (!newButton & oldButton) mode++; // CHanged mode on release button
  if (mode > 2) mode = 0;
  oldButton = newButton;
    
  
  if (value != rotation){ // we use the DT pin to find out which way we turning.
    if (digitalRead(DT) != value)
    {  // Clockwise
      switch(mode)
      {
      case 0 : 
        hueVal++;
        break;
      case 1 : 
        satVal++;
        break;
      case 2 : 
        valVal++;
        break;
      }
      LeftRight = true;
    }
    else 
    { //Counterclockwise
      switch(mode)
      {
      case 0 : 
        hueVal--;
        break;
      case 1 : 
        satVal--;
        break;
      case 2 : 
        valVal--;
        break;
      }
      LeftRight = false; 
    }
    
    hueVal = max(0, min(360, hueVal));
    satVal = max(0, min(100, satVal));
    valVal = max(0, min(100, valVal));
    
    HSV_to_RGB(hueVal, satVal, valVal, &redVal, &grnVal, &bluVal);
    analogWrite(redPin, redVal);
    analogWrite(grnPin, grnVal);
    analogWrite(bluPin, bluVal);
  }
  rotation = value;

}

/* ***** Below is this is code from Clay Shirky *** */

/*
 * Code for cross-fading 3 LEDs, red, green and blue (RGB) 
 * To create fades, you need to do two things: 
 *  1. Describe the colors you want to be displayed
 *  2. List the order you want them to fade in
 *
 * DESCRIBING A COLOR:
 * A color is just an array of three percentages, 0-100, 
 *  controlling the red, green and blue LEDs
 *
 * Red is the red LED at full, blue and green off
 *   int red = { 100, 0, 0 }
 * Dim white is all three LEDs at 30%
 *   int dimWhite = {30, 30, 30}
 * etc.
 *
 * Some common colors are provided below, or make your own
 * 
 * LISTING THE ORDER:
 * In the main part of the program, you need to list the order 
 *  you want to colors to appear in, e.g.
 *  crossFade(red);
 *  crossFade(green);
 *  crossFade(blue);
 *
 * Those colors will appear in that order, fading out of 
 *    one color and into the next  
 *
 * In addition, there are 5 optional settings you can adjust:
 * 1. The initial color is set to black (so the first color fades in), but 
 *    you can set the initial color to be any other color
 * 2. The internal loop runs for 1020 interations; the 'wait' variable
 *    sets the approximate duration of a single crossfade. In theory, 
 *    a 'wait' of 10 ms should make a crossFade of ~10 seconds. In 
 *    practice, the other functions the code is performing slow this 
 *    down to ~11 seconds on my board. YMMV.
 * 3. If 'repeat' is set to 0, the program will loop indefinitely.
 *    if it is set to a number, it will loop that number of times,
 *    then stop on the last color in the sequence. (Set 'return' to 1, 
 *    and make the last color black if you want it to fade out at the end.)
 * 4. There is an optional 'hold' variable, which pasues the 
 *    program for 'hold' milliseconds when a color is complete, 
 *    but before the next color starts.
 * 5. Set the DEBUG flag to 1 if you want debugging output to be
 *    sent to the serial monitor.
 *
 *    The internals of the program aren't complicated, but they
 *    are a little fussy -- the inner workings are explained 
 *    below the main loop.
 *
 * April 2007, Clay Shirky <clay.shirky@nyu.edu> 
 */

/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
 * 
 * The program works like this:
 * Imagine a crossfade that moves the red LED from 0-10, 
 *   the green from 0-5, and the blue from 10 to 7, in
 *   ten steps.
 *   We'd want to count the 10 steps and increase or 
 *   decrease color values in evenly stepped increments.
 *   Imagine a + indicates raising a value by 1, and a -
 *   equals lowering it. Our 10 step fade would look like:
 * 
 *   1 2 3 4 5 6 7 8 9 10
 * R + + + + + + + + + +
 * G   +   +   +   +   +
 * B     -     -     -
 * 
 * The red rises from 0 to 10 in ten steps, the green from 
 * 0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
 * 
 * In the real program, the color percentages are converted to 
 * 0-255 values, and there are 1020 steps (255*4).
 * 
 * To figure out how big a step there should be between one up- or
 * down-tick of one of the LED values, we call calculateStep(), 
 * which calculates the absolute gap between the start and end values, 
 * and then divides that gap by 1020 to determine the size of the step  
 * between adjustments in the value.
 */

int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero, 
    step = 1020/step;              //   divide by 1020
  } 
  return step;
}

/* The next function is calculateVal. When the loop value, i,
 *  reaches the step size appropriate for one of the
 *  colors, it increases or decreases the value of that color by 1. 
 *  (R, G, and B are each calculated separately.)
 */

int calculateVal(int step, int val, int i) {

  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;           
    } 
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    } 
  }
  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  } 
  else if (val < 0) {
    val = 0;
  }
  return val;
}

/* crossFade() converts the percentage colors to a 
 *  0-255 range, then loops 1020 times, checking to see if  
 *  the value needs to be updated each time, then writing
 *  the color values to the correct pins.
 */

void crossFade(int color[3]) {
  // Convert to 0-255
  int R = (color[0] * 255) / 100;
  int G = (color[1] * 255) / 100;
  int B = (color[2] * 255) / 100;

  int stepR = calculateStep(prevR, R);
  int stepG = calculateStep(prevG, G); 
  int stepB = calculateStep(prevB, B);

  for (int i = 0; i <= 1020; i++) {
    redVal = calculateVal(stepR, redVal, i);
    grnVal = calculateVal(stepG, grnVal, i);
    bluVal = calculateVal(stepB, bluVal, i);

    analogWrite(redPin, redVal);   // Write current values to LED pins
    analogWrite(grnPin, grnVal);      
    analogWrite(bluPin, bluVal); 

    delay(wait); // Pause for 'wait' milliseconds before resuming the loop
  }
  // Update current values for next loop
  prevR = redVal; 
  prevG = grnVal; 
  prevB = bluVal;
  delay(hold); // Pause for optional 'wait' milliseconds before resuming the loop
}

void HSV_to_RGB(float h, float s, float v, int *r, int *g, int *b)
{
  int i;
  float f,p,q,t;

  h = max(0.0, min(360.0, h));
  s = max(0.0, min(100.0, s));
  v = max(0.0, min(100.0, v));

  s /= 100;
  v /= 100;

  if(s == 0) {
    // Achromatic (grey)
    *r = *g = *b = round(v*255);
    return;
  }

  h /= 60; // sector 0 to 5
  i = floor(h);
  f = h - i; // factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch(i) {
  case 0:
    *r = round(255*v);
    *g = round(255*t);
    *b = round(255*p);
    break;
  case 1:
    *r = round(255*q);
    *g = round(255*v);
    *b = round(255*p);
    break;
  case 2:
    *r = round(255*p);
    *g = round(255*v);
    *b = round(255*t);
    break;
  case 3:
    *r = round(255*p);
    *g = round(255*q);
    *b = round(255*v);
    break;
  case 4:
    *r = round(255*t);
    *g = round(255*p);
    *b = round(255*v);
    break;
  default: // case 5:
    *r = round(255*v);
    *g = round(255*p);
    *b = round(255*q);
  }
}

