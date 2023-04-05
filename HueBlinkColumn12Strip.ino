/* HueBlink example for ArduinoHttpClient library

   Uses ArduinoHttpClient library to control Philips Hue
   For more on Hue developer API see http://developer.meethue.com

  To control a light, the Hue expects a HTTP PUT request to:

  http://hue.hub.address/api/hueUserName/lights/lightNumber/state

  The body of the PUT request looks like this:
  {"on": true} or {"on":false}

  This example  shows how to concatenate Strings to assemble the
  PUT request and the body of the request.

   modified 15 Feb 2016 
   by Tom Igoe (tigoe) to match new API
*/

#include <WiFiNINA.h>
#include <SPI.h>
// #include <WiFi101.h>
#include <ArduinoHttpClient.h>
#include "arduino_secrets.h"

#include <Arduino_LSM6DS3.h>

int lastHue;
int secsSameReading;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;  // the Wifi radio's status

char hueHubIP[] = "172.22.151.226";                               // IP address of the HUE bridge
String hueUserName = "JY5Q2GgPBHmDJWx5WGma3FEWeDoDiDbJY-GJ96AZ";  // hue bridge username
int lightNum = 3;
bool isOn;

// make a wifi instance and a HttpClient instance:
WiFiClient wifi;
HttpClient httpClient = HttpClient(wifi, hueHubIP);


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
    ;  // wait for serial port to connect.


  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1)
      ;
  }

  // Serial.print("Accelerometer sample rate = ");
  // Serial.print(IMU.accelerationSampleRate());
  // Serial.println(" Hz");
  // Serial.println();
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network IP = ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  sendRequest(lightNum, "on", "true");
  isOn = true;
  sendRequest(lightNum, "sat", "254");
  sendRequest(lightNum, "bri", "254");
}

void loop() {
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    float mappedx = (x + 1) / 2;
    float mappedy = (y + 1) / 2;
    float mappedz = (z + 1) / 2;

    if (mappedx > 1) mappedx = 1;
    if (mappedy > 1) mappedy = 1;
    if (mappedz > 1) mappedz = 1;

    float hue;
    hue = rgb2hsv(mappedx, mappedy, mappedz);
    // Serial.println(mappedx);
    // Serial.println(mappedy);
    // Serial.println(mappedz);
    hue = hue * 65535;
    hue = int(round(hue));
    // Serial.println(int(hue));
    // if still for last 30 sec, don't send
    if (secsSameReading < 5) {
      if (!isOn) {
        Serial.println("motion detected, turning light on");
        isOn = true;
        sendRequest(lightNum, "on", "true");
      }
      sendRequest(lightNum, "hue", String(int(hue)));
    } else if (isOn) {
      Serial.println("motion not detected for more than 5 seconds, turning light off");
      isOn = false;
      sendRequest(lightNum, "on", "false");
    }

    if (abs(int(hue) - lastHue) < 50) {
      secsSameReading++;
    } else {
      Serial.println("motion detected");      
      secsSameReading = 0;
    }
    lastHue = int(hue);
    Serial.println(secsSameReading);
    Serial.println(" seconds since motion detected");
    delay(1000);
  }
}

void sendRequest(int light, String cmd, String value) {
  // make a String for the HTTP request path:
  String request = "/api/" + hueUserName;
  request += "/lights/";
  request += light;
  request += "/state/";

  String contentType = "application/json";

  // make a string for the JSON command:
  String hueCmd = "{\"" + cmd;
  hueCmd += "\":";
  hueCmd += value;
  hueCmd += "}";
  // see what you assembled to send:
  Serial.print("PUT request to server: ");
  Serial.println(request);
  Serial.print("JSON command to server: ");

  // make the PUT request to the hub:
  httpClient.put(request, contentType, hueCmd);

  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.println(hueCmd);
  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
}


// hsv rgb converter https://gist.github.com/postspectacular/2a4a8db092011c6743a7
float mix(float a, float b, float t) {
  return a + (b - a) * t;
}

float step(float e, float x) {
  return x < e ? 0.0 : 1.0;
}

float rgb2hsv(float r, float g, float b) {
  float s = step(b, g);
  float px = mix(b, g, s);
  float py = mix(g, b, s);
  float pz = mix(-1.0, 0.0, s);
  float pw = mix(0.6666666, -0.3333333, s);
  s = step(px, r);
  float qx = mix(px, r, s);
  float qz = mix(pw, pz, s);
  float qw = mix(r, px, s);
  float d = qx - min(qw, py);
  float hsvhue = abs(qz + (qw - py) / (6.0 * d + 1e-10));
  // hsv[1] = d / (qx + 1e-10);
  // hsv[2] = qx;
  return hsvhue;
}