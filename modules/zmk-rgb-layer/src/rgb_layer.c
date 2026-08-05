/*
 * Copyright (c) 2024 Custom ZMK RGB Layer Driver
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led_strip.h>

#include <zmk/rgb_layer.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *led_strip;
static struct zmk_rgb_layer_config rgb_config;
static uint8_t current_layer = 0;

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_rgb_layer)

#define RGB_LAYER_LAYER_COLOR(n) DT_INST_PROP_BY_IDX(0, layer_colors, n)
#define RGB_LAYER_KEY_COLOR(key, layer) DT_INST_PROP_BY_IDX(0, key_colors, (key * 16 + layer))

static int rgb_layer_init(void) {
    led_strip = DEVICE_DT_GET(DT_CHOSEN(zmk_underglow));

    if (!device_is_ready(led_strip)) {
        LOG_ERR("LED strip device not ready");
        return -ENODEV;
    }

    /* Initialize layer colors from device tree */
    rgb_config.total_layers = DT_INST_PROP_LEN(0, layer_colors) / sizeof(uint32_t);
    rgb_config.total_keys = DT_INST_PROP_LEN(0, key_colors) / (rgb_config.total_layers * sizeof(uint32_t));

    for (int i = 0; i < rgb_config.total_layers && i < 16; i++) {
        rgb_config.layer_colors[i] = DT_INST_PROP_BY_IDX(0, layer_colors, i);
    }

    /* Initialize key override flags to false */
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 16; j++) {
            rgb_config.key_override[i][j] = false;
        }
    }

    LOG_INF("ZMK RGB Layer Driver initialized with %d layers", rgb_config.total_layers);
    zmk_rgb_layer_update_layer(0);

    return 0;
}

static void update_led_strip(void) {
    struct led_rgb pixels[50];
    uint32_t color;

    /* Get the keymap to know how many keys we have */
    uint8_t keys_on_layer = zmk_keymap_layer_size();
    if (keys_on_layer > 50) keys_on_layer = 50;

    for (uint8_t i = 0; i < keys_on_layer; i++) {
        /* Check if key has custom color for current layer */
        if (rgb_config.key_override[i][current_layer]) {
            color = rgb_config.key_colors[i][current_layer];
        } else {
            /* Use layer base color */
            color = rgb_config.layer_colors[current_layer];
        }

        /* Convert RGB color (0xRRGGBB) to led_rgb struct */
        pixels[i].red = (color >> 16) & 0xFF;
        pixels[i].green = (color >> 8) & 0xFF;
        pixels[i].blue = color & 0xFF;
    }

    /* Update LED strip */
    led_strip_update_rgb(led_strip, pixels, keys_on_layer);
}

void zmk_rgb_layer_update_layer(uint8_t layer) {
    if (layer >= rgb_config.total_layers) {
        LOG_WRN("Layer %d out of bounds", layer);
        return;
    }

    current_layer = layer;
    LOG_DBG("RGB Layer changed to %d, color: 0x%06X", layer, rgb_config.layer_colors[layer]);
    update_led_strip();
}

void zmk_rgb_layer_set_layer_color(uint8_t layer, uint32_t color) {
    if (layer >= 16) {
        LOG_WRN("Layer %d out of bounds", layer);
        return;
    }

    rgb_config.layer_colors[layer] = color;
    LOG_DBG("Set layer %d color to 0x%06X", layer, color);

    if (layer == current_layer) {
        update_led_strip();
    }
}

void zmk_rgb_layer_set_key_color(uint8_t key_index, uint8_t layer, uint32_t color) {
    if (key_index >= 50 || layer >= 16) {
        LOG_WRN("Key %d or layer %d out of bounds", key_index, layer);
        return;
    }

    rgb_config.key_colors[key_index][layer] = color;
    rgb_config.key_override[key_index][layer] = true;
    LOG_DBG("Set key %d layer %d color to 0x%06X", key_index, layer, color);

    if (layer == current_layer) {
        update_led_strip();
    }
}

static int rgb_layer_layer_changed(const zmk_event_t *event) {
    const struct zmk_layer_state_changed *layer_event = as_zmk_layer_state_changed(event);

    if (layer_event == NULL) {
        return -ENOTSUP;
    }

    if (layer_event->state) {
        zmk_rgb_layer_update_layer(layer_event->layer);
    }

    return 0;
}

SYS_INIT(rgb_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

ZMK_LISTENER(zmk_rgb_layer, rgb_layer_layer_changed);
ZMK_SUBSCRIPTION(zmk_rgb_layer, zmk_layer_state_changed);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(zmk_rgb_layer) */
