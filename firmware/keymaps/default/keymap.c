// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "bmp/anim.h"
#include "drivers/oled/glcdfont.c"
#include <string.h>

#define HOME_TEXT "~Eliise~"  // <- change this text

enum custom_keycodes {
    HLP = SAFE_RANGE,
    FN_TT,
};

static bool    keyboard_ready  = false;

static bool     fn_layer_locked = false;
static uint8_t  fn_tap_count    = 0;
static uint32_t fn_press_ts     = 0;

static uint8_t  anim_frame   = 0;
static uint32_t anim_frame_ts = 0;

static bool     rgb_show        = false;
static uint32_t rgb_show_ts     = 0;
static bool     rgb_show_active = false;
static uint8_t  rgb_show_type   = 0;
#define RGB_SHOW_MS 2000

static bool     help_show        = false;
static uint32_t help_show_ts     = 0;
static bool     help_show_active = false;
static uint8_t  help_page        = 255; // wraps to 0 on first press
#define HELP_SHOW_MS 4000

static const struct { const char *key; const char *desc; } help_entries[] = {
    {"F4",      "Control Panel"},
    {"Space",   "Power"},
    {"BkSp",    "Delete"},
    {"F7",      "Disp Bri-"},
    {"F8",      "Disp Bri+"},
    {"F9",      "Mute"},
    {"F10",     "Vol-"},
    {"F11",     "Vol+"},
    {"M",       "Mute"},
    {"C",       "Calculator"},
    {"V",       "Play/Pause"},
    {"B",       "Prev track"},
    {"N",       "Next track"},
    {"Up",      "RGB Bri+"},
    {"Left",    "RGB Mode-"},
    {"Down",    "RGB Hue+"},
    {"Right",   "RGB Mode+"},
    {"Np-",     "EEPROM clear"},
    {"Np/",     "Reboot"},
    {"Np*",     "Boot mode"},
    {"H",       "Help (cycle)"},
};
#define HELP_ENTRY_COUNT 21
#define HELP_PER_PAGE    3
#define HELP_PAGE_COUNT  ((HELP_ENTRY_COUNT + HELP_PER_PAGE - 1) / HELP_PER_PAGE)

static void scale2x(uint8_t b, uint8_t *lo, uint8_t *hi);

// Render HOME_TEXT at 2x width + 2x height (12x16px per glyph), vertically centered
// in pages 2+3 (rows 16-31), horizontally centered in the left 96px text area.
static void draw_home_screen(void) {
    static uint8_t buf[512];
    memset(buf, 0, sizeof(buf));

    const char *text = HOME_TEXT;
    uint8_t     tpx  = (uint8_t)strlen(text) * 12; // 12px per glyph (2x width)
    uint8_t     xcol = tpx < 96 ? (96 - tpx) / 2 : 0;

    for (const char *s = text; *s && xcol + 1 < 96; s++) {
        for (uint8_t fi = 0; fi < 6 && xcol + 1 < 96; fi++, xcol += 2) {
            uint8_t lo, hi;
            scale2x(pgm_read_byte(&font[(uint8_t)*s * 6 + fi]), &lo, &hi);
            // each source column written twice for 2x width
            buf[2 * 128 + xcol]     = lo;  buf[3 * 128 + xcol]     = hi;
            buf[2 * 128 + xcol + 1] = lo;  buf[3 * 128 + xcol + 1] = hi;
        }
    }

    // Advance animation frame
    if (ANIM_FRAME_COUNT > 1 && timer_elapsed32(anim_frame_ts) >= ANIM_FRAME_MS) {
        anim_frame    = (anim_frame + 1) % ANIM_FRAME_COUNT;
        anim_frame_ts = timer_read32();
    }

    // Animation: columns 96-127, all 4 pages
    for (uint8_t page = 0; page < 4; page++) {
        for (uint8_t c = 0; c < 32; c++) {
            buf[page * 128 + 96 + c] =
                pgm_read_byte(&anim_frames[anim_frame][page * 128 + 96 + c]);
        }
    }

    oled_set_cursor(0, 0);
    oled_write_raw((char *)buf, 512);
}

