#include "esp_camera.h"
#include <WiFi.h>
#include "AsyncUDP.h"
#include <Ticker.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

#define touch1 T2 // Read, long-press is explain
#define touch2 T7 // Pin 9, both GPIO and D9 // Navigation, long-press is scene explanation

#define NE 1
#define NW 4
#define SE 5
#define SW 6


Ticker touchTicker;
float threshold = 0.05;
int initialVal1;
int initialVal2;

int touchDownTime1 = -1; // Time since touchDown, becomes negative if not yet touchedDown
int touchDownTime2 = -1;
int streakTimeOut = 800; // Time required for it to be considered a long press

// ===========================
// Enter your WiFi credentials
// ===========================

const char *ssid = "Auroma2";
const char *password = "airtel1624";
AsyncUDP udp;

void startCameraServer();
void setupLedFlash(int pin);

void checkTouch() {
  int touchVal1 = touchRead(touch1);
  int touchVal2 = touchRead(touch2);
  bool isTouch1Happening = (touchVal1 < ((1.0-threshold)*(initialVal1)))||(touchVal1 > ((1.0+threshold)*(initialVal1)));
  bool isTouch2Happening = (touchVal2 < ((1.0-threshold)*(initialVal2)))||(touchVal2 > ((1.0+threshold)*(initialVal2)));

  //////////////////////////

  if ((touchDownTime1 == -1)&&(isTouch1Happening)) { // This means it was previously not pressed and is now pressed
    touchDownTime1 = 0; // Start the clock!
  }
  
  if ((touchDownTime1 != -1)&&(isTouch1Happening)) { // This means it was previously touched and is still touched
    touchDownTime1 += 50; // Since this runs every 50 ms
  }

  if ((touchDownTime1 != -1)&&(!isTouch1Happening)) { // This means it was previously touched and is not touched anymore
    if (touchDownTime1 > streakTimeOut) {
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      delay(200);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      udp.broadcast("2");
      delay(100);
    } else {
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      udp.broadcast("1");
      delay(100);
    }

    touchDownTime1 = -1;
  }

  // If previously not pressed and still not pressed, don't do anything

  ///////////////////////////

  if ((touchDownTime2 == -1)&&(isTouch2Happening)) { // This means it was previously not pressed and is now pressed
    touchDownTime2 = 0; // Start the clock!
  }
  
  if ((touchDownTime2 != -1)&&(isTouch2Happening)) { // This means it was previously touched and is still touched
    touchDownTime2 += 50; // Since this runs every 50 ms
  }

  if ((touchDownTime2 != -1)&&(!isTouch2Happening)) { // This means it was previously touched and is not touched anymore
    if (touchDownTime2 > streakTimeOut) {
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      delay(200);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      udp.broadcast("4");
      delay(100);
    } else {
      digitalWrite(4, HIGH);
      digitalWrite(5, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      digitalWrite(5, LOW);
      udp.broadcast("3");
      delay(100);
    }

    touchDownTime2 = -1;
  }

  // If previously not pressed and still not pressed, don't do anything
}

void hapticLeft() {
  digitalWrite(NW, HIGH);
  digitalWrite(SW, HIGH);
  digitalWrite(NE, LOW);
  digitalWrite(SE, LOW);
  delay(300);
  digitalWrite(NW, LOW);
  digitalWrite(SW, LOW);
  digitalWrite(NE, LOW);
  digitalWrite(SE, LOW);
  delay(300);
}

void hapticRight() {
  digitalWrite(NE, HIGH);
  digitalWrite(SE, HIGH);
  digitalWrite(NW, LOW);
  digitalWrite(SW, LOW);
  delay(300);
  digitalWrite(NW, LOW);
  digitalWrite(SW, LOW);
  digitalWrite(NE, LOW);
  digitalWrite(SE, LOW);
  delay(300);
}

void hapticFront() {
  digitalWrite(NW, HIGH);
  digitalWrite(NE, HIGH);
  digitalWrite(SW, LOW);
  digitalWrite(SE, LOW);
  delay(300);
  digitalWrite(NW, LOW);
  digitalWrite(NE, LOW);
  digitalWrite(SW, LOW);
  digitalWrite(SE, LOW);
  delay(300);
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(NE, OUTPUT);
  pinMode(NW, OUTPUT);
  pinMode(SE, OUTPUT);
  pinMode(SW, OUTPUT);

  initialVal1 = touchRead(touch1);
  initialVal2 = touchRead(touch2);

  touchTicker.attach_ms(50, checkTouch);

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
    return;
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

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");



  if (udp.listen(1234)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("Received Message: ");
      
      int message = (int)packet.data();

      if (message == 1070464506) {
        hapticLeft();
      }
      // if (message) {
      //   hapticRight();
      // }
      // if (message == 3) {
      //   hapticFront();
      // }

      Serial.println(message);
      //reply to the client
    });
  }
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}