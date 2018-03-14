#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "Stanford"; //WIFI Name, WeMo will only connect to a 2.4GHz network.
const char* password = "";

int relayPin = BUILTIN_LED;//D1; // The Shield uses pin 1 for the relay
ESP8266WebServer server(80);

IPAddress ip(10, 0, 0, 69); // where xx is the desired IP Address
IPAddress gateway(10, 0, 0, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

void setup() {
  Serial.begin(9600);
  Serial.print("Hello world!");

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  setup_wifi();
  // Start the server
  server.on("/", handleRoot);
  server.on("/LED", HTTP_POST, handleLED);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void setup_wifi() {
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

void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients
}

void handleRoot() {
  server.send(200, "text/html", "<form action=\"/LED\" method=\"POST\"><input type=\"submit\" value=\"Toggle LED\"></form>");
}

void handleLED() {                          // If a POST request is made to URI /LED
  digitalWrite(relayPin,!digitalRead(relayPin));      // Change the state of the LED
  server.sendHeader("Location","/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
