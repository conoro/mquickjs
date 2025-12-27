# Building MQuickJS for ESP32-H2 with GPIO Support

This guide covers building and flashing MQuickJS for ESP32-H2 with GPIO control support.

## Prerequisites

1. **Install ESP-IDF v5.x or later**
   - Follow the [official ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32h2/get-started/)
   - On macOS: `brew install esp-idf` or use the official installer

2. **Export ESP-IDF environment** (required in each new terminal session):
   ```bash
   # If installed via official installer:
   . $HOME/esp/esp-idf/export.sh
   
   # Or if IDF_PATH is set:
   . ${IDF_PATH}/export.sh
   ```

## Complete Build Steps for ESP32-H2

### Step 1: Generate 32-bit Headers (Required First Time)

ESP32 chips are 32-bit, while your development machine is likely 64-bit. You must generate 32-bit headers before building:

```bash
make esp32
```

This creates `mqjs_stdlib.h` and `mquickjs_atom.h` with 32-bit data structures.

**Important:** If you run `make` or `make clean` later, you must run `make esp32` again before building for ESP32.

### Step 2: Clean Previous Builds (If Switching Targets)

If you previously built for a different ESP32 target, delete the build directory:

```bash
rm -rf build
```

### Step 3: Set Target to ESP32-H2

```bash
idf.py set-target esp32h2
```

This command:
- Creates/updates `sdkconfig` from `sdkconfig.defaults` + `sdkconfig.defaults.esp32h2`
- Configures the build system for ESP32-H2
- Sets default JavaScript heap size to 128 KB (H2 has less RAM than S3/C6)

### Step 4: Build the Project

```bash
idf.py build
```

First build takes several minutes. Subsequent builds are much faster.

The build includes:
- MQuickJS JavaScript engine
- LED library (WS2812 RGB LEDs)
- **GPIO library** (standard GPIO control)
- REPL over USB Serial/JTAG

### Step 5: Find Your USB Port

```bash
# macOS - list USB serial devices:
ls /dev/cu.usb*

# Linux:
ls /dev/ttyACM* /dev/ttyUSB*

# Common ESP32-H2 ports:
# macOS: /dev/cu.usbmodem14101 (or similar)
# Linux: /dev/ttyACM0
```

### Step 6: Flash to Device

```bash
# macOS example:
idf.py flash -p /dev/cu.usbmodem14101

# Linux example:
idf.py flash -p /dev/ttyACM0

# Or combine build and flash:
idf.py build flash -p /dev/cu.usbmodem14101
```

### Step 7: Open REPL Monitor

```bash
# macOS:
idf.py monitor -p /dev/cu.usbmodem14101

# Linux:
idf.py monitor -p /dev/ttyACM0
```

- Press `Enter` a few times if no prompt appears
- Exit monitor with `Ctrl+]`
- Baud rate is 115200

## Example: Blink a Simple LED

### Hardware Setup

Connect an LED to GPIO 8:
- LED anode (+) → GPIO 8
- LED cathode (-) → 220-470Ω resistor → GND

### JavaScript Code

In the REPL, type or paste:

```javascript
// Initialize GPIO 8 as output
gpio.init(8, "out")

// Blink function
function blink() {
  gpio.write(8, 1)  // Turn LED on
  setTimeout(function() {
    gpio.write(8, 0)  // Turn LED off
    setTimeout(blink, 500)  // Repeat after 500ms
  }, 500)
}

// Start blinking
blink()
```

The LED should now blink every 500ms.

### Alternative: Load from File

If you have the example file `tests/blink_led.js`, you can load it:

```javascript
load("tests/blink_led.js")
```

## GPIO API Reference

```javascript
// Initialize pin as output
gpio.init(pin, "out")

// Initialize pin as input
gpio.init(pin, "in")              // Floating (no pull)
gpio.init(pin, "in_pullup")       // With pull-up resistor
gpio.init(pin, "in_pulldown")     // With pull-down resistor

// Write output (for output pins)
gpio.write(pin, 1)  // High (3.3V)
gpio.write(pin, 0)  // Low (0V)

// Read input (for input pins)
var value = gpio.read(pin)  // Returns 0 or 1

// Change pull resistor (for input pins)
gpio.setPull(pin, "up")    // Enable pull-up
gpio.setPull(pin, "down")  // Enable pull-down
gpio.setPull(pin, "none")  // Disable pull (floating)
```

## Troubleshooting

**"target not set" error:**
```bash
idf.py set-target esp32h2
```

**Build fails after switching targets:**
```bash
rm -rf build
idf.py set-target esp32h2
idf.py build
```

**"initializer element is not computable" errors:**
Regenerate 32-bit headers:
```bash
make esp32
rm -rf build
idf.py set-target esp32h2
idf.py build
```

**Can't find USB port:**
- Ensure USB cable supports data (not charge-only)
- Try different USB port
- Check device manager (Windows) or `system_profiler SPUSBDataType` (macOS)

**No REPL prompt:**
- Press `Enter` several times
- USB Serial/JTAG needs a moment to sync after reset
- Try disconnecting and reconnecting USB cable

**Out of memory:**
- ESP32-H2 has limited RAM (default 128 KB JS heap)
- Reduce code complexity
- Increase heap via `idf.py menuconfig` → `MicroQuickJS` → `MQJS_MEM_SIZE` (if PSRAM available)

## Quick Reference: Complete Build Command Sequence

```bash
# First time setup (or after running 'make clean')
make esp32

# Clean previous build (if switching targets)
rm -rf build

# Set target and build
idf.py set-target esp32h2
idf.py build

# Flash and monitor (replace with your port)
idf.py flash monitor -p /dev/cu.usbmodem14101
```

## Next Steps

- Try the example in `tests/blink_led.js`
- Experiment with different GPIO pins
- Read button inputs with `gpio.read()`
- Control multiple LEDs
- Combine with the `led` library for RGB LED control

