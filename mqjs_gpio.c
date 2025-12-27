/*
 * MicroQuickJS GPIO Driver for ESP32
 * General Purpose Input/Output control
 *
 * JavaScript API:
 *   gpio.init(pin, mode)    - Initialize GPIO pin (mode: "in", "out", "in_pullup", "in_pulldown")
 *   gpio.write(pin, level)  - Set GPIO output level (0 or 1)
 *   gpio.read(pin)          - Read GPIO input level (returns 0 or 1)
 *   gpio.setPull(pin, pull) - Set pull-up/pull-down (pull: "up", "down", "none")
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mquickjs.h"

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "mqjs_gpio";

/* Track initialized pins */
#define MAX_GPIO_PINS 64
static uint8_t gpio_initialized[MAX_GPIO_PINS] = {0};

static int gpio_hw_init(int pin, const char *mode_str)
{
    if (pin < 0 || pin >= MAX_GPIO_PINS) {
        ESP_LOGE(TAG, "Invalid GPIO pin: %d", pin);
        return -1;
    }
    
    gpio_config_t io_conf = {0};
    
    /* Parse mode string */
    if (strcmp(mode_str, "out") == 0) {
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else if (strcmp(mode_str, "in") == 0) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else if (strcmp(mode_str, "in_pullup") == 0) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else if (strcmp(mode_str, "in_pulldown") == 0) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    } else {
        ESP_LOGE(TAG, "Invalid mode: %s (use: out, in, in_pullup, in_pulldown)", mode_str);
        return -1;
    }
    
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", pin, esp_err_to_name(ret));
        return -1;
    }
    
    gpio_initialized[pin] = 1;
    ESP_LOGI(TAG, "GPIO %d initialized as %s", pin, mode_str);
    return 0;
}

static int gpio_hw_write(int pin, int level)
{
    if (pin < 0 || pin >= MAX_GPIO_PINS || !gpio_initialized[pin]) {
        ESP_LOGW(TAG, "GPIO %d not initialized - call gpio.init() first", pin);
        return -1;
    }
    
    esp_err_t ret = gpio_set_level(pin, level ? 1 : 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d level: %s", pin, esp_err_to_name(ret));
        return -1;
    }
    return 0;
}

static int gpio_hw_read(int pin)
{
    if (pin < 0 || pin >= MAX_GPIO_PINS || !gpio_initialized[pin]) {
        ESP_LOGW(TAG, "GPIO %d not initialized - call gpio.init() first", pin);
        return -1;
    }
    
    return gpio_get_level(pin);
}

static int gpio_hw_set_pull(int pin, const char *pull_str)
{
    if (pin < 0 || pin >= MAX_GPIO_PINS || !gpio_initialized[pin]) {
        ESP_LOGW(TAG, "GPIO %d not initialized - call gpio.init() first", pin);
        return -1;
    }
    
    if (strcmp(pull_str, "up") == 0) {
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    } else if (strcmp(pull_str, "down") == 0) {
        gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
    } else if (strcmp(pull_str, "none") == 0) {
        gpio_set_pull_mode(pin, GPIO_FLOATING);
    } else {
        ESP_LOGE(TAG, "Invalid pull mode: %s (use: up, down, none)", pull_str);
        return -1;
    }
    
    ESP_LOGI(TAG, "GPIO %d pull set to %s", pin, pull_str);
    return 0;
}

#else
/* Stub for non-ESP builds */
static uint8_t gpio_initialized[64] = {0};
static int gpio_levels[64] = {0};

static int gpio_hw_init(int pin, const char *mode_str) {
    if (pin < 0 || pin >= 64) {
        printf("[GPIO] Invalid pin: %d\n", pin);
        return -1;
    }
    printf("[GPIO] init(pin=%d, mode=%s)\n", pin, mode_str);
    gpio_initialized[pin] = 1;
    return 0;
}

static int gpio_hw_write(int pin, int level) {
    if (pin < 0 || pin >= 64 || !gpio_initialized[pin]) {
        printf("[GPIO] Pin %d not initialized\n", pin);
        return -1;
    }
    printf("[GPIO] write(pin=%d, level=%d)\n", pin, level);
    gpio_levels[pin] = level;
    return 0;
}

static int gpio_hw_read(int pin) {
    if (pin < 0 || pin >= 64 || !gpio_initialized[pin]) {
        printf("[GPIO] Pin %d not initialized\n", pin);
        return -1;
    }
    printf("[GPIO] read(pin=%d) -> %d\n", pin, gpio_levels[pin]);
    return gpio_levels[pin];
}

static int gpio_hw_set_pull(int pin, const char *pull_str) {
    if (pin < 0 || pin >= 64 || !gpio_initialized[pin]) {
        printf("[GPIO] Pin %d not initialized\n", pin);
        return -1;
    }
    printf("[GPIO] setPull(pin=%d, pull=%s)\n", pin, pull_str);
    return 0;
}
#endif

/*
 * JavaScript bindings
 */

/* gpio.init(pin, mode) - Initialize GPIO pin */
JSValue js_gpio_init(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int pin;
    const char *mode = "out";
    JSCStringBuf mode_buf;
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "gpio.init() requires at least pin number");
    }
    
    if (JS_ToInt32(ctx, &pin, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    if (argc >= 2) {
        mode = JS_ToCString(ctx, argv[1], &mode_buf);
        if (!mode) {
            return JS_EXCEPTION;
        }
    }
    
    if (gpio_hw_init(pin, mode) < 0) {
        return JS_ThrowInternalError(ctx, "failed to initialize GPIO %d", pin);
    }
    
    return JS_UNDEFINED;
}

/* gpio.write(pin, level) - Set GPIO output level */
JSValue js_gpio_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int pin, level;
    
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "gpio.write() requires pin and level");
    }
    
    if (JS_ToInt32(ctx, &pin, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    if (JS_ToInt32(ctx, &level, argv[1]) < 0) {
        return JS_EXCEPTION;
    }
    
    if (gpio_hw_write(pin, level) < 0) {
        return JS_ThrowInternalError(ctx, "failed to write GPIO %d", pin);
    }
    
    return JS_UNDEFINED;
}

/* gpio.read(pin) - Read GPIO input level */
JSValue js_gpio_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int pin;
    
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "gpio.read() requires pin number");
    }
    
    if (JS_ToInt32(ctx, &pin, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    int level = gpio_hw_read(pin);
    if (level < 0) {
        return JS_ThrowInternalError(ctx, "failed to read GPIO %d", pin);
    }
    
    return JS_NewInt32(ctx, level);
}

/* gpio.setPull(pin, pull) - Set pull-up/pull-down */
JSValue js_gpio_setPull(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int pin;
    const char *pull = "none";
    JSCStringBuf pull_buf;
    
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "gpio.setPull() requires pin and pull mode");
    }
    
    if (JS_ToInt32(ctx, &pin, argv[0]) < 0) {
        return JS_EXCEPTION;
    }
    
    pull = JS_ToCString(ctx, argv[1], &pull_buf);
    if (!pull) {
        return JS_EXCEPTION;
    }
    
    if (gpio_hw_set_pull(pin, pull) < 0) {
        return JS_ThrowInternalError(ctx, "failed to set pull for GPIO %d", pin);
    }
    
    return JS_UNDEFINED;
}

