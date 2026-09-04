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
 * @file vcomponent_FirmwareUpdateService.h
 * @brief Service-level helper for retrieving the canonical AIDL service name.
 *
 * This is intentionally kept out of the AIDL stub header to avoid mixing
 * service-binary concerns with the AIDL skeleton definition.
 */

#include <com/rdk/hal/firmwareupdate/IFirmwareUpdate.h>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

/**
 * @brief Get the well-known service name for publication/lookup.
 *
 * IFirmwareUpdate::serviceName() returns a std::string temporary. Cache it once
 * and return a stable C-string for logging and binder service registration.
 *
 * @return Service name C-string (stable for program lifetime).
 */
// PUBLIC_INTERFACE
inline char const* getFirmwareUpdateServiceName()
{
    static const auto kServiceName = IFirmwareUpdate::serviceName();
    return kServiceName.c_str();
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
