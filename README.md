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

On this keyboard they do differ. Both halves carry the same UF2 bootloader
(`0.5.0-dirty`, `Board-ID: nRF52840-pca10056-v1`, Apr 17 2021), but only one has
a SoftDevice:

| Half | `SoftDevice:` | Flash |
| --- | --- | --- |
| Left | `not found` | `vua_left-nosd`, `settings_reset-vua_left-nosd` |
| Right | `S140 version 6.1.1` | `vua_right`, `settings_reset-vua_right` |

Re-check with `INFO_UF2.TXT` if a board's bootloader is ever reflashed.

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
layer:

| Keys | Action |
| --- | --- |
| `Q` / `W` / `E` | Select BLE profile 0 / 1 / 2 |
| `R` / `T` | `BT_CLR` / `BT_CLR_ALL` |
| `Z` / `X` | Pin HID output to USB / to BLE |
| `M6` | `&studio_unlock`, see [ZMK Studio](#zmk-studio) |
| Outer top corners | `&bootloader` and `&sys_reset` |

Reset behaviour has `EVENT_SOURCE` locality, so `&bootloader` bound to a key on
the **right** half resets the right half — but only while the halves are
connected. A disconnected peripheral runs no keymap at all, which is what the
hold-to-reset keys below are for.

### Hold-to-reset

Four keys carry a local reset, wired through a kscan sideband entry rather than
the keymap so they work on the peripheral even when it has lost the central.
**Hold for 3 seconds and release**; a tap does nothing, so each key keeps its
normal meaning.

| Half | Key | Action |
| --- | --- | --- |
| Left | `M1` (top-left macro key) | Reboot |
| Left | `M2` (below/right of it) | Reboot into the bootloader |
| Right | Top-right outer key (`Home`) | Reboot |
| Right | The key under it (`PgUp`) | Reboot into the bootloader |

⚠️ `M2` is `&mo 1` — the function layer. Holding it for 3 seconds drops the left
half into the bootloader, and holding the function layer that long is an ordinary
thing to do. If that bites, move the bootloader entry to another column in
`config/boards/deemen17/vua/vua_left_nrf52840.dts`, or move `&mo 1` off `M2`.

## ZMK Studio

The `vua_left` images are built with [ZMK Studio](https://zmk.studio) enabled, so
the keymap can be changed live without reflashing.

1. Plug the **left** half in over USB. Studio speaks over a USB CDC ACM endpoint,
   which only the central has — the right half cannot be connected to.
2. Open <https://my.zmk.studio> in a WebSerial-capable browser (Chrome, Edge) or
   the desktop app, and connect to the device.
3. Press `Fn` + `M6` (`&studio_unlock`, the second macro key on the home row) to
   unlock. Until then the keymap is read-only. It re-locks after 10 minutes idle
   and whenever Studio disconnects.

## Debugging a BLE-only fault

ZMK's output selection is a stored preference, not a function of what is plugged
in, so the keyboard can run over the radio while USB stays connected purely for
power and a serial console:

1. Flash `vua_left-nosd-logging.uf2` (and `vua_right-logging.uf2` if the fault is
   on that half — the peripheral has no USB HID but the console still works).
2. Plug in and open the serial port (`/dev/tty.usbmodem*`, 115200).
3. Press `Fn` + `X` to pin the output to BLE. `Fn` + `Z` puts it back on USB.
4. Reproduce. A reboot reprints the ZMK boot banner, and the hold-to-reset
   behaviour logs `Held for <n> ms, resetting` before it fires — so the log
   distinguishes a spurious reset from a dropped BLE link.

USB still supplies power here, so this will not reproduce a fault that depends on
the battery supply. That needs RTT over SWD instead.

`vua_left-nosd-logging.elf` and `vua_right-logging.elf` are the matching symbol
files, for `addr2line` on a fault address.

### Bisecting the BLE instability

Two changes landed close together and either could explain BLE dropouts: the
kscan sideband hold-to-reset (`2827409`) and the restored high-voltage DC/DC
stage (`8d6a7ae`). The overlays in `debug/` build the current firmware with
exactly one of them removed, so the three sets differ by a single variable:

| Set | Left | Right |
| --- | --- | --- |
| Baseline | `vua_left-nosd-logging` | `vua_right-logging` |
| No sideband | `vua_left-nosd-nosideband-logging` | `vua_right-nosideband-logging` |
| No HV DC/DC | `vua_left-nosd-nohvdcdc-logging` | `vua_right-nohvdcdc-logging` |

Flash **both halves** from the same set — either half can be the one dropping.
All six carry the keymap and the serial console, so `Fn` + `X` works throughout.

```sh
. scripts/env.sh
west build -p -s zmk/app -d build/left-no-sideband -b vua_left \
    -S nrf52840-nosd -S zmk-usb-logging -- \
    -DZMK_CONFIG="$PWD/config" -DEXTRA_DTC_OVERLAY_FILE="$PWD/debug/no-sideband.overlay"
```

Changes are stored in the settings partition, so they survive a reboot but are
wiped by the `settings_reset-…` images. Reflashing a normal `vua_left…` image
keeps them — the stored keymap wins over the one compiled in, so a keymap edit
made here will not appear to take effect until the stored keymap is reset from
Studio's "Restore Stock Settings".

Studio only knows about keys that appear in the physical layout in
`config/boards/deemen17/vua/vua.dtsi`. That list and the matrix transform above
it describe the same 80 keys in the same order; changing one without the other
will mislabel or drop keys in the Studio UI.

## Building locally

The board lives in `config/boards/deemen17/vua` (HWMv2).

### One-time setup

The west workspace lives in this repo (`.west/config` is checked in); `zephyr/`,
`zmk/`, `modules/`, `.venv/` and `build/` are all gitignored.

```sh
brew install ninja dtc wget ccache libmagic

python3 -m venv .venv
.venv/bin/pip install west
.venv/bin/west update --narrow -o=--depth=1
.venv/bin/pip install -r zephyr/scripts/requirements-base.txt
.venv/bin/pip install protobuf grpcio-tools   # nanopb, for ZMK Studio builds
```

Then the Zephyr SDK. **The version must match `zephyr/SDK_VERSION`** — 0.17.0 at
the time of writing. 0.17.4 fails to compile Zephyr 4.1's picolibc glue with
`conflicting types for '__lock___libc_recursive_mutex'`.

```sh
cd ~
curl -LO https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.0/zephyr-sdk-0.17.0_macos-aarch64_minimal.tar.xz
tar xf zephyr-sdk-0.17.0_macos-aarch64_minimal.tar.xz
cd zephyr-sdk-0.17.0 && ./setup.sh -t arm-zephyr-eabi -c
```

### Building

```sh
. scripts/env.sh
west build -p -s zmk/app -d build/left -b vua_left -- \
    -DZMK_CONFIG="$PWD/config"
```

The `.uf2` lands in `build/left/zephyr/zmk.uf2`. `scripts/env.sh` sets
`ZEPHYR_BASE`, `Zephyr_DIR`, the SDK path, and — importantly — puts `.venv/bin`
on `PATH`, which generated build tools need in order to find `protobuf`.

Add `-S nrf52840-nosd` for the no-SoftDevice layout, `-S zmk-usb-logging` for a
CDC console on USB, and `-DSHIELD=settings_reset` for a reset image. For a
Studio-enabled left half, add `-S studio-rpc-usb-uart -- -DCONFIG_ZMK_STUDIO=y`;
`-S` may be repeated to combine it with `nrf52840-nosd`.

Note that only the `zmk-usb-logging` builds present a serial port. The normal
images expose HID only, so the absence of a `/dev/tty.usbmodem*` or COM port
means nothing by itself.
