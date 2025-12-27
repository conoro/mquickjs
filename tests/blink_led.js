/*
 * Simple LED Blink Example for ESP32-H2
 * 
 * Hardware Setup:
 *   - Connect an LED (with current-limiting resistor ~220-470Ω) to a GPIO pin
 *   - Connect LED anode (+) to GPIO pin (e.g., GPIO 8)
 *   - Connect LED cathode (-) through resistor to GND
 * 
 * Usage:
 *   - Load this script in the REPL or modify the GPIO pin number below
 */

// Initialize GPIO pin 8 as output (change this to your LED pin)
var LED_PIN = 8;
gpio.init(LED_PIN, "out");

// Blink function
function blink() {
  // Turn LED on (set to high)
  gpio.write(LED_PIN, 1);
  
  // Wait 500ms, then turn off
  setTimeout(function() {
    gpio.write(LED_PIN, 0);
    
    // Wait another 500ms, then repeat
    setTimeout(blink, 500);
  }, 500);
}

// Start blinking
blink();

// Alternative: Simple toggle pattern
function toggle() {
  var state = gpio.read(LED_PIN);
  gpio.write(LED_PIN, state ? 0 : 1);
  setTimeout(toggle, 500);
}

// Uncomment to use toggle instead:
// toggle();

