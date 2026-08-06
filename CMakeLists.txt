target_include_directories(app PRIVATE modules/zmk-rgb-layer/include)
target_sources_ifdef(CONFIG_ZMK_RGB_LAYER app PRIVATE modules/zmk-rgb-layer/src/rgb_layer.c)
target_sources_ifdef(CONFIG_ZMK_RGB_LAYER app PRIVATE modules/zmk-rgb-layer/src/behavior_rgb_layer_set.c)
