#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <TFT_eSPI.h> // Include the TFT_eSPI library
#include <vector>
// #include <DigitsHB9IIU36pt7b.h> //  https://rop.nl/truetype2gfx/   https://fontforge.org/en-US/

struct SignalRegion
{
  size_t start;
  size_t end;
  uint16_t averageStrength;
  size_t centerPosition;
};

TFT_eSPI tft = TFT_eSPI(); // Initialize the TFT display

uint16_t navyBlue = tft.color565(0, 0, 128);
uint16_t darkBlue = tft.color565(0, 9, 75);
uint16_t bgColor = darkBlue;

const char *websocketServer = "185.83.169.27"; // IP of the server
const int websocketPort = 443;                 // Port for WSS

//

WebSocketsClient webSocket;

// Array to store the previous fft values
// length of payload =1844, fft value takes 2 bytes -> 922 values
// we take only every 2nd value thus 461 values (width of TFT Display is 480
uint16_t previousYcoordValue[461];

// Spectrum parameters
const float START_FREQ = 10490.5; // MHz
const float BANDWIDTH = 4.5;      // MHz
// for gradient calculation
int yTopBeacon = 320;
unsigned long previousMillisForClock = 0; // will store the last time the function was triggered
const long intervalForClock = 1000;       // interval at which to trigger the function (1000 ms = 1 second)
char previousTime[9] = "00:00:00";        // Optimized to only store 8 characters + null terminator

//---------------------------------------------------------------------------------------
// functions forward declarations
void connectToWifi();
void getLocalTime();

// function to convert fft value to db
float dbFromFTTvalue(uint16_t fftValue);
uint16_t yCordFromFFTvalue(uint16_t fftValue);
float mapXToFrequency(int x);
int mapFrequencyToX(float frequency);
void calculateThresholds(uint16_t *fftValues, size_t numFFTValues, uint16_t &noiseFloor, uint16_t &signalThreshold);

//---------------------------------------------------------------------------------------

// Function to map fftValue to a color gradient (dark green to red)
uint16_t mapValueToColor(uint16_t value)
{
  int topGreen = 90;                                              // higher the value, lower the upper margin
  uint8_t red = map(value, 320, yTopBeacon - topGreen, 255, 0);   // Increase red with value
  uint8_t green = map(value, 320, yTopBeacon - topGreen, 0, 255); // Decrease green with value
  // uint8_t blue = map(value, 320, yTopBeacon - topGreen, 0, 255);  // Decrease green with value

  return tft.color565(red, green, 0); // Convert RGB to 16-bit color
}

