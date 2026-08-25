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
 * @file vcomponent_FirmwareUpdateListenerForwarder.h
 * @brief Skeleton helper that can forward IFirmwareUpdateListener callbacks.
 *
 * This is a placeholder to show where middleware-specific bridging logic would live
 * (e.g., mapping binder callbacks to an internal event bus).
 */

#include <com/rdk/hal/firmwareupdate/BnFirmwareUpdateListener.h>
#include <com/rdk/hal/firmwareupdate/FirmwareUpdateResult.h>
#include <com/rdk/hal/firmwareupdate/IFirmwareUpdateListener.h>

#include <binder/Status.h>
#include <utils/StrongPointer.h>

#include <string>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

class FirmwareUpdateListenerForwarder final : public BnFirmwareUpdateListener
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct a forwarder.
     *
     * @param[in] name A label used for logging/identification.
     */
    explicit FirmwareUpdateListenerForwarder(std::string name);

    ~FirmwareUpdateListenerForwarder() override = default;

    FirmwareUpdateListenerForwarder(const FirmwareUpdateListenerForwarder&) = delete;
    FirmwareUpdateListenerForwarder& operator=(const FirmwareUpdateListenerForwarder&) = delete;

    android::binder::Status onProgress(int32_t percentComplete) override;

    android::binder::Status onCompleted(
        FirmwareUpdateResult result,
        const std::string& report) override;

private:
    std::string m_name;
};

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
