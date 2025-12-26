/*
 * MicroQuickJS LED Driver for ESP32-S3
 * WS2812 RGB LED control via RMT peripheral
 *
 * JavaScript API:
 *   led.init(gpio)    - Initialize LED on specified GPIO (default 38)
 *   led.rgb(r, g, b)  - Set LED color (0-255)
 *   led.on()          - Turn LED on (restores last color)
 *   led.off()         - Turn LED off (remembers color)
 */

#include <stdio.h>
#include <stdlib.h>
#include "mquickjs.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

static const char *TAG = "mqjs_led";

/* WS2812 timing (RMT resolution 10MHz = 100ns per tick) */
#define WS2812_T0H_TICKS 4   /* ~350ns high for bit 0 */
#define WS2812_T0L_TICKS 9   /* ~850ns low for bit 0 */
#define WS2812_T1H_TICKS 8   /* ~700ns high for bit 1 */
#define WS2812_T1L_TICKS 5   /* ~500ns low for bit 1 */
#define RMT_RESOLUTION_HZ 10000000

/* LED state */
static rmt_channel_handle_t led_rmt_chan = NULL;
static rmt_encoder_handle_t led_rmt_encoder = NULL;
static int led_gpio_pin = -1;
static uint8_t led_saved_r = 64, led_saved_g = 64, led_saved_b = 64;

/* WS2812 encoder */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} ws2812_encoder_t;

static size_t ws2812_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                            const void *primary_data, size_t data_size,
                            rmt_encode_state_t *ret_state)
{
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded = 0;

    switch (ws->state) {
    case 0:
        encoded += ws->bytes_encoder->encode(ws->bytes_encoder, channel,
                                             primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) ws->state = 1;
        if (session_state & RMT_ENCODING_MEM_FULL) { state |= RMT_ENCODING_MEM_FULL; goto out; }
        /* fall through */
    case 1:
        encoded += ws->copy_encoder->encode(ws->copy_encoder, channel,
                                            &ws->reset_code, sizeof(ws->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) { ws->state = 0; state |= RMT_ENCODING_COMPLETE; }
        if (session_state & RMT_ENCODING_MEM_FULL) { state |= RMT_ENCODING_MEM_FULL; goto out; }
    }
out:
    *ret_state = state;
    return encoded;
}

static esp_err_t ws2812_reset(rmt_encoder_t *encoder) {
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encoder_reset(ws->bytes_encoder);
    rmt_encoder_reset(ws->copy_encoder);
    ws->state = 0;
    return ESP_OK;
}

static esp_err_t ws2812_delete(rmt_encoder_t *encoder) {
    ws2812_encoder_t *ws = __containerof(encoder, ws2812_encoder_t, base);
    rmt_del_encoder(ws->bytes_encoder);
    rmt_del_encoder(ws->copy_encoder);
    free(ws);
    return ESP_OK;
}

static int led_hw_init(int gpio_pin)
{
    esp_err_t ret;
    
    /* Clean up if already initialized */
    if (led_rmt_chan) {
        rmt_disable(led_rmt_chan);
        rmt_del_channel(led_rmt_chan);
        led_rmt_chan = NULL;
    }
    if (led_rmt_encoder) {
        led_rmt_encoder->del(led_rmt_encoder);
        led_rmt_encoder = NULL;
    }
    
    ESP_LOGI(TAG, "Initializing WS2812 LED on GPIO %d", gpio_pin);
    
    rmt_tx_channel_config_t tx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_pin,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ret = rmt_new_tx_channel(&tx_cfg, &led_rmt_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT channel: %s", esp_err_to_name(ret));
        return -1;
    }
    
    /* Create WS2812 encoder */
    ws2812_encoder_t *ws = calloc(1, sizeof(ws2812_encoder_t));
    if (!ws) {
        rmt_del_channel(led_rmt_chan);
        led_rmt_chan = NULL;
        return -1;
    }
    
    ws->base.encode = ws2812_encode;
    ws->base.del = ws2812_delete;
    ws->base.reset = ws2812_reset;
    
    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = { .level0 = 1, .duration0 = WS2812_T0H_TICKS, .level1 = 0, .duration1 = WS2812_T0L_TICKS },
        .bit1 = { .level0 = 1, .duration0 = WS2812_T1H_TICKS, .level1 = 0, .duration1 = WS2812_T1L_TICKS },
        .flags.msb_first = 1,
    };
    ret = rmt_new_bytes_encoder(&bytes_cfg, &ws->bytes_encoder);
    if (ret != ESP_OK) {
        free(ws);
        rmt_del_channel(led_rmt_chan);
        led_rmt_chan = NULL;
        return -1;
    }
    
    rmt_copy_encoder_config_t copy_cfg = {};
    ret = rmt_new_copy_encoder(&copy_cfg, &ws->copy_encoder);
    if (ret != ESP_OK) {
        rmt_del_encoder(ws->bytes_encoder);
        free(ws);
        rmt_del_channel(led_rmt_chan);
        led_rmt_chan = NULL;
        return -1;
    }
    
    /* Reset code: low for ~280us */
    ws->reset_code = (rmt_symbol_word_t){ .level0 = 0, .duration0 = 280, .level1 = 0, .duration1 = 280 };
    led_rmt_encoder = &ws->base;
    
    ret = rmt_enable(led_rmt_chan);
    if (ret != ESP_OK) {
        led_rmt_encoder->del(led_rmt_encoder);
        led_rmt_encoder = NULL;
        rmt_del_channel(led_rmt_chan);
        led_rmt_chan = NULL;
        return -1;
    }
    
    led_gpio_pin = gpio_pin;
    ESP_LOGI(TAG, "LED initialized on GPIO %d", gpio_pin);
    return 0;
}

