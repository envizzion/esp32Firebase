#include <FastLED.h>
#include <DebounceEvent.h>
#include <EasyButton.h>

#include "EEPROM.h"
#define EEPROM_SIZE 128
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>




//-----------------configration variables---------------------------
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool receveivedNewBleData = false;
boolean inBleMode = false;
unsigned long wifiTimeoutLimit=10000;
unsigned long wifiTimeout=0;
unsigned long bleTimeoutLimit=30000;
unsigned long bleTimeout=0;
unsigned long reconnectCheckTimeLimit = 500;
unsigned long reconnectCheckTime = 0;
 


//const int ledPin = 22;
const int modeAddr = 0;
const int wifiAddr = 10;
const int hasCredAddr = 5;
int modeIdx;
int hasCredentials;
//---------------------------------------------------------------------


//--------firebase variables-------------------------------------------

#define FIREBASE_HOST "jesus-light.firebaseio.com"
#define FIREBASE_AUTH "JXL1ken4JNvM4dAKpzDXTtTbkkasoYhRCwp6URxA"
FirebaseData firebaseData1;
unsigned long sendDataPrevMillis = 0;
String path = "/devices/1234";
uint16_t count = 0;
bool firebaseActive = false;
void printResult(FirebaseData &data);
void printResult(StreamData &data);

//----------------------------------------------------------------------


//---------------------LED variables---------------------------------

#define NumPixels 98       // number of LEDs in strip - count starts at 0, not 1
#define DataRate_Mhz 4     // how fast data refreshes at - [----CHECK THIS----] - slower rates are more successful when timing is not essential!!!!!
#define DataPin 13        // data pin
#define ClockPin 14        // clock Pin

#define CUSTOM_DEBOUNCE_DELAY   50 //Define dutton debounce delay
#define CUSTOM_REPEAT_DELAY     500 // Time the library waits for a second (or more) clicks, set to 0 to disable double clicks but get a faster response

#define UPDATES_PER_SECOND 100
//------------------------------------------------------------------------


void ChangeLightScene(uint8_t);
void InitializeGIPOs();
void TurnOn();
void Rainbow();
void ChangeBrightness();
void TurnOff();
void IlluminateCenterCross(int, int);
void IlluminateCross(int, int);

int hue(int h){
  int hue = h*256/360; 
  return hue;
}

int val(int v){
  int val = v*255/100;
  return val;
}

const uint8_t buttonPin = 15; // Pin defined for button

//Before changing full colour, try increasing Brightness value

//Here we define Warm White Light Palette
CRGBPalette16 WarmWhiteLight_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(59),val(37),val(100))
};

//Here we define Natural White Light Palette
CRGBPalette16 NaturalWhiteLight_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(0),val(0),val(100))
};

//Here we define CozyCandle Palette
CRGBPalette16 CozyCandle_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(3),val(85),val(95)),
  CHSV(hue(9),val(85),val(94)),
  CHSV(hue(13),val(85),val(94)),
  CHSV(hue(13),val(85),val(94)),
  
  CHSV(hue(16),val(86),val(95)),
  CHSV(hue(16),val(86),val(95)),
  CHSV(hue(19),val(86),val(95)),
  CHSV(hue(19),val(86),val(95)),
  
  CHSV(hue(25),val(87),val(95)),
  CHSV(hue(25),val(87),val(95)),  
  CHSV(hue(27),val(87),val(96)),
  CHSV(hue(27),val(87),val(96)),
  
  CHSV(hue(30),val(87),val(96)),
  CHSV(hue(33),val(88),val(97)),
  CHSV(hue(33),val(88),val(97)),
  CHSV(hue(36),val(89),val(98))
};

//Here we define SundayCoffee Palette
CRGBPalette16 SundayCoffee_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(21),val(55),val(97)),
  CHSV(hue(15),val(46),val(97)),
  CHSV(hue(8),val(36),val(97)),
  CHSV(hue(8),val(36),val(97)),
  
  CHSV(hue(358),val(30),val(96)),
  CHSV(hue(358),val(30),val(96)),
  CHSV(hue(335),val(28),val(96)),
  CHSV(hue(335),val(28),val(96)),
  
  CHSV(hue(321),val(27),val(87)),
  CHSV(hue(319),val(27),val(87)),
  CHSV(hue(296),val(23),val(78)),
  CHSV(hue(275),val(27),val(77)),
  
  CHSV(hue(275),val(27),val(77)),  
  CHSV(hue(260),val(29),val(75)),
  CHSV(hue(244),val(31),val(74)),
  CHSV(hue(244),val(31),val(74)),
};