// 2x vertical scale for the RGB overlay (pages 0+1 or 2+3)
static void scale2x(uint8_t b, uint8_t *lo, uint8_t *hi) {
    *lo = *hi = 0;
    for (uint8_t i = 0; i < 4; i++) {
        if (b & (1 << i))     *lo |= (3 << (i * 2));
        if (b & (1 << (i+4))) *hi |= (3 << (i * 2));
    }
}

static void oled_write_2x(const char *str, uint8_t page) {
    uint8_t lo[128] = {0}, hi[128] = {0};
    uint8_t col = 0;
    for (; *str && col < 128; str++) {
        for (uint8_t fi = 0; fi < 6 && col < 128; fi++, col++) {
            scale2x(pgm_read_byte(&font[(uint8_t)*str * 6 + fi]), &lo[col], &hi[col]);
        }
    }
    oled_set_cursor(0, page);
    oled_write_raw((char *)lo, 128);
    oled_set_cursor(0, page + 1);
    oled_write_raw((char *)hi, 128);
}

static void draw_help_screen(void) {
    static uint8_t buf[512];
    memset(buf, 0, sizeof(buf));

    char line[22];
    uint8_t base = help_page * HELP_PER_PAGE;

    for (uint8_t i = 0; i < HELP_PER_PAGE; i++) {
        uint8_t idx = base + i;
        if (idx < HELP_ENTRY_COUNT) {
            snprintf(line, sizeof(line), "%-8s %-12s", help_entries[idx].key, help_entries[idx].desc);
        } else {
            snprintf(line, sizeof(line), "%-21s", "");
        }
        uint8_t col = 0;
        for (const char *s = line; *s && col < 128; s++) {
            for (uint8_t fi = 0; fi < 6 && col < 128; fi++, col++) {
                buf[i * 128 + col] = pgm_read_byte(&font[(uint8_t)*s * 6 + fi]);
            }
        }
    }

    // page 2 stays blank (already zeroed); write page counter on page 3
    snprintf(line, sizeof(line), "Fn+H: pg %u / %u", (unsigned)(help_page + 1), (unsigned)HELP_PAGE_COUNT);
    uint8_t col = 0;
    for (const char *s = line; *s && col < 128; s++) {
        for (uint8_t fi = 0; fi < 6 && col < 128; fi++, col++) {
            buf[3 * 128 + col] = pgm_read_byte(&font[(uint8_t)*s * 6 + fi]);
        }
    }

    oled_set_cursor(0, 0);
    oled_write_raw((char *)buf, 512);
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    [0] = LAYOUT(
        KC_ESC,  KC_F1,  KC_F2,  KC_F3,  KC_F4,          KC_F5,  KC_F6,  KC_F7,  KC_F8,             KC_F9,   KC_F10, KC_F11,  KC_F12,       KC_PMNS,  KC_PPLS,
        KC_GRV,  KC_1,   KC_2,   KC_3,   KC_4,   KC_5,   KC_6,   KC_7,   KC_8,   KC_9,   KC_0,      KC_MINS, KC_EQL, KC_BSPC, KC_NUM_LOCK,  KC_PSLS,  KC_PAST,
        KC_TAB,  KC_Q,   KC_W,   KC_E,   KC_R,   KC_T,   KC_Y,   KC_U,   KC_I,   KC_O,   KC_P,      KC_LBRC, KC_RBRC,KC_ENT,  KC_P7,        KC_P8,    KC_P9,
        KC_CAPS, KC_A,   KC_S,   KC_D,   KC_F,   KC_G,   KC_H,   KC_J,   KC_K,   KC_L,   KC_SCLN,   KC_QUOT, KC_SLSH,KC_P4,   KC_P5,        KC_P6,
        KC_LSFT, KC_Z,   KC_X,   KC_C,   KC_V,   KC_B,   KC_N,   KC_M,   KC_COMM,KC_DOT, KC_RSFT,   KC_UP,   KC_NUBS,KC_P1,   KC_P2,        KC_P3,    KC_PENT,
        KC_LCTL, KC_LGUI,KC_LALT,                KC_SPC,               KC_RALT,FN_TT,  KC_LEFT,   KC_DOWN, KC_RGHT,         KC_P0,                  KC_PDOT
    ),

    [1] = LAYOUT(
        _______, _______, _______, _______, KC_CPNL,          _______, _______, KC_BRID, KC_BRIU,          KC_MUTE, KC_VOLD, KC_VOLU, _______, EE_CLR, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, KC_DEL, _______, QK_RBT, QK_BOOT,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, HLP,     _______, _______,_______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, KC_CALC, _______, KC_MPRV, KC_MNXT, KC_MUTE, _______, _______, _______, UG_VALU, KC_SLSH, _______, _______,  _______, _______,
        MO(2), _______, _______,                   KC_MPLY,                   FN_TT, _______, UG_PREV, UG_HUEU, UG_NEXT,          _______,          _______
    ),

    [2] = LAYOUT(
        _______, _______, _______, _______, _______,          _______, _______, _______, _______,          _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, MS_UP, _______, _______, _______, _______, _______,
        _______, _______, _______,                   _______,                   _______, _______, MS_LEFT, MS_DOWN, MS_RGHT,          _______,          _______
    )
};

