#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "wifipass.h" // WIFIPASS_AP and WIFIPASS_PASS. Comment out to use softAP
#include "sha256.h"

ESP8266WebServer server(80);

BYTE secret[SHA256_BLOCK_SIZE]; // 32 bytes of secret

#define HEX(n) ((char)((n) < 10 ? (n)+'0' : ((n)-10)+'a'))
#define UNIB(n) ((n)>>4)
#define LNIB(n) ((n)&15)

static inline int32_t asm_ccount() {
    int32_t r;
    asm volatile ("rsr %0, ccount" : "=r"(r));
    return r;
}

void echo(String & msg, const String & str) { // helper
  Serial.print(str);
  msg += str;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  Serial.begin(115200);

#ifndef __WIFIPASS_H
  Serial.println(WiFi.softAP("ESPsoftAP_01") ? "SoftAP ready" : "SoftAP failed!");
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
#else
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFIPASS_AP, WIFIPASS_PASS);
  
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.println("."); }
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());
#endif

  server.on("/", []() {
    digitalWrite(LED_BUILTIN, 0);
    String msg = "";
    
    if(server.method() == HTTP_POST) {
      String str = "Command: ";
      echo(msg, str + server.arg("cmd") + '\n');
      
      if(server.arg("cmd") == "generate") cmdGenerate(msg);
      else if(server.arg("cmd") == "print") cmdPrint(msg);
      else if(server.arg("cmd") == "write") cmdWrite(msg);
      else if(server.arg("cmd") == "read") cmdRead(msg);
      else if(server.arg("cmd") == "set") cmdSet(msg);
      else if(server.arg("cmd") == "sign") cmdSign(msg);
      else echo(msg, "Unknown command!\n");
    }
    
    server.send(200, "text/plain", msg);
    digitalWrite(LED_BUILTIN, 1);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void cmdWrite(String & msg) {
  EEPROM.begin(SHA256_BLOCK_SIZE);
  for(int i=0; i<SHA256_BLOCK_SIZE; i++) EEPROM.write(i, secret[i]);
  echo(msg, EEPROM.commit() ? "Secret written to EEPROM.\n" : "EEPROM write failed!\n");
}

void cmdRead(String & msg) {
  EEPROM.begin(SHA256_BLOCK_SIZE);
  for(int i=0; i<SHA256_BLOCK_SIZE; i++) secret[i] = EEPROM.read(i);
  echo(msg, "Secret read from EEPROM.\n");
}

void cmdGenerate(String & msg) {
  // Seed elements
  const char * data = server.arg("data").c_str();
  int adc = analogRead(A0);
  int32_t clk = asm_ccount();

  SHA256_CTX ctx;

  sha256_init(&ctx);
  sha256_update(&ctx, (const BYTE *)data, strlen(data));
  sha256_update(&ctx, (const BYTE *)&adc, sizeof(adc));
  sha256_update(&ctx, (const BYTE *)&clk, sizeof(clk));
  sha256_final(&ctx, secret);

  echo(msg, "New secret generated! Use 'write' to commit to EEPROM.\n");
}

void cmdPrint(String & msg) {
  int sum = 0;
  for(int i=0; i<SHA256_BLOCK_SIZE; i++) {
    Serial.print(HEX(UNIB(secret[i])));
    Serial.print(HEX(LNIB(secret[i])));
    sum += secret[i];
  }
  String str = "Sum of secret ";
  echo(msg, str + sum + "\n");
}

void cmdSet(String & msg) {
  echo(msg, "Not implemented yet!\n");
}

void cmdSign(String & msg) {
  echo(msg, "Not implemented yet!\n");
}
