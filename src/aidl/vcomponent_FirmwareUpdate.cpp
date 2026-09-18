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

#include "aidl/vcomponent_FirmwareUpdate.h"

#include "common/logger.h"
#include "utility/vcomponent_FirmwareUpdateHelper.h"

#include <binder/Status.h>

#include <chrono>
#include <fstream>
#include <system_error>
#include <thread>
#include <utility>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdate>";
constexpr std::chrono::milliseconds kProgressDelay{25};

const char* defaultReportForResult(FirmwareUpdateResult result)
{
    switch (result)
    {
        case FirmwareUpdateResult::ERROR_GENERAL:
            return "Simulated firmware-update configuration failure.";
        case FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL:
            return "Simulated firmware image file open failure.";
        case FirmwareUpdateResult::ERROR_IMAGE_INVALID_TYPE:
            return "Simulated invalid firmware image type.";
        case FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIGNATURE:
            return "Simulated pre-update signature verification failure.";
        case FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIZE:
            return "Simulated invalid firmware image size.";
        case FirmwareUpdateResult::ERROR_IMAGE_INVALID_PRODUCT:
            return "Simulated firmware image product mismatch.";
        case FirmwareUpdateResult::ERROR_FW_UPDATE_WRITE_FAILED:
            return "Simulated firmware image write failure.";
        case FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_FAILED:
            return "Simulated post-update verification failure.";
        case FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED:
            return "Simulated post-update signature verification failure.";
        case FirmwareUpdateResult::SUCCESS:
            return "";
    }

    return "Simulated firmware-update failure.";
}
} // namespace

FirmwareUpdate::FirmwareUpdate()
{
    LOGF_INFO("%s: Initialized FirmwareUpdate simulation service.", logPrefix);
}

FirmwareUpdate::~FirmwareUpdate()
{
    if (m_lifecycleWorker.joinable())
    {
        m_lifecycleWorker.join();
    }
}

android::binder::Status FirmwareUpdate::updateFirmwareFromFile(
    const std::string& filename,
    const android::sp<IFirmwareUpdateListener>& listener,
    bool* _aidl_return)
{
    if (_aidl_return == nullptr)
    {
        LOGF_ERR("%s: updateFirmwareFromFile: null _aidl_return", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    *_aidl_return = false;

    if (listener == nullptr)
    {
        LOGF_WARN("%s: updateFirmwareFromFile: null listener", logPrefix);
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_ARGUMENT);
    }

    const std::string trimmedFilename = vcomponent::utility::trim(filename);

    // Admission and release are both protected so only one worker can own the
    // simulated update lifecycle at any time. A completed joinable worker is
    // reclaimed before its thread object is reused for this request.
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_updateInProgress.load())
        {
            LOGF_INFO("%s: updateFirmwareFromFile: rejected (update already in progress)", logPrefix);
            return android::binder::Status::ok();
        }

        if (m_lifecycleWorker.joinable())
        {
            m_lifecycleWorker.join();
        }

        m_updateInProgress.store(true);
    }

    // Control-plane scenario selection is intentionally not implemented. The
    // worker still receives an immutable default snapshot so every documented
    // lifecycle stage has explicit, deterministic handling once a future
    // approved integration supplies a validated scenario.
    const SimulationScenario scenario{};

    try
    {
        // Capture all request-specific state by value. The service owns the
        // worker and joins it during destruction, preventing the worker from
        // outliving access to this instance's synchronization state.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lifecycleWorker = std::thread(
            &FirmwareUpdate::runUpdateLifecycle,
            this,
            trimmedFilename,
            listener,
            scenario);
    }
    catch (const std::system_error& error)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_updateInProgress.store(false);

        LOGF_ERR(
            "%s: updateFirmwareFromFile: unable to start lifecycle worker: %s",
            logPrefix,
            error.what());
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }
    catch (const std::exception& error)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_updateInProgress.store(false);

        LOGF_ERR(
            "%s: updateFirmwareFromFile: unable to construct lifecycle worker: %s",
            logPrefix,
            error.what());
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_ILLEGAL_STATE);
    }

    *_aidl_return = true;
    LOGF_INFO(
        "%s: updateFirmwareFromFile admitted. filename=%s",
        logPrefix,
        trimmedFilename.c_str());
    return android::binder::Status::ok();
}

