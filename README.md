# FirmwareUpdate vcomponent (stub)

This repository folder contains a **basic stub** for a Firmware Update component wrapper/service that is expected to consume the RDK HALIF FirmwareUpdate AIDL interfaces.

## Authoritative AIDL specification reference

The authoritative AIDL files for this workspace live in:

- `rdk-halif-aidl/firmwareupdate/current/com/rdk/hal/firmwareupdate/`

Key interfaces:
- `IFirmwareUpdate.aidl`
- `IFirmwareUpdateListener.aidl`
- `FirmwareUpdateResult.aidl`

## Layout

This stub mirrors the same layout pattern as the original workspace, but renamed for FirmwareUpdate:

- `include/aidl/` : component-facing C++ header(s) (binder service stubs/helpers)
- `src/aidl/` : component implementation stub(s)
- `src/service/` : service `main()` stub
- `include/common/` : basic logging header
- `include/utility/` + `src/utility/` : small helpers used by the stub
- `aidl_lib/Makefile` : placeholder AIDL generation/packaging entrypoint

## Build (local)

```sh
./build.sh Target=linux
```

This will configure and build into `./build/`.

## Running the service

The service requires a configuration YAML path as the first argument:

- `vcomponent_FirmwareUpdateService <config.yaml>`

> Note: Real AIDL/Binder builds require Binder headers/libraries plus generated AIDL headers. This repo expects those artifacts to be staged (see `build.sh`), and the default CMake configure step will fail fast if they are missing (e.g., missing `hal_aidl`, `binder`, `utils`, or `log` discovered via `find_library(... )` + `message(FATAL_ERROR ...)`). For local builds, the expected layout is under `build/usr/include`, `build/usr/lib`, and generated AIDL headers under `build/current/h`; for sysroot-based builds, the same artifacts are expected under `${CMAKE_SYSROOT}/usr/include` and `${CMAKE_SYSROOT}/usr/lib`.

## Status

- This is **not** a functional Firmware Update implementation.
- It is a skeleton that publishes `IFirmwareUpdate` and exercises the expected listener callback flow deterministically.
