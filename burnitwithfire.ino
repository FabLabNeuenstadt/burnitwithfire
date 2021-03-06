/*
Originally written by Gilad Dayagi for Rainbowduino and Colorduino
Link to original code:
https://github.com/giladaya/arduino-led-matrix/blob/master/fire/fire.ino
Code modified by Andy Modla to work with Adafruit.com NeoPixel NeoMatrix 8x8 - 64 RGB LED Pixel Matrix
http://www.adafruit.com/products/1487
compiled with Arduino 1.0.5 for neopixel library support
*/
 
#include <Adafruit_NeoPixel.h>
#define N_PIXELS  76  // Number of pixels
 
#define M_WIDTH 8  // matrix width
#define M_HEIGHT 9 // matrix height
#define LED_PIN 2  // NeoPixel LED strand data
#define BUTTON_PIN 4 // Our button

unsigned char reduce = 4; // used to reduce brightness
bool high_flame = false;

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} ColorRGB;
 
//a color with 3 components: h, s and v
typedef struct
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} ColorHSV;
 
//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[M_WIDTH][M_HEIGHT]={
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
    {32, 64 , 64 , 32 , 32 , 64 , 64 , 32, 32},
};

//these are the hues for the fire,
//should be between 0 (red) to about 13 (yellow)
const unsigned char hueMask[M_WIDTH][M_HEIGHT]={
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
    {3, 3, 3, 3, 3, 3, 3, 3, 3},
};
 
// The origin of the screen, (0,0) is at the bottom left corner.
// Neopixel has origin at top of screen left corner
// This difference accounted for in setPixel() function below
 
Adafruit_NeoPixel  strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
unsigned char matrix[M_WIDTH][M_HEIGHT];
unsigned char line[M_WIDTH];
int pcnt = 0;
 
//Converts an HSV color to RGB color
void HSVtoRGB(void *vRGB, void *vHSV)
{
  float r, g, b, h, s, v; //this function works with floats between 0 and 1
  float f, p, q, t;
  int i;
  ColorRGB *colorRGB=(ColorRGB *)vRGB;
  ColorHSV *colorHSV=(ColorHSV *)vHSV;
 
  h = (float)(colorHSV->h / 256.0);
  s = (float)(colorHSV->s / 256.0);
  v = (float)(colorHSV->v / 256.0);
 
  //if saturation is 0, the color is a shade of grey
  if(s == 0.0) {
    b = v;
    g = b;
    r = g;
  }
  //if saturation > 0, more complex calculations are needed
  else
  {
    h *= 6.0; //to bring hue to a number between 0 and 6, better for the calculations
    i = (int)(floor(h)); //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
    f = h - i;//the fractional part of h
 
    p = (float)(v * (1.0 - s));
    q = (float)(v * (1.0 - (s * f)));
    t = (float)(v * (1.0 - (s * (1.0 - f))));
 
    switch(i)
    {
      case 0: r=v; g=t; b=p; break;
      case 1: r=q; g=v; b=p; break;
      case 2: r=p; g=v; b=t; break;
      case 3: r=p; g=q; b=v; break;
      case 4: r=t; g=p; b=v; break;
      case 5: r=v; g=p; b=q; break;
      default: r = g = b = 0; break;
    }
  }
  colorRGB->r = (int)(r * 255.0);
  colorRGB->g = (int)(g * 255.0);
  colorRGB->b = (int)(b * 255.0);
}
 
// The origin of the screen, (0,0) is at the bottom left corner in colorduino.
// Neopixel has origin at top of screen left corner
void setPixel(unsigned char x, unsigned char y, unsigned char colorR, unsigned char colorG, unsigned char colorB){
//#if BOARD == 'c'
//    Colorduino.SetPixel(x, y, colorR, colorG, colorB);
//#else
//    Rb.setPixelXY(M_HEIGHT-y-1, x, colorR, colorG, colorB);
//#endif
 
// shift origin and adjust white balance (by adding more green to show yellow)
 // original: strip.setPixelColor(M_WIDTH*(M_HEIGHT-y-1) + x, colorR/reduce, 5*colorG/reduce, colorB/reduce);
    strip.setPixelColor(M_WIDTH*(y+1) + x, colorR/reduce, 5*colorG/reduce, colorB/reduce);
}
 
