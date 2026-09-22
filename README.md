# rdk-halif-aidl-vcomponent-firmwareupdate

The Firmware Update vComponent is a virtual implementation of the RDK HALIF FirmwareUpdate AIDL interface. It publishes an `IFirmwareUpdate` Binder service and simulates an asynchronous firmware-update lifecycle for development and test environments.

The component does not copy, flash, inspect, verify, install, activate, or persist firmware. It is a simulation service only.

## Contents

- [Overview](#overview)
- [Build](#build)
- [Run the Service](#run-the-service)
- [Simulated Update Lifecycle](#simulated-update-lifecycle)
- [Control-Plane Result Injection](#control-plane-result-injection)
- [Limitations](#limitations)

## Overview

`RDKFirmwareUpdateService` creates the `FirmwareUpdate` Binder service, starts a UT control-plane listener, registers the service using the canonical name returned by the generated `IFirmwareUpdate` AIDL interface, and joins the Binder thread pool.

The service permits one asynchronous update request at a time:

- A request with a null listener is rejected with an illegal-argument Binder exception.
- A request with a null AIDL return pointer is rejected with a null-pointer Binder exception.
- If an update is already active, the Binder call succeeds but returns `false` through the AIDL return value. No worker or callbacks are created for that request.
- An admitted request returns `true` immediately and completes on a background worker.
- Leading and trailing whitespace is removed from the supplied filename before the worker starts.

Each accepted request captures the current injected control-plane result. Later control-plane messages do not alter a lifecycle that is already in progress.

## Build

### Prerequisites

A local build requires Git, CMake, and a C++17-capable compiler. The `build.sh` script obtains and builds the required RDK HALIF AIDL and Binder artifacts, prepares UT-Core and common headers, then configures, builds, and installs this component.

For direct CMake use, the required Binder SDK, HALIF headers and libraries, UT control library, and common include directory must be supplied. The project CMake configuration recommends using `build.sh` unless building against a configured sysroot.

### Build Variables

The build script recognizes these optional environment variables:

| Variable | Default | Purpose |
|---|---|---|
| `RDK_HALIF_AIDL_VERSION` | `main` | Branch or Git reference to clone for `rdk-halif-aidl`. |
| `VCOMPONENT` | `firmwareupdate` | HALIF component selected by the build script. |
| `VCOMPONENT_VERSION` | `0.2.0.0` | Firmware Update HALIF component version. |
| `HAL_DBG_LEVEL=INFO` / `HAL_DBG_LEVEL=DEBUG` (argument) | Unset | Pass as a build-script argument to enable the corresponding component logging level.  |

### Build Commands

From the component repository root:

```bash
./build.sh Target=linux
```

```bash
./build.sh Target=arm
```

The script configures CMake in `build/`, produces the `RDKFirmwareUpdateService` executable, installs component artifacts below `build/out/`, and copies the generated FirmwareUpdate HALIF shared library into `build/`.

To remove build output:

```bash
./build.sh clean
```

To remove build output along with the checked-out HALIF and UT-Core directories:

```bash
./build.sh dist_clean
```

Use the following command to print build-script usage:

```bash
./build.sh help
```

## Run the Service

Start the service after the Binder service manager and runtime libraries required by the build are available:

```bash
./build/RDKFirmwareUpdateService
```

The UT control-plane listener uses TCP port `8087` by default. Provide another port with `--port`; valid ports are decimal values from `1` through `65535`.

```bash
./build/RDKFirmwareUpdateService --port 8090
```

The executable accepts `--help` and `-h` to print usage. Unknown arguments, missing port values, and invalid port values cause argument parsing to fail.

On successful startup, the process initializes the control-plane endpoint, registers the Firmware Update Binder service using the generated AIDL interface’s canonical service name, starts the Binder thread pool, and remains in that pool until stopped.

## Simulated Update Lifecycle

Calls to `updateFirmwareFromFile` execute asynchronously after admission. The service does not open or read the supplied file; an empty filename is the only filename-specific failure modeled by the implementation.

For an admitted request with a non-empty filename and no injected result, the lifecycle is:

1. The service emits `onProgress(0)`.
2. It emits `onProgress` at every ten-percent increment through `100`.
3. It sends exactly one `onCompleted(SUCCESS, "Simulated firmware update completed successfully.")` callback.

An empty filename completes asynchronously with:

```text
ERROR_FILE_OPEN_FAIL
Unable to open firmware image file
```

No progress callbacks are emitted for an empty filename.

A write-failure injection is evaluated immediately after the `50` percent progress callback. Post-validation failures are evaluated after the `100` percent callback. The worker attempts one terminal `onCompleted` callback before releasing the active-request slot.

`FirmwareUpdateListenerForwarder` is a logging implementation and an extension point for middleware-specific callback forwarding. It records `onProgress` and `onCompleted` callbacks but does not bridge them to another event system.

## Control-Plane Result Injection

The service registers a UT control-plane callback for the lowercase message key `firmwareupdate`. A control-plane message replaces the in-memory injected result for subsequent accepted update requests.

The implementation requires the following string fields:

| Field | Required value or purpose |
|---|---|
| `firmwareupdate.command` | Must be `inject_firmware_update_result`. |
| `firmwareupdate.result` | Selects the simulated terminal `FirmwareUpdateResult`. |

Supported result values and their simulated lifecycle locations are:

| Result | Simulated location |
|---|---|
| `SUCCESS` | Successful completion after progress reaches `100`. |
| `ERROR_GENERAL` | Before progress callbacks. |
| `ERROR_FILE_OPEN_FAIL` | Before progress callbacks. |
| `ERROR_IMAGE_INVALID_TYPE` | Before progress callbacks. |
| `ERROR_IMAGE_INVALID_SIGNATURE` | Before progress callbacks. |
| `ERROR_IMAGE_INVALID_SIZE` | Before progress callbacks. |
| `ERROR_IMAGE_INVALID_PRODUCT` | Before progress callbacks. |
| `ERROR_FW_UPDATE_WRITE_FAILED` | Immediately after the `50` percent progress callback. |
| `ERROR_FW_UPDATE_VERIFY_FAILED` | After progress reaches `100`. |
| `ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED` | After progress reaches `100`. |

Each supported injected failure includes a default human-readable report in its terminal `onCompleted` callback. The report identifies the simulated failure category; callers do not supply a report in the control-plane payload.

For example, the following payload configures an injected write failure:

```yaml
firmwareupdate:
  command: inject_firmware_update_result
  result: ERROR_FW_UPDATE_WRITE_FAILED
```

Malformed payloads, a different command value, or an unsupported result replace the prior configuration with an error. A later non-empty update request then completes with `ERROR_GENERAL` and the configuration-error message, rather than silently reusing an earlier injected result.

The `SUCCESS` injection restores the default success outcome for subsequent accepted requests.

## Limitations

This vComponent is not a firmware updater. It does not validate source-file availability beyond treating an empty filename as a simulated open failure, and it does not read firmware bytes, validate image content or signatures, verify product compatibility, write an image, or alter device firmware.

Its progress callbacks, lifecycle stages, and terminal outcomes are deterministic simulations intended to exercise Binder clients and control-plane-driven test cases.
