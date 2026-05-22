
#include "quantum.h"
#include "i2c_master.h"

const rgblight_segment_t PROGMEM caps_lock_layer[] = RGBLIGHT_LAYER_SEGMENTS(
    {64, 1, 0, 0, 128}
);

const rgblight_segment_t PROGMEM num_lock_layer[] = RGBLIGHT_LAYER_SEGMENTS(
    {16, 1, 170, 255, 128}
);

// Layer 1 active: turn all LEDs off, then highlight functional keys.
// Replace the 0-placeholders below with the actual LED index for each key
// once you know the strip layout. {LED_INDEX, 1, HUE, SAT, VAL}
const rgblight_segment_t PROGMEM fn_layer_rgb[] = RGBLIGHT_LAYER_SEGMENTS(
    {0,  93, 0, 0, 0},    // all off (base — must come first)
    {4,  1, 168, 255, 180}, // F4    → Control Panel
    {17,  1,   0, 255, 180}, // BkSp  → Delete
    {14,  1,   0,   0, 180}, // Np-   → EEPROM clear
    {15,  1,   0,   0, 180}, // Np/   → Reboot
    {16,  1,   0,   0, 180}, // Np*   → Boot mode
    {7,  1,  128, 255, 180}, // F7    → Disp Bri-
    {8,  1,  128, 255, 180}, // F8    → Disp Bri+
    {9,  1,  85, 255, 180}, // F9    → Mute
    {10,  1,  85, 255, 180}, // F10   → Vol-
    {11,  1,  85, 255, 180}, // F11   → Vol+
    {72,  1,  85, 255, 180}, // M     → Mute
    {68,  1, 213, 255, 180}, // C     → Calculator
    {89,  1,  43, 255, 180}, // Space → Play/Pause
    {70,  1,  43, 255, 180}, // B     → Prev track
    {71,  1,  43, 255, 180}, // N     → Next track
    {76,  1, 170, 255, 180}, // Up    → RGB Bri+
    {86,  1, 170, 255, 180}, // Left  → RGB Mode-
    {85,  1, 170, 255, 180}, // Down  → RGB Hue+
    {84,  1, 170, 255, 180}, // Right → RGB Mode+
    {58,  1, 213, 255, 180}  // H     → Help
);

const rgblight_segment_t* const PROGMEM rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    caps_lock_layer,
    num_lock_layer,
    fn_layer_rgb
);

void keyboard_post_init_kb(void) {
    rgblight_layers = rgb_layers;
    keyboard_post_init_user();
}

bool led_update_kb(led_t led_state) {
    if (led_update_user(led_state)) {
        rgblight_set_layer_state(0, led_state.caps_lock);
        rgblight_set_layer_state(1, led_state.num_lock);
    }
    return true;
}
