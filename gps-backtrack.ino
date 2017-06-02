
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NMEAGPS.h>
#include <NeoSWSerial.h>

// GPS
#define RX_PIN 0
#define TX_PIN 1

// OLED
#define OLED_RESET 4
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#include "GPSport.h"
#include "Streamers.h"

#ifdef NeoHWSerial_h
#define DEBUG_PORT NeoSerial
#else
#define DEBUG_PORT Serial
#endif

static NMEAGPS  gps;
static gps_fix  fix;

Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   {
    Serial.begin(9600);
    gps_port.begin( 9600 );
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    display.setTextSize(0);
    display.setTextColor(WHITE);
}

void display_line1 (float lat, int heading){
    char* lat_prefix;

    if (lat < 0){
        lat = fabsf(lat);
        lat_prefix = "S:";
    }
    else {
        lat_prefix = "N:";
    }

    display.print(lat_prefix);
    display.print(lat, 7);
    display.print(" H:");
    display.println(heading);
}

void display_line2 (float lon){
    char* lon_prefix;

    if (lon < 0){
        lon = fabsf(lon);
        lon_prefix = "W:";
    }
    else {
        lon_prefix = "E:";
    }

    display.print(lon_prefix);
    display.println(lon, 7);
}

void display_line3(int alt, float speed){
    display.print("A:");
    display.print(alt);
    display.print(" S:");
    display.print(speed);

}

void loop() {

    while (gps.available( gps_port )) {
        fix = gps.read();

        display.setCursor(0, 0);
        display.clearDisplay();

        String date = String(fix.dateTime.year + "-" + fix.dateTime.month);

        display.println(date);
        //display.println("-" + fix.dateTime.month);

        display_line1(fix.latitude(), fix.heading());
        display_line2(fix.longitude());
        display_line3(fix.alt.whole, fix.speed_kph());

        display.display();
    }
}
