#define DEBUG 0
#include <Adafruit_NeoPixel.h>
#include "Adafruit_VL53L0X.h"

#define PIXELCOUNT 300
#define NUMSTAIRS 14
#define PIXELSPERSTEP (PIXELCOUNT/NUMSTAIRS)
#define PIN 6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELCOUNT,PIN,NEO_GRB + NEO_KHZ800);

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
// address we will assign if dual sensor is present
#define LOX1_ADDRESS 0x20
#define LOX2_ADDRESS 0x31

// set the pins to shutdown
#define SHT_LOX1 15
#define SHT_LOX2 16

// objects for the vl53l0x
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();

// this holds the measurement
VL53L0X_RangingMeasurementData_t upstairsDistance;
VL53L0X_RangingMeasurementData_t downstairsDistance;

#define UPSTAIRSDISTANCE upstairsDistance.RangeMilliMeter
#define DOWNSTAIRSDISTANCE downstairsDistance.RangeMilliMeter

long downstairsDuration;
long upstairsDuration;

// Create an IntervalTimer object 
IntervalTimer distanceMeasurementTimer;
unsigned long endTimeout = 0;
unsigned long upstairsRetriggerDelay, downstairsRetriggerDelay;
#define RETRIGGERSENSITIVITY 3000
bool isWaiting, isWalking, isTimedOut;
bool isGoingUp, isGoingDown, isAtBottom, isAtTop;
bool hasBottomCleared, hasTopCleared; //flags to determine if initial trigger has cleared - prevents subsequent triggering until it does.  Timeout doesn't start until it happens.
int i,j,k;

#define TIMEOUT 5000 //millis
#define LOWERBOUND 30 //mm
#define UPPERBOUND 400 //mm

#define BRIGHTNESS 10 //Set brightness to 1/10th
#define FULL (255/Brightness)
#define DIMLIGHTS 100

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */
void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  // activating LOX1 and reseting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);

  // initing LOX1
  if(!lox1.begin(LOX1_ADDRESS)) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while(1);
  }
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  if(!lox2.begin(LOX2_ADDRESS)) {
    Serial.println(F("Failed to boot second VL53L0X"));
    while(1);
  }
}

void read_dual_sensors() {
  
  lox1.rangingTest(&downstairsDistance, false); // pass in 'true' to get debug data printout!
  lox2.rangingTest(&upstairsDistance, false); // pass in 'true' to get debug data printout!

  if (DEBUG)
  {
    // print sensor one reading
    Serial.print("1: ");
    if(DOWNSTAIRSDISTANCE != 4) {     // if not out of range
      Serial.print(DOWNSTAIRSDISTANCE);
    } else {
      Serial.print("Out of range");
    }
    
    Serial.print(" ");
  
    // print sensor two reading
    Serial.print("2: ");
    if(UPSTAIRSDISTANCE != 4) {
      Serial.print(UPSTAIRSDISTANCE);
    } else {
      Serial.print("Out of range");
    }
    
    Serial.println();
  }
}

void determineState()
{
  
}

// The interrupt will blink the LED, and keep
// track of how many times it has blinked.
int ledState = LOW;
const int ledPin = 13;

//volatile unsigned long blinkCount = 0; // use volatile for shared variables

void clearStrip(){
  for (int l=0; l<strip.numPixels(); l++){
    strip.setPixelColor(l, 0);
  }
  strip.show();     
}

void clearStripDown(){
  colourWipeDown(strip.Color(0, 0, 0), 25 );  // Warm White   
}

void clearStripUp(){
  colourWipeUp(strip.Color(0, 0, 0), 25 );  // Warm White     
}

// Fade light each step strip
void colourWipeDown(uint32_t c, uint16_t wait) {
  noInterrupts();
  for (uint16_t j = 0; j < NUMSTAIRS; j++){
    int start = strip.numPixels()/NUMSTAIRS *j;
    //Serial.println(j);
    for (uint16_t i = start; i < start + PIXELSPERSTEP; i++){
      strip.setPixelColor(i, c);
    }
    delay(wait);
    strip.show();  
   }
   interrupts();
 }
 
 void topdown() {
    if (DEBUG) { Serial.println ("detected top"); }                 // Helpful debug message
    colourWipeDown(strip.Color(50, 50, 50), 25 );  // Warm White
 }
 
void colourWipeUp(uint32_t c, uint16_t wait) {
     noInterrupts();
     for (uint16_t j = NUMSTAIRS; j > 0; j--){
     int start = strip.numPixels()/NUMSTAIRS *j;
     for (uint16_t i = start; i > start - PIXELSPERSTEP; i--){
       strip.setPixelColor(i-1, c);
     }
     delay(wait);
     strip.show();
     interrupts();
   }  
 }

 void bottomup() {
    if (DEBUG) { Serial.println ("detected bottom"); }            // Helpful debug message
    colourWipeUp(strip.Color(50, 50, 50), 25);  // Warm White
  }

  void intializeLights()
  {
    strip.begin();
    bottomup();
    clearStripUp();
    topdown();
    clearStripDown();

    //strip.show(); // Initialize all pixels to 'off'
   
  }
