/*
 * 
 * this script reads the serial port of the Lelit MaraX espresso machine and displays on a TTGO ESP32
 * 
 * 
 * Normal Mode:
 * display current HX temperature
 * display temp graph of last +-5 minutes
 * 
 * Detail Mode:
 * display all stuff
 * 
 * Debug mode: 
 * invent values for if there's no serial data
 * 
 * TTGO resolution: 135X240
 * 
*/


// resolution of screen 135X240 ... resolution of our graph: XRES, YRES mapped to temp range LOWTEMP, HIGHTEMP
#define XRES 135
#define YRES 120
#define LOWTEMP 70
#define HIGHTEMP 110

// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

//chracter length of read string
#define LEN 24

//#define TFT_BL 4 // Display backlight control pin

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>


#include <Button2.h>
#define BUTTONA_PIN  0
#define BUTTONB_PIN  35
Button2 buttonA = Button2(BUTTONA_PIN);
Button2 buttonB = Button2(BUTTONB_PIN);

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

// read data variable
unsigned long previousMillis = 0;
unsigned long previousMillisSerial = 0;
unsigned long currentMillis;
const int serialInterval = 400; //time to update read serial
const int interval = 3000; //time to update graph
char maraxstatus[30];
char ch;
char maraMode = 'X'; // C -offee or V -apour prio
int steamTempActual;
int steamTempTarget;
int hxTempActual;
int boostTime;
int heating = 0;
int i = 0;
int j = 0;
boolean debug = false;
boolean displayDetails = false;
int hxTemps[XRES];
int ypos;
int xpos;
int arrayOffset = 1;
String intstr;

/////////////////////////////////////////////
//scale using Loacell an HX711 amp
#include "HX711.h"
HX711 scale;
#define LOADCELL_DOUT_PIN  12
#define LOADCELL_SCK_PIN  13
#define LOADCELL_CALIBRATION_FACTOR 122.40
unsigned long previousMillisScale = 0;
unsigned long shotTimerMillis;
#define SCALE_INTERVAL 10000 //time to reset when nothing changes
float currentWeight; //scale.get_units() returns a float
float previousWeight = 0;

/*



void addtosetup() {

  
}

void addtoloop() {
  if(scale.get_units() > 1000) scaleView(); 
}


  
  
  }
*/
/////////////////////////////////////////////


void setup() {
  // display
  tft.init();
  tft.setRotation(0);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // serial connection to MaraX
  Serial2.begin(9600, SERIAL_8N1, 25, 26); //9600 baud, using Pin 25 RX and 26 TX
  delay(100);

  // buttons
  buttonA.setClickHandler(buttonClick);
  buttonB.setClickHandler(buttonClick);


  // scale setup
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(LOADCELL_CALIBRATION_FACTOR); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
}

void loop() {

  buttonA.loop();
  buttonB.loop();

  loopScale();

  loopSerial();

}

void loopSerial() {
  // read serial info every time interval
  currentMillis = millis();
  if (currentMillis - previousMillisSerial >= serialInterval) {
    previousMillisSerial = currentMillis;
    
    if (debug) {
      strcpy(maraxstatus, "V1.99,199,111,099,0777,0");
    } else {
      readSerial();
    }
    displayData();
  }
}

void buttonClick(Button2& btn) {
  if (btn == buttonA) {
    if(millis()>1000) displayDetails = !displayDetails; //toggle details mode, but not at until 1s after startup
    tft.fillScreen(TFT_BLACK);
    delay(WAIT);
  }
  if (btn == buttonB) {
    if(millis()>1000) debug = !debug; //toggle debug mode, but not at until 1s after startup
    tft.fillScreen(TFT_BLACK);
    delay(WAIT);
  }
}

void readSerial(){
  // read the serial buffer
  while (Serial2.available()) {
    ch = Serial2.read();
    if ( (ch == 'C') || (ch == 'V')) {
      i = 0;
    }
    if (i < LEN) {
      maraxstatus[i] = ch;
      i = i + 1;
    }
    if (i >= LEN) {
      i = 0;
      break;
    }
  }
}

void displayData() {

  // separate data from read serial string
  maraMode = maraxstatus[0];
  steamTempActual = atoi( &maraxstatus[6] );
  steamTempTarget = atoi( &maraxstatus[10] );
  hxTempActual = atoi( &maraxstatus[14] );
  boostTime = atoi( &maraxstatus[18] );
  heating = atoi( &maraxstatus[23] );


  // inform always when in debug mode
  if (debug) tft.drawString("Debug Mode", 0, 210, 2);

  if (displayDetails) {
    xpos = 90;
    ypos = 0;
    //tft.fillScreen(TFT_BLACK);

    intstr = String(maraxstatus);
    tft.drawString(intstr, 0, 0, 2);
    ypos = ypos + 26;

    tft.drawString("Mode: ", 0, ypos, 4);
    tft.drawChar(maraMode, xpos, ypos, 4);
    ypos = ypos + 26;

    tft.drawString("Goal: ", 0, ypos, 4);
    intstr = String(steamTempTarget);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;
    
    tft.drawString("Steam: ", 0, ypos, 4);
    intstr = String(steamTempActual);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;

    tft.drawString("Hx: ", 0, ypos, 4);
    intstr = String(hxTempActual);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;

    tft.drawString("Boost: ", 0, ypos, 4);
    intstr = String(boostTime);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;

    tft.drawString("Heater: ", 0, ypos, 4);
    intstr = String(heating);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;

    tft.drawString("Time: ", 0, ypos, 4);
    intstr = String(currentMillis / 1000);
    tft.drawString(intstr, xpos, ypos, 4);
    ypos = ypos + 26;
    
  }
  if (!displayDetails) { // when !displayDetails,we draw a nicer screen
    if (heating) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    }
    else {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    if (hxTempActual < 100) {
      intstr = "0" + String(hxTempActual); //add zero to keep it 3 digits
    } else {
      intstr = String(hxTempActual);
    }
    tft.drawString(intstr, 20, 26, 7);

    // update graph every time interval
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time update
      previousMillis = currentMillis;
      maraxDrawTempGraph(); //draw thing
    }
  }
}

