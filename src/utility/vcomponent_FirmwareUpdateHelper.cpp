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
#include <cctype>
#include <fstream>
#include <sstream>

namespace vcomponent {
namespace utility {

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

std::optional<std::string> readFileToString(const std::string& path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in)
    {
        return std::nullopt;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace utility
} // namespace vcomponent