// Function to handle FFT data and update the display
void handleFFTData(uint8_t *payload, size_t length)
{
  static bool isFirstRun = true;
  static bool isBeacon = true;
  int leftMargin = 5;
  int topMarging = 60;
  if (isFirstRun)
  {
    for (int i = 0; i < 461; i++)
    {
      previousYcoordValue[i] = 320; // Initialize all to the bottom of the display
    }
    isFirstRun = false;
  }

  // Ensure payload length is valid
  if (length % 2 != 0)
  {
    Serial.println("Invalid payload length.");
    return;
  }
  // Clean right margin
  for (int i = leftMargin + 460; i <= 480; i++)
  {

    tft.drawLine(i, topMarging, i, 320, bgColor); // Mark center of signal
  }
  const size_t numFFTValues = length / 4; // Using every second value, hence length/4
  uint16_t fftValues[numFFTValues];
  size_t xCoord = 0;

  Serial.println("Plotting FFT values:");
  for (size_t i = 0; i < length && xCoord < numFFTValues; i += 4)
  {
    uint16_t fftValue = payload[i] | (payload[i + 1] << 8);
    fftValues[xCoord] = fftValue;
    uint16_t yCoord = yCordFromFFTvalue(fftValue);
    uint16_t previousYcoord = previousYcoordValue[xCoord];

    tft.drawLine(xCoord + leftMargin, topMarging, xCoord + leftMargin, yCoord, bgColor);

    for (int y = 320; y > yCoord; y -= 1)
    {
      uint16_t dotColor = mapValueToColor(y);
      tft.drawPixel(xCoord + leftMargin, y, dotColor);
    }
    // }
    previousYcoordValue[xCoord] = yCoord;

    if (yCoord < yTopBeacon && xCoord < 312)
    {
      yTopBeacon = yCoord;
    }
    xCoord++;
  }

  // Signal Detection
  Serial.println("Signal Detection:");
  const uint16_t SIGNAL_THRESHOLD = 16000;

  bool inSignal = false;
  size_t startSignal = 0;

  for (size_t i = 0; i < numFFTValues; i++)
  {
    uint16_t fftValue = fftValues[i];

    if (fftValue >= SIGNAL_THRESHOLD)
    { // Signal start or continuation
      if (!inSignal)
      {
        inSignal = true;
        startSignal = i; // Mark the start of the signal
      }
    }
    else
    { // Signal ends
      if (inSignal)
      {
        inSignal = false;
        size_t endSignal = i - 1; // Mark the end of the signal

        // Process the detected signal region
        size_t signalLength = endSignal - startSignal + 1;
        uint32_t strengthSum = 0;
        for (size_t j = startSignal; j <= endSignal; j++)
        {
          strengthSum += fftValues[j];
        }
        uint16_t averageStrength = signalLength > 0 ? strengthSum / signalLength : 0;
        size_t centerPosition = (startSignal + endSignal) / 2;
        // Calculate the FFT value at the center position
        uint16_t fftValueAtCenter = fftValues[centerPosition];

        // Debug: Print the FFT value at the center position
        Serial.printf("FFT Value at CenterPos %zu: %u\n", centerPosition, fftValueAtCenter);
        // Debug: Print signal region details
        Serial.printf("Signal detected: Start=%zu, End=%zu, Length=%zu, AvgStrength=%u, CenterPos=%zu\n",
                      startSignal, endSignal, signalLength, averageStrength, centerPosition);

        // Plot signal markers
        // tft.drawLine(startSignal + 10, 0, startSignal + 10, 320, TFT_GREEN);       // Mark start of signal
        // tft.drawLine(centerPosition + 10, 0, centerPosition + 10, 320, TFT_WHITE); // Mark center of signal
        // tft.drawLine(endSignal + 10, 0, endSignal + 10, 320, TFT_RED);             // Mark end of signal

        float dbValue = dbFromFTTvalue(fftValueAtCenter);
        String dbLabel;
        static float dBbeacon;

        if (isBeacon)
        {
          dbLabel = String(dbValue, 1) + " dB"; // Convert float to String with 1 decimal place
          dBbeacon = dbValue;
          isBeacon = false;
        }
        else
        {
          float deltadB = abs(dbValue - dBbeacon); // Calculate the absolute difference
          String deltaLabel = String(deltadB, 1);  // Convert the delta to String with 1 decimal place

          // Add the sign (+ or -) in front of deltaLabel
          if (dbValue - dBbeacon >= 0)
          {
            deltaLabel = "+" + deltaLabel; // Positive delta
          }
          else
          {
            deltaLabel = "-" + deltaLabel; // Negative delta
          }
          dbLabel = deltaLabel; // Update dbLabel with the signed delta
        }

        int yatCenter = yCordFromFFTvalue(fftValueAtCenter);
        int textWidth = tft.textWidth(dbLabel);                        // Get the width of the text in pixels
        int xPosition = centerPosition + leftMargin - (textWidth / 2); // Calculate horizontal position
        if (xPosition > 0)
        {
          tft.setFreeFont(&FreeMono9pt7b);         // Use the FreeMono9pt7b font
          tft.setCursor(xPosition, yatCenter - 8); // Set the cursor position
          tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Set the text color (foreground, background)
          tft.print(dbLabel);                      // Print the text
        }
      }
    }
  }

  // Handle case where a signal continues until the end of the data
  if (inSignal)
  {
    size_t endSignal = numFFTValues - 1;
    size_t signalLength = endSignal - startSignal + 1;
    uint32_t strengthSum = 0;
    for (size_t j = startSignal; j <= endSignal; j++)
    {
      strengthSum += fftValues[j];
    }
    uint16_t averageStrength = signalLength > 0 ? strengthSum / signalLength : 0;
    size_t centerPosition = (startSignal + endSignal) / 2;

    // Debug: Print signal region details
    Serial.printf("Signal detected: Start=%zu, End=%zu, Length=%zu, AvgStrength=%u, CenterPos=%zu\n",
                  startSignal, endSignal, signalLength, averageStrength, centerPosition);

    // Plot signal markers
    tft.drawLine(startSignal + leftMargin, 0, startSignal + leftMargin, 320, TFT_GREEN);       // Mark start of signal
    tft.drawLine(centerPosition + leftMargin, 0, centerPosition + leftMargin, 320, TFT_WHITE); // Mark center of signal
    tft.drawLine(endSignal + leftMargin, 0, endSignal + leftMargin, 320, TFT_RED);             // Mark end of signal
  }

  isBeacon = true;
  getLocalTime();
}

