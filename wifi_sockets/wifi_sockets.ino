#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <SoftwareSerial.h>
SoftwareSerial ESPserial(2, 4);

const char* ssid = "Stanford"; //"Stanford"; //WIFI Name, WeMo will only connect to a 2.4GHz network.
const char* password = ""; //"";
const int BUFFER_SIZE = 300;

char updater[BUFFER_SIZE];

int relayPin = BUILTIN_LED;//D1; // The Shield uses pin 1 for the relay
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

IPAddress ip(10, 0, 0, 69); // where xx is the desired IP Address
IPAddress gateway(10, 0, 0, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

void setup() {
  Serial.begin(9600);
  ESPserial.begin(9600);
  Serial.print("Hello world!");

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  setup_wifi();
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  startSPIFFS();
  startWebSocket();
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();  // start the websocket server
  webSocket.onEvent(webSocketEvent);  // if there's an incoming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
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
  //Arduino --> write to serial
  if (ESPserial.available()) {
    size_t idx = 0;
    while (ESPserial.available() && idx < BUFFER_SIZE) {
      updater[idx] = ESPserial.read();
      Serial.print(updater[idx]);
      idx += 1;
    }
    updater[idx] = '\0';
    Serial.println(updater);
    // webSocket.broadcastTXT(String(updater));
     delay(15);
  }
  server.handleClient();                    // Listen for HTTP requests from clients
  webSocket.loop();
   // serial user input --> Arduino
//   if (Serial.available()) {
//       ESPserial.write(Serial.read());
//   }
}

void startSPIFFS() {  // Start the SPIFFS and list all contents
  SPIFFS.begin(); // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {  // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

void startServer() {
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      ESPserial.println((char *) payload);
//      payload = (char *) payload;
//      ESPserial.write(payload + '\n');
      break;
  }
}
