
## ESP32 (ESP-IDF) REPL

An ESP-IDF application is included to run the MQuickJS REPL over the USB
serial console of supported ESP32 chips.

### Supported Targets

| Target     | Console Interface | Default JS Heap | Notes |
|------------|-------------------|-----------------|-------|
| ESP32-S3   | USB CDC           | 256 KB          | Most devkits have built-in WS2812 LED |
| ESP32-C6   | USB Serial/JTAG   | 256 KB          | WiFi 6 + BLE 5 + Zigbee |
| ESP32-H2   | USB Serial/JTAG   | 128 KB          | Thread/Zigbee focused, less RAM |

### Prerequisites

1. **Install ESP-IDF v5.x or later** following the official guide:
   - [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
   - On macOS, you can also use Homebrew: `brew install esp-idf`

2. **Export the ESP-IDF environment** (required in each new terminal session):
   ```bash
   # If installed via the official installer:
   . $HOME/esp/esp-idf/export.sh
   
   # Or if IDF_PATH is set:
   . ${IDF_PATH}/export.sh
   ```
   
   You should see output like: `Done! You can now compile ESP-IDF projects.`

### Quick Start

Building for ESP32 requires two tools: GNU `make` (for generating headers) and
ESP-IDF's `idf.py` (for compiling and flashing). Here's the complete workflow:

```bash
# Step 1: Generate 32-bit headers (required before first ESP32 build)
make esp32

# Step 2: Select target and build with ESP-IDF
idf.py set-target esp32s3   # or esp32c6, esp32h2
idf.py build

# Step 3: Flash and run
idf.py flash monitor -p /dev/cu.usbmodem14101
```

## Example LED Blinken-Light
- WS2812 RGB LED on GPIO 48 on the ESP32-S3 DevKit 1 (certain versions)
- WS2812 RGB LED on GPIO 8 on the ESP32-C6 Super Mini
- WS2812 RGB LED on GPIO 8 on the ESP32-H2 Super Mini


```js
led.init(8)

function blink() {
  led.rgb(0, 0, 255)
  setTimeout(function() {
    led.off();
    setTimeout(blink, 500)
  }, 500)
}
blink()
```




#### Step-by-Step Details

1. **Generate 32-bit headers** (required before the first ESP32 build):
   ```bash
   make esp32
   ```
   
   This builds a host tool and generates `mqjs_stdlib.h` and `mquickjs_atom.h`
   with 32-bit data structures. ESP32 chips are 32-bit, while your Mac is 64-bit,
   so these headers must be generated specifically for ESP32.
   
   **Important:** If you later run `make` or `make clean` (for host/macOS builds),
   you must run `make esp32` again before your next ESP32 build.

2. **Select your target chip**:
   ```bash
   idf.py set-target esp32s3   # or esp32c6, esp32h2
   ```
   
   This command:
   - Creates/updates `sdkconfig` from `sdkconfig.defaults` + `sdkconfig.defaults.<target>`
   - Configures the build system for the selected chip
   - **Note:** Switching targets requires deleting the `build/` directory first

3. **Build the project**:
   ```bash
   idf.py build
   ```
   
   First build takes several minutes. Subsequent builds are much faster.

4. **Flash to your device**:
   ```bash
   # macOS - find your port with: ls /dev/cu.usb*
   idf.py flash -p /dev/cu.usbmodem14101
   
   # Linux - typically /dev/ttyACM0 or /dev/ttyUSB0
   idf.py flash -p /dev/ttyACM0
   
   # Or combine build and flash:
   idf.py build flash -p /dev/cu.usbmodem14101
   ```

5. **Open the REPL**:
   ```bash
   idf.py monitor -p /dev/cu.usbmodem14101
   ```
   
   - Press `Enter` a few times if no prompt appears
   - Exit monitor with `Ctrl+]`
   - The baud rate is 115200 (any serial terminal works)

### Multi-line Input

The REPL supports multi-line input with automatic bracket/brace counting:

```javascript
mqjs > function greet(name) {
... >   return "Hello, " + name + "!";
... > }
mqjs > greet("ESP32")
"Hello, ESP32!"
```

### LED Control (ESP32-S3)

A `led` object is available for controlling WS2812 RGB LEDs:

```javascript
led.init(38)      // Initialize on GPIO 38 (ESP32-S3-DevKitC-1 built-in)
led.rgb(255,0,0)  // Set to red
led.off()         // Turn off (remembers color)
led.on()          // Restore last color
```

### Build Configuration

Per-target configuration files are provided:
- `sdkconfig.defaults` - Base configuration
- `sdkconfig.defaults.esp32s3` - ESP32-S3 specific (USB CDC console)
- `sdkconfig.defaults.esp32c6` - ESP32-C6 specific (USB Serial/JTAG)
- `sdkconfig.defaults.esp32h2` - ESP32-H2 specific (smaller heap)

Key configuration options (adjustable via `idf.py menuconfig`):

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_MQJS_MEM_SIZE` | 256 KB (S3/C6), 128 KB (H2) | JavaScript heap size |
| `CONFIG_ESP_MAIN_TASK_STACK_SIZE` | 12 KB | Main task stack |
| `CONFIG_ESP_TASK_WDT_EN` | Disabled | Task watchdog (disabled for REPL) |

### Switching Between Targets

**Important:** When switching to a different ESP32 target, you must delete the
`build/` directory first. The build system caches target-specific settings that
are incompatible between chips.

```bash
# WRONG - will fail or produce incorrect builds:
idf.py set-target esp32c6   # After previously building for esp32s3

# CORRECT - delete build directory before switching:
rm -rf build
idf.py set-target esp32c6
idf.py build
```

Note: `idf.py fullclean` can also be used, but if the build directory becomes
corrupted, you may need to use `rm -rf build` instead.

### Building for Multiple Targets

To maintain builds for multiple targets simultaneously, save each build
directory before switching:

```bash
# First, generate 32-bit headers (only needed once)
make esp32

# Build for ESP32-S3
rm -rf build
idf.py set-target esp32s3
idf.py build
cp -r build build_s3

# Build for ESP32-C6
rm -rf build
idf.py set-target esp32c6
idf.py build
cp -r build build_c6

# Build for ESP32-H2
rm -rf build
idf.py set-target esp32h2
idf.py build
cp -r build build_h2
```

To flash a saved build without rebuilding:
```bash
esptool.py --chip esp32s3 -p /dev/cu.usbmodem14101 write_flash @build_s3/flash_args
```

### Project Structure

```
mquickjs/
├── CMakeLists.txt          # ESP-IDF project file
├── main/
│   ├── CMakeLists.txt      # Component configuration
│   ├── Kconfig.projbuild   # Menu configuration
│   └── mqjs_main.c         # ESP32 entry point
├── mqjs_led.c              # WS2812 LED driver
├── mqjs_led.h
├── sdkconfig.defaults*     # Per-target configurations
└── (MQuickJS core sources)
```

### Understanding USB Connections

ESP32 devkits typically have one or two USB-C ports:

| Target   | USB Type | Port Label | Notes |
|----------|----------|------------|-------|
| ESP32-S3 | USB CDC (OTG) | "USB" or "COM" | Native USB, appears as `/dev/cu.usbmodem*` |
| ESP32-S3 | USB-UART | "UART" | Via CP2102/CH340, appears as `/dev/cu.SLAB_USBtoUART*` |
| ESP32-C6 | USB Serial/JTAG | Single port | Built-in, appears as `/dev/cu.usbmodem*` |
| ESP32-H2 | USB Serial/JTAG | Single port | Built-in, appears as `/dev/cu.usbmodem*` |

For ESP32-S3 devkits with two USB ports, use the "COM" or "USB" port (not "UART")
for the best REPL experience with the default configuration.

### Troubleshooting

**Can't find the serial port:**
```bash
# macOS - list all USB serial devices:
ls /dev/cu.usb*

# Linux:
ls /dev/ttyACM* /dev/ttyUSB*
```

**"No such file or directory" when flashing:**
- Ensure the USB cable supports data (not charge-only)
- Try a different USB port
- Check if the device is recognized: `system_profiler SPUSBDataType` (macOS)

**No prompt appears in monitor:**
- Press `Enter` several times
- For C6/H2: The USB Serial/JTAG interface needs a moment to synchronize after reset
- Try disconnecting and reconnecting the USB cable
- Ensure you're using the correct port (see USB Connections above)

**Build fails with "target not set":**
```bash
idf.py set-target esp32s3  # Run this first!
```

**Build fails after switching targets:**
```bash
rm -rf build               # Delete the build directory
idf.py set-target <target>
idf.py build
```

**"initializer element is not computable at load time" errors:**
This happens when 64-bit headers are used instead of 32-bit. ESP32 is 32-bit.
Regenerate the headers:
```bash
make esp32              # Generate 32-bit headers
rm -rf build            # Clean the build directory
idf.py set-target esp32s3  # or your target
idf.py build
```

**"Directory doesn't seem to be a CMake build directory" error:**
The build directory is corrupted. Delete it manually:
```bash
rm -rf build
idf.py set-target <target>
idf.py build
```

**Important:** If you run `make` or `make clean` for the host/macOS build, it
will regenerate or delete the headers with 64-bit versions. Always run
`make esp32` again before building for ESP32.

**"input too long" error:**
The REPL buffer is 4 KB. For larger scripts, use the bytecode loader or
increase `REPL_BUF_SIZE` in `mqjs.c`.

**Out of memory / JS heap exhausted:**
- Increase `CONFIG_MQJS_MEM_SIZE` via `idf.py menuconfig` if your board has PSRAM
- Default is 256 KB for S3/C6, 128 KB for H2
- Reduce the complexity of your JavaScript code

**Garbled output or wrong baud rate:**
The default baud rate is 115200. If using a standalone serial terminal:
```bash
# macOS/Linux with screen:
screen /dev/cu.usbmodem14101 115200

# Or with minicom:
minicom -D /dev/cu.usbmodem14101 -b 115200
```

