#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Turn on debug statements to the serial output
#define  DEBUG  0

#if debug part
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else

#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// Define the number of devices we have in the chain and the hardware interface
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

// HARDWARE SPI
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// WiFi login parameters
const char* ssid = "User_SSID";
const char* password = "Wifi_Password";

// Custom device name for ESP8266
const char* deviceName = "MaxMatrix";

// WiFi Server object
WiFiServer server(80);

// Scrolling parameters
uint8_t frameDelay = 50;  // default frame delay value
textEffect_t	scrollEffect = PA_SCROLL_LEFT;

// Message buffers shared by Wifi and Scrolling functions
#define BUF_SIZE  512
char curMessage[BUF_SIZE] = "Waiting for WiFi connection..."; // Default message
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;

const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";

// Enhanced HTML Web Page with CSS and JavaScript
const char WebPage[] =
"<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Message Control by Siddhartrockx</title>"
"<style>"
"body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #1a1a1a; color: #f4f4f4; }"
"h1 { text-align: center; color: #f4f4f4; }"
"form { max-width: 600px; margin: auto; padding: 20px; background-color: #333; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5); }"
"label { font-size: 18px; margin-bottom: 10px; display: block; color: #f4f4f4; }"
"input[type='text'], input[type='range'] { width: 100%; padding: 10px; margin-bottom: 20px; border-radius: 5px; border: 1px solid #555; font-size: 16px; background-color: #444; color: #fff; }"
"input[type='text']:focus, input[type='range']:focus { border-color: #4CAF50; outline: none; }"
"input[type='radio'] { margin-right: 10px; }"
"input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }"
"input[type='submit']:hover { background-color: #45a049; }"
".form-group { margin-bottom: 15px; }"
"</style>"
"<script>"
"function sendData(event) {"
"  event.preventDefault();" // Prevent form from refreshing the page
"  var request = new XMLHttpRequest();"
"  var msg = document.getElementById('message').value.trim();"
"  var scroll = document.querySelector('input[name=\"scrollType\"]:checked').value;"
"  var invert = document.querySelector('input[name=\"invert\"]:checked').value;"
"  var speed = document.getElementById('speed').value;"
"  var url = '/&MSG=' + encodeURIComponent(msg) + '&SD=' + scroll + '&I=' + invert + '&SP=' + speed;"
"  request.open('GET', url, true);"
"  request.send();"
"  alert('Message Sent!');"
"}"
"</script>"
"</head>"
"<body>"
"<h1>MD_Parola Message Control</h1>"
"<form onsubmit='sendData(event)'>"
"<div class='form-group'>"
"<label for='message'>Message:</label>"
"<input type='text' id='message' name='message' maxlength='255' required>"
"</div>"
"<div class='form-group'>"
"<label>Scroll Direction:</label>"
"<input type='radio' id='left' name='scrollType' value='L' checked> Left"
"<input type='radio' id='right' name='scrollType' value='R'> Right"
"</div>"
"<div class='form-group'>"
"<label>Invert Display:</label>"
"<input type='radio' id='normal' name='invert' value='0' checked> Normal"
"<input type='radio' id='inverse' name='invert' value='1'> Inverse"
"</div>"
"<div class='form-group'>"
"<label for='speed'>Speed:</label>"
"Fast <input type='range' id='speed' name='speed' min='10' max='200' value='50'> Slow"
"</div>"
"<input type='submit' value='Send Data'>"
"</form>"
"</body>"
"</html>";



// Function to handle incoming HTTP requests
void handleRequest(char *message)
{
  char *pStart, *pEnd;

  // Check for message
  pStart = strstr(message, "/&MSG=");
  if (pStart != NULL)
  {
    pStart += 6;  // skip to start of data
    pEnd = strchr(pStart, '&');
    if (pEnd != NULL)
    {
      strncpy(newMessage, pStart, pEnd - pStart);
      newMessage[pEnd - pStart] = '\0';
      newMessageAvailable = true;
    }
  }

  // Check for scroll direction
  pStart = strstr(message, "&SD=");
  if (pStart != NULL)
  {
    pStart += 4;
    scrollEffect = (*pStart == 'R') ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT;
    P.setTextEffect(scrollEffect, scrollEffect);
  }

  // Check for invert option
  pStart = strstr(message, "&I=");
  if (pStart != NULL)
  {
    pStart += 3;
    P.setInvert((*pStart == '1') ? true : false);
  }

  // Check for speed option
  pStart = strstr(message, "&SP=");
  if (pStart != NULL)
  {
    pStart += 4;
    int speed = atoi(pStart);
    frameDelay = speed;
    P.setSpeed(speed);
  }
}

void handleWiFi()
{
  WiFiClient client = server.available();
  if (client)
  {
    String req = client.readStringUntil('\r');
    client.flush();

    if (req.indexOf("GET") != -1)
    {
      handleRequest((char*)req.c_str());

      client.print(WebResponse);
      client.print(WebPage);
    }
    client.stop();
  }
}

void setup()
{
  Serial.begin(115200);

  // Initialize the Parola display
  P.begin();
  P.displayClear();
  P.displaySuspend(false);

  // Boot screen with welcome message animation
const char* bootMessage = "SidNaz";  // Increase this value to make the animation slower
P.displayText(bootMessage, PA_CENTER, frameDelay, 25, PA_SCAN_HORIZ, PA_NO_EFFECT);



  // Animate the boot screen
  while (!P.displayAnimate()) {
    // Wait for the boot screen to finish
  }
  
  // Set the ESP's name for the Wi-Fi network
  WiFi.hostname("MaxMatrix v1.0");  // Setting the ESP name
  WiFi.begin(ssid, password);

  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
// Starting the server for data transfer from MAX to ESP32 
  // Start the server
  server.begin();

  // Display IP address after successful Wi-Fi connection
  sprintf(curMessage, "IP: %03d.%03d.%03d.%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  P.displayScroll(curMessage, PA_LEFT, scrollEffect, frameDelay);

}

void loop()
{
  handleWiFi();
  
  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
}