// Function to handle WebSocket events
// Declare a global variable to track the last execution time
unsigned long lastFFTProcessTime = 0; // In milliseconds

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_BIN:
    // Process FFT data only if 1 second has passed since the last processing
    if (millis() - lastFFTProcessTime >= 1500)
    {
      handleFFTData(payload, length); // Call the processing function
      lastFFTProcessTime = millis();  // Update the last execution time
    }
    break;

  case WStype_DISCONNECTED:
    Serial.println("WebSocket Disconnected");
    break;

  case WStype_CONNECTED:
    Serial.println("WebSocket Connected");
    webSocket.sendTXT("Hello, ESP32 here!");
    break;

  default:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Sketch Size: " + String(ESP.getSketchSize()) + " bytes");
  Serial.println("Free Sketch Space: " + String(ESP.getFreeSketchSpace()) + " bytes");
  Serial.println("Flash Chip Size: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
  Serial.println("Flash Frequency: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz");

  // Initialize the TFT display
  pinMode(TFT_BLP, OUTPUT);
  digitalWrite(TFT_BLP, HIGH); // Turn on the backlight

  tft.begin(); // Initialize the TFT
  tft.fillScreen(bgColor);
  tft.setRotation(1); // Set rotation if needed

  String text = "OSCAR 100 Wideband Spectrum Monitor"; // The text to display
  tft.setTextFont(4);
  tft.setTextColor(TFT_WHITE); // Set text color
  tft.setTextSize(1);          // Set text size (you can adjust this value)
  // Calculate the text width
  int16_t textWidth = tft.textWidth(text);

  // Calculate the starting x position to center the text
  int xPos = (480 - textWidth) / 2;
  tft.setCursor(xPos, 5);
  tft.print(text);


 text = "Offered freely by HB9IIU"; // The text to display
  tft.setTextColor(TFT_CYAN);           // Set text color
  textWidth = tft.textWidth(text);
  xPos = (480 - textWidth) / 2;
  tft.setCursor(xPos, 80); // 100 is the y position (you can adjust it)
  tft.print(text);



   text = "Connecting to Internet....."; // The text to display
  tft.setTextColor(TFT_GOLD);           // Set text color
  textWidth = tft.textWidth(text);
  xPos = (480 - textWidth) / 2;
  tft.setCursor(xPos, 150); // 100 is the y position (you can adjust it)
  tft.print(text);

  connectToWifi();
  text = "Getting Time....";  // The text to display
  tft.setTextColor(TFT_GOLD); // Set text color
  textWidth = tft.textWidth(text);
  xPos = (480 - textWidth) / 2;
  tft.setCursor(xPos, 190); // 100 is the y position (you can adjust it)
  tft.print(text);

  getLocalTime();
  //  Connect to WebSocket server
  webSocket.beginSSL(websocketServer, websocketPort, "/wb/fft", "", ""); // Skip certificate validation
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop(); // Maintain WebSocket connection
  // Get the current time
  unsigned long currentMillis = millis();

  // Check if 1000 ms has passed
  if (currentMillis - previousMillisForClock >= intervalForClock)
  {
    // Save the last time the function was triggered
    previousMillisForClock = currentMillis;
  }
}

