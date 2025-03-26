# Core Tests Common
This folder contains the shared fixtures and classes used across the cores for
HIL testing. More specifically this includes:
- board targets, e.g. nRF9161DK
- module and session level test fixures
- command line arguments

## Fixtures
The command line arguments are used to inform the fixtures and test behaviour.
This is best understood by reviewing the command line arguments available in
`plugin.py`.

Presently, the following command line options are available:
- `--board`: the target board
- `--port`: the target board tty port
- `--baud`: the baudrate of the serial connection
- `--fw-image`: the binary image to be programmed onto the board
- `--serial-number`: the serial number of the on-board debugger

## Adding Boards
The board targets are subclasses of the abstract base class `Board` defined
in `board.py`. New boards should follow the approach of the `nRF9161DK` class
defined in `nrf9161dk.py`.

## Acknowledgements
The `pytest` HIL infrastrucure used in this folder is adapted from
[Golioth's Firmware SDK][golioth-sdk]. Shout-out to them for being an
open-source-centric company and using a permissive license. It's a major boon
to young embedded engineers like myself, as it allows us to learn directly
(i.e. the code itself) from the best. I also recommend reading
[Golioth's introductory pytest article][golioth-pytest] if you haven't already.

[golioth-sdk]:https://github.com/golioth/golioth-firmware-sdk
[golioth-pytest]:https://blog.golioth.io/automated-hardware-testing-using-pytest/
