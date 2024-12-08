# ESP32-OSCAR-WB-Spectrum-Monitor

This repository contains the code for an ESP32-based application that displays the wideband spectrum of the Qatar-OSCAR 100 (QO-100) satellite on a 4" TFT display (ILI9488). The spectrum display is similar to the one available on the [BATC Wideband Spectrum Monitor page](https://eshail.batc.org.uk/wb/).

## Inspiration

This project is inspired by Tom's [Oscar 100 Wideband Spectrum Clock](https://www.zr6tg.co.za/2022/06/23/oscar-100-wideband-spectrum-clock/). I want to acknowledge that this is not an original idea of mine; instead, it builds upon Tom's work. While his implementation integrates with Open Tuner software, my adaptation is designed to run independently on an ESP32 connected to the Internet via WiFi.

## About This Project

The key difference in this project is that the functionality has been ported to run on an ESP32 microcontroller with a connected 4" ILI9488 TFT display. The ESP32's limited resources mean that the application does not run as fast or smoothly as the BATC webpage, but it serves as a compact, standalone solution for monitoring QO-100 wideband transponder activity. The display provides a visual indication of band activity, making it useful for quick monitoring.

## Demo

<center> Check out the project in action on YouTube: <a href="https://www.youtube.com/watch?v=hN2jycwo034"> <img src="https://img.youtube.com/vi/hN2jycwo034/0.jpg" alt="Watch the demo"> </a> *(Right-click on the video link to open in a new tab.)* </center>

## Screenshots

Here are a few screenshots of the project:

<div align="center"> <table> <tr> <td><img src="https://github.com/HB9IIU/ESP32-OSCAR-WB-Spectrum-Monitor/blob/main/doc/ScreenShots/IMG_7672.png" alt="Screenshot 1" width="200"></td> <td><img src="https://github.com/HB9IIU/ESP32-OSCAR-WB-Spectrum-Monitor/blob/main/doc/Enclosure3DprintFiles/Renderings/TFTESP32enclsoure_1.png" alt="Screenshot 2" width="200"></td> <td><img src="https://github.com/HB9IIU/ESP32-OSCAR-WB-Spectrum-Monitor/blob/main/doc/Enclosure3DprintFiles/Renderings/TFTESP32enclsoure_5.png" alt="Screenshot 3" width="200"></td> </tr> </table> </div>

### Features

- **Standalone Solution**: No need for a PC or additional software.
- **Compact Hardware**: Powered by an ESP32 and a 4" TFT display.
- **Simple Spectrum Display**: Offers a real-time indication of transponder activity.
- **Lightweight Design**: Optimized to work within the constraints of the ESP32 platform.

### Limitations

- Due to the ESP32's limited processing power, the refresh rate and overall performance are slower than Tom's implementation or the BATC webpage.
- The display is for quick monitoring and is not as detailed or feature-rich as the BATC online monitor or Tom’s PC-based solution.

### Hardware Requirements

- ESP32 microcontroller
- 4" TFT display (ILI9488)
- Power supply for the ESP32 and display

### Software Requirements

- PlatformIO for compiling and uploading the code
- Required libraries are already included in the `lib` folder, so no additional installation is needed.

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/ESP32-OSCAR-WB-Spectrum-Monitor.git
   ```
2. Open the project in your preferred development environment.
3. Compile and upload the code to your ESP32.

## Acknowledgments

- Special thanks to the [BATC](https://batc.org.uk/) and [AMSAT-UK](https://amsat-uk.org/) for their contributions to the amateur radio community and tools like the [Wideband Spectrum Monitor](https://eshail.batc.org.uk/wb/).
- Thanks to Tom ([ZR6TG](https://www.zr6tg.co.za)) for his original work on the Oscar 100 Wideband Spectrum Clock, which inspired this project.