//Here we define Meditation Palette
CRGBPalette16 Meditation_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(130),val(71),val(71)),
  CHSV(hue(130),val(71),val(71)),
  CHSV(hue(145),val(74),val(71)),
  CHSV(hue(164),val(84),val(72)),
  
  CHSV(hue(178),val(99),val(72)),
  CHSV(hue(178),val(99),val(72)),
  CHSV(hue(188),val(95),val(73)),
  CHSV(hue(188),val(95),val(73)),
  
  CHSV(hue(202),val(99),val(78)),
  CHSV(hue(202),val(99),val(78)),
  CHSV(hue(203),val(95),val(76)),
  CHSV(hue(203),val(95),val(76)),
  
  CHSV(hue(221),val(53),val(73)),  
  CHSV(hue(241),val(37),val(71)),
  CHSV(hue(272),val(38),val(69)),
  CHSV(hue(272),val(38),val(69)),
};

//Here we define Enchanted Forest Palette
CRGBPalette16 EnchantedForest_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(64),val(84),val(87)),
  CHSV(hue(64),val(84),val(87)),
  CHSV(hue(70),val(77),val(84)),
  CHSV(hue(70),val(77),val(84)),
  
  CHSV(hue(79),val(70),val(80)),
  CHSV(hue(79),val(70),val(80)),
  CHSV(hue(90),val(67),val(76)),
  CHSV(hue(108),val(63),val(73)),
  
  CHSV(hue(108),val(63),val(73)),
  CHSV(hue(137),val(79),val(68)),
  CHSV(hue(137),val(79),val(68)),
  CHSV(hue(144),val(92),val(65)),
  
  CHSV(hue(149),val(99),val(62)),  
  CHSV(hue(149),val(99),val(62)),
  CHSV(hue(150),val(99),val(59)),
  CHSV(hue(151),val(99),val(56)),
};

//Here we define Night Adventure Palette
CRGBPalette16 NightAdventure_p =
{
  //Here you can enter HUE value in degrees. Saturation and Brightness in %
  CHSV(hue(238),val(68),val(57)),
  CHSV(hue(238),val(68),val(57)),
  CHSV(hue(268),val(61),val(59)),
  CHSV(hue(268),val(61),val(59)),
  
  CHSV(hue(289),val(55),val(61)),
  CHSV(hue(310),val(53),val(68)),
  CHSV(hue(321),val(59),val(78)),
  CHSV(hue(321),val(59),val(78)),
  
  CHSV(hue(331),val(64),val(94)),
  CHSV(hue(331),val(64),val(94)),
  CHSV(hue(343),val(79),val(68)),
  CHSV(hue(343),val(70),val(94)),
  
  CHSV(hue(349),val(78),val(93)),  
  CHSV(hue(353),val(81),val(93)),
  CHSV(hue(358),val(88),val(93)),
  CHSV(hue(358),val(88),val(93)),
};


CRGB leds[NumPixels];
CRGBPalette16 currentPalette(CozyCandle_p);
uint8_t i;
static uint8_t startPoint = 0;
static uint8_t paletteQuantityLength = 10; //value between 0 and 10, how much of the lights should be lit up at once, 0 being all
TBlendType blendingType; //tBlendType is a type of value like int/char/uint8_t etc., use to choose LINERBLEND and NOBLEND

DebounceEvent *Button;
EasyButton button(buttonPin);

bool turnedon = false;
bool canLava = false;
uint8_t currScene = 0, lastScene = 0, brightness = 2;

void setup()
{
//------------------------LED--------------------------
  delay(1000);                                      // saftey first  
  FastLED.addLeds<APA102,DataPin,ClockPin,BGR,DATA_RATE_MHZ(DataRate_Mhz)>(leds,NumPixels).setCorrection(TypicalSMD5050);
  
  button.begin();
  button.onPressedFor(5000, TurnOff); // Add the callback function to be called when the button is pressed for at least the given time.
  Button = new DebounceEvent(buttonPin, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP, CUSTOM_DEBOUNCE_DELAY, CUSTOM_REPEAT_DELAY);
  FastLED.setBrightness(val(brightness));
  fill_solid(leds, NumPixels, CRGB(0,0,0));         // fill all black
  FastLED.show();                                   // show 
  Serial.begin(115200);                             // monitor speed
  blendingType = LINEARBLEND;                       // options are LINEARBLEND or NOBLEND - linear is 'cleaner
//-----------------------------------------------------

//----------------------WIRELESS-----------------------
 InitialWiFiCredentialCheckAndUpdate();
 initializeFirebase();
 bleTimeout = millis() + bleTimeoutLimit ; 
 wifiTimeout = millis() + wifiTimeoutLimit ; 
 reconnectCheckTime = reconnectCheckTimeLimit + millis(); 

//-----------------------------------------------------


}

