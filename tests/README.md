# Tests
This directory stores the tests the SENSE IoT Platform Firmware. Tests are only
available for the control core at present.

## Cores
The `cores` directory stores tests associated with each of the SENSE firmware
cores. Integration tests for the cores are written in `pytest`. The `common`
subfolder acts as a pytest plugin, setting up the required fixtures and board
interface logic.

Currently, the only supported target is the nRF9161DK. To add support for more
boards, review `common/README.md`.

### Test Architecture
Running tests is currently a multi-step process. This is because the tests are
`Kconfig` aware. What this means is that tests will execute different logic
depending on the the Kconfig options built into the image. This design decision
was made to both consolidate tests and reduce test complexity.

To faciliate this, it's necessary to generate a special `build map`. The
build map is a JSON file which associates each of the firmware images in the
test suite with a set of Kconfig options. An example is given below.

```json
{
    "firmware_images": [
        {
            "name": "app.default",
            "platform": "nrf9161dk/nrf9161/ns",
            "binary": "build/app.default.hex",
            "config": {
                "CONFIG_APP_LINK_WITH_MBEDTLS": "y",
                "CONFIG_APP_LINK_WITH_FS": "y",
                "CONFIG_APP_SENSOR_DEFAULT_POLL_FREQ": "1000",
                "CONFIG_APP_USE_SHT40": "y",
                "CONFIG_APP_USE_IMU": "y",
                "CONFIG_APP_USE_GNSS": "y",
                "CONFIG_APP_GNSS_DEFAULT_FIX_INTERVAL": "120",
                "CONFIG_APP_GNSS_DEFAULT_PERIODIC_TIMEOUT": "60"
            }
        },
    ...
```

The script to generate the build map is available at
`script/ci/generate_build_map.py`.

### Test Procedure
The general test procedure is as follows:
1. Build test images
2. Generate hardware map
3. Run tests

Images are built using `west twister`, which builds each of the images defined
in the core's `sample.yaml`.

```shell
west twister -v \
    -T cores/control \
    -O twister-out-control \
    --inline-logs \
    --integration
```

Once these are built, the build map is generated. Note that the `--app-name`
argument must match the target directory we are building, i.e. if we build
`cores/control` then the `--app-name` must be `control`. The `--config-prefix`
argument indicates the subset of Kconfig options we want to collect. It's worth
noting that only options that are set, i.e. not `=n` will be collected.

```shell
python scripts/ci/generate_build_map.py \
    --app-name control \
    --config-prefix CONFIG_APP_USE
    --output-dir build \
    --twister-out-dir twister-out-control \
    --clean
```

The generated build map and final `merged.hex` binaries will be
outputted to the `--output-dir`.

Once the images and build map are available, we can run the tests. In the
example below, we are: disabling caching via `-p no:cacheprovider`, and
specifying the board, port, baud, firmware image, debugger serial number, and
build map. Additionally, we write the JUnit XML results to the summary
directory.

```shell
pytest -p no:cacheprovider tests/cores/control \
    --board nrf9161dk \
    --port /dev/serial/by-id/usb-SEGGER_J-Link_001050965037-if00 \
    --baud 115200 \
    --fw-image build/app.sensors.both \
    --serial-number 001050965037 \
    --build-map build/build_map.json \
    --junit-xml=summary/app.sensors.both.xml
```

Pytest needs to be run against each image to ensure that all image permutations
pass the tests. Instead of running each test directly, this procedure has
been automated by the helper `scripts/ci/run_cores_tests.py`. Note that this
should only be used locally as the CI procedure is slightly different.

```shell
python scripts/ci/run_cores_tests.py \
    --binary-dir build/ \
    --build-map build/build_map.json \
    --board nrf9161dk \
    --port /dev/serial/by-id/usb-SEGGER_J-Link_001050965037-if00 \
    --baud 115200 \
    --serial-number 001050965037 \
    --summary-dir summary/ \
    --clean
```

Under normal operation, the script will summarise each image test set after it
executes, and print a final summary table at the end. You may additonally add
the command line argument `--print-test-output` to print the pytest output
directly to stdout.

The series of steps described above needs to be repeated for each core under
`cores/`.
