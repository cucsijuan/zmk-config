if (CONFIG_ZMK_RGB_LAYER)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/rgb_layer.c)

endif()