void loop() 
{  
  button.read();

  if (unsigned int event = Button->loop()) {
    if (event == EVENT_RELEASED) {
      int buttonCount = Button->getEventCount();
      int timePressed = Button -> getEventLength();

      if(buttonCount == 1 && timePressed <= 1000 && turnedon == false){
        RunEffect();

        if(currScene-1 == -1){
          currScene = 0;
          ChangeLightScene(currScene);
        }
        ChangeLightScene(currScene-1);
        //Serial.print("CurrScene in loop: "); Serial.println(currScene);
        turnedon = true;
      
      }

      if(buttonCount == 1 && timePressed <= 1000 && turnedon){
        //Serial.println("Button clicked");
        if(currScene + 1 == 8){
          currScene = 0;
          ChangeLightScene(0);
        }else{
          ChangeLightScene(currScene+1);
        }
      }
        
      if(buttonCount == 2 && turnedon){
        ChangeBrightness();
      }
    }
  }

  startPoint = startPoint + 1;   //if it is 0, then it will all stay the same
  
  if(canLava && turnedon){
    fill_palette(leds, NumPixels, startPoint, 10, currentPalette, 20, blendingType);
    FastLED.show(); 
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
  reconnectWifi();
}

void RunEffect(){
  int h, v;
  
  switch(currScene){
    case 0:
      h = 59;
      v = 37;
      break;
    
    case 1:
      h = 0;
      v = 0;
      break;

    case 2:
      h = 22;
      v = 86;
      break;
    
    case 3:
      h = 335;
      v = 28;
      break;
    
    case 4:
      h = 188;
      v = 95;
      break;
    
    case 5:
      h = 108;
      v = 63;
      break;
    
    case 6:
      h = 321;
      v = 59;
      break;
    
    case 7:
      h = random8( 360);
      v = random8();
      break;
  }
  
  IlluminateCenterCross(hue(h),val(v));
  IlluminateCross(hue(h),val(v));
  canLava = true;
}

void IlluminateCenterCross(int h, int s){
  for(int j = 0; i < 3; i++){
    for(int i = 0; i < 255; i++){
      leds[53].setHSV( h, s, i);
      FastLED.show();  
    }
    for(int i = 255; i > 0; i--){
      leds[53].setHSV( h, s, i);
      FastLED.show(); 
    }
  }
}

void IlluminateCross(int h, int s){

  for(int j = 0; j < 2; j++){
    for(int b = 0; b < 255; b++){
      for(int i = 24; i < 33; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 36; i < 43; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 49; i < 56; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 62; i < 69; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 72; i < 78; i++){
        leds[i].setHSV( h, s, b);
      }
      FastLED.show(); 
    }
  
    for(int b = 255; b > 0; b--){
      for(int i = 24; i < 33; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 36; i < 43; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 48; i < 56; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 62; i < 69; i++){
        leds[i].setHSV( h, s, b);
      }
      for(int i = 72; i < 78; i++){
        leds[i].setHSV( h, s, b);
      }
      FastLED.show(); 
    }
  }
}

void ChangeBrightness(){
  switch(brightness){
    case 100:
      brightness = 2;//You can introduce brightness in % Eg. 2%
      break;

    case 2:
      brightness = 10;//You can introduce brightness in % Eg. 10%
      break;

    case 10:
      brightness = 40;//You can introduce brightness in % Eg. 40%
      break;

    case 40:
      brightness = 100;//You can introduce brightness in % Eg. 100%
      break;
  }
  FastLED.setBrightness(val(brightness));
  FastLED.show();
}


void ChangeLightScene(uint8_t scene){
  Serial.print("\n");
  Serial.print("Scene: "); Serial.println(scene);
  switch(scene){
    //Warm White Light
    case 0:
      currentPalette = WarmWhiteLight_p;
      scene = 1;
      currScene = 0;
      Serial.println("Now it is Warm White");
      break;
      
    //Natural White Light
    case 1:
      currentPalette = NaturalWhiteLight_p;
      scene = 2;
      currScene = 1;
      Serial.println("Now it is Natural White");
      break;
    
    //Cozy Candle
    case 2:
      currentPalette = CozyCandle_p;
      scene = 3;
      currScene = 2;
      Serial.println("Now it is Cozy Candle");
      break;
      
    //Sunday Coffee
    case 3:
      currentPalette = SundayCoffee_p;
      scene = 4;
      currScene = 3;
      Serial.println("Now it is Sunday Coffee");
      break;
      
    //Meditation
    case 4:
      currentPalette = Meditation_p;
      scene = 5;
      currScene = 4;
      Serial.println("Now it is Meditation");
      break;
      
    //Enchanted Forest
    case 5:
      currentPalette = EnchantedForest_p;
      scene = 6;
      currScene = 5;
      Serial.println("Now it is Enchanted Forest");
      break;
      
    //Night Adventure
    case 6:
      currentPalette = NightAdventure_p;
      scene = 7;
      currScene = 6;
      Serial.println("Now it is Night Adventure");
      break;
    
    //Rainbow!
    case 7:
      currentPalette = RainbowColors_p;
      scene = 0;
      currScene = 7;
      Serial.println("Now it is Rainbow");
      break;
  }
}

void TurnOff(){
  fill_solid(leds, NumPixels, CRGB(0,0,0));         // fill all black
  turnedon = false;
  canLava = false;
  FastLED.show(); 
}
