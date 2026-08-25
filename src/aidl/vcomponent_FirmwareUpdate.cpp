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

#include <fstream>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdate>";
} // namespace

FirmwareUpdate::FirmwareUpdate()
{
    LOGF_INFO("%s: Initialized FirmwareUpdate skeleton service.", logPrefix);
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
        return android::binder::Status::fromExceptionCode(android::binder::Status::EX_NULL_POINTER);
    }

    const std::string trimmed = vcomponent::utility::trim(filename);

    // Enforce single in-flight operation.
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_updateInProgress.load())
        {
            LOGF_INFO("%s: updateFirmwareFromFile: rejected (update already in progress)", logPrefix);
            *_aidl_return = false;
            return android::binder::Status::ok();
        }
        m_updateInProgress.store(true);
    }

    // File validation is part of the asynchronous operation contract. Keep the
    // Binder transaction successful and communicate the validation outcome via
    // exactly one completion callback for a valid listener.
    std::ifstream imageFile;
    if (!trimmed.empty())
    {
        imageFile.open(trimmed, std::ios::in | std::ios::binary);
    }

    if (!imageFile.is_open())
    {
        LOGF_WARN(
            "%s: updateFirmwareFromFile: unable to open image file '%s'",
            logPrefix,
            trimmed.c_str());

        (void)listener->onCompleted(
            FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL,
            std::string("Unable to open firmware image file"));

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_updateInProgress.store(false);
        }

        // The request was accepted and its failure was reported through the
        // listener; do not overload the boolean return with Binder status.
        *_aidl_return = true;
        return android::binder::Status::ok();
    }

    LOGF_INFO("%s: updateFirmwareFromFile accepted. filename=%s", logPrefix, trimmed.c_str());

    // Skeleton: the validated image is not flashed.
    // We still exercise the listener callback flow deterministically.
    (void)listener->onProgress(0);
    (void)listener->onProgress(100);
    (void)listener->onCompleted(FirmwareUpdateResult::SUCCESS, std::string("stub: no-op update complete"));

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_updateInProgress.store(false);
    }

    *_aidl_return = true;
    return android::binder::Status::ok();
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
