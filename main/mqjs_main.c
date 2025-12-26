#include <stdio.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mquickjs.h"
/* js_stdlib is defined in mqjs_stdlib.h within mqjs.c */
extern const JSSTDLibraryDef js_stdlib;

/* Provided by mqjs.c */
void mqjs_repl(JSContext *ctx);

#ifndef CONFIG_MQJS_MEM_SIZE
#define CONFIG_MQJS_MEM_SIZE (256 * 1024)
#endif

#define MQJS_MEM_SIZE CONFIG_MQJS_MEM_SIZE

static const char *TAG = "mqjs";

/* Log function for JS_PrintValueF and other debug output */
static void esp_js_log_func(void *opaque, const void *buf, size_t buf_len)
{
    fwrite(buf, 1, buf_len, stdout);
}

void app_main(void)
{
    static uint8_t js_mem[MQJS_MEM_SIZE];
    JSContext *ctx;
    struct timeval tv;

    /* Ensure console I/O is unbuffered over USB/UART */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    ESP_LOGI(TAG, "Starting MicroQuickJS REPL (heap %d bytes)", (int)sizeof(js_mem));
    ESP_LOGI(TAG, "Connect a terminal to the USB serial/JTAG port to interact.");

    ctx = JS_NewContext(js_mem, sizeof(js_mem), &js_stdlib);
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to create JS context");
        return;
    }

    /* Set log function so JS_PrintValueF works (used by print()) */
    JS_SetLogFunc(ctx, esp_js_log_func);

    gettimeofday(&tv, NULL);
    JS_SetRandomSeed(ctx, ((uint64_t)tv.tv_sec << 32) | tv.tv_usec);

    mqjs_repl(ctx);

    JS_FreeContext(ctx);
    vTaskDelay(portMAX_DELAY); /* keep task alive after REPL exit */
}

