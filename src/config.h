// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <TFT_eSPI.h> 
TFT_eSPI tft = TFT_eSPI(); // Initialize the TFT display

// Wifi Credentials
const char *ssid = "NO WIFI FOR YOU!!!";
const char *password = "Nestle2010Nestle";

// NTP Server
const char *ntpServer = "pool.ntp.org";  // You can use any valid NTP server

// Time zone offset in seconds (example: UTC -5 for Eastern Standard Time, UTC +1 for Central European Time)
const long utcOffsetInSeconds = +1 * 3600;  // Adjust this as per your time zone
// display options
uint16_t navyBlue = tft.color565(0, 0, 128);
uint16_t darkBlue = tft.color565(0, 9, 75);
uint16_t forestGreen = tft.color565(2, 48, 32);
uint16_t purple = tft.color565(88, 24, 69);

// apply one of the above background colors as default
uint16_t bgColor = TFT_BLACK;
const bool displayDBvalues=true;
const int fftProcessDelay=1000;  // increase if facing issues

// do not change
// Store the colors in an array
uint16_t colors[] = {navyBlue, darkBlue, forestGreen, purple, TFT_BLACK};
#endif // CONFIG_H



// 73! de HB9IIU @JN36kl
// December 2024
// https://github.com/HB9IIU/ESP32-OSCAR-WB-Spectrum-Monitor