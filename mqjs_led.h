/*
 * MicroQuickJS LED Driver for ESP32-S3
 * WS2812 RGB LED control
 */
#ifndef MQJS_LED_H
#define MQJS_LED_H

#include "mquickjs.h"

/* JavaScript binding functions */
JSValue js_led_init(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_led_rgb(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_led_on(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_led_off(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#endif /* MQJS_LED_H */

