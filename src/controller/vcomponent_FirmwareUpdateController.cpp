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

#include "controller/vcomponent_FirmwareUpdateController.h"

#include "common/logger.h"

#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace com {
namespace rdk {
namespace hal {
namespace firmwareupdate {

namespace {
constexpr const char* logPrefix = "[VDEVICE_FIRMWAREUPDATE]<FirmwareUpdateController>";
} // namespace

// PUBLIC_INTERFACE
/** Retain the service that receives commands on the configured TCP port. */
FirmwareUpdateController::FirmwareUpdateController(
    uint16_t port, const android::sp<FirmwareUpdate>& service)
    : m_port(port), m_service(service)
{
}

// PUBLIC_INTERFACE
/** Replace in-memory configuration from required control-plane KVP fields; return validity. */
bool FirmwareUpdate::configureScenario(ut_kvp_instance_t* payload)
{
    // Serialize replacement with admission. No callback-owned data escapes.
    std::lock_guard<std::mutex> lock(m_mutex);
    SimulationScenario scenario;
    const auto readField = [&](const char* key, std::string& value) {
        // ut_kvp_getStringField uses strncpy and may not terminate a truncated
        // value. Grow the owned buffer rather than silently truncating reports.
        std::vector<char> buffer(256);
        for (;;)
        {
            if (payload == nullptr
                || ut_kvp_getStringField(payload, key, buffer.data(),
                    static_cast<uint32_t>(buffer.size())) != UT_KVP_STATUS_SUCCESS)
            {
                scenario.configurationError =
                    std::string("Missing or non-scalar scenario field: ") + key;
                return false;
            }
            if (std::memchr(buffer.data(), '\0', buffer.size()) != nullptr)
            {
                value.assign(buffer.data());
                return true;
            }
            if (buffer.size() > std::numeric_limits<uint32_t>::max() / 2)
            {
                scenario.configurationError =
                    std::string("Scenario field exceeds KVP size limit: ") + key;
                return false;
            }
            buffer.resize(buffer.size() * 2);
        }
    };

    std::string command;
    std::string name;
    std::string stage;
    std::string result;
    if (readField("firmwareupdate.command", command)
        && readField("firmwareupdate.params.name", name)
        && readField("firmwareupdate.params.stage", stage)
        && readField("firmwareupdate.params.final_result", result)
        && readField("firmwareupdate.params.report", scenario.report))
    {
        struct Outcome
        {
            const char* name;
            const char* stageName;
            const char* resultName;
            SimulationStage stage;
            FirmwareUpdateResult result;
        };
        static const Outcome outcomes[] = {
            {"success", "pre_validation", "SUCCESS",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::SUCCESS},
            {"general_error", "pre_validation", "ERROR_GENERAL",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_GENERAL},
            {"file_open_fail", "pre_validation", "ERROR_FILE_OPEN_FAIL",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_FILE_OPEN_FAIL},
            {"invalid_image_type", "pre_validation", "ERROR_IMAGE_INVALID_TYPE",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_IMAGE_INVALID_TYPE},
            {"invalid_signature", "pre_validation", "ERROR_IMAGE_INVALID_SIGNATURE",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIGNATURE},
            {"invalid_size", "pre_validation", "ERROR_IMAGE_INVALID_SIZE",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_IMAGE_INVALID_SIZE},
            {"invalid_product", "pre_validation", "ERROR_IMAGE_INVALID_PRODUCT",
                SimulationStage::PRE_VALIDATION, FirmwareUpdateResult::ERROR_IMAGE_INVALID_PRODUCT},
            {"write_failed", "write", "ERROR_FW_UPDATE_WRITE_FAILED",
                SimulationStage::WRITE, FirmwareUpdateResult::ERROR_FW_UPDATE_WRITE_FAILED},
            {"verify_failed", "post_validation", "ERROR_FW_UPDATE_VERIFY_FAILED",
                SimulationStage::POST_VALIDATION, FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_FAILED},
            {"verify_signature_failed", "post_validation", "ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED",
                SimulationStage::POST_VALIDATION, FirmwareUpdateResult::ERROR_FW_UPDATE_VERIFY_SIGNATURE_FAILED}
        };

        if (command != "set_scenario")
        {
            scenario.configurationError =
                "Invalid firmwareupdate.command: expected set_scenario.";
        }
        else
        {
            scenario.configurationError = "Unsupported firmwareupdate.params.name.";
            for (const auto& outcome : outcomes)
            {
                if (name != outcome.name)
                    continue;

                if (stage != outcome.stageName || result != outcome.resultName)
                {
                    scenario.configurationError =
                        std::string("Incompatible scenario stage/final_result: expected ")
                        + outcome.stageName + "/" + outcome.resultName + ".";
                }
                else
                {
                    scenario.stage = outcome.stage;
                    scenario.result = outcome.result;
                    scenario.configurationError.clear();
                }
                break;
            }
        }
    }

    m_scenario = std::move(scenario);
    if (m_scenario.configurationError.empty())
    {
        // These identifiers have been validated against the supported outcomes.
        LOGF_INFO(
            "%s: Control-plane scenario configured: command=set_scenario, "
            "scenario=%s, stage=%s, final_result=%s.",
            logPrefix, name.c_str(), stage.c_str(), result.c_str());
    }
    else
    {
        LOGF_INFO(
            "%s: Invalid control-plane scenario configuration retained: %s",
            logPrefix, m_scenario.configurationError.c_str());
    }
    return m_scenario.configurationError.empty();
}

// PUBLIC_INTERFACE
/** Join callbacks before releasing their user data and service reference. */
FirmwareUpdateController::~FirmwareUpdateController()
{
    if (m_controlPlane != nullptr)
    {
        LOGF_INFO("%s: Shutting down control-plane integration.", logPrefix);
        UT_ControlPlane_Exit(m_controlPlane);
    }
}

// PUBLIC_INTERFACE
/** Register firmwareupdate commands and start the transport listener. */
bool FirmwareUpdateController::start()
{
    if (m_controlPlane != nullptr)
    {
        LOGF_DEBUG("%s: Control-plane integration already initialized.", logPrefix);
        return true;
    }
    if (m_service == nullptr || m_port == 0)
    {
        LOGF_ERR(
            "%s: Cannot initialize control-plane integration (serviceAvailable=%s, port=%u).",
            logPrefix,
            m_service == nullptr ? "false" : "true",
            static_cast<unsigned int>(m_port));
        return false;
    }

    LOGF_INFO(
        "%s: Initializing control-plane integration on port %u.",
        logPrefix,
        static_cast<unsigned int>(m_port));
    m_controlPlane = UT_ControlPlane_Init(m_port);
    if (m_controlPlane == nullptr)
    {
        LOGF_ERR("%s: Unable to initialize control-plane endpoint.", logPrefix);
        return false;
    }

    char messageKey[] = "firmwareupdate";
    const auto status = UT_ControlPlane_RegisterCallbackOnMessage(
        m_controlPlane, messageKey, &FirmwareUpdateController::onMessage, this);
    if (status != UT_CONTROL_PLANE_STATUS_OK)
    {
        LOGF_ERR("%s: Unable to register firmwareupdate callback.", logPrefix);
        UT_ControlPlane_Exit(m_controlPlane);
        m_controlPlane = nullptr;
        return false;
    }

    UT_ControlPlane_Start(m_controlPlane);
    LOGF_INFO(
        "%s: Control-plane integration initialized on port %u.",
        logPrefix,
        static_cast<unsigned int>(m_port));
    return true;
}

void FirmwareUpdateController::onMessage(
    char* key, ut_kvp_instance_t* payload, void* userData)
{
    LOGF_INFO(
        "%s: Received control-plane message (key=%s, payloadAvailable=%s).",
        logPrefix,
        key == nullptr ? "<null>" : key,
        payload == nullptr ? "false" : "true");
    auto* controller = static_cast<FirmwareUpdateController*>(userData);
    if (controller == nullptr)
    {
        LOGF_ERR("%s: Ignoring control-plane message with null controller context.", logPrefix);
        return;
    }

    try
    {
        if (!controller->m_service->configureScenario(payload))
        {
            LOGF_WARN("%s: Invalid scenario retained for subsequent updates.", logPrefix);
        }
        else
        {
            LOGF_DEBUG("%s: Control-plane scenario command processed successfully.", logPrefix);
        }
    }
    catch (...)
    {
        // Exceptions must not cross the C transport boundary. Invalidate any
        // previous scenario so a failed command cannot silently reuse it.
        LOGF_ERR("%s: Unable to process scenario command.", logPrefix);
        try
        {
            controller->m_service->configureScenario(nullptr);
        }
        catch (...)
        {
            LOGF_ERR("%s: Unable to retain configuration error.", logPrefix);
        }
    }
}

// PUBLIC_INTERFACE
/** Return the configured TCP port. */
uint16_t FirmwareUpdateController::port() const noexcept
{
    return m_port;
}

} // namespace firmwareupdate
} // namespace hal
} // namespace rdk
} // namespace com