/**
 * Randomly generate the next line (matrix row)
 */
void generateLine(){
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      line[x] = random(64, 255);
  }
}
 
/**
 * shift all values in the matrix up one row
 */
void shiftUp(){
  ColorRGB colorRGB;
 
  for (unsigned char y=M_WIDTH-1;y>0;y--) {
    for(unsigned char x=0;x<M_HEIGHT;x++) {
        matrix[x][y] = matrix[x][y-1];
    }
  }
 
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      matrix[x][0] = line[x];
  }
}
 
/**
 * draw a frame, interpolating between 2 "key frames"
 * @param pcnt percentage of interpolation
 */
void drawFrame(int pcnt){
  ColorRGB colorRGB;
  ColorHSV colorHSV;
  int nextv;
 
  //each row interpolates with the one before it
  for (unsigned char y=M_WIDTH-1;y>0;y--) {
    for (unsigned char x=0;x<M_HEIGHT;x++) {
        colorHSV.h = hueMask[y][x];
        colorHSV.s = 255;
        unsigned char mask = valueMask[y][x];
        if (high_flame) {
            mask = 0;
            }
             else { mask += 64; }
        nextv =
            (((100.0-pcnt)*matrix[x][y]
          + pcnt*matrix[x][y-1])/100.0)
          - mask;
        colorHSV.v = (unsigned char)max(0, nextv);
       
        HSVtoRGB(&colorRGB, &colorHSV);
        setPixel(x, y, colorRGB.r, colorRGB.g, colorRGB.b);
    }
  }
 
  //first row interpolates with the "next" line
  for(unsigned char x=0;x<M_HEIGHT;x++) {
      colorHSV.h = hueMask[0][x];
      colorHSV.s = 255;
      colorHSV.v = (unsigned char)(((100.0-pcnt)*matrix[x][0] + pcnt*line[x])/100.0);
     
      HSVtoRGB(&colorRGB, &colorHSV);
      setPixel(x, 0, colorRGB.r, colorRGB.g, colorRGB.b);
  }
  strip.show();
}
 
void setup()
{
  pinMode(BUTTON_PIN, INPUT);
//#if BOARD == 'c'    
//  Colorduino.Init(); // initialize the board
//  
//  // compensate for relative intensity differences in R/G/B brightness
//  // array of 6-bit base values for RGB (0~63)
//  // whiteBalVal[0]=red
//  // whiteBalVal[1]=green
//  // whiteBalVal[2]=blue
//  unsigned char whiteBalVal[3] = {36,63,7}; // for LEDSEE 6x6cm round matrix
//  Colorduino.SetWhiteBal(whiteBalVal);
//#else
//    Rb.init();
//#endif
  strip.begin();
  strip.show();
 
  randomSeed(analogRead(0));
  generateLine();
 
  //init all pixels to zero
  for (unsigned char y=0;y<M_WIDTH;y++) {
    for(unsigned char x=0;x<M_HEIGHT;x++) {
        matrix[x][y] = 0;
    }
  }
}
 
void loop()
{
    const int BUTTON_STATE = digitalRead(BUTTON_PIN);
    if (BUTTON_STATE == HIGH)
        high_flame = true;
    else
        high_flame = false;

    if (pcnt >= 100){
        delay(30);
        shiftUp();
        generateLine();
        pcnt = 0;
    }
    drawFrame(pcnt);
//#if BOARD == 'c'    
//    Colorduino.FlipPage(); // swap screen buffers to show it
//#endif
    pcnt+=30;
}
