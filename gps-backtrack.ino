#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <NeoSWSerial.h>
#include <NMEAGPS.h>
#include <SPI.h>
#include <Wire.h>

// GPS

#define RX_PIN 0
#define TX_PIN 1

#include "GPSport.h"
#include "Streamers.h"

// OLED

#define OLED_RESET 4
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const int SCREEN_BUTTON_PIN = 2;

static int screen_button_count = 0;

static NMEAGPS  gps;
static gps_fix  fix;

static int return_mode = 0; // manual toggle for return display
static int eeprom_read = 0; // has the saved coords been read

static float saved_lat;
static float saved_lon;

void  setup()   {
    Serial.begin(9600);
    gps_port.begin( 9600 );

    pinMode(SCREEN_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(0, screen_button, RISING);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    display.setTextSize(0);
    display.setTextColor(WHITE);
}
char* prefix_lat (float lat){
    char* lat_prefix;

    if (lat < 0){
        lat = fabsf(lat);
        lat_prefix = "S:";
    }
    else {
        lat_prefix = "N:";
    }

    return lat_prefix;
}
char* prefix_lon (float lon){
    char* lon_prefix;

    if (lon < 0){
        lon = fabsf(lon);
        lon_prefix = "W:";
    }
    else {
        lon_prefix = "E:";
    }

    return lon_prefix;
}
float unsign (float deg){
    if (deg < 0){
        deg = fabsf(deg);
    }
    return deg;
}
void display_line1 (float lat, int heading){

    char* lat_prefix = prefix_lat(lat);
    lat = unsign(lat);

    display.print(lat_prefix);
    if (lat < 100){
        display.print(0);
    }
    display.print(lat, 5);
    display.print(" H:");
    display.println(heading);
}
void display_line2 (float lon, int satellites){

    char* lon_prefix = prefix_lon(lon);
    lon = unsign(lon);

    display.print(lon_prefix);
    if (lon < 100){
        display.print(0);
    }
    display.print(lon, 5);
    display.print(" s:");
    display.println(satellites);
}
void display_line3(int alt, float speed){
    display.print("A:");
    display.print(alt);
    display.print(" S:");
    display.print(speed);

}
void display_home_screen (){
    display.print(fix.dateTime.year);
    display.print(".");
    if (fix.dateTime.month < 10){
        display.print(0);
    }
    display.print(fix.dateTime.month);
    display.print(".");
    if (fix.dateTime.date < 10){
        display.print(0);
    }
    display.print(fix.dateTime.date);
    display.print(" ");
    if (fix.dateTime.hours < 10){
        display.print(0);
    }
    display.print(fix.dateTime.hours);
    display.print(":");
    if (fix.dateTime.minutes < 10){
        display.print(0);
    }
    display.print(fix.dateTime.minutes);
    display.print(":");
    if (fix.dateTime.seconds < 10){
        display.print(0);
    }
    display.print(fix.dateTime.seconds);
    display.println(" UTC");

    display_line1(fix.latitude(), fix.heading());
    display_line2(fix.longitude(), fix.satellites);
    display_line3(fix.alt.whole, fix.speed_kph());
    display.display();
}
void display_return_screen (float saved_lat, float saved_lon){
    NeoGPS::Location_t saved(saved_lat, saved_lon);

    char* saved_lat_prefix = prefix_lat(saved_lat);
    saved_lat = unsign(saved_lat);

    char* saved_lon_prefix = prefix_lon(saved_lon);
    saved_lon = unsign(saved_lon);

    float lat = fix.latitude();
    float lon = fix.longitude();

    char* lat_prefix = prefix_lat(lat);
    lat = unsign(lat);

    char* lon_prefix = prefix_lon(lon);
    lon = unsign(lon);

    display.print(saved_lat_prefix);
    if (lat < 100){
        display.print(0);
    }
    display.print(saved_lat, 5);
    display.print(" K:");
    display.println(fix.location.DistanceKm(saved));
    display.print(saved_lon_prefix);
    if (lon < 100){
        display.print(0);
    }
    display.print(saved_lon, 5);
    display.print(" D:");
    float bearing = fix.location.BearingToDegrees(saved);
    if (bearing < 100){
        display.print("0");
    }
    display.println(bearing);
    display.print(lat_prefix);
    if (lat < 100){
        display.print(0);
    }
    display.print(lat, 5);
    display.print(" H:");
    float heading = fix.heading();
    if (heading < 100){
        display.print("0");
    }
    display.println(heading);
    display.print(lon_prefix);
    if (lon < 100){
        display.print(0);
    }
    display.print(lon, 5);
    display.print(" S:");
    display.println(fix.speed_kph());

    display.display();
}
void coords_save (float lat, float lon){
    int addr = 0;
    EEPROM.put(addr, lat);
    addr += sizeof(float);
    EEPROM.put(addr, lon);
}
void screen_button (){
    screen_button_count++;
}
void loop() {
    if (! eeprom_read){
        coords_save(50.9535522,-114.5805601);
        int addr = 0;
        EEPROM.get(addr, saved_lat);
        addr += sizeof(float);
        EEPROM.get(addr, saved_lon);
        eeprom_read = 1;
    }

    while (gps.available(gps_port)) {
        fix = gps.read();

        display.setCursor(0, 0);
        display.clearDisplay();

        if ((screen_button_count == 0 || screen_button_count % 2 == 0) && ! return_mode){
            display_return_screen(saved_lat, saved_lon);
        }
        else {
            display_home_screen();
        }
    }
}
