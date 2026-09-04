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
 * @brief Minimal AIDL binder service stub for Firmware Update.
 *
 * Implemented AIDL interface:
 *   com.rdk.hal.firmwareupdate.IFirmwareUpdate
 *
 * This is a skeleton only. It validates that an image file can be opened and
 * reports a deterministic completion result via listener callbacks (no real
 * flashing).
 */

#include <com/rdk/hal/firmwareupdate/BnFirmwareUpdate.h>
#include <com/rdk/hal/firmwareupdate/FirmwareUpdateResult.h>
#include <com/rdk/hal/firmwareupdate/IFirmwareUpdate.h>
#include <com/rdk/hal/firmwareupdate/IFirmwareUpdateListener.h>

#include <binder/BinderService.h>
#include <binder/Status.h>
#include <utils/StrongPointer.h>

#include <atomic>
#include <mutex>
#include <string>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

/**
 * @brief Binder implementation of the Firmware Update AIDL interface.
 *
 * The service validates a requested firmware-image path and drives the
 * listener callback sequence. This virtual-device implementation deliberately
 * does not flash firmware; a readable image completes successfully as a
 * deterministic no-operation update.
 */
class FirmwareUpdate final : public android::BinderService<FirmwareUpdate>, public BnFirmwareUpdate
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a FirmwareUpdate skeleton service.
     *
     * Initializes the service in its idle state. The service may process one
     * update request at a time.
     */
    FirmwareUpdate();

    // PUBLIC_INTERFACE
    /**
     * @brief Destroy the Firmware Update service.
     */
    ~FirmwareUpdate() override = default;

    FirmwareUpdate(const FirmwareUpdate&) = delete;
    FirmwareUpdate& operator=(const FirmwareUpdate&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Request an asynchronous firmware update from the specified image file.
     *
     * Skeleton behavior:
     * - Allows only one in-flight request at a time.
     * - A null listener is rejected through Binder EX_NULL_POINTER and receives
     *   no callback.
     * - An empty, nonexistent, or unreadable filename is accepted for
     *   asynchronous validation and completes exactly once with
     *   ERROR_FILE_OPEN_FAIL; it never reports a successful update.
     * - If the image is readable, it accepts the request and sends:
     *   - onProgress(0)
     *   - onProgress(100)
     *   - onCompleted(SUCCESS, "stub")
     * - If a request is already active, the AIDL return value is false.
     *
     * Binder status and the boolean AIDL return value have distinct meanings:
     * a Binder exception indicates an invalid invocation, while a successful
     * Binder status with a true return value means the request was accepted for
     * processing and its final outcome is delivered through the listener.
     *
     * @param[in] filename Path to the firmware image. Leading and trailing
     * whitespace is ignored before the file is opened.
     * @param[in] listener Recipient of progress and completion callbacks. It
     * must not be null.
     * @param[out] _aidl_return Set to @c true when the request is accepted;
     * set to @c false when another update is already active.
     * @return @c android::binder::Status::ok() for a valid Binder transaction,
     * including asynchronous image-validation failures. Returns
     * @c EX_NULL_POINTER when @p listener or @p _aidl_return is null.
     */
    android::binder::Status updateFirmwareFromFile(
        const std::string& filename,
        const android::sp<IFirmwareUpdateListener>& listener,
        bool* _aidl_return) override;

private:
    std::mutex m_mutex;
    std::atomic<bool> m_updateInProgress{false};
};

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
