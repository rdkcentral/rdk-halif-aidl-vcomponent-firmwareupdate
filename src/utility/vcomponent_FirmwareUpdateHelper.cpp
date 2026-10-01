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

#include "utility/vcomponent_FirmwareUpdateHelper.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace vcomponent {
namespace utility {

namespace {
constexpr uint16_t kDefaultControlPlanePort = 8087;
} // namespace

static inline bool isSpace(unsigned char c)
{
    return std::isspace(c) != 0;
}

std::string trim(const std::string& s)
{
    auto begin = s.begin();
    while (begin != s.end() && isSpace(static_cast<unsigned char>(*begin)))
    {
        ++begin;
    }

    auto end = s.end();
    while (end != begin && isSpace(static_cast<unsigned char>(*(end - 1))))
    {
        --end;
    }

    return std::string(begin, end);
}

void printUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " [--port <port_number>]" << std::endl;
}

bool parsePort(const char* value, uint16_t* port)
{
    if (value == nullptr || port == nullptr || *value == '\0')
    {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long parsedPort = std::strtoul(value, &end, 10);

    if (errno != 0 || end == value || *end != '\0' || parsedPort == 0
        || parsedPort > std::numeric_limits<uint16_t>::max())
    {
        return false;
    }

    *port = static_cast<uint16_t>(parsedPort);
    return true;
}

ArgumentParseResult parseArguments(int argc, char** argv, uint16_t* port)
{
    if (port == nullptr)
    {
        return ArgumentParseResult::InvalidArguments;
    }

    *port = kDefaultControlPlanePort;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument(argv[index]);

        if (argument == "--port")
        {
            if (index + 1 >= argc || !parsePort(argv[++index], port))
            {
                std::cerr << "Error: --port requires a value between 1 and 65535" << std::endl;
                return ArgumentParseResult::InvalidArguments;
            }
            continue;
        }

        if (argument == "--help" || argument == "-h")
        {
            return ArgumentParseResult::HelpRequested;
        }

        std::cerr << "Error: Unknown argument '" << argument << "'" << std::endl;
        return ArgumentParseResult::InvalidArguments;
    }

    return ArgumentParseResult::Success;
}

} // namespace utility
} // namespace vcomponent
