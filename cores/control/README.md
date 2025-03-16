# SENSE Control Core
This folder contains the firmware for the control core, i.e. the nRF9161.

## Testing
Basic functional tests have been setup. To build the test images, you can
run:

```shell
west twister -T cores/control -O twister-out-control -v --inline-logs \
   --integration -vvv
```
The above command is what CI uses with additional filtering. For local
development where access to the hardware prototype is available, use the
command below:

```shell
west twister -T cores/control -O twister-out-control --inline-logs \
    --integration --device-testing  --west-flash=--recover \
    --hardware-map hw_map.yaml -vvv
```

The runs the tests using the hardware targeted by the hardware map file.
This is not run in CI as HIL testing is not supported. A self-hosted runner
will be added at a later date.

For the control core, the hardware map should contain the following:

```yaml
- connected: true
  id: "001050965037"
  platform: nrf9161dk/nrf9161
  product: J-Link
  runner: nrfjprog
  serial: /dev/serial/by-id/usb-SEGGER_J-Link_001050965037-if00
```

You will need to update the `id` and `serial` fields to match the board you
are using.