// converts fft value to db
float dbFromFTTvalue(uint16_t fftValue)
{
  return (15.0f * (float)fftValue) / 65536.0f;
}

// Function to convert fft value to y coordinate on TFT
uint16_t yCordFromFFTvalue(uint16_t fftValue)
{
  return 320 - ((fftValue - 8000) / 140); // 8000: to lower noise floor, 140: scaling factor;
}
// Function to map x-coordinate to frequency
float mapXToFrequency(int x)
{
  return 10.491 + (4.5 * x / 1844);
}
// Function to map frequency to x-coordinate
int mapFrequencyToX(float frequency)
{
  return (frequency - START_FREQ) * 461 / BANDWIDTH / 2;
}

// Function to calculate noise floor and threshold

void calculateThresholds(uint16_t *fftValues, size_t numFFTValues, uint16_t &noiseFloor, uint16_t &signalThreshold)
{
  // Copy FFT values for sorting
  uint16_t sortedValues[numFFTValues];
  memcpy(sortedValues, fftValues, numFFTValues * sizeof(uint16_t));

  // Sort the values
  std::sort(sortedValues, sortedValues + numFFTValues);

  // Noise floor: Use the 10th percentile
  noiseFloor = sortedValues[numFFTValues / 10];

  // Signal threshold: Noise floor + margin (15% of max FFT value)
  uint16_t maxFFTValue = sortedValues[numFFTValues - 1];
  signalThreshold = noiseFloor + (0.15 * maxFFTValue);

  // Debugging output
  Serial.printf("Refined Noise Floor: %u, Signal Threshold: %u\n", noiseFloor, signalThreshold);
}
// Function to handle Wi-Fi connection
void connectToWifi()
{
  static unsigned long lastAttemptTime = 0;
  static int connectionAttempts = 0;
  const unsigned long delayBetweenAttempts = 5000; // 5 seconds between each attempt
  const int maxAttempts = 5;                       // Max number of attempts before rebooting

  // Print the first attempt immediately
  if (connectionAttempts == 0)
  {
    connectionAttempts++;
    Serial.print("Attempting to connect to WiFi... (Attempt ");
    Serial.print(connectionAttempts);
    Serial.println(")");
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - lastAttemptTime >= delayBetweenAttempts)
    {
      lastAttemptTime = millis();
      connectionAttempts++;

      // Print attempt number only after subsequent retries
      Serial.print("Attempting to connect to WiFi... (Attempt ");
      Serial.print(connectionAttempts);
      Serial.println(")");

      WiFi.begin(ssid, password);

      // If maximum attempts reached, reboot the ESP32
      if (connectionAttempts >= maxAttempts)
      {
        Serial.println("Max connection attempts reached. Rebooting...");

        tft.fillScreen(bgColor);

        String text = "Unsecsessfull Connection!";
        tft.setTextFont(4);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        int16_t textWidth = tft.textWidth(text);
        int xPos = (480 - textWidth) / 2;
        tft.setCursor(xPos, 100);
        tft.print(text);

        text = "Check your credentials!";
        tft.setTextFont(4);
        tft.setTextColor(TFT_YELLOW);
        tft.setTextSize(1);
        textWidth = tft.textWidth(text);
        xPos = (480 - textWidth) / 2;
        tft.setCursor(xPos, 160);
        tft.print(text);

        text = "Rebooting.......";
        tft.setTextFont(4);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        textWidth = tft.textWidth(text);
        xPos = (480 - textWidth) / 2;
        tft.setCursor(xPos, 220);
        tft.print(text);
        delay(5000);
        ESP.restart();
      }
    }

    delay(100); // Small delay to avoid busy-waiting
  }

  Serial.println("Connected to WiFi!");
}
void getLocalTime()
{
  // Synchronize time with NTP server
  configTime(utcOffsetInSeconds, utcOffsetInSeconds, ntpServer); // Set time zone offset and NTP server

  // Get local time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  // Format time into a string
  char timeString[9]; // Optimized to only store 8 characters + null terminator
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  // Set text font and color
  tft.setTextFont(4); // Set text font (adjust if needed)
  tft.setTextColor(TFT_GOLD);

  // If there is a previous time, print it in black to clear it
  if (strcmp(previousTime, "") != 0)
  {
    // Calculate the previous text width and print it in black to clear it
    tft.setTextColor(bgColor); // Set text color to black to clear the previous time
    tft.setCursor(175, 30);
    tft.print(previousTime); // Print previous time in black to clear it
  }

  tft.setTextColor(TFT_GREENYELLOW); // Set text color for new time
  tft.setCursor(175, 30);
  tft.print(timeString); // Print the new time
  // Save the current time for the next cycle
  strcpy(previousTime, timeString);
}
/* NOT WORKING DUE TO MEMORY ISSUES
void display7segmentClockNICE(unsigned long unixTime, int xOffset, int yOffset, uint16_t textColor, bool refresh)
{

  // Static variables to track previous state and colon visibility
  static int previousArray[6] = {-1, -1, -1, -1, -1, -1}; // Initialize previous digit array


  if (refresh == true)
  {
    for (int i = 0; i < 6; i++)
    {
      previousArray[i] = -1;
    }
    refresh = false;
  }
                                       // Define the TFT_MIDGREY color as a local constant
  const uint16_t TFT_MIDGREY = 0x39a7; // Darker grey https://rgbcolorpicker.com/565
  // uint16_t TFT_MIDGREY = TFT_DARKGREY;
  int gap = 40;
  int gap2 = 33; // for :
  int xCoordinates[6] = {xOffset, xOffset + gap, xOffset + 2 * gap + gap2, xOffset + 3 * gap + gap2, xOffset + 4 * gap + 2 * gap2, xOffset + 5 * gap + 2 * gap2};

  // Set the custom font
  tft.setFreeFont(&DigitsHB9IIU36pt7b);


  tft.setTextColor(textColor, TFT_BLACK);
  tft.setCursor(xCoordinates[2] - 24, yOffset);
  tft.print(":");


  // Calculate hours, minutes, and seconds
  int hours = (unixTime % 86400L) / 3600; // Hours since midnight
  int minutes = (unixTime % 3600) / 60;   // Minutes
  int seconds = unixTime % 60;            // Seconds

  // Current time digit array
  int timeArray[6] = {
      hours / 10,   // Tens digit of hours
      hours % 10,   // Units digit of hours
      minutes / 10, // Tens digit of minutes
      minutes % 10, // Units digit of minutes
      seconds / 10, // Tens digit of seconds
      seconds % 10  // Units digit of seconds
  };

  // Mapped characters for 0-9
  char mappedChars[10] = {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};

  // Update only changed digits
  for (int i = 0; i < 6; i++)
  {
    if (timeArray[i] != previousArray[i])
    {
      // Clear the previous digit
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.setCursor(xCoordinates[i], yOffset);
      tft.print(previousArray[i]);

      // Print the new digit
      tft.setTextColor(textColor, TFT_BLACK);
      tft.setCursor(xCoordinates[i], yOffset);
      tft.print(timeArray[i]);

      // Print the mapped character below the digit, but skip if the mapped character is 'H'
      tft.setTextColor(TFT_MIDGREY, TFT_BLACK);
      //tft.setTextColor(TFT_GREEN, TFT_BLACK);

      char mappedChar = mappedChars[timeArray[i]]; // Get the mapped character
      if (mappedChar != 'H')
      {
        tft.setCursor(xCoordinates[i], yOffset); // Adjust Y offset for character display
        tft.print(mappedChar);
      }
    }
  }

  // Update the previous array
  for (int i = 0; i < 6; i++)
  {
    previousArray[i] = timeArray[i];
  }
}
*/
