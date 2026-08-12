# zmk-config-vua

ZMK firmware for the VU.A, built as a two-piece split: the **left half is the
central** (it holds the USB/BLE connection to the host and talks to the right
half) and the **right half is a peripheral**. There is no dongle.

## Which image do I flash?

Two things decide this, and getting either wrong looks like dead hardware.

### 1. Left or right

| Half | Image | Plugged into USB |
| --- | --- | --- |
| Left | `vua_left` | Works as a USB keyboard |
| Right | `vua_right` | Enumerates, but **cannot type** |

The right half is a split peripheral, so ZMK will not build USB HID for it —
`ZMK_USB` depends on `!ZMK_SPLIT || ZMK_SPLIT_ROLE_CENTRAL`. Plugged into a host
it appears as an unrecognised USB device. **Only the left half is a USB
keyboard.** Over BLE, the right half advertises with no name at all, so it also
shows up as an unnamed/"Unknown" device — that is normal, never pair with it.

### 2. Bootloader flash layout

Every target is built twice:

| Image | Application linked at | For a bootloader that |
| --- | --- | --- |
| `vua_left`, `vua_right` | `0x26000` | reserves space for a SoftDevice |
| `vua_left-nosd`, `vua_right-nosd` | `0x1000` | was built without a SoftDevice |

Check a board by putting it in bootloader mode and reading `INFO_UF2.TXT` from
the drive it mounts:

```sh
cat /Volumes/*/INFO_UF2.TXT     # macOS
```

Look at the `SoftDevice:` line. `SoftDevice: not found` means that board needs
the `-nosd` image.

**The two halves may need different images** — bootloaders are per board, not
per keyboard, and mixed sets are common on hand-built boards.

Flashing the wrong layout fails silently and completely. The bootloader accepts
the file, reboots, jumps to an address the image is not at, and locks up: no USB
device, no BLE advertisement, no keys, nothing on the console. The bootloader
itself keeps working, so the board still takes flashes and still looks fine.
There is no error message anywhere.

## Flashing

1. Flash `settings_reset-…` to each half and let it boot for ~5 s. This clears
   stored BLE bonds, which is necessary when changing which board is the central
   (e.g. migrating from a dongle) — otherwise the halves advertise at hosts they
   are still bonded to and never pair.
2. Flash `vua_left…` to the left half and `vua_right…` to the right half.
3. Forget the keyboard on any paired host and pair it again.

Plug the **left** half in over USB, or pair it over BLE. The right half joins the
left automatically.

On Windows, copying a `.uf2` reports
`0x80070022: The wrong diskette is in the drive`. This is expected — the
bootloader resets and unmounts the drive as the last block lands, and Windows
misreports it. The write succeeded. macOS says "Disk Not Ejected Properly" for
the same reason.

## Keymap

`config/vua.keymap`, three layers (base, function, gaming). On the function
layer: `&bootloader` and `&sys_reset` at the outer top corners of both halves,
and BLE profile control on the left half at `Q`/`E`/`R`/`T`
(`BT_PRV`/`BT_NXT`/`BT_CLR`/`BT_CLR_ALL`).

Reset behaviour has `EVENT_SOURCE` locality, so `&bootloader` bound to a key on
the **right** half resets the right half — but only while the halves are
connected. A disconnected peripheral runs no keymap at all and can only be reset
with its physical button.

## Building locally

The board lives in `config/boards/deemen17/vua` (HWMv2). With a west workspace
whose `zmk/` and `zephyr/` are checked out:

```sh
west build -p -s zmk/app -d build/left -b vua_left -- \
    -DZMK_CONFIG="$PWD/config"
```

Add `-S nrf52840-nosd` for the no-SoftDevice layout, `-S zmk-usb-logging` for a
CDC console on USB, and `-DSHIELD=settings_reset` for a reset image.

Note that only the `zmk-usb-logging` builds present a serial port. The normal
images expose HID only, so the absence of a `/dev/tty.usbmodem*` or COM port
means nothing by itself.
