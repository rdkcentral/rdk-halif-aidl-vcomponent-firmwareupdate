/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
    LOGF_DEBUG(
        "%s: updateFirmwareFromFile request received (filenameLength=%zu)",
        logPrefix,
        trimmedFilename.size());

    // Admission and release are both protected so only one worker can own the
    // simulated update lifecycle at any time.
    SimulationScenario scenario;
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


        scenario = m_scenario;
        m_updateInProgress.store(true);
    }

    LOGF_DEBUG(
        "%s: updateFirmwareFromFile accepted for asynchronous execution "
        "(injectedResultStage=%d, injectedResult=%d, hasConfigurationError=%s)",
        logPrefix,
        static_cast<int>(scenario.stage),
        static_cast<int>(scenario.result),
        scenario.configurationError.empty() ? "false" : "true");

    try
    {
        // Capture all request-specific state by value. The worker retains this
        // state and the service until it attempts terminal completion.
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
    LOGF_DEBUG(
        "%s: Firmware-update lifecycle worker started "
        "(scenarioStage=%d, scenarioResult=%d)",
        logPrefix,
        static_cast<int>(scenario.stage),
        static_cast<int>(scenario.result));

    const auto releaseActiveOperation = [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_updateInProgress.store(false);
        LOGF_DEBUG("%s: Firmware-update lifecycle admission released.", logPrefix);
    };

    const auto complete = [&](FirmwareUpdateResult result, const std::string& report) {
        LOGF_INFO(
            "%s: Firmware-update lifecycle completing (result=%d, reportLength=%zu)",
            logPrefix,
            static_cast<int>(result),
            report.size());
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

    const auto failForScenario = [&](SimulationStage stage,
                                     FirmwareUpdateResult result,
                                     const char* validationName) {
        if (scenario.stage != stage || scenario.result != result)
        {
            return false;
        }

        LOGF_INFO(
            "%s: %s failed by configured scenario (result=%d).",
            logPrefix,
            validationName,
            static_cast<int>(result));
        complete(result, scenario.report);
        return true;
    };

    // An empty path is accepted by Binder admission but fails source
    // pre-validation asynchronously, without progress notifications.
    if (filename.empty())
    {
        LOGF_WARN("%s: Source file open validation failed: empty firmware image filename.", logPrefix);
        complete(
            FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL,
            std::string("Unable to open firmware image file"));
        return;
    }

    // Source validation takes precedence over control-plane configuration.
    // Invalid commands replace previous injected results rather than selecting
    // success.
    if (!scenario.configurationError.empty() || !isValidScenario(scenario))
    {
        LOGF_WARN("%s: Firmware-update injected-result validation failed.", logPrefix);
        complete(
            FirmwareUpdateResult::ERROR_GENERAL,
            scenario.configurationError.empty()
                ? std::string("Invalid firmware-update injected result.")
                : scenario.configurationError);
        return;
    }

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_GENERAL,
            "General pre-validation"))
    {
        return;
    }
    

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL,
            "Source file open validation"))
    {
        return;
    }

    LOGF_INFO("%s: Source file opened successfully.", logPrefix);

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_IMAGE_INVALID_TYPE,
            "Image type validation"))
    {
        return;
    }
    LOGF_INFO("%s: Image type validation passed.", logPrefix);

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIGNATURE,
            "Image signature validation"))
    {
        return;
    }
    LOGF_INFO("%s: Image signature validation passed.", logPrefix);

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIZE,
            "Image-size validation"))
    {
        return;
    }
    LOGF_INFO("%s: Image-size validation passed.", logPrefix);

    if (failForScenario(
            SimulationStage::PRE_VALIDATION,
            FirmwareUpdateResult::ERROR_IMAGE_INVALID_PRODUCT,
            "Product-compatibility validation"))
    {
        return;
    }
    LOGF_INFO("%s: Product-compatibility validation passed.", logPrefix);

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
    LOGF_INFO("%s: Simulated write started.", logPrefix);
    reportProgress(0);
    for (int32_t percentComplete = 10; percentComplete <= 100; percentComplete += 10)
    {
        std::this_thread::sleep_for(kProgressDelay);
        reportProgress(percentComplete);

        if (percentComplete == 50
            && failForScenario(
                SimulationStage::WRITE,
                FirmwareUpdateResult::ERROR_FW_UPDATE_WRITE_FAILED,
                "Simulated write"))
        {
            return;
        }
    }
    LOGF_INFO("%s: Simulated write completed.", logPrefix);

    if (failForScenario(
            SimulationStage::POST_VALIDATION,
            FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_FAILED,
            "Post-write integrity validation"))
    {
        return;
    }
    LOGF_INFO("%s: Post-write integrity validation passed.", logPrefix);

    if (failForScenario(
            SimulationStage::POST_VALIDATION,
            FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED,
            "Post-write signature validation"))
    {
        return;
    }
    LOGF_INFO("%s: Post-write signature validation passed.", logPrefix);

    LOGF_INFO("%s: Firmware-update lifecycle completed successfully.", logPrefix);
    complete(FirmwareUpdateResult::SUCCESS, "Simulated firmware update completed successfully.");
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