void setup() {
  intializeLights();
  
  //Serial Port begin
  Serial.begin (115200);
  // wait until serial port opens for native USB devices
  //while (! Serial) { delay(1); }
  
  pinMode(ledPin, OUTPUT);
  
  //Set up the i2c bus and appropriate address for the sensors
  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  Serial.println("Shutdown pins inited...");
  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  Serial.println("Both in reset mode...(pins are low)");
  Serial.println("Setting i2c IDs");
  setID();
  Serial.println("Starting...");

  //Initialize state variables
  isWaiting = true;
  isWalking = false;
  isTimedOut = false;
  isAtTop = false;
  isAtBottom = false;
  hasTopCleared = false;
  hasBottomCleared = false;
  
  upstairsRetriggerDelay = millis();
  downstairsRetriggerDelay = millis();
  
  //minimum 100000 microseconds, otherwise the interrupt takes too much time from other tasks
  //Also, don't have it started before all the i2c processes are done or the interrupt will
  //interfere with that.
  distanceMeasurementTimer.begin(read_dual_sensors, 100000); 
}
 
void loop() {
  
  if (isWaiting)
  {
    if ((UPSTAIRSDISTANCE > LOWERBOUND && UPSTAIRSDISTANCE < UPPERBOUND) && upstairsRetriggerDelay < millis())
    {
      Serial.println("Was waiting, triggered upstairs sensor, running topdown, Entering IsWalking State, IsGoingDown");
      isWaiting = false;
      isGoingUp = false;
      isGoingDown = true;
      isWalking = true;
      hasTopCleared = false;
      topdown();
    }
    else if ((DOWNSTAIRSDISTANCE > LOWERBOUND && DOWNSTAIRSDISTANCE < UPPERBOUND) && downstairsRetriggerDelay < millis())
    {
      Serial.println("Was waiting, triggered downstairs sensor, running bottomup, Entering IsWalking State, IsGoingUp");
      isWaiting = false;
      isGoingUp = true;
      isGoingDown = false;
      isWalking = true;
      hasBottomCleared = false;
      bottomup();
    }
    
    if (isWalking)
    {
      Serial.println("Starting to walk");
      //Set an initial timeout, but can get it extended until the sensor clears
      endTimeout = millis() + TIMEOUT;
      Serial.print("Current time: ");
      Serial.print(millis());
      Serial.print(" Timeout: ");
      Serial.println(endTimeout);
    }
  }
  else if (isWalking)
  {
    if (endTimeout < millis())
    {
      Serial.println("Hit timeout, clearing flags, entering IsTimedOut State");
      isTimedOut = true;
      isWalking = false;
      isWaiting = false;
      isGoingUp = false;
      isGoingDown = false;
    }
    else if (isGoingDown)
    {
      if (!hasTopCleared)
      {
        endTimeout = millis() + TIMEOUT;
        if (DEBUG) {
          Serial.print("Top Sensor has not been cleared, updating timeout counter to ");
          Serial.println(endTimeout);
        }
        if (!(UPSTAIRSDISTANCE > LOWERBOUND && UPSTAIRSDISTANCE < UPPERBOUND))
        {
          if (DEBUG) { Serial.println("Top Sensor has been cleared, updating flag"); }
          hasTopCleared = true;
        }
      }
      else if (DOWNSTAIRSDISTANCE > LOWERBOUND && DOWNSTAIRSDISTANCE < UPPERBOUND)
      {
        Serial.println("Going Down, triggered downstairs sensor, wiping lights, entering Waiting State");
        
        isAtTop = false;
        isAtBottom = true;
        isWaiting = false;
        isWalking = false;
        isTimedOut = false;
        isGoingDown = false;
        isGoingUp = false;
        downstairsRetriggerDelay = millis() + RETRIGGERSENSITIVITY;
      }
    }
    else if (isGoingUp)
    {
      if (!hasBottomCleared)
      {
        endTimeout = millis() + TIMEOUT;
        if (DEBUG)
        {
          Serial.print("Bottom Sensor has not been cleared, begginngng timeout counter to ");
          Serial.println(endTimeout);
        }
        if (!(DOWNSTAIRSDISTANCE > LOWERBOUND && DOWNSTAIRSDISTANCE < UPPERBOUND))
        {
          Serial.println("Bottom Sensor has been cleared, updating flag");
          hasBottomCleared = true;
        }
      }
      else if (UPSTAIRSDISTANCE > LOWERBOUND && UPSTAIRSDISTANCE < UPPERBOUND)
      {
        Serial.println("Going Up, triggered upstairs sensor, wiping lights, entering Waiting State");
        isAtTop = true;
        isAtBottom = false;
        isWaiting = false;
        isWalking = false;
        isTimedOut = false;
        isGoingDown = false;
        isGoingUp = false;
        upstairsRetriggerDelay = millis() + RETRIGGERSENSITIVITY;
      }
    }
  }
  else if (isAtTop)
  {
    if (!(UPSTAIRSDISTANCE > LOWERBOUND && UPSTAIRSDISTANCE < UPPERBOUND))
    {
      clearStripUp();
      isAtTop = false;
      isAtBottom = false;
      isWaiting = true;
      isTimedOut = false;
      isGoingDown = false;
      isGoingUp = false;
    }
  }
  else if (isAtBottom)
  {
    if (!(DOWNSTAIRSDISTANCE > LOWERBOUND && DOWNSTAIRSDISTANCE < UPPERBOUND))
    {
      clearStripDown();
      isAtTop = false;
      isAtBottom = false;
      isWaiting = true;
      isTimedOut = false;
      isGoingDown = false;
      isGoingUp = false;
    }
  }
  else if (isTimedOut)
  {
    Serial.println("Hit timeout, clearing strip, entering Waiting State");
    if (isGoingUp)
    {
      clearStripUp();
    }
    else
    {
      clearStripDown();
    }
    isGoingUp = false;
    isGoingDown = false;

    hasTopCleared = false;
    hasBottomCleared = false;
    
    isTimedOut = false;
    isWalking = false;
    isWaiting = true;
  }
}
