/*
 *  This sketch acts as the HTTP server for the SirMixabot. 
 *  
 *  It handles incoming requests and in turn makes requests to the UNO.
 */

#include <ESP8266WiFi.h>

enum {
  SSID_MAX_LENGTH           = 33,//32 characters plus terminator
  WPA2_MAX_PASSWORD_LENGTH  = 64,//63 characters plus terminator
};
char ssid[SSID_MAX_LENGTH];
char password[WPA2_MAX_PASSWORD_LENGTH];

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(9600);
  delay(1000);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  bool startup_received = false;
  bool ssid_received = false;
  bool handshake_complete = false;
  for (;; handshake_complete == false) {
    //Startup... waiting for the UNO

    //Send startup message
    if (startup_received == false) {
      Serial.print("startup\n");
      delay(1000);
    }
    
    char serial_input[128];
    memset(serial_input, 0, sizeof(serial_input));
    int characters_received = Serial.readBytesUntil('\n', serial_input, sizeof(serial_input));
    if (characters_received > 0) {
      //look for an echo
      int compare = strncmp(serial_input, "startup", sizeof("startup") - 1);
      if (0 == compare) {
        Serial.print("1");
        startup_received = true;
      } else if (startup_received && !ssid_received) {
        Serial.print("2");
        strncpy(ssid, serial_input, sizeof(ssid));
        ssid_received = true;
      } else if (ssid_received) {
        Serial.print("3");
        strncpy(password, serial_input, sizeof(password));
        Serial.print(" ESP got credentials!\n");
        Serial.print(ssid);
        Serial.print("\n");
        Serial.print(password);
        Serial.print("\n");
        Serial.flush();
        handshake_complete = true;
        break;
      }
    }
  }
  
  //Trim the newline character from the end of ssid and password
  char * newline = strchr(ssid, '\n');
  if (newline) {
    *newline = '\0';//Replace with a terminator
  }
  newline = strchr(password, '\n');
  if (newline) {
    *newline = '\0';//Replace with a terminator
  }
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  
  Serial.print("Connecting to ");
  
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}


void loop() {
  char serialInput[64];
  if (Serial.available()) {
    int characters_received = Serial.readBytesUntil('\n', serialInput, sizeof(serialInput));
    if (characters_received > 0) {
      switch (serialInput[0]) {
        case 'u':
          Serial.println("o;1:1.0,2:1.5,10:0.5");
        break;
        default:
        //Nothing we can do, nowhere to log the problem.
        break;
      }
    }
  }
  delay(2000);  
}

void loop2() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;
  if (req.indexOf("/gpio/0") != -1)
    val = 0;
  else if (req.indexOf("/gpio/1") != -1)
    val = 1;
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(2, val);
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val)?"high":"low";
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}
