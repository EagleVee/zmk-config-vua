# zmk-config-vua
VUA

Build Simple Dongle
```bash
west build -p always -b nice_nano_v2 -d build/donglesimple zmk/app -- -DSHIELD=via_dongle_simple -DSNIPPET=studio-rpc-usb-uart -DCONFIG_ZMK_STUDIO=y
```

Build Dongle Display

```bash
west build -p always -b nice_nano_v2 -d build/dongledisplay zmk/app -- -DSHIELD="vua_dongle_eyeslash dongle_display" -DSNIPPET=studio-rpc-usb-uart -DCONFIG_ZMK_STUDIO=y -DCONFIG_ZMK_STUDIO_LOCKING=n
```


Build Left

```bash
west build -p always -b nice_nano_v2 -d build/left zmk/app -- -DSHIELD=vua_left
```

Build Right

```bash
west build -p always -b nice_nano_v2 -d build/right zmk/app -- -DSHIELD=vua_right
```
