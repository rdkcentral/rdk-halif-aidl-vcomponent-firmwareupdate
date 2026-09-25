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
#include "controller/vcomponent_FirmwareUpdateController.h"
#include "service/vcomponent_FirmwareUpdateService.h"
#include "utility/vcomponent_FirmwareUpdateHelper.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <cstdint>
#include <string>

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdateService>";
} // namespace

int main(int argc, char** argv)
{
    uint16_t controlPlanePort = 0;
    const auto parseResult = vcomponent::utility::parseArguments(argc, argv, &controlPlanePort);
    if (parseResult == vcomponent::utility::ArgumentParseResult::HelpRequested)
    {
        vcomponent::utility::printUsage(argv[0]);
        return 0;
    }
    if (parseResult == vcomponent::utility::ArgumentParseResult::InvalidArguments)
    {
        vcomponent::utility::printUsage(argv[0]);
        return 1;
    }

    LOGF_INFO("%s ===============================", logPrefix);
    LOGF_INFO("%s FirmwareUpdate Service 0.1.0.", logPrefix);
    LOGF_INFO("%s ===============================", logPrefix);
    LOGF_INFO(
        "%s: Starting FirmwareUpdate binder service (serviceName=%s, controlPlanePort=%u)",
        logPrefix,
        com::rdk::hal::firmwareupdate::getFirmwareUpdateServiceName(),
        static_cast<unsigned int>(controlPlanePort));

    android::sp<com::rdk::hal::firmwareupdate::FirmwareUpdate> firmwareUpdate =
        new com::rdk::hal::firmwareupdate::FirmwareUpdate();
    com::rdk::hal::firmwareupdate::FirmwareUpdateController controller(
        controlPlanePort, firmwareUpdate);
    if (!controller.start())
    {
        LOGF_ERR("%s: Failed to initialize control-plane integration", logPrefix);
        return 1;
    }

    // Publish the service using the AIDL interface's canonical service name.
    const std::string serviceName = com::rdk::hal::firmwareupdate::IFirmwareUpdate::serviceName();
    LOGF_DEBUG(
        "%s: Resolving Binder service manager before registering '%s'.",
        logPrefix,
        serviceName.c_str());

    android::sp<android::IServiceManager> sm = android::defaultServiceManager();
    if (sm == nullptr)
    {
        LOGF_ERR("%s: defaultServiceManager() returned null", logPrefix);
        return 1;
    }

    android::sp<android::IBinder> service = firmwareUpdate;
    LOGF_INFO("%s: Registering Binder service '%s'.", logPrefix, serviceName.c_str());
    const android::status_t status = sm->addService(android::String16(serviceName.c_str()), service);
    if (status != android::OK)
    {
        LOGF_ERR("%s: Failed to add service '%s' (status=%d)", logPrefix, serviceName.c_str(), status);
        return 1;
    }

    LOGF_INFO("%s: Binder service registration succeeded; starting Binder thread pool.", logPrefix);
    android::ProcessState::self()->startThreadPool();
    LOGF_INFO("%s: FirmwareUpdate service ready; joining Binder thread pool.", logPrefix);
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}