static void led_hw_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (!led_rmt_chan) {
        ESP_LOGW(TAG, "LED not initialized - call led.init(gpio) first");
        return;
    }
    
    /* WS2812 uses GRB order */
    uint8_t grb[3] = { g, r, b };
    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    rmt_transmit(led_rmt_chan, led_rmt_encoder, grb, sizeof(grb), &tx_cfg);
    rmt_tx_wait_all_done(led_rmt_chan, portMAX_DELAY);
}

#else
/* Stub for non-ESP builds */
static int led_gpio_pin = -1;
static uint8_t led_saved_r = 64, led_saved_g = 64, led_saved_b = 64;

static int led_hw_init(int gpio_pin) {
    printf("[LED] init(gpio=%d)\n", gpio_pin);
    led_gpio_pin = gpio_pin;
    return 0;
}

static void led_hw_set(uint8_t r, uint8_t g, uint8_t b) {
    if (led_gpio_pin < 0) {
        printf("[LED] not initialized - call led.init(gpio) first\n");
        return;
    }
    printf("[LED] rgb(%d, %d, %d) on GPIO %d\n", r, g, b, led_gpio_pin);
}
#endif

/*
 * JavaScript bindings
 */

/* led.init(gpio) - Initialize LED on specified GPIO */
JSValue js_led_init(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int gpio = 38; /* default for some ESP32-S3 devkits */
    if (argc >= 1) JS_ToInt32(ctx, &gpio, argv[0]);
    
    if (led_hw_init(gpio) < 0) {
        return JS_ThrowInternalError(ctx, "failed to initialize LED on GPIO %d", gpio);
    }
    return JS_UNDEFINED;
}

/* led.rgb(r, g, b) - Set LED color */
JSValue js_led_rgb(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int r = 255, g = 255, b = 255;
    if (argc >= 1) JS_ToInt32(ctx, &r, argv[0]);
    if (argc >= 2) JS_ToInt32(ctx, &g, argv[1]);
    if (argc >= 3) JS_ToInt32(ctx, &b, argv[2]);
    
    /* Clamp to 0-255 */
    r = (r < 0) ? 0 : (r > 255) ? 255 : r;
    g = (g < 0) ? 0 : (g > 255) ? 255 : g;
    b = (b < 0) ? 0 : (b > 255) ? 255 : b;
    
    led_saved_r = r; led_saved_g = g; led_saved_b = b;
    led_hw_set(r, g, b);
    return JS_UNDEFINED;
}

/* led.on() - Turn LED on with saved color */
JSValue js_led_on(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    led_hw_set(led_saved_r, led_saved_g, led_saved_b);
    return JS_UNDEFINED;
}

/* led.off() - Turn LED off */
JSValue js_led_off(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    led_hw_set(0, 0, 0);
    return JS_UNDEFINED;
}

