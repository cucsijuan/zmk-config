/*
 * Copyright (c) 2024 Custom ZMK RGB Layer Driver
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/types.h>

struct zmk_rgb_layer_config {
    uint32_t layer_colors[16]; /* RGB colors for each layer (up to 16 layers) */
    uint32_t key_colors[50][16]; /* RGB colors for individual keys per layer (up to 50 keys, 16 layers) */
    bool key_override[50][16]; /* Whether key has custom color for this layer */
    uint8_t total_keys;
    uint8_t total_layers;
};

void zmk_rgb_layer_update_layer(uint8_t layer);
void zmk_rgb_layer_set_layer_color(uint8_t layer, uint32_t color);
void zmk_rgb_layer_set_key_color(uint8_t key_index, uint8_t layer, uint32_t color);
