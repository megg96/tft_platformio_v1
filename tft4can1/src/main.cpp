//----------------------------------BIBLIOTEKI---------------------------------------------------------------
#include "mbed.h"
#include "Adafruit_ST7735.h"
#include "Adafruit_GFX.h"

//----------------------------------DEFINICJE---------------------------------------------------------------
#define ce    PB_1    //chip select
#define dc    PB_2    //data/command
#define mosi  PB_15   //mosi
#define sclk  PC_7    //clock
#define miso  PB_14   //miso, nieuzywane
#define rst   PC_4    //reset

#define GREEN2RED 4
#define BLUE2BLUE 2

#define ST7735_GREY   0x2104

#define DKBLUE        0x000D
#define DKTEAL        0x020C
#define DKGREEN       0x03E0
#define DKCYAN        0x03EF
#define DKRED         0x6000
#define DKMAGENTA     0x8008
#define DKYELLOW      0x8400
#define DKORANGE      0x8200
#define DKPINK        0x9009
#define DKPURPLE      0x4010
#define DKGREY        0x4A49

//----------------------------------ZMIENNE---------------------------------------------------------------
Adafruit_ST7735 screen(mosi, miso, sclk, ce, dc, rst);   //stworzenie obiektu klasy ekranu

typedef unsigned char byte;     //w mbed nie ma zmiennej byte, zastepuje sie ja zmienna char

int H = ST7735_TFTWIDTH;        //wysokosc lc, wymiary sa pozamieniane bo wyswietlacz oryginalnie ma poczatek w pionie
int W = ST7735_TFTHEIGHT;       //szerokosc lcd
int lastmapp = 0;               //poprzednia wartosc predkosci

char *p,  buf[1];               //tablice uzywane do poprawnego wyswietlania cyfr(funkcja sprintf)
char buf1[2], buf2[2], buf3[2];

boolean redrawDial = true;      //zmienne ktore okreslaja czy funkcja ma rysowac caly wykres czy tylko zaktualizowac obraz o odpowiednie dane
boolean redrawBarV1 = true;
boolean redrawBarV2 = true;
boolean redrawBarH = true;

float curval = 7;               //zmienna testowa

//----------------------------------DEKLARACJA FUNKCJI------------------------------------------------------
long map(long x, long in_min, long in_max, long out_min, long out_max);  //przeskalowanie wielkosci zmiennych
void base(Adafruit_ST7735 screen);                                       //baza graficzna, nie zmienia sie
void DrawDial(Adafruit_ST7735 & d, int cx, int cy, int r, float loval , float hival , float inc, float sa, float curval, unsigned int needlecolor, unsigned int dialcolor, unsigned int  textcolor, char label[], boolean & redraw);  //funkcja rysujaca zegar
void DrawBarChartV(Adafruit_ST7735 & d, float x , float y , float w, float h , float loval , float hival , float inc , float curval , unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, char label[], boolean & redraw); //funkcja rysujaca pionowy pasek
void DrawBarChartH(Adafruit_ST7735 & d, float x , float y , float w, float h, float loval, float hival, float inc, float curval, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, char label[], boolean & redraw); //funkcja rysujaca poziomy pasek
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme); //funkcja rysujaca pasek kolowy
unsigned int rainbow(byte value); //funkcja liczaca gradient kolorow
float sineWave(int phase); //funkcja sinusa

