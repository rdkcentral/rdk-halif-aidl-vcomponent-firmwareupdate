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
#include "service/vcomponent_FirmwareUpdateService.h"
#include "utility/vcomponent_FirmwareUpdateHelper.h"

#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <string>

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdateService>";
} // namespace

int main(int argc, char** argv)
{
    LOGF_INFO("%s ===============================", logPrefix);
    LOGF_INFO("%s FirmwareUpdate Service 0.1.0.", logPrefix);
    LOGF_INFO("%s ===============================", logPrefix);
    LOGF_INFO(
        "%s: Starting FirmwareUpdate binder service (serviceName=%s)",
        logPrefix,
        // Use the AIDL-generated interface's well-known name (IFirmwareUpdate::serviceName()).
        com::rdk::hal::firmwareupdate::getFirmwareUpdateServiceName());

    // Publish the service using the AIDL interface's canonical service name.
    // This avoids hardcoding service instance names in the service binary.
    const std::string serviceName = com::rdk::hal::firmwareupdate::IFirmwareUpdate::serviceName();

    // The AIDL stub implements the interface; register it with servicemanager.
    // (We intentionally use addService() with the AIDL-derived name.)
    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    if (sm == nullptr)
    {
        LOGF_ERR("%s: defaultServiceManager() returned null", logPrefix);
        return 1;
    }

    android::sp<android::IBinder> service = new com::rdk::hal::firmwareupdate::FirmwareUpdate();
    android::status_t status = sm->addService(android::String16(serviceName.c_str()), service);
    if (status != android::OK)
    {
        LOGF_ERR("%s: Failed to add service '%s' (status=%d)", logPrefix, serviceName.c_str(), status);
        return 1;
    }

    // Use native binder threadpool APIs.
    android::ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}
