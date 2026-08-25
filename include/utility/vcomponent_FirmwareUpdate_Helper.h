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

#include <optional>
#include <string>

namespace vcomponent {
namespace utility {

/**
 * @brief Trim leading/trailing whitespace.
 *
 * @param[in] s Input string.
 * @return Trimmed copy.
 */
std::string trim(const std::string& s);

/**
 * @brief Read an entire file into a string.
 *
 * @param[in] path File path.
 * @return Contents if readable; std::nullopt otherwise.
 */
std::optional<std::string> readFileToString(const std::string& path);

} // namespace utility
} // namespace vcomponent
