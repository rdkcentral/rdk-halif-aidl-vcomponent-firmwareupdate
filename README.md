# rdk-halif-aidl-vcomponent-firmwareupdate

The Firmware Update vDevice module provides an emulated Firmware Update HAL implementation backed by a Binder/AIDL service. The service consumes the RDK HALIF FirmwareUpdate AIDL interfaces, publishes the Binder service under the AIDL-defined Firmware Update service name, and exercises the expected listener callback flow deterministically.

## Table of Contents

- [Firmware Update (vDevice) README](#rdk-halif-aidl-vcomponent-firmwareupdate)
  - [Acronyms, Terms and Abbreviations](#acronyms-terms-and-abbreviations)
  - [Build vcomponent_FirmwareUpdateService](#build-vcomponent_firmwareupdateservice)
  - [Run Firmware Update](#run-firmware-update)

## Acronyms, Terms and Abbreviations

| Acronym / Term | Description |
|----------------|-------------|
| **AIDL** | Android Interface Definition Language |
| **Binder** | Android inter-process communication framework used by the AIDL service |
| **HAL** | Hardware Abstraction Layer |
| **RDK** | Reference Design Kit |
| **VTS** | Vendor Test Suite |
| **YAML** | Yet Another Markup Language (configuration format) |

## Build vcomponent_FirmwareUpdateService

### Prerequisites

This module requires Linux Binder service-manager binaries, Binder headers and libraries, and generated RDK HALIF FirmwareUpdate AIDL headers.

For local builds, `build.sh` stages required artifacts under:

- `build/usr/include`
- `build/usr/lib`
- `build/current/h`

For sysroot-based builds, the same Binder artifacts must be available under:

- `${CMAKE_SYSROOT}/usr/include`
- `${CMAKE_SYSROOT}/usr/lib`

The CMake configuration fails early when required libraries such as `hal_aidl`, `binder`, `utils`, or `log` cannot be found.

### Clone the Repository

```bash
git clone https://github.com/rdkcentral/rdk-halif-aidl-vcomponent-firmwareupdate.git

cd rdk-halif-aidl-vcomponent-firmwareupdate
```

### Environment variables

The build is driven by `./build.sh` in this repository. It uses (or defaults) the following environment variables:

- `UT_CORE_VERSION`: Specific version of UT-Core to build. If not set, the script checks out the latest tag.
- `RDK_HALIF_AIDL_VERSION`: Git ref used if the script must clone `rdk-halif-aidl`. The script defaults to `main`.

Example:

```bash
export UT_CORE_VERSION=5.1.0
export RDK_HALIF_AIDL_VERSION=0.22.0
```

### Build Command (Target Linux)

From the repository root:

```bash
./build.sh Target=linux
```

At a high level, the `build.sh` script:

1. Stages or locates Linux Binder service-manager binaries, headers, and libraries.
2. Makes the generated FirmwareUpdate AIDL headers available to the build.
3. Configures and builds the Firmware Update Binder service using CMake.
4. Produces the service executable `RDKFirmwareUpdateService` in the `build/` output directory.

## Run Firmware Update

### Run the Service on a Target Device

To run the Firmware Update Binder service:

1. Copy the repository's `build/` folder to the target device, or otherwise ensure that the built binary, staged Binder dependencies, generated AIDL headers, and configuration YAML are available on the target filesystem.
2. Run the Firmware Update service binary and provide the configuration YAML path as its first argument.

The service executable is named:

- `RDKFirmwareUpdateService`

The current implementation is a vDevice skeleton: it publishes `IFirmwareUpdate` and exercises the expected listener callback flow deterministically; it does not perform a real firmware update.
