#pragma once

#define RGBLIGHT_LAYERS
#define RGBLIGHT_LAYERS_OVERRIDE_RGB_OFF

// settings for the oled keyboard demo with Adafruit 0.91" OLED display on the Stemma QT port
#define OLED_DISPLAY_128X32
#define OLED_DISPLAY_ADDRESS 0x3C
#define OLED_BRIGHTNESS 255

#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN GP26
#define I2C1_SCL_PIN GP27
#define WS2812_PIO_USE_PIO0

#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_RGB

#define TAPPING_TOGGLE 2
