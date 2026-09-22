# rdk-halif-aidl-vcomponent-firmwareupdate

The Firmware Update vComponent is a virtual implementation of the RDK HALIF FirmwareUpdate AIDL interface. It publishes an `IFirmwareUpdate` Binder service and simulates a firmware-update lifecycle for development and test environments. The component validates whether the supplied source path can be opened, emits deterministic listener callbacks, and can inject configured terminal outcomes through its control-plane endpoint.

The component never copies, flashes, verifies, installs, activates, or persists firmware. It is a simulation service only.

## Contents

- [Overview](#overview)
- [Build](#build)
- [Run the Service](#run-the-service)
- [Simulated Update Lifecycle](#simulated-update-lifecycle)
- [Control-Plane Scenarios](#control-plane-scenarios)
- [Limitations](#limitations)

## Overview

`RDKFirmwareUpdateService` registers the canonical service name supplied by the generated `IFirmwareUpdate` AIDL interface, then joins the Binder thread pool. It also starts a UT control-plane listener for `firmwareupdate` messages.

The service permits one asynchronous update request at a time. A request with a null listener is rejected as an invalid Binder invocation. If an update is already active, the Binder call completes successfully but returns `false` through the AIDL return value, and no new lifecycle worker or listener callbacks are created.

For an admitted request, leading and trailing whitespace is removed from the source path before the worker begins. Empty or non-openable paths complete asynchronously with `ERROR_FILE_OPEN_FAIL` and do not receive progress callbacks.

## Build

### Prerequisites

A local build requires Git, CMake, a C++17-capable compiler, and the dependencies retrieved by `build.sh`. The script builds the required Binder tooling and RDK HALIF FirmwareUpdate interfaces, prepares UT-Core and common headers, then configures, builds, and installs this component.

The generated Binder SDK, HALIF headers, libraries, and UT control library are required by CMake. For sysroot-based builds, the corresponding Binder and HALIF artifacts must be available in the sysroot.

### Build Variables

The build script accepts these optional environment variables:

| Variable | Default | Purpose |
|---|---|---|
| `RDK_HALIF_AIDL_VERSION` | `main` | Branch or Git reference used when cloning `rdk-halif-aidl`. |
| `VCOMPONENT` | `firmwareupdate` | HALIF component selected by the script. |
| `VCOMPONENT_VERSION` | `0.2.0.0` | Firmware Update HALIF component version to build. |
| `UT_CORE_VERSION` | Script-managed | UT-Core revision used by the build process. |

### Build Commands

From the repository root, build for Linux or ARM:

```bash
./build.sh Target=linux
```

```bash
./build.sh Target=arm
```

The script configures CMake in `build/`, produces the `RDKFirmwareUpdateService` executable, installs component artifacts below `build/out/`, and copies the generated FirmwareUpdate HALIF shared library into `build/`.

To remove build output, use:

```bash
./build.sh clean
```

To remove build output together with the checked-out HALIF and UT-Core directories, use:

```bash
./build.sh dist_clean
```

## Run the Service

Start the service after the Binder service manager and the runtime libraries required by the staged build are available.

```bash
./build/RDKFirmwareUpdateService
```

The control-plane endpoint listens on TCP port `8087` by default. Supply a different port with `--port`; valid values are decimal values from `1` through `65535`.

```bash
./build/RDKFirmwareUpdateService --port 8090
```

Use `--help` or `-h` to print the command usage. Invalid arguments, including an invalid port, cause the service to exit with a nonzero status.

At startup, the process initializes the control-plane listener, registers the Firmware Update Binder service under the canonical AIDL service name, starts the Binder thread pool, and remains in that thread pool until it is stopped.

## Simulated Update Lifecycle

Calls to `updateFirmwareFromFile` are handled asynchronously after admission. The service retains an immutable snapshot of the current control-plane scenario for each accepted request, so scenario changes do not affect an already active lifecycle.

Without an injected failure scenario, an admitted request produces this behavior:

1. The service rejects an empty source path with `ERROR_FILE_OPEN_FAIL`.
2. The service validates the configured scenario and evaluates pre-validation failures.
3. The service emits progress callbacks at `0`, then every ten percent through `100`.
4. The service evaluates write and post-validation failures.
5. The service sends exactly one `onCompleted` callback with `SUCCESS` when no failure is selected.

A write failure is evaluated immediately after the `50` percent callback. Source-path validation and configured failures complete through `onCompleted`; the source image contents are not read or processed beyond the component's simulated behavior.

The listener callback forwarder included by this repository is a logging implementation and extension point. It records `onProgress` and `onCompleted` callbacks but does not bridge them to another middleware system.

## Control-Plane Scenarios

The service listens for the `firmwareupdate` control-plane message and accepts a complete `set_scenario` payload. The scenario is stored in memory and is applied to subsequent admitted updates. A malformed payload replaces the prior configuration with an error, preventing a previous scenario from being silently reused.

A valid payload of control commands contains these fields:

| Field | Purpose |
|---|---|
| `firmwareupdate.command` | Must be `set_scenario`. |
| `firmwareupdate.params.name` | Selects the named scenario. |
| `firmwareupdate.params.stage` | Identifies where the outcome is injected. |
| `firmwareupdate.params.final_result` | Selects the terminal `FirmwareUpdateResult`. |
| `firmwareupdate.params.report` | Provides the report string passed to `onCompleted`. |

The following name, stage, and result combinations are supported:

| Scenario name | Stage | Final result |
|---|---|---|
| `success` | `pre_validation` | `SUCCESS` |
| `general_error` | `pre_validation` | `ERROR_GENERAL` |
| `file_open_fail` | `pre_validation` | `ERROR_FILE_OPEN_FAIL` |
| `invalid_image_type` | `pre_validation` | `ERROR_IMAGE_INVALID_TYPE` |
| `invalid_signature` | `pre_validation` | `ERROR_IMAGE_INVALID_SIGNATURE` |
| `invalid_size` | `pre_validation` | `ERROR_IMAGE_INVALID_SIZE` |
| `invalid_product` | `pre_validation` | `ERROR_IMAGE_INVALID_PRODUCT` |
| `write_failed` | `write` | `ERROR_FW_UPDATE_WRITE_FAILED` |
| `verify_failed` | `post_validation` | `ERROR_FW_UPDATE_VERIFY_FAILED` |
| `verify_signature_failed` | `post_validation` | `ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED` |

The `stage` and `final_result` values must match the selected scenario exactly. The control-plane schema includes a representative `file_open_fail` scenario and the complete list of supported values.

## Limitations

This vComponent is not a firmware updater. It does not validate firmware content, inspect image signatures, verify product compatibility, write an image, or alter device firmware. The lifecycle stages and terminal outcomes are deterministic simulations intended to exercise Binder clients and control-plane-driven test cases.
