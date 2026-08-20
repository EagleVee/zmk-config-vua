# Environment for local builds. Source it from the repo root:
#
#   . scripts/env.sh
#   west build -p -s zmk/app -d build/left -b vua_left -- -DZMK_CONFIG="$PWD/config"
#
# See README.md "Building locally" for how the workspace is created.

ZMK_ROOT="${ZMK_ROOT:-$PWD}"

# The venv must be on PATH, not just invoked as .venv/bin/west. Generated build
# tools shebang `#!/usr/bin/env python3` and would otherwise pick up the system
# interpreter, which lacks the protobuf module the ZMK Studio build needs.
export PATH="$ZMK_ROOT/.venv/bin:$PATH"

export ZEPHYR_BASE="$ZMK_ROOT/zephyr"

# zmk/app does `find_package(Zephyr HINTS ../zephyr)`, which resolves to
# zmk/zephyr and does not exist here -- Zephyr sits at the workspace root. Point
# at the package directly rather than registering it in the CMake user package
# registry, so this workspace stays self-contained.
export Zephyr_DIR="$ZMK_ROOT/zephyr/share/zephyr-package/cmake"

# Must match zephyr/SDK_VERSION. 0.17.4 fails to compile Zephyr 4.1's picolibc
# glue (conflicting types for __lock___libc_recursive_mutex).
export ZEPHYR_SDK_INSTALL_DIR="${ZEPHYR_SDK_INSTALL_DIR:-$HOME/zephyr-sdk-0.17.0}"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
