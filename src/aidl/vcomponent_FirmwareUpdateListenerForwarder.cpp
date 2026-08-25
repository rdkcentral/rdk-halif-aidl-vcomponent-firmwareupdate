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

#include "aidl/vcomponent_FirmwareUpdateListenerForwarder.h"

#include "common/logger.h"

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdateListenerForwarder>";
} // namespace

FirmwareUpdateListenerForwarder::FirmwareUpdateListenerForwarder(std::string name)
    : m_name(std::move(name))
{
    if (m_name.empty())
    {
        m_name = logPrefix;
    }
}

android::binder::Status FirmwareUpdateListenerForwarder::onProgress(int32_t percentComplete)
{
    LOGF_INFO("%s: onProgress=%d", m_name.c_str(), static_cast<int>(percentComplete));
    return android::binder::Status::ok();
}

android::binder::Status FirmwareUpdateListenerForwarder::onCompleted(
    FirmwareUpdateResult result,
    const std::string& report)
{
    LOGF_INFO(
        "%s: onCompleted result=%d report=%s",
        m_name.c_str(),
        static_cast<int>(result),
        report.c_str());
    return android::binder::Status::ok();
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