//----------------------------------SETUP-------------------------------------------------------------------
int main() {  //setup 
    
  Serial pc(USBTX, USBRX);            // tx, rx
    
  screen.initR(INITR_BLACKTAB);       //inicjalizacja ekranu
  wait_ms(100);
  screen.setRotation(3);              //orientacja ekranu
  screen.setTextSize(1);              //wybor wielkosci fontu
  screen.setTextColor(WHITE, BLACK);  //ustawienie kolorow tekstu(i tla)
  screen.setTextWrap(true);
  screen.fillScreen(BLACK);           //wypelnienie calego ekranu kolorem
  screen.setCursor(0, 0);             //ustawienie kursora w wybrane miejsce

  DigitalOut led2(LED2);              //LED
  led2 = true;

  CAN can1(PB_12, PB_13);             //CAN
  CANMessage msg;
  can1.frequency(1000000);

//----------------------------------LOOP--------------------------------------------------------------------
  while(1) {  //main loop

    //Do testow zostaly przygotowe zestawy wykresow, wystarczy je skomentowac/odkomentowac

    /*//zestaw 1
    DrawDial(screen, W/2, H/2, 40, 0, 10, 1, 240, curval, RED, WHITE, BLACK, "Meas", redrawDial);
    DrawBarChartV(screen, 10,  100, 10, 90, 0, 10 , 1, 3, BLUE, DKBLUE, BLUE, WHITE, BLACK, "V", redrawBarV1);
    DrawBarChartV(screen, 125,  100, 10, 90, 0, 10 , 1, 5, RED, DKYELLOW, RED, WHITE, BLACK, "A", redrawBarV2);
    */

    //zestaw 2
    DrawBarChartH(screen, 15, 100, 130, 10, -8, 8, 2, 2.5, GREEN, DKGREEN, GREEN, WHITE, BLACK, "Offset", redrawBarH);
    wait_ms(33);  //tu ustala sie jak szybko ma sie odswiezac grafiki
    
    //CAN
    if(can1.read(msg))
    {
      int i=msg.data[0];
      can1.write(CANMessage(1337, msg.data,1));
    }
    
  }
}
//----------------------------------FUNKCJE---------------------------------------------------------------
long map(long x, long in_min, long in_max, long out_min, long out_max)     //przeskalowywuje dane z jednego zakresu na drugi
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme)  //draw the meter on the screen, returns x coord of righthand side
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  x += r; y += r;   // Calculate coords of centre of ring
  int w = r / 4;    // Width of outer ring is 1/4 of radius
  int angle = 150;  // Half the sweep angle of meter (300 degrees)
  int text_colour = 0; // To hold the text colour
  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 5; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc) {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme) {
      case 0: colour = ST7735_RED; break; // Fixed colour
      case 1: colour = ST7735_GREEN; break; // Fixed colour
      case 2: colour = ST7735_BLUE; break; // Fixed colour
      case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
      case 4: colour = rainbow(map(i, -angle, angle, 63, 127)); break; // Green to red (high temperature etc)
      case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
      default: colour = ST7735_BLUE; break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      screen.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      screen.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      screen.fillTriangle(x0, y0, x1, y1, x2, y2, ST7735_GREY);
      screen.fillTriangle(x1, y1, x2, y2, x3, y3, ST7735_GREY);
    }
  }

  // Convert value to a string
  char buf[10];
  byte len = 4; if (value > 999) len = 5;
  sprintf(buf, "%i", value);
  p = buf;
  screen.print(p);

  // Set the text colour to default
  screen.setTextColor(ST7735_WHITE, ST7735_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  // screen.setTextColor(text_colour, ST7735_BLACK);
  
  // Print value, if the meter is large then use big font 6, othewise use 4
  //if (r > 84) screen.drawCentreString(buf, x - 5, y - 20, 6); // Value in middle
  //else screen.drawCentreString(buf, x - 5, y - 20, 4); // Value in middle

  // Print units, if the meter is large then use big font 4, othewise use 2
  screen.setTextColor(ST7735_WHITE, ST7735_BLACK);
  //if (r > 84) screen.drawCentreString(units, x, y + 30, 4); // Units display
  //else screen.drawCentreString(units, x, y + 5, 2); // Units display

  // Calculate and return right hand side x coordinate
  return x + r;
}


unsigned int rainbow(byte value)  //return a 16 bit rainbow colour
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}


float sineWave(int phase) {         //return a value in range -1 to +1 for a given phase angle in degrees
  return sin(phase * 0.0174532925);
}


