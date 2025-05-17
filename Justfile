# Variables
default_image := "app.debug"
default_control_target := "sense_core/nrf9161/ns"
default_interface_target := "sense_core/nrf5340/cpuapp/ns"
hardware_map := "hw_map.yaml"

build-control:
    west build -p \
        -b {{default_control_target}} \
        -d cores/control/build \
        -T {{default_image}} \
        cores/control

    # Fix clangd warnings
    @sed -i 's/-fno-reorder-functions//g' cores/control/build/control/compile_commands.json
    @sed -i 's/-fno-printf-return-value//g' cores/control/build/control/compile_commands.json
    @sed -i 's/-mfp16-format=ieee//g' cores/control/build/control/compile_commands.json

test-control:
    # Delete the twister-out-control directory if it exists
    [ -d "twister-out-control" ] && rm -rf "twister-out-control" || true

    # Build the images
    west twister -v \
        --inline-logs \
        --integration \
        -T cores/control \
        -O twister-out-control

    # Generate the build artifacts
    python scripts/ci/generate_build_map.py \
        --app-name control \
        --config-prefix CONFIG_APP_USE \
        --output-dir build \
        --twister-out-dir twister-out-control \
        --clean

    # Run tests
    python scripts/ci/run_cores_tests.py \
        --binary-dir build/ \
        --build-map build/build_map.json \
        --board sense_core/nrf9161 \
        --port /dev/serial/by-id/usb-SEGGER_J-Link_00001050032045-if00 \
        --baud 115200 \
        --serial-number 001050032045 \
        --summary-dir summary/ \
        --print-test-output \
        --clean

flash-control:
    west flash -d cores/control/build

build-interface:
    west build -p \
        -b {{default_interface_target}} \
        -d cores/interface/build \
        -T {{default_image}} \
        cores/interface

    # Fix clangd warnings
    @sed -i 's/-fno-reorder-functions//g' cores/interface/build/interface/compile_commands.json
    @sed -i 's/-fno-printf-return-value//g' cores/interface/build/interface/compile_commands.json
    @sed -i 's/-mfp16-format=ieee//g' cores/interface/build/interface/compile_commands.json

flash-interface:
    west flash -d cores/interface/build

test-lib:
    # Remove previous build and coverage artifacts
    [ -d "twister-out-lib" ] && rm -rf "twister-out-lib" || true
    [ -d "gcov_html" ] && rm -rf "gcov_html" || true
    find . -name '*.gcda' -delete
    find . -name '*.gcno' -delete

    # Run tests
    west twister -v \
        --inline-logs \
        --integration \
        -T tests/lib/ \
        -O twister-out-lib

    # Fix clangd warnings
    find twister-out-lib/native_sim/ -type f -name compile_commands.json -exec sed -i 's/-fno-reorder-functions//g' {} +
    find twister-out-lib/native_sim/ -type f -name compile_commands.json -exec sed -i 's/-fno-freestanding//g' {} +

    # Generate HTML report
    mkdir gcov_html
    gcovr \
        --root "$(pwd)" \
        --filter 'lib/' \
        --exclude-branches-by-pattern '.*LOG_(INF|DBG|WRN|ERR)\(' \
        --print-summary \
        --gcov-delete \
        --html --html-details -o gcov_html/index.html

    xdg-open gcov_html/index.html

    python scripts/ci/twister_summary_terminal.py twister-out-lib/twister.json

cloc:
    cloc --not-match-d='build' boards/ cores/ lib/ include/ tests/ scripts/

cloc-c:
    cloc --include-ext=c,h --not-match-d='build' boards/ cores/ lib/ include/ tests/ scripts/
