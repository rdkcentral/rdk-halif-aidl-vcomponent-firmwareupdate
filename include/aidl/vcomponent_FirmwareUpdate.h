/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/**
 * @file vcomponent_FirmwareUpdate.h
 * @brief Asynchronous AIDL Binder service for virtual Firmware Update simulation.
 *
 * Implemented AIDL interface:
 *   com.rdk.hal.firmwareupdate.IFirmwareUpdate
 *
 * The vComponent simulates a firmware-update lifecycle only. It does not copy,
 * flash, verify, install, activate, or persist firmware.
 */

#include <com/rdk/hal/firmwareupdate/BnFirmwareUpdate.h>
#include <com/rdk/hal/firmwareupdate/FirmwareUpdateResult.h>
#include <com/rdk/hal/firmwareupdate/IFirmwareUpdate.h>
#include <com/rdk/hal/firmwareupdate/IFirmwareUpdateListener.h>

#include <binder/BinderService.h>
#include <binder/Status.h>
#include <utils/StrongPointer.h>

#include <ut_kvp.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

/**
 * @brief Binder implementation of the Firmware Update AIDL interface.
 *
 * The service admits a single asynchronous update request at a time. A worker
 * validates only the supplied source path's openability, emits deterministic
 * progress for valid sources, and sends exactly one terminal completion
 * callback. This virtual-device implementation does not alter firmware.
 *
 * Control-plane KVP configures pre-validation, write, and post-validation
 * outcomes in memory only. Configuration replacement and update admission use
 * the same mutex, so each accepted request receives an immutable snapshot.
 * Invalid configuration is retained and reported after source validation.
 */
class FirmwareUpdate final : public android::BinderService<FirmwareUpdate>, public BnFirmwareUpdate
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a FirmwareUpdate simulation service.
     *
     * Initializes the service in its idle state. The service may process one
     * update request at a time.
     */
    FirmwareUpdate();

    // PUBLIC_INTERFACE
    /**
     * @brief Destroy the Firmware Update service.
     */
    ~FirmwareUpdate() override;

    FirmwareUpdate(const FirmwareUpdate&) = delete;
    FirmwareUpdate& operator=(const FirmwareUpdate&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Request an asynchronous firmware-update simulation from an image file.
     *
     * A non-null listener request is admitted when no lifecycle is active. The
     * Binder transaction then returns immediately; source-file validation,
     * progress notifications, completion reporting, and release of the active
     * request slot occur on a background worker.
     *
     * Empty, missing, unreadable, and otherwise non-openable source paths are
     * valid asynchronous requests. They complete once with
     * @c ERROR_FILE_OPEN_FAIL and do not receive progress callbacks. A readable
     * source follows the deterministic progress sequence 0 through 100 and
     * completes once with @c SUCCESS while no control-plane scenario is
     * configured. No firmware bytes are read beyond the ability to open the
     * requested file.
     *
     * Binder status and the boolean AIDL return value have distinct meanings:
     * Binder exceptions report invalid method invocation, while
     * @c Status::ok() with a true result means a request was admitted for
     * asynchronous processing. A successful Binder status with a false result
     * means another update lifecycle is already active; no worker or callbacks
     * are created for that rejected request.
     *
     * @param[in] filename Path to the firmware image. Leading and trailing
     * whitespace is removed before worker validation.
     * @param[in] listener Recipient of progress and completion callbacks. It
     * must not be null.
     * @param[out] _aidl_return Set to @c true when the request is admitted;
     * set to @c false when another update is already active.
     * @return @c android::binder::Status::ok() for valid Binder transactions,
     * including asynchronous source validation failures. Returns
     * @c EX_NULL_POINTER when @p _aidl_return is null and
     * @c EX_ILLEGAL_ARGUMENT when @p listener is null.
     */
    android::binder::Status updateFirmwareFromFile(
        const std::string& filename,
        const android::sp<IFirmwareUpdateListener>& listener,
        bool* _aidl_return) override;

    // PUBLIC_INTERFACE
    /**
     * @brief Replace the scenario using a transient control-plane KVP payload.
     * @param payload Full firmwareupdate command payload; never retained.
     * @return True for a complete, compatible set_scenario command.
     *
     * Copies all required fields into owned memory. Missing or malformed fields,
     * including a null payload, replace previous configuration with an actionable
     * error. Active workers retain their original snapshots. No file is accessed.
     */
    bool configureScenario(ut_kvp_instance_t* payload);

private:
    /**
     * @brief Simulation lifecycle boundary at which a terminal result is injected.
     */
    enum class SimulationStage
    {
        NONE,
        PRE_VALIDATION,
        WRITE,
        POST_VALIDATION
    };

    /**
     * @brief Immutable worker snapshot of a simulated lifecycle outcome.
     * An error is retained independently of the selected lifecycle result.
     */
    struct SimulationScenario
    {
        SimulationStage stage{SimulationStage::NONE};
        FirmwareUpdateResult result{FirmwareUpdateResult::SUCCESS};
        std::string report;
        std::string configurationError;
    };

    /**
     * @brief Execute an admitted firmware-update simulation lifecycle.
     *
     * The worker owns source validation, lifecycle outcome evaluation,
     * callbacks, and release of the active request slot after attempting its
     * terminal completion callback.
     *
     * @param filename Trimmed firmware source filename.
     * @param listener Strong listener reference retained for the worker.
     * @param scenario Immutable simulation outcome snapshot for this request.
     */
    void runUpdateLifecycle(
        std::string filename,
        android::sp<IFirmwareUpdateListener> listener,
        SimulationScenario scenario);

    /**
     * @brief Validate that an internal lifecycle scenario is stage-compatible.
     *
     * Invalid internal scenarios fail before progress with @c ERROR_GENERAL,
     * preventing an inconsistent lifecycle from appearing partially written.
     *
     * @param scenario Scenario to validate.
     * @return @c true when the result is permitted at the selected stage.
     */
    static bool isValidScenario(const SimulationScenario& scenario);

    std::mutex m_mutex;
    SimulationScenario m_scenario; // Protected by m_mutex; defaults to success.
    std::atomic<bool> m_updateInProgress{false};
    std::thread m_lifecycleWorker; // Joined before reuse and during destruction.
};

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