void DrawBarChartV(Adafruit_ST7735 & d, float x , float y , float w, float h, float loval, float hival, float inc, float curval, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, char *label, boolean & redraw)
{
  /*
  This method will draw a vertical bar graph for single input
  it has a rather large arguement list and is as follows

  &d = display object name
  x = position of bar graph (lower left of bar)
  y = position of bar (lower left of bar
  w = width of bar graph
  h =  height of bar graph (does not need to be the same as the max scale)
  loval = lower value of the scale (can be negative)
  hival = upper value of the scale
  inc = scale division between loval and hival
  curval = date to graph (must be between loval and hival)
  barcolor = color of bar graph
  voidcolor = color of bar graph background
  bordercolor = color of the border of the graph
  textcolor = color of the text
  backcolor = color of the bar graph's background
  label = bottom lable text for the graph
  redraw = flag to redraw display only on first pass (to reduce flickering)
  */
  float stepval, range;
  float my, level;
  float i, data;
  // draw the border, scale, and label once
  // avoid doing this on every update to minimize flicker
  if (redraw == true) {
    redraw = false;
    d.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
    d.setTextColor(textcolor, backcolor);
    d.setTextSize(2);
    d.setCursor(x, y + 10);
    d.print(label);
    // step val basically scales the hival and low val to the height
    // deducting a small value to eliminate round off errors
    // this val may need to be adjusted
    stepval = ( inc) * (float (h) / (float (hival - loval))) - .001;
    for (i = 0; i <= h; i += stepval) {
      my =  y - h + i;
      d.drawFastHLine(x + w + 1, my,  5, textcolor);
      // draw lables
      d.setTextSize(1);
      d.setTextColor(textcolor, backcolor);
      d.setCursor(x + w + 12, my - 3 );
      data = hival - ( i * (inc / stepval));
      sprintf(buf2, "%1.0f", data);
      p = buf2;
      d.print(p);
    }
  }
  // compute level of bar graph that is scaled to the  height and the hi and low vals
  // this is needed to accompdate for +/- range
  level = (h * (((curval - loval) / (hival - loval))));
  // draw the bar graph
  // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
  d.fillRect(x, y - h, w, h - level,  voidcolor);
  d.fillRect(x, y - level, w,  level, barcolor);
  // write the current value
  d.setTextColor(textcolor, backcolor);
  d.setTextSize(2);
  d.setCursor(x , y - h - 23);
  sprintf(buf3, "%1.0f", curval);
  p = buf3;
  //d.print(p);

}


void DrawDial(Adafruit_ST7735 & d, int cx, int cy, int r, float loval , float hival , float inc, float sa, float curval, unsigned int needlecolor, unsigned int dialcolor, unsigned int  textcolor, char label[], boolean & redraw) {
  /*
  This method will draw a dial-type graph for single input
  it has a rather large arguement list and is as follows

  &d = display object name
  cx = center position of dial
  cy = center position of dial
  r = radius of the dial
  loval = lower value of the scale (can be negative)
  hival = upper value of the scale
  inc = scale division between loval and hival
  sa = sweep angle for the dials scale
  curval = date to graph (must be between loval and hival)
  needlecolor = color of the needle
  dialcolor = color of the dial
  textcolor = color of all text (background is dialcolor)
  label = bottom lable text for the graph
  redraw = flag to redraw display only on first pass (to reduce flickering)
  */
  float ix, iy, ox, oy, tx, ty, lx, rx, ly, ry, i, offset, stepval, data, angle;
  float degtorad = .0174532778;
  static float pix = cx, piy = cy, plx = cx, ply = cy, prx = cx, pry = cy;
  // draw the dial only one time--this will minimize flicker
  if ( redraw == true) {
    redraw = false;
    d.fillCircle(cx, cy, r - 2, dialcolor);
    d.drawCircle(cx, cy, r, needlecolor);
    d.drawCircle(cx, cy, r - 1, needlecolor);
    d.setTextColor(dialcolor, textcolor);
    d.setTextSize(2);
    d.setCursor(cx - 23, cy + 45);
    d.print(label);
  }
  // draw the current value
  d.setTextSize(2);
  d.setTextColor(textcolor, dialcolor);
  d.setCursor(cx - 3, cy + 17 );
  sprintf(buf, "%1.0f", curval);
  p = buf;
  d.print(p);
  // center the scale about the vertical axis--and use this to offset the needle, and scale text
  offset = (270 +  sa / 2) * degtorad;
  // find the scale step value based on the hival low val and the scale sweep angle
  // deducting a small value to eliminate round off errors
  // this val may need to be adjusted
  stepval = ( inc) * (float (sa) / (float (hival - loval))) + .00;
  // draw the scale and numbers
  // note draw this each time to repaint where the needle was
  for (i = 0; i <= sa; i += stepval) {
    angle = ( i  * degtorad);
    angle = offset - angle ;
    ox =  (r - 2) * cos(angle) + cx;
    oy =  (r - 2) * sin(angle) + cy;
    ix =  (r - 7) * cos(angle) + cx;
    iy =  (r - 7) * sin(angle) + cy;
    tx =  (r - 13) * cos(angle) + cx;
    ty =  (r - 13) * sin(angle) + cy;
    d.drawLine(ox, oy, ix, iy, textcolor);
    d.setTextSize(1);
    d.setTextColor(textcolor, dialcolor);
    d.setCursor(tx - 2, ty - 3);
    data = hival - ( i * (inc / stepval));
    sprintf(buf2, "%1.0f", data);
    p = buf2;
    d.print(p);
  }
  // compute and draw the needle
  angle = (sa * (1 - (((curval - loval) / (hival - loval)))));
  angle = angle * degtorad;
  angle = offset - angle;
  ix =  (r - 10) * cos(angle) + cx;
  iy =  (r - 10) * sin(angle) + cy;
  // draw a triangle for the needle (compute and store 3 vertiticies)
  lx =  5 * cos(angle - 90 * degtorad) + cx;
  ly =  5 * sin(angle - 90 * degtorad) + cy;
  rx =  5 * cos(angle + 90 * degtorad) + cx;
  ry =  5 * sin(angle + 90 * degtorad) + cy;
  // first draw the previous needle in dial color to hide it
  d.fillTriangle (pix, piy, plx, ply, prx, pry, dialcolor);
  d.drawTriangle (pix, piy, plx, ply, prx, pry, dialcolor);
  // then draw the old needle in need color to display it
  d.fillTriangle (ix, iy, lx, ly, rx, ry, needlecolor);
  d.drawTriangle (ix, iy, lx, ly, rx, ry, textcolor);
  // draw a cute little dial center
  d.fillCircle(cx, cy, 6, textcolor);
  //save all current to old so the previous dial can be hidden
  pix = ix;
  piy = iy;
  plx = lx;
  ply = ly;
  prx = rx;
  pry = ry;
}


