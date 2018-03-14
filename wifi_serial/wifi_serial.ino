//This example will use a static IP to control the switching of a relay. Over LAN using a web browser. 
//A lot of this code have been resued from the example on the ESP8266 Learning Webpage below. 
//http://www.esp8266learning.com/wemos-webserver-example.php

/*
 * References
 * ----------
 * Hardware
 * - D1 mini board: https://wiki.wemos.cc/products:d1:d1_mini
 * - Arduino micro: https://store.arduino.cc/usa/arduino-micro
 * 
 * Libraries
 * ---------
 * ESP8266WiFi
 * - GitHub: https://github.com/esp8266/Arduino
 * - Documentation: https://arduino-esp8266.readthedocs.io/en/latest/
 * Arduino serial: https://www.arduino.cc/reference/en/language/functions/communication/serial/
 * Arduino WiFi: https://www.arduino.cc/en/Reference/WiFi
 */

#include <ESP8266WiFi.h>

int testPin = BUILTIN_LED;//D1; // The Shield uses pin 1 for the relay

// WiFi network information
const char* ssid = "Stanford"; // must be a 2.4GHz network
const char* password = "";
WiFiServer server(80);
IPAddress ip(128, 12, 8, 41);
IPAddress dns(171, 67, 1, 234);
IPAddress gateway(10, 31, 240, 1);
IPAddress subnet(255, 255, 240, 0);

// serial input data
const unsigned int MAX_INPUT = 200;     // maximum length of input line
char serial_input[MAX_INPUT];           // buffer to store serial input
// serial_input[MAX_INPUT - 1] = '\0';                  // null terminate string

void setup() {
  Serial.begin(9600);

  pinMode(testPin, OUTPUT);
  digitalWrite(testPin, LOW);
  
  setup_WiFi();
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/"); 
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void setup_WiFi() {
  // Configure WiFi shield and connect to WiFi network
  Serial.print("Setting static ip to : ");
  Serial.println(ip);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // WiFi.config(ip, dns, gateway, subnet); 
  WiFi.begin(ssid, password);
  //Trying to connect it will display dots
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

//void loop is where you put all your code. it is a funtion that returns nothing and will repeat over and over again
//6
void loop() {
  String wifi_in;

  serialInput();  // read Arduino data to buffer    -- done
  WiFiOutput();   // write Arduino data to WiFi     -- done
  wifi_in = WiFiInput();    // read WiFi data to buffer       -- in progress
  serialOutput(wifi_in); // transmit WiFi data to Arduino

}

void serialOutput(String request) {
  //Match the request, checking to see what the currect state is
  if (request.indexOf("/relay=ON") != -1) {
    // Serial.println("request = high");
  } else {
    // Serial.println("request = low");
  }
}

String WiFiInput() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return String("");
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  if (!client.available()) {
    Serial.println("client not available");
    return String("");
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
  return request;
}

void serialInput() {
  int input_pos = 0;
  byte inByte;

  while (Serial.available() > 0) {
    Serial.println("here");
    inByte = Serial.read();
    switch(inByte) {

      // newline --> reset buffer
      case '\n':
        // terminating null byte
        serial_input[input_pos] = 0;
        
        // reset buffer for next time
        input_pos = 0;  
        break;

      // carriage return --> discard
      case '\r':   
        break;

      // actual character --> add to buffer
      default:
        // keep adding if not full ... allow for terminating null byte
        if (input_pos < (MAX_INPUT - 1)) {
          serial_input[input_pos++] = inByte;
        }
        break;
    }
  }

  Serial.print(serial_input);
}

void WiFiOutput() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.println("Data");
  client.println(serial_input);
  client.println("");
  client.println("");
 
  client.println("<br><br><br>");
  client.println("<a href=\"/relay=ON\">Click here to engage (Turn ON) the relay.</a><br><br><br>");
  client.println("<a href=\"/relay=OFF\">Click here to disengage (Turn OFF) the relay.</a><br>");
  client.println("</html>");

  Serial.println("Client disconnected");
  Serial.println("");
}
