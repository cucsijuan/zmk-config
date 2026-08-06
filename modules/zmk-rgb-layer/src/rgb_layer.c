/*
 * Copyright (c) 2024 Custom ZMK RGB Layer Driver
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_rgb_layer

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led_strip.h>

#include <zmk/rgb_layer.h>
#include <zmk/event_manager.h>
#include <zmk/matrix.h>

/* Layer state (and its change event) only exists on the half that runs
 * the keymap - the central half in a split build, or the whole board in
 * a non-split build. The peripheral half of a split never sees layers,
 * so it just keeps whatever color rgb_layer_init() set at boot. */
#define RGB_LAYER_HAS_LAYER_EVENTS (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#if RGB_LAYER_HAS_LAYER_EVENTS
#include <zmk/behavior.h>
#include <zmk/events/layer_state_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static const struct device *led_strip;
static struct zmk_rgb_layer_config rgb_config;
static uint8_t current_layer = 0;

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_rgb_layer)

static const uint32_t dt_layer_colors[] = DT_INST_PROP(0, layer_colors);

#if DT_INST_NODE_HAS_PROP(0, key_colors)
static const uint32_t dt_key_colors[] = DT_INST_PROP(0, key_colors);
#endif

static int rgb_layer_init(void) {
    led_strip = DEVICE_DT_GET(DT_CHOSEN(zmk_underglow));

    if (!device_is_ready(led_strip)) {
        LOG_ERR("LED strip device not ready");
        return -ENODEV;
    }

    /* Initialize layer colors from device tree */
    rgb_config.total_layers = ARRAY_SIZE(dt_layer_colors);
    if (rgb_config.total_layers > 16) {
        rgb_config.total_layers = 16;
    }

    for (int i = 0; i < rgb_config.total_layers; i++) {
        rgb_config.layer_colors[i] = dt_layer_colors[i];
    }

    /* Initialize key override flags to false */
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 16; j++) {
            rgb_config.key_override[i][j] = false;
        }
    }

#if DT_INST_NODE_HAS_PROP(0, key_colors)
    /* Populate per-key overrides from device tree; a color of 0x000000
     * means "no override, use the layer color" for that key/layer. */
    rgb_config.total_keys = ARRAY_SIZE(dt_key_colors) / rgb_config.total_layers;
    if (rgb_config.total_keys > 50) {
        rgb_config.total_keys = 50;
    }

    for (int key = 0; key < rgb_config.total_keys; key++) {
        for (int layer = 0; layer < rgb_config.total_layers; layer++) {
            uint32_t color = dt_key_colors[key * rgb_config.total_layers + layer];
            if (color != 0) {
                rgb_config.key_colors[key][layer] = color;
                rgb_config.key_override[key][layer] = true;
            }
        }
    }
#endif

    LOG_INF("ZMK RGB Layer Driver initialized with %d layers", rgb_config.total_layers);
    zmk_rgb_layer_update_layer(0);

    return 0;
}

static void update_led_strip(void) {
    struct led_rgb pixels[50];
    uint32_t color;

    /* Total physical key count, known at compile time from the matrix transform */
    uint8_t keys_on_layer = ZMK_KEYMAP_LEN;
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
        pixels[i].r = (color >> 16) & 0xFF;
        pixels[i].g = (color >> 8) & 0xFF;
        pixels[i].b = color & 0xFF;
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

#if RGB_LAYER_HAS_LAYER_EVENTS
static int rgb_layer_layer_changed(const zmk_event_t *event) {
    const struct zmk_layer_state_changed *layer_event = as_zmk_layer_state_changed(event);

    if (layer_event == NULL) {
        return -ENOTSUP;
    }

    if (layer_event->state) {
        /* Invoke rather than call zmk_rgb_layer_update_layer() directly: this
         * behavior's GLOBAL locality makes ZMK's split transport forward the
         * same invocation to the peripheral half, which has no other way to
         * learn the active layer. It also runs locally here on the central
         * half, same as a direct call would. */
        struct zmk_behavior_binding binding = {
            .behavior_dev = "RGB_LAYER_SET",
            .param1 = layer_event->layer,
        };
        struct zmk_behavior_binding_event event = {
            .layer = layer_event->layer,
            .position = 0,
            .timestamp = k_uptime_get(),
        };
        zmk_behavior_invoke_binding(&binding, event, true);
    }

    return 0;
}

ZMK_LISTENER(zmk_rgb_layer, rgb_layer_layer_changed);
ZMK_SUBSCRIPTION(zmk_rgb_layer, zmk_layer_state_changed);
#endif /* RGB_LAYER_HAS_LAYER_EVENTS */

SYS_INIT(rgb_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(zmk_rgb_layer) */