void maraxDrawTempGraph() {

  hxTemps[arrayOffset] = hxTempActual;// get value and increase arrayOffset
  if ( arrayOffset >= XRES -1 ) {
    arrayOffset = 0;
  } else {
    arrayOffset++;
  }

  // for testing:
  if (debug) {
    for (i = 0; i < XRES; i++) {
      hxTemps[i] = (LOWTEMP + i % (HIGHTEMP - LOWTEMP)) ;
    }
    arrayOffset = 42; //random, i diced it
  }

  //draw array from 0 to XRES
  int graphYPposition = 240 - YRES; // to be at the bottom
  tft.fillRect(0, graphYPposition, XRES, YRES, TFT_BLACK);
  tft.drawRect(0, graphYPposition, XRES, YRES, TFT_WHITE);

  for (i = 0; i <= XRES; i++) {
    xpos = ( ( arrayOffset + i ) % ( XRES - 1) ); // shift and loop through array starting with with arrayOffset
    if (LOWTEMP < hxTemps[xpos] < HIGHTEMP) {
      ypos = graphYPposition + YRES - (YRES / (HIGHTEMP - LOWTEMP) * (hxTemps[xpos] - LOWTEMP)) ; 
      //down to graph, down graph height, up temp value in px
      if (hxTemps[xpos] < 91) {
        for (j = ypos; j < (graphYPposition + YRES); j++) {
          tft.drawPixel(i, j, TFT_BLUE);
        }
      } else if (hxTemps[xpos] > 96) {
        for (j = ypos; j < (graphYPposition + YRES); j++) {
          tft.drawPixel(i, j, TFT_RED);
        }
      } else {
        for (j = ypos; j < (graphYPposition + YRES); j++) {
          tft.drawPixel(i, j, TFT_GREEN);
        }
      }
    }
  }
}

void dayDream() {
  //tft.fillScreen(TFT_YELLOW);
 // if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
 //   pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
 //   digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
 // }

 //   esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,1); //1 = High, 0 = Low

}


void loopScale() {
  currentMillis = millis();
  currentWeight = scale.get_units();
  if ( currentWeight < 0 ) currentWeight = 0;

  //  check interval when weight is added, reset timerstopping(break condition)
  if ( ( currentMillis - previousMillisScale ) > 1000 && currentWeight - previousWeight > 0.5 ){ 
    previousMillisScale = currentMillis; //reset break timer
    previousWeight = currentWeight;
    scaleView();
    tft.fillScreen(TFT_BLACK);
    previousWeight = 0;
    scale.tare();
  }
}

void scaleView() {

  previousWeight = -1;
  scale.tare();
  previousMillisScale = millis();
  shotTimerMillis = millis();

  tft.fillScreen(TFT_BLACK);
  // graph
  int maxShotTime = 30000; //30s
  float maxShotWeight = 40;
  int graphYPposition = 240 - YRES; // to be at the bottom
  tft.fillRect(0, graphYPposition, XRES, YRES, TFT_BLACK);
  tft.drawRect(0, graphYPposition, XRES, YRES, TFT_WHITE);
  int shotXpos = 0;
  int shotYpos = 0;
  tft.drawString("40g", 5, (241-YRES), 2);
  tft.drawString("0", 5, 220, 2);
  tft.drawString("30s", 110, 220, 2);
  
  while(true) {
    currentMillis = millis();
    currentWeight = scale.get_units();
    if ( currentWeight < 0 ) currentWeight = 0;

    //  check interval when weight is added, reset timerstopping(break condition)
    if ( ( currentMillis - previousMillisScale ) > 500 && currentWeight - previousWeight > 0.5 ){ 
      previousMillisScale = currentMillis; //reset break timer
      previousWeight = currentWeight;
    }
    //  escape after weight is same for scaleInterval for TODO: add tolerance!!
    if (currentMillis - previousMillisScale > SCALE_INTERVAL) { 
      break; // exit loop
    }

    // draw time and weight
    xpos = 5;
    ypos = 0;

    intstr = String( (currentMillis - shotTimerMillis) / 1000 );
    if ( ( ( currentMillis - shotTimerMillis)  / 1000 ) < 10) {
      intstr = "0" + intstr; //add zero to keep it 2 digits
    }
    tft.drawString(intstr, xpos, ypos, 7);
    ypos = ypos + 50;
    
    intstr = String(currentWeight);
    if ( currentWeight < 1 ) intstr = "0" + intstr;
    tft.drawString(intstr, xpos, ypos, 7);


    // plot time and weight
    // draw just ONE pixel - cheap version
    shotXpos = XRES * float( float( currentMillis - shotTimerMillis ) / maxShotTime );
    if ( shotXpos > XRES ) break;
    shotYpos =  240 - ( float( currentWeight / maxShotWeight ) * YRES ); // 240 = display height
    
    //shotYpos = 200;
    tft.drawPixel(shotXpos, shotYpos, TFT_GREEN);
    
    //intstr = String(shotXpos) + " : " + String(shotYpos);
    //tft.drawString(intstr, 10, 180, 2);
  }
}
