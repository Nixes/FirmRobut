#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "";
const char* password = "";
const char* custom_hostname = "esp-robut";
WiFiUDP server;

// number of milliseconds to wait before stopping motors
const unsigned int packet_timeout = 1000;

unsigned long last_packet; // what the time was when the last packet was received

/*
controller pins
  arduino pin | nodeMCU pin | function
  5 | D1 | Motor A - PWM Speed
  4 | D2 | Motor B - PWM Speed
  0 | D3 | Motor A Direction - Digital
  2 | D4 | Motor B Direction - Digital
*/
const int pin_a_speed = 5;
const int pin_b_speed = 4;
const int pin_a_dir = 0;
const int pin_b_dir = 2;

// note that the led pin is tha sme as pin_b_dir, slightly annoying
const int pin_led = 2;


// set everything to resonable defaults
void resetController() {
  analogWrite(pin_a_speed, 0);
  analogWrite(pin_b_speed, 0);
  digitalWrite(pin_a_dir, 0);
  digitalWrite(pin_b_dir, 0);
}

void resetTimeout() {
  last_packet=millis();
}

void checkTimeout() {
  if ( (millis()-last_packet) > packet_timeout) {
    resetTimeout();

    //Serial.println("UDP Connection Timed Out");
    resetController();
  }
}

// arguments: signed integer between -255 and +255 with the sign indicating motor direction, value the motor speed
void setController (int motor_a, int motor_b) {
  unsigned int motor_a_dir = 0;
  unsigned int motor_b_dir = 0;
  unsigned int motor_a_speed = 0;
  unsigned int motor_b_speed = 0;
  // determine motor direction
  if (motor_a > 0) {
    motor_a_dir = 1;
  }
  if (motor_b > 0) {
    motor_b_dir = 1;
  }

  // determine motor speed
  motor_a_speed = abs(motor_a);
  motor_b_speed = abs(motor_b);

  // send to controller
  analogWrite(pin_a_speed, motor_a_speed);
  analogWrite(pin_b_speed, motor_b_speed);
  digitalWrite(pin_a_dir, motor_a_dir);
  digitalWrite(pin_b_dir, motor_b_dir);

}

void readAscii (Stream& read_stream) {
  Serial.write("Got serial command\n");
  String motor_a_string;
  String motor_b_string;
  int motor_a_int;
  int motor_b_int;

  // read till first delimiter
  char stream_byte = read_stream.read();
  while (stream_byte != ',' ) {
    motor_a_string.concat(stream_byte);
    stream_byte = read_stream.read();
  }
  // get that delimiter out of the buffer
  stream_byte = read_stream.read();

  while (stream_byte != ']') {
    motor_b_string.concat(stream_byte);
    stream_byte = read_stream.read();
  }
  motor_a_int = motor_a_string.toInt();
  motor_b_int = motor_b_string.toInt();

  Serial.write("motor a read: ");
  Serial.print(motor_a_int);
  Serial.println();

  Serial.write("motor b read: ");
  Serial.print(motor_b_int);
  Serial.println();

  setController(motor_a_int,motor_b_int);
}

void checkSerial () {
  if(Serial.available()) {
    delay(100);
    if(Serial.read() == '[') {
      // we got a motor command
      readAscii(Serial);
      resetTimeout();
    }
  }
}

void checkUdp() {
  int cb = server.parsePacket();
  if (cb) {
    while (server.available()) {
      Serial.write("Got udp packet\n");
      if (server.read() == '[') {
        // we got a motor command
        readAscii(server);
        resetTimeout();
      }
    }
  }
}

void setup() {
  // setup the motor controller output pins
  pinMode(pin_a_dir, OUTPUT);
  pinMode(pin_b_dir, OUTPUT);
  pinMode(pin_a_speed, OUTPUT);
  pinMode(pin_b_speed, OUTPUT);

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(custom_hostname); // set hostname
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname(custom_hostname);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Remote Update Start");
    resetController(); // you don't want to be driving motors while updating ;)
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nRemote Update Complete, Rebooting");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin(3000); // start udp server
}

void loop() {
  ArduinoOTA.handle();
  checkSerial();
  checkUdp();
  checkTimeout();
}
