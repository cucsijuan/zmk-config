# ZMK RGB Layer - Configuration Example

## 1. Enable the module in west.yml

Add to `config/west.yml`:
```yaml
projects:
  - name: zmk-rgb-layer
    path: modules/zmk-rgb-layer
```

## 2. Enable in kyria_rev3.conf

```
CONFIG_ZMK_RGB_LAYER=y
```

## 3. Configure in overlay

Add to `config/kyria_rev3.overlay`:

```devicetree
/ {
    chosen {
        zmk,underglow = &led_strip;
    };

    rgb_layer: rgb_layer {
        compatible = "zmk,rgb-layer";
        
        /* Layer colors (RGB format: 0xRRGGBB) */
        layer-colors = <
            0xFF0000  /* Layer 0: Red */
            0x00FF00  /* Layer 1: Green */
            0x0000FF  /* Layer 2: Blue */
            0xFFFF00  /* Layer 3: Yellow */
        >;

        /* Per-key colors (optional) - organized as [key0_layer0, key0_layer1, ..., key1_layer0, ...] */
        key-colors = <
            /* For 50 keys, 4 layers example */
            /* Key 0 */ 0xFF0000 0xFF0000 0xFF0000 0xFF0000
            /* Key 1 */ 0x00FF00 0x00FF00 0x00FF00 0x00FF00
            /* ... more keys ... */
            /* Padding for remaining keys */
            0x000000 0x000000 0x000000 0x000000  /* Key 2 */
            /* ... repeat for keys 3-49 ... */
        >;
    };
};
```

## 4. Runtime API (for future use)

```c
/* Change layer base color */
zmk_rgb_layer_set_layer_color(layer_number, 0xRRGGBB);

/* Set individual key color for a specific layer */
zmk_rgb_layer_set_key_color(key_index, layer_number, 0xRRGGBB);
```

## Color Format

Colors are specified as 24-bit RGB values in hexadecimal:
- `0xFF0000` = Red (FF red, 00 green, 00 blue)
- `0x00FF00` = Green
- `0x0000FF` = Blue
- `0xFFFFFF` = White
- `0x000000` = Off/Black

## Key Indices

The key indices correspond to your keymap positions. For Kyria with 50 keys total,
the indices go from 0-49 in left-to-right, top-to-bottom order.
