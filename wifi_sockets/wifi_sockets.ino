/*
 * Prerequisites
 * -------------
 * - Arduino IDE Libraries
 *   - ESP8266 Core: https://github.com/esp8266/Arduino
 *     - Documentation: https://arduino-esp8266.readthedocs.io/en/latest/
 *   - WebSockets: https://github.com/Links2004/arduinoWebSockets
 * - SPIFFS tool for Arduino IDE: https://github.com/esp8266/arduino-esp8266fs-plugin
 * 
 * Notes
 * -----
 * Serial Monitor cannot be open when uploading sketch via ESP Sketch Data Upload
 * 
 * References
 * ----------
 * Hardware
 * - D1 mini board: https://wiki.wemos.cc/products:d1:d1_mini
 * - Arduino micro: https://store.arduino.cc/usa/arduino-micro
 * 
 * Tutorials / Documentation
 * -------------------------
 * Hosting webserver: https://techtutorialsx.com/2016/10/03/esp8266-setting-a-simple-http-webserver/
 * Uploading sketches: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
 */


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <SoftwareSerial.h>
SoftwareSerial ESPserial(2, 4);

const char* ssid = "Stanford Residences"; // 2.4 GHz networks only
const char* password = "";
const int BUFFER_SIZE = 300;

char updater[BUFFER_SIZE];
bool webSocketReady = 0;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

IPAddress ip(128, 12, 8, 41);
IPAddress dns(171, 67, 1, 234);
IPAddress gateway(128, 12, 8, 1);
IPAddress subnet(255, 255, 240, 0);

void setup() {
  Serial.begin(115200);
  ESPserial.begin(115200);

  Serial.setDebugOutput(true);

  // connect to network
  setup_wifi();

  // mount SPIFFS
  startSPIFFS();

  // start server
  startServer();

  Serial.flush();
  delay(1000);

  // start websocket
  startWebSocket();
}

void startServer() {
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");
}

// start a WebSocket server
void startWebSocket() {
  webSocket.begin();

  // if there's an incoming websocket message, go to function 'webSocketEvent'
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started.");
}

void setup_wifi() {
  // Configure WiFi shield
//  Serial.print("Setting static IP to: ");
//  Serial.println(ip);
//  Serial.print("Connecting to (SSID): ");
//  Serial.println(ssid);
//  WiFi.config(ip, dns, gateway, subnet);

  // Attempt to connect to network based on configuration
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Print the device network information
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Default Gateway: ");
  Serial.println(WiFi.gatewayIP());
}

void loop(void) {
  // Listen for HTTP requests from clients
  server.handleClient();
  webSocket.loop();

  // Read data from Arduino
  if (ESPserial.available()) {
    int idx = 0;
    char value = 0;
    while (ESPserial.available() && idx < BUFFER_SIZE - 2) {
      value = ESPserial.read();
      updater[idx++] = value;

      // empirically, SoftwareSerial read occurs much faster than SoftwareSerial write
      delayMicroseconds(100);
    }

    // add null terminator
    updater[idx] = '\0';
    flushSerial();
    
    if (webSocketReady) {
      String data = String(updater);

      // send to all clients
      webSocket.broadcastTXT(updater);
    }
  }
}

/*
 * Clear SoftwareSerial input.
 */
void flushSerial() {
  while(ESPserial.available()) {
    ESPserial.read();
  }
}

/*
 * Mount the SPIFFS (SPI Flash File System) and list all contents and file sizes.
 */
void startSPIFFS() {
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS file system mounted. Contents:");

    // List the file system contents
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  } else {
    Serial.println("SPIFFS file system failed to mount.");
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

/*
 * If the requested file or page doesn't exist, return a 404 not found error
 */
void handleNotFound() {
  // check if the file exists in the flash memory (SPIFFS), if so, send it
  if (!handleFileRead(server.uri())) {
    server.send(404, "text/plain", "404: File Not Found");
  }
}

/*
 * Send requested file client
 * Source: https://github.com/esp8266/Arduino/blob/61cd8d83859524db0066a647de3de3f6a0039bb2/libraries/ESP8266WebServer/examples/FSBrowser/FSBrowser.ino
 */
bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);

  // If a folder is requested, send the index file
  if (path.endsWith("/")) {
    path += "index.html";
  }

  // Get the MIME type
  String contentType = getContentType(path);

  // Check for compressed version
  if (SPIFFS.exists(path + ".gz")) {
      path += ".gz";
  }

  // If the file exists
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");                    // Open the file
    server.streamFile(file, contentType);                  // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }

  // If the file doesn't exist, return false
  Serial.println(String("\tFile Not Found: ") + path);
  return false;
}

String getContentType(String filename) {
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
      webSocketReady = 0;
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        webSocketReady = 1;
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      ESPserial.println((char *) payload);
      break;
  }
}

