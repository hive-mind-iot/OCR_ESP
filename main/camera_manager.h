#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_camera.h"

#define BOARD_ESP32CAM_AITHINKER 1

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

esp_err_t camera_init(void);
camera_fb_t* camera_get_frame(void);

#endif