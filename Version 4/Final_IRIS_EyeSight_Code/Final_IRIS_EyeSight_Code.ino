#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "camera_index.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#endif

#define CONFIG_ESP_FACE_RECOGNITION_ENABLED 0
#define CONFIG_ESP_FACE_DETECT_ENABLED      0

#define LED_LEDC_GPIO            4
#define CONFIG_LED_MAX_INTENSITY 255

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
