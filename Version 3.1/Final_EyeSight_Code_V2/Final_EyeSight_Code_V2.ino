#include "esp_camera.h"
#include <WiFi.h>
#include "AsyncUDP.h"
#include<bits/stdc++.h>

AsyncUDP udp;

using namespace std;

#define CAMERA_MODEL_AI_THINKER // Currently the only supported model of board

// Pin definition
#define pinSW 12
#define pinNW 13
#define pinNE 14
#define pinSE 15

int timeSinceOn = 0;
bool isPressed = false;
IPAddress DeviceIPAddress = IPAddress("192.168.0.100");
uint16_t DevicePort = 0;
bool receivedFirstTransmission = false;

//  NW    NE
// 
//  SW    SE

#include "camera_pins.h"

const char *STASSID = "ACT102597331974";
const char *STAPWD = "86255264";

void startCameraServer();

void setup() {
  Serial.begin(115200);

  camera_config_t config = setupConfig();

  WiFi.begin(STASSID, STAPWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("Starting camera server");

  startCameraServer();

  Serial.println("Started camera server");

  if (udp.listen(1234)) {
    Serial.print("Listening on IP: ");
    Serial.println(WiFi.localIP());

    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("Received Package: ");
      int data = *packet.data();
      
      if (data==1) {
        digitalWrite(pinSW, HIGH);
        digitalWrite(pinNW, HIGH);
        digitalWrite(pinNE, LOW);
        digitalWrite(pinSE, LOW);
      }
      else if (data==2) {
        digitalWrite(pinSW, LOW);
        digitalWrite(pinNW, HIGH);
        digitalWrite(pinNE, HIGH);
        digitalWrite(pinSE, LOW);
      }
      else if (data==3) {
        digitalWrite(pinSW, LOW);
        digitalWrite(pinNW, LOW);
        digitalWrite(pinNE, HIGH);
        digitalWrite(pinSE, HIGH);
      }
      else if (data==4) {
        digitalWrite(pinSW, LOW);
        digitalWrite(pinNW, LOW);
        digitalWrite(pinNE, LOW);
        digitalWrite(pinSE, LOW);
      }

      Serial.println();
      packet.printf("Recieved data");

      if (!receivedFirstTransmission) {
        DeviceIPAddress = packet.remoteIP();
        Serial.println(DeviceIPAddress);
        DevicePort = 1235;//packet.remotePort();
        packet.print("received, thanks");
        digitalWrite(4, HIGH);
        delay(1000);
        digitalWrite(4, LOW);
        receivedFirstTransmission = true;
      }
    });
  }

  Serial.println("Set handler for UDP");

  // -----------------------

  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, INPUT);
  pinMode(4, OUTPUT);

  Serial.print("After connecting to the WiFi network ");
  Serial.print(STASSID);
  Serial.print(", use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {

  // if (receivedFirstTransmission) {

  //   Serial.println("Checking pin 16");

  //   bool isPressedNow = !digitalRead(16);

  //   if (isPressed && isPressedNow) {
  //     timeSinceOn += 100;
  //   }
  //   else if (!isPressed && isPressedNow) {
  //     timeSinceOn = 0;
  //   }
  //   else if (isPressed && !isPressedNow) {
  //     // handle things here
  //     if (timeSinceOn > 1500) {
  //       udp.connect(IPAddress(DeviceIPAddress), DevicePort);
  //       udp.print("Long");
  //       udp.close();
  //     }
  //     else {
  //       udp.connect(IPAddress(DeviceIPAddress), DevicePort);
  //       udp.print("Short");
  //       udp.close();
  //     }

  //     timeSinceOn = 0;
  //   }

  //   isPressed = isPressedNow;
  // }

   delay(100);
}

















camera_config_t setupConfig() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return config;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  return config;
}
