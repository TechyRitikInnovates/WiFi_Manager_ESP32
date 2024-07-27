#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

const char* apSSID = "AQD-ESP01";
const char* apPassword = "12345678";
const int resetPin = 5; // GPIO 5

WebServer server(80);
Preferences preferences;

String htmlPage = "<!DOCTYPE html>\
<html>\
<head><title>ESP32 WiFi Setup</title></head>\
<body>\
<h2>Enter Wi-Fi Credentials</h2>\
<form action=\"/submit\" method=\"POST\">\
  SSID:<br>\
  <input type=\"text\" name=\"ssid\"><br>\
  Password:<br>\
  <input type=\"password\" name=\"password\"><br><br>\
  <input type=\"submit\" value=\"Submit\">\
</form>\
</body>\
</html>";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSubmit() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    Serial.println("Received SSID: " + ssid);
    Serial.println("Received Password: " + password);

    // Save the credentials to preferences
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    server.send(200, "text/html", "Credentials received, ESP32 will now try to connect to the Wi-Fi.");

    // Attempt to connect to the received Wi-Fi credentials
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    const int maxAttempts = 30; // Increase the number of attempts for a better chance to connect
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      server.send(200, "text/html", "Successfully connected to Wi-Fi. IP Address: " + WiFi.localIP().toString());

      // Stop the Access Point
      WiFi.softAPdisconnect(true);
      Serial.println("Access Point stopped.");
    } else {
      Serial.println("Failed to connect to Wi-Fi.");
      server.send(200, "text/html", "Failed to connect to Wi-Fi. Please try again.");
    }
  } else {
    server.send(400, "text/html", "Invalid request");
  }
}

void setup() {
  Serial.begin(115200);

  // Setup the reset pin
  pinMode(resetPin, INPUT_PULLUP);

  preferences.begin("wifi", true);
  String storedSSID = preferences.getString("ssid", "");
  String storedPassword = preferences.getString("password", "");
  preferences.end();

  if (storedSSID.length() > 0 && storedPassword.length() > 0) {
    Serial.print("Connecting to stored SSID: ");
    Serial.println(storedSSID);
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

    int attempts = 0;
    const int maxAttempts = 30;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(1000);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      return;
    } else {
      Serial.println("Failed to connect to stored Wi-Fi. Starting AP mode.");
    }
  }

  // Start AP mode if no stored credentials or failed to connect
  startAPMode();
}

void loop() {
  server.handleClient();

  // Check if the reset button is held down
  static unsigned long buttonPressTime = 0;
  if (digitalRead(resetPin) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime > 5000) {
      // Button held for more than 5 seconds, reset the Wi-Fi credentials
      preferences.begin("wifi", false);
      preferences.clear();
      preferences.end();
      Serial.println("Wi-Fi credentials erased. Restarting in AP mode...");
      ESP.restart();
    }
  } else {
    buttonPressTime = 0;
  }
}

void startAPMode() {
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();
  Serial.println("HTTP server started");
}
