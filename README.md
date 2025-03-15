# SENSE Firmware

<a href="https://github.com/skwort/sense-firmware/actions/workflows/build-using-docker.yml?query=branch%3Amain">
  <img src="https://github.com/skwort/sense-firmware/actions/workflows/build-using-docker.yml/badge.svg?event=push">
</a>
<a href="https://github.com/skwort/sense-firmware/actions/workflows/docs.yml?query=branch%3Amain">
  <img src="https://github.com/skwort/sense-firmware/actions/workflows/docs.yml/badge.svg?event=push">
</a>
<a href="https://skwort.github.io/sense-firmware">
  <img alt="Documentation" src="https://img.shields.io/badge/documentation-3D578C?logo=sphinx&logoColor=white">
</a>
<a href="https://skwort.github.io/sense-firmware/doxygen">
  <img alt="API Documentation" src="https://img.shields.io/badge/API-documentation-3D578C?logo=c&logoColor=white">
</a>

This repository contains the firmware for the SENSE IoT Platform. Its structure
is based on [ncs-example-application][ncs-example].


[ncs-example]: https://github.com/nrfconnect/ncs-example-application

## Getting started

Before getting started, make sure you have a proper nRF Connect SDK development environment.
Follow the official
[Installation guide](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html).

### Initialisation

The first step is to initialise the workspace folder (``sense-workspace``) where
``sense-firmware`` and all nRF Connect SDK modules will be cloned. Run the following
command:

```shell
# initialise sense-workspace for sense-firmware (main branch)
west init -m https://github.com/skwort/sense-firmware --mr main sense-workspace
# update nRF Connect SDK modules
cd my-workspace
west update
```

### Building and running

The SENSE IoT port has two MCUs, an nRF9161 and a nRF5340. Each MCU is referred
to as a *core*, comprising specific functionality. The nRF9161 is the *control*
*core*, responsible for orchestrating the system. The nRF5340 is the *interface*
*core*, responsible for digital and user interfaces. Each MCU has separate
firmware which needs to built and flashed independently.

To build the control core, run the following command:

```shell
west build -p -b $BOARD -d cores/control/build cores/control
```

To build the interface core, run the following command:

```shell
west build -p -b $BOARD -d cores/interface/build cores/interface
```

In both of these cases `$BOARD` is the target board. The BSP will be added at
a later date.

A debug configuration is also provided. To apply it, run the previously
specified commands, appending ``debug.conf``

```shell
west build ... -- -DEXTRA_CONF_FILE=debug.conf
```

Once you have built the applications, you can flash them as follows: 

```shell
west flash -d cores/control/build
west flash -d cores/interface/build
```

### Testing

To execute Twister integration tests, run the following command:

```shell
west twister -T tests --integration
```

Note that no tests are currently implemented.

### Documentation

A minimal documentation setup is provided for Doxygen and Sphinx. To build the
documentation first change to the ``doc`` folder:

```shell
cd doc
```

Before continuing, check if you have Doxygen installed. It is recommended to
use the same Doxygen version used in [CI](.github/workflows/docs.yml). To
install Sphinx, make sure you have a Python installation in place and run:

```shell
pip install -r requirements.txt
```

API documentation (Doxygen) can be built using the following command:

```shell
doxygen
```

The output will be stored in the ``_build_doxygen`` folder. Similarly, the
Sphinx documentation (HTML) can be built using the following command:

```shell
make html
```

The output will be stored in the ``_build_sphinx`` folder. You may check for
other output formats other than HTML by running ``make help``.
