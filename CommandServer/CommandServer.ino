#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "wifipass.h" // WIFIPASS_AP and WIFIPASS_PASS

ESP8266WebServer server(80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  Serial.begin(115200);

  /*
  Serial.println(WiFi.softAP("ESPsoftAP_01") ? "SoftAP ready" : "SoftAP failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
  */
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFIPASS_AP, WIFIPASS_PASS);
  
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.println("."); }
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    digitalWrite(LED_BUILTIN, 1);

    if(server.method() == HTTP_POST) {
      String msg = "Command: " + server.arg("cmd") + "\n";
      
      if(server.arg("cmd") == "write") cmdWrite(msg);
      else if(server.arg("cmd") == "read") cmdRead(msg);
      else msg += "Unknown command!\n";
      
      server.send(200, "text/plain", msg);
    } else {
      server.send(200, "text/plain", "Hello, GETter!\r\n");
    }
    
    digitalWrite(LED_BUILTIN, 0);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void cmdWrite(String & msg) {
  const String & data = server.arg("data");
  int len = server.arg("data").length();
  if(len > 127) len = 127;
  EEPROM.begin(128);
  EEPROM.write(0, (char)len);
  for(int i=0; i<len; i++) EEPROM.write(1+i, (char)data[i]);
  if(EEPROM.commit()) {
    msg += "EEPROM saved!";
    Serial.println("EEPROM successfully committed");
  } else {
    msg += "EEPROM failed!";
    Serial.println("EEPROM commit failed!");
  }
}

void cmdRead(String & msg) {
  msg += "EEPROM content: ";
  EEPROM.begin(128);
  int len = EEPROM.read(0);
  for(int i=0; i<len && i<127; i++) msg += (char)EEPROM.read(1+i);
  msg += "\n";
}
