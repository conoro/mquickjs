/*
 * MicroQuickJS GPIO Driver for ESP32
 * General Purpose Input/Output control
 */
#ifndef MQJS_GPIO_H
#define MQJS_GPIO_H

#include "mquickjs.h"

/* JavaScript binding functions */
JSValue js_gpio_init(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_gpio_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_gpio_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_gpio_setPull(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

#endif /* MQJS_GPIO_H */