bool FirmwareUpdate::isValidScenario(const SimulationScenario& scenario)
{
    switch (scenario.stage)
    {
        case SimulationStage::NONE:
            return scenario.result == FirmwareUpdateResult::SUCCESS;

        case SimulationStage::PRE_VALIDATION:
            return scenario.result == FirmwareUpdateResult::SUCCESS
                || scenario.result == FirmwareUpdateResult::ERROR_GENERAL
                || scenario.result == FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL
                || scenario.result == FirmwareUpdateResult::ERROR_IMAGE_INVALID_TYPE
                || scenario.result == FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIGNATURE
                || scenario.result == FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIZE
                || scenario.result == FirmwareUpdateResult::ERROR_IMAGE_INVALID_PRODUCT;

        case SimulationStage::WRITE:
            return scenario.result == FirmwareUpdateResult::ERROR_FW_UPDATE_WRITE_FAILED;

        case SimulationStage::POST_VALIDATION:
            return scenario.result == FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_FAILED
                || scenario.result == FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED;
    }

    return false;
}

void FirmwareUpdate::runUpdateLifecycle(
    std::string filename,
    android::sp<IFirmwareUpdateListener> listener,
    SimulationScenario scenario)
{
    const auto releaseActiveOperation = [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_updateInProgress.store(false);
    };

    const auto complete = [&](FirmwareUpdateResult result, const std::string& report) {
        const android::binder::Status callbackStatus = listener->onCompleted(result, report);
        if (!callbackStatus.isOk())
        {
            LOGF_WARN(
                "%s: runUpdateLifecycle: onCompleted callback failed (result=%d)",
                logPrefix,
                static_cast<int>(result));
        }

        // Completion is attempted before another update can be admitted.
        releaseActiveOperation();
    };

    // An empty path is accepted by Binder admission but fails source
    // pre-validation asynchronously, without progress notifications.
    if (filename.empty())
    {
        LOGF_WARN("%s: runUpdateLifecycle: empty firmware image filename", logPrefix);
        complete(
            FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL,
            std::string("Unable to open firmware image file"));
        return;
    }

    // Future scenario delivery must provide a fully validated snapshot. Treat
    // any incompatible internal state as an actionable configuration failure
    // before simulated write progress begins.
    if (!isValidScenario(scenario))
    {
        LOGF_WARN("%s: runUpdateLifecycle: invalid simulation scenario", logPrefix);
        complete(
            FirmwareUpdateResult::ERROR_GENERAL,
            std::string("Invalid firmware-update simulation scenario."));
        return;
    }

    // Pre-validation errors are reported
    // before any simulated write progress is emitted.
    if (scenario.stage == SimulationStage::PRE_VALIDATION
        && scenario.result != FirmwareUpdateResult::SUCCESS)
    {
        const std::string report = scenario.report.empty()
            ? defaultReportForResult(scenario.result)
            : scenario.report;
        complete(scenario.result, report);
        return;
    }

    const auto reportProgress = [&](int32_t percentComplete) {
        const android::binder::Status callbackStatus = listener->onProgress(percentComplete);
        if (!callbackStatus.isOk())
        {
            LOGF_WARN(
                "%s: runUpdateLifecycle: onProgress callback failed (percent=%d)",
                logPrefix,
                static_cast<int>(percentComplete));
        }
    };

    // Generate a bounded, monotonic lifecycle. No image bytes are copied and
    // no target firmware file is created. A write-stage failure terminates
    // immediately after the 50 percent callback is attempted.
    reportProgress(0);
    for (int32_t percentComplete = 10; percentComplete <= 100; percentComplete += 10)
    {
        std::this_thread::sleep_for(kProgressDelay);
        reportProgress(percentComplete);

        if (percentComplete == 50 && scenario.stage == SimulationStage::WRITE)
        {
            const std::string report = scenario.report.empty()
                ? defaultReportForResult(scenario.result)
                : scenario.report;
            complete(scenario.result, report);
            return;
        }
    }

    // Post-validation begins only after progress reaches 100 percent. It is a
    // lifecycle boundary only: no read-back, signature check, or firmware
    // verification is performed.
    if (scenario.stage == SimulationStage::POST_VALIDATION)
    {
        const std::string report = scenario.report.empty()
            ? defaultReportForResult(scenario.result)
            : scenario.report;
        complete(scenario.result, report);
        return;
    }

    complete(FirmwareUpdateResult::SUCCESS, std::string());
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