void keyboard_post_init_user(void) {
    keyboard_ready = true;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    if (!layer_state_cmp(state, 1)) {
        fn_layer_locked = false;
        if (keyboard_ready) {
            rgblight_reload_from_eeprom();
        }
        rgblight_set_layer_state(2, false);
    }
    return state;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case FN_TT:
            if (record->event.pressed) {
                if (fn_layer_locked) {
                    fn_layer_locked = false;
                    fn_tap_count    = 0;
                    layer_off(1);
                } else {
                    fn_press_ts = timer_read32();
                    layer_on(1);
                }
            } else {
                if (!fn_layer_locked) {
                    if (timer_elapsed32(fn_press_ts) > TAPPING_TERM) {
                        fn_tap_count = 0;
                        layer_off(1);
                    } else {
                        fn_tap_count++;
                        if (fn_tap_count >= TAPPING_TOGGLE) {
                            fn_layer_locked = true;
                            fn_tap_count    = 0;
                            rgblight_set_layer_state(2, true);
                        } else {
                            layer_off(1);
                        }
                    }
                }
            }
            return false;
        default: break;
    }
    if (!record->event.pressed) return true;
    switch (keycode) {
        case UG_NEXT: case UG_PREV: rgb_show_type = 1; break;
        case UG_HUEU: case UG_HUED: rgb_show_type = 2; break;
        case UG_VALU: case UG_VALD: rgb_show_type = 3; break;
        case HLP:
            help_page = (uint8_t)(help_page + 1) % HELP_PAGE_COUNT;
            help_show    = true;
            help_show_ts = timer_read32();
            return false;
        default: return true;
    }
    rgb_show    = true;
    rgb_show_ts = timer_read32();
    return true;
}

#ifdef OLED_ENABLE
bool oled_task_user(void) {

    bool rgb_active  = rgb_show  && timer_elapsed32(rgb_show_ts)  < RGB_SHOW_MS;
    bool help_active = help_show && timer_elapsed32(help_show_ts) < HELP_SHOW_MS;
    bool any_overlay = rgb_active || help_active;
    bool was_overlay = rgb_show_active || help_show_active;

    if (any_overlay && !was_overlay) {
        oled_clear();
    }
    rgb_show_active  = rgb_active;
    help_show_active = help_active;

    if (help_active) {
        draw_help_screen();
        return false;
    }

    if (rgb_active) {
        char big[32], sm[32];
        switch (rgb_show_type) {
            case 1: snprintf(big, sizeof(big), "Mode: %u", (unsigned)rgblight_get_mode()); break;
            case 2: snprintf(big, sizeof(big), "Hue:  %u", (unsigned)rgblight_get_hue());  break;
            case 3: snprintf(big, sizeof(big), "Val:  %u", (unsigned)rgblight_get_val());  break;
            default: return false;
        }
        oled_write_2x(big, 0);
        snprintf(sm, sizeof(sm), "M:%-3u H:%-3u V:%-3u",
            (unsigned)rgblight_get_mode(),
            (unsigned)rgblight_get_hue(),
            (unsigned)rgblight_get_val());
        oled_set_cursor(0, 2);
        oled_write(sm, false);
        return false;
    }

    draw_home_screen();
    return false;
}
#endif
