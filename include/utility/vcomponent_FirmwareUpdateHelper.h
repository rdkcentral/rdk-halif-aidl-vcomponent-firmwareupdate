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
 * @file vcomponent_FirmwareUpdateHelper.h
 * @brief String and command-line helpers used by the Firmware Update service.
 */

#include <cstdint>
#include <string>

namespace vcomponent {
namespace utility {

/**
 * @namespace vcomponent::utility
 * @brief Reusable implementation utilities for the virtual Firmware Update component.
 */

// PUBLIC_INTERFACE
/**
 * @brief Trim leading/trailing whitespace.
 *
 * @param[in] s Source string to normalize.
 * @return A copy of @p s without leading or trailing characters for which
 * @c std::isspace evaluates to true.
 */
std::string trim(const std::string& s);

// PUBLIC_INTERFACE
/**
 * @brief Print Firmware Update service command-line usage.
 *
 * @param[in] programName Name used to invoke the service executable.
 */
void printUsage(const char* programName);

// PUBLIC_INTERFACE
/**
 * @brief Parse a numeric TCP port value.
 *
 * @param[in] value Null-terminated decimal port value.
 * @param[out] port Parsed port number when parsing succeeds.
 * @return @c true when @p value is a complete decimal value in the range
 * 1 through 65535; otherwise @c false.
 */
bool parsePort(const char* value, uint16_t* port);

/**
 * @brief Outcome of parsing service command-line arguments.
 */
enum class ArgumentParseResult
{
    Success,
    HelpRequested,
    InvalidArguments
};

// PUBLIC_INTERFACE
/**
 * @brief Parse Firmware Update service command-line arguments.
 *
 * Supports @c --port <port_number> and @c --help / @c -h. The control-plane
 * port defaults to 8087 when no port argument is supplied.
 *
 * @param[in] argc Number of command-line arguments.
 * @param[in] argv Command-line argument vector.
 * @param[out] port Parsed or defaulted control-plane port.
 * @return Success for valid arguments, HelpRequested for a help flag, or
 * InvalidArguments for an error. The caller handles usage output.
 */
ArgumentParseResult parseArguments(int argc, char** argv, uint16_t* port);

} // namespace utility
} // namespace vcomponent
