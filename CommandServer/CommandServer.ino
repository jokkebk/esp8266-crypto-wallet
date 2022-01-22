#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "wifipass.h" // WIFIPASS_AP and WIFIPASS_PASS
#include "sha256.h"

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
      else if(server.arg("cmd") == "sha") cmdSha(msg);
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

#include "sha256.h"

void appendHex(String & msg, BYTE hex) {
  int U = hex >> 4, L = hex & 15;
  msg += (char)((U < 10) ? U+'0' : U+'A');
  msg += (char)((L < 10) ? L+'0' : L+'A');
}
void cmdSha(String & msg) {
  const char * data = server.arg("data").c_str();
  BYTE buf[SHA256_BLOCK_SIZE];
  SHA256_CTX ctx;
  int idx;
  int pass = 1;

  sha256_init(&ctx);
  sha256_update(&ctx, (const BYTE *)data, strlen(data));
  sha256_final(&ctx, buf);
  msg += "SHA256 digest ";
  for(int i=0; i<SHA256_BLOCK_SIZE; i++)
    appendHex(msg, buf[i]);
}
