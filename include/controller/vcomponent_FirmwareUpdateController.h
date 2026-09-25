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

#pragma once

/**
 * @file vcomponent_FirmwareUpdateController.h
 * @brief Control-plane endpoint for in-memory Firmware Update scenarios.
 */

#include "aidl/vcomponent_FirmwareUpdate.h"
#include <ut_control_plane.h>
#include <cstdint>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

/**
 * @brief Owns the Firmware Update control-plane listener.
 * Forwards transient firmwareupdate KVP messages to the published service.
 * Destruction joins transport callbacks before releasing the service.
 */
class FirmwareUpdateController final
{
public:
    // PUBLIC_INTERFACE
    /**
     * @brief Construct the controller for a TCP control-plane port.
     *
     * @param[in] port Valid non-zero TCP port assigned to the control plane.
     * @param[in] service The same service instance published through Binder.
     */
    FirmwareUpdateController(uint16_t port, const android::sp<FirmwareUpdate>& service);

    // PUBLIC_INTERFACE
    /** @brief Stop callbacks and release the transport endpoint. */
    ~FirmwareUpdateController();

    FirmwareUpdateController(const FirmwareUpdateController&) = delete;
    FirmwareUpdateController& operator=(const FirmwareUpdateController&) = delete;

    // PUBLIC_INTERFACE
    /**
     * @brief Start control-plane integration.
     *
     * Call from the service startup thread; repeated calls are idempotent.
     * @return False if the service, endpoint, or callback registration is invalid.
     * The transport's void Start API does not expose thread creation failures.
     */
    bool start();

    // PUBLIC_INTERFACE
    /**
     * @brief Return the configured control-plane TCP port.
     *
     * @return Non-zero TCP port supplied when this controller was created.
     */
    uint16_t port() const noexcept;

private:
    /** Consume the framework-owned payload synchronously without retaining it. */
    static void onMessage(char* key, ut_kvp_instance_t* payload, void* userData);

    uint16_t m_port;
    android::sp<FirmwareUpdate> m_service;
    ut_controlPlane_instance_t* m_controlPlane{nullptr};
};

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
