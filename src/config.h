// config.h
#ifndef CONFIG_H
#define CONFIG_H


//const char *ssid = "NO WIFI FOR YOU!!!";

const char *ssid = "MESH";


const char *password = "Nestle2010Nestle";

// NTP Server
const char *ntpServer = "pool.ntp.org";  // You can use any valid NTP server

// Time zone offset in seconds (example: UTC -5 for Eastern Standard Time, UTC +1 for Central European Time)
const long utcOffsetInSeconds = +1 * 3600;  // Adjust this as per your time zone

const int fftProcessDelay=1000;  // increase if facing issues
#endif // CONFIG_H
// 73! de HB9IIU @JN36kl
// December 2024
// https://github.com/HB9IIU/ESP32-OSCAR-WB-Spectrum-Monitor