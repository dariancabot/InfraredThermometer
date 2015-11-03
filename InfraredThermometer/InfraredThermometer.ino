#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRTemp.h>
#include <SPI.h>
#include <Wire.h>


/*
 ===============================================================================
 
 Infrared Thermometer
 Version:  0.5
 Build:    20151103
 
 Darian Cabot
 http://dariancabot.com/category/projects/infrared-thermometer/
 
 -------------------------------------------------------------------------------
 
 Hardware modules used:
 
 Freetronics IRTemp
 http://www.freetronics.com.au/products/irtemp-ir-temperature-sensor-module
 
 Adafruit SSD1306 OLED
 https://github.com/adafruit/Adafruit_SSD1306
 
 ===============================================================================
 */


// IRTemp configuration...
static const byte PIN_DATA    = 2;
static const byte PIN_CLOCK   = 3; // Must be either pin 2 or pin 3
static const byte PIN_ACQUIRE = 4;
static const TempUnit SCALE=CELSIUS; // Options are CELSIUS, FAHRENHEIT

IRTemp irTemp(PIN_ACQUIRE, PIN_CLOCK, PIN_DATA);


// OLED configuration...
// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


// Temperature variables...
int tMin;
int tMax;
int tSmooth;
float ftMin;
float ftMax;
float ftSmooth;
int count = 0;
int smoothCount = 0;

// Graph variables...
int gWidth = 128;
int gHeight = 39;
int gLeft = 30;
int gTop = 25;
int gXStep = gWidth / 64;
float gYMin = 0;
float gYMax = 100;
float oldGYMin = 0;
float oldGYMax = 100;
int oldX = gLeft;
int oldY;
int gRescale;


void setup()
{
  Serial.begin(19200);
  Serial.println("IR Temperature v0.5");
  Serial.println();

  display.begin(SSD1306_SWITCHCAPVCC); // Generate the high voltage from the 3.3v line internally.
  display.clearDisplay(); // clears the screen and buffer

  ftMin = irTemp.getIRTemperature(SCALE);
  ftMax = ftMin;
  ftSmooth = ftMin;

  tMin = round(ftMin * 100);
  tMax = tMin;
  tSmooth = tMin;

  oldY = gTop + gHeight / 2;
  
  count = gLeft;
}


void loop()
{

  display.fillRect(0, 0, display.width(), gTop - 1, BLACK);  // Clear big number.
  display.fillRect(0, gTop, gLeft - 1, gHeight, BLACK);  // Clear graph min/max.
  display.drawLine(0, gTop - 1, display.width(), gTop - 1, WHITE); // Draw graph top line.  

  float ftIr = irTemp.getIRTemperature(SCALE);
  int tIr = round(ftIr * 100.0f);

  Serial.println(ftIr);

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(20,0);
  display.print(ftIr, 2);

  boolean isMin = 0;
  boolean isMax = 0;
  boolean isSmooth = 0;

  if (!isnan(ftIr))
  {
    if (ftIr <= ftMin)
    {
      ftMin = ftIr;
      tMin = tIr;
      isMin = 1;
    }

    if (ftIr >= ftMax)
    {
      ftMax = ftIr;
      tMax = tIr;
      isMax = 1;
    }

    int tSmoothOld = tSmooth;
    ftSmooth = (tSmooth * 19.0f + tIr) / 20.0f; // Math needs to be done on float, otherwise it's too coarse.
    tSmooth = round(ftSmooth);

    // Do comparison at only 1 decimal place (same as display)
    if ((tSmoothOld / 10) == (tSmooth / 10))
    {
      smoothCount ++;
    }
    else
    {
      smoothCount = 0;
    }
  }

  // Print stats...

  display.setTextSize(1);
  display.setCursor(1, gTop + 2);

  if (isMax)
  {
    display.setTextColor(BLACK, WHITE); // 'inverted' text
    display.fillRect(0, gTop + 1, 25, 9, WHITE);
    display.print(tMax / 100.0f, 1);
    display.setTextColor(WHITE, BLACK);
  }
  else
  {
    display.print(tMax / 100.0f, 1);
  }


  display.setCursor(1, gTop + gHeight / 2 - 3);

  if (smoothCount > 19)
  {
    display.setTextColor(BLACK, WHITE); // 'inverted' text
    display.fillRect(0, gTop + gHeight / 2 - 4, 25, 9, WHITE);
    display.print(tSmooth / 100.0f, 1);
    display.setTextColor(WHITE, BLACK);
  }
  else
  {
    display.print(tSmooth / 100.0f, 1);
  }


  display.setCursor(1, gTop + gHeight - 8);

  if (isMin)
  {
    display.setTextColor(BLACK, WHITE); // 'inverted' text
    display.fillRect(0, gTop + gHeight - 9, 25, 9, WHITE);
    display.print(tMin / 100.0f, 1);
    display.setTextColor(WHITE, BLACK);
  }
  else
  {
    display.print(tMin / 100.0f, 1);
  }


  oldGYMin = gYMin;
  oldGYMax = gYMax;

  // GRAPH: Update graph axis...
  gYMin = ftMin;
  gYMax = ftMax;


  if (gRescale < 3)
  {
    display.fillRect(gLeft, gTop, gWidth, gHeight, BLACK);
    display.setCursor(gLeft + 8, gTop + gHeight / 2 - 4);
    display.print("Rescaling...");    
    gRescale ++;
    count = 1000; // force restart.
  }
  else if (gRescale == 3)
  {
    // Blank graph to remove 'rescaling' text, ready for drawing...
    display.fillRect(gLeft, gTop, gWidth, gHeight, BLACK);
    gRescale ++;
  }
  else
  {
    // GRAPH: Calculate X and Y values...
    oldX = count;
    count = count + gXStep;
    int gYVal = round(gTop + gHeight - 1 - (ftIr - gYMin) * ((gHeight - 1)  / (gYMax - gYMin)));

    // GRAPH: Draw...
    display.fillRect(oldX + 1, gTop, 7, gHeight, BLACK);
    display.drawLine(oldX, oldY, count, gYVal, WHITE);

    oldY = gYVal;

    // Mark span changes...
    if ((gYMax - gYMin) > (oldGYMax - oldGYMin))
    {
      gRescale = 0;
    }

    // Check if edge of screen reached, and wrap if required...
    if (count >= gWidth)
    {
      count = gLeft;
      oldX = gLeft;
      display.fillRect(gLeft, gTop, (gXStep * 4), gHeight, BLACK);
    }
  }

  display.display();
}