void DrawBarChartH(Adafruit_ST7735 & d, float x , float y , float w, float h, float loval, float hival, float inc, float curval, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, char label[], boolean & redraw)
{
  /*
  This method will draw a horizontal bar graph for single input
  it has a rather large arguement list and is as follows

  &d = display object name
  x = position of bar graph (upper left of bar)
  y = position of bar (upper left of bar (add some vale to leave room for label)
  w = width of bar graph (does not need to be the same as the max scale)
  h =  height of bar graph
  loval = lower value of the scale (can be negative)
  hival = upper value of the scale
  inc = scale division between loval and hival
  curval = date to graph (must be between loval and hival)
  dig = format control to set number of digits to display (not includeing the decimal)
  dec = format control to set number of decimals to display (not includeing the decimal)
  barcolor = color of bar graph
  voidcolor = color of bar graph background
  bordercolor = color of the border of the graph
  textcolor = color of the text
  back color = color of the bar graph's background
  label = bottom lable text for the graph
  redraw = flag to redraw display only on first pass (to reduce flickering)
*/
  float stepval, range;
  float mx, level;
  float i, data;
  // draw the border, scale, and label once
  // avoid doing this on every update to minimize flicker
  // draw the border and scale
  if (redraw == true) {
    redraw = false;
    d.drawRect(x , y , w, h, bordercolor);
    d.setTextColor(textcolor, backcolor);
    d.setTextSize(2);
    d.setCursor(x , y - 20);
    d.print(label);
    // step val basically scales the hival and low val to the width
    stepval =  inc * (float (w) / (float (hival - loval))) - .00001;
    // draw the text
    for (i = 0; i <= w; i += stepval) {
      d.drawFastVLine(i + x , y + h + 1,  5, textcolor);
      // draw lables
      d.setTextSize(1);
      d.setTextColor(textcolor, backcolor);
      d.setCursor(i + x - 8 , y + h + 10);
      // addling a small value to eliminate round off errors
      // this val may need to be adjusted
      data =  ( i * (inc / stepval)) + loval + 0.00001;
      sprintf(buf2, "%2.0f", data);
      p = buf2;
      d.print(p);
    }
  }
  // compute level of bar graph that is scaled to the width and the hi and low vals
  // this is needed to accompdate for +/- range capability
  // draw the bar graph
  // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
  level = (w * (((curval - loval) / (hival - loval))));
  d.fillRect(x + level + 1, y + 1, w - level - 2, h - 2,  voidcolor);
  d.fillRect(x + 1, y + 1 , level - 1,  h - 2, barcolor);
  // write the current value
  d.setTextColor(textcolor, backcolor);
  d.setTextSize(2);
  d.setCursor(x + w + 10 , y + 5);
  sprintf(buf3, "%2.0f", curval);
  p = buf3;
}
//----------------------------------KONIEC---------------------------------------------------------------------