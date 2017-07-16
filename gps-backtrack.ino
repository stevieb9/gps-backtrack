#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#include <NeoSWSerial.h>
#include <NMEAGPS.h>
#include <SPI.h>
#include <Wire.h>

// user modifiable

#define FLOAT_DEC 5
#define LCD_HEIGHT 32

const uint8_t SCREEN_BUTTON_PIN = 3;  // display mode
const uint8_t SAVE_BUTTON_PIN = 4;    // save current coords

// GPS

#define RX_PIN 0
#define TX_PIN 1

#include "GPSport.h"
#include "Streamers.h"

static NMEAGPS gps;
static gps_fix fix;

// OLED

#define OLED_RESET 4
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != LCD_HEIGHT)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// core variables

static boolean gps_available = false;    // GPS connected to serial?
static boolean gps_initialized = false;  // GPS initialized?

static uint8_t screen_button_count = 0;
static uint8_t save_button_count = 0;

static boolean eeprom_read = 0; // has the saved coords been read?

static float saved_lat;
static float saved_lon;

// forward declarations

void reset_display();
String fstr (String str, float f, int len);
String prefix_lat (float lat);
String prefix_lon (float lon);
float unsign (float deg);
void display_line_2 ();
void display_line_3 ();
void display_line_4 ();
String build_date ();
void display_home_screen ();
void display_return_screen (float saved_lat, float saved_lon);
void coords_save ();
void screen_button ();

void setup(){
    Serial.begin(9600);
    gps_port.begin(9600);

    pinMode(SCREEN_BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(1, screen_button, RISING);

    pinMode(SAVE_BUTTON_PIN, INPUT_PULLUP);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    reset_display();
    display.setTextSize(0);
    display.setTextColor(WHITE);
}
String fstr (String str, float f, int len){
    return str + String(f, len);
}
String prefix_lat (float lat){
    String lat_prefix;

    if (lat < 0){
        lat = fabsf(lat);
        lat_prefix = "S:";
    }
    else {
        lat_prefix = "N:";
    }

    return lat_prefix;
}
String prefix_lon (float lon){
    String lon_prefix;

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
void display_line2 (){

    float lat = fix.latitude();
    String lat_str = prefix_lat(lat);

    lat = unsign(lat);

    if (lat < 100){
        lat_str = lat_str + "0";
    }

    lat_str = fstr(lat_str, lat, FLOAT_DEC);
    display.println(lat_str + " H:" + String(fix.heading()));
}
void display_line3 (){

    float lon = fix.longitude();
    uint8_t satellites = fix.satellites;

    String lon_str = prefix_lon(lon);

    lon = unsign(lon);

    if (lon < 100){
        lon_str = lon_str + "0";
    }

    float volts = (analogRead(A2) * (5.0 / 1023));
    String volt_str = fstr(" v:", volts, 1);

    if (volts <= 3.45){
        volt_str = " BATT-";
    }
    
    lon_str = fstr(lon_str, lon, FLOAT_DEC);
    String sat_str;

    if (satellites < 10){
        sat_str = " s:" + String(satellites);
    }
    else {
        sat_str = " s" + String(satellites);
    }

    display.println(lon_str + sat_str + volt_str);
}
void display_line4 (){
    String str = "A:" + String(fix.alt.whole) + " S:";
    str = fstr(str, fix.speed_kph(), 2);
    display.println(str);
}
String build_date (){
    String date_str = "";

    // year
    date_str = date_str + String(fix.dateTime.year) + ".";

    // month
    if (fix.dateTime.month < 10){
        date_str = date_str + "0";
    }
    date_str = date_str + String(fix.dateTime.month) + ".";

    // day
    if (fix.dateTime.date < 10){
        date_str = date_str + "0";
    }
    date_str = date_str + String(fix.dateTime.date) + " ";

    // hours
    if (fix.dateTime.hours < 10){
        date_str = date_str + "0";
    }
    date_str = date_str + String(fix.dateTime.hours) + ":";

    // minutes
    if (fix.dateTime.minutes < 10){
        date_str = date_str + "0";
    }
    date_str = date_str + String(fix.dateTime.minutes) + ":";

    // seconds
    if (fix.dateTime.seconds < 10){
        date_str = date_str + "0";
    }

    date_str = date_str + String(fix.dateTime.seconds) + " UTC";

    return date_str;
}
void display_home_screen (){
    display.println(build_date());
    display_line2();
    display_line3();
    display_line4();
    display.display();
}
void display_return_screen (float saved_lat, float saved_lon){
    NeoGPS::Location_t saved(saved_lat, saved_lon);

    String saved_lat_prefix = prefix_lat(saved_lat);
    saved_lat = unsign(saved_lat);

    String saved_lon_prefix = prefix_lon(saved_lon);
    saved_lon = unsign(saved_lon);

    float lat = fix.latitude();
    float lon = fix.longitude();

    String lat_prefix = prefix_lat(lat);
    lat = unsign(lat);

    String lon_prefix = prefix_lon(lon);
    lon = unsign(lon);

    display.print(saved_lat_prefix);
    if (lat < 100){
        display.print(0);
    }
    display.print(saved_lat, 5);
    display.print(" K:");
    display.println(fix.location.DistanceKm(saved), 3);
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
    uint16_t addr = 0;
    EEPROM.put(addr, lat);
    addr += sizeof(float);
    EEPROM.put(addr, lon);
}
void save_button (){
    save_button_count++;
    float lat = fix.latitude();
    float lon = fix.longitude();
    uint8_t save_confirmed = 0;

    for (uint8_t i=5; i>0; i--){
        if (save_confirmed > 1){
            reset_display();
            coords_save(fix.latitude(), fix.longitude());
            display.println("");
            display.println("Saving...");
            display.display();
            eeprom_read = 0;
            delay(500);
            return;
        }

        reset_display();
        display.println("Save these coords?");

        if (digitalRead(SAVE_BUTTON_PIN) == LOW){
            save_confirmed++;
        }

        display.println("Lat:" + String(lat, FLOAT_DEC));
        display.println("Lon:" + String(lon, FLOAT_DEC));
        display.println("Aborting in " + String(i) + " seconds");
        display.display();
        delay(1000);
    }

    reset_display();

    display.println("");
    display.println("Aborted...");
    display.display();
    delay(500);
}
void screen_button (){
    screen_button_count++;
}
void reset_display (){
    display.setCursor(0, 0);
    display.clearDisplay();
}
void loop() {
    reset_display();

    if (! gps_available && ! Serial.available() > 0){
        display.println("");
        display.println("GPS device not found");
        display.display();
        delay(1000);
    }
    else {
        gps_available = true;
    }

    if (gps_available){
        if (digitalRead(SAVE_BUTTON_PIN) == LOW){
            save_button();
        }
        
        if (! eeprom_read){
            uint16_t addr = 0;
            EEPROM.get(addr, saved_lat);
            addr += sizeof(float);
            EEPROM.get(addr, saved_lon);
            eeprom_read = 1;
        }

        while (gps.available(gps_port)){
            reset_display();

            fix = gps.read();

            if (! gps_initialized){
                if (fix.latitude() == 0){
                    display.println("");
                    display.print("Initializing GPS");

                    for (uint8_t i=0; i<3; i++){
                        delay(500);
                        display.print(".");
                        display.display();
                    }
                }
                else {
                    gps_initialized = true;
                }
            }
            else {
                if ((screen_button_count > 0 && screen_button_count % 2 == 0)){
                    display_return_screen(saved_lat, saved_lon);
                }
                else {
                    display_home_screen();
                }            
            }
        }
    }
}

