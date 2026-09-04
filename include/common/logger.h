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
#ifndef VCOMPONENT_COMMON_LOGGER_H
#define VCOMPONENT_COMMON_LOGGER_H
/* Keep build working when LOG_PRI is defined by multiple headers. */
#if defined(LOG_PRI)
#pragma push_macro("LOG_PRI")
#undef LOG_PRI
#define VCOMPONENT_LOGGER_RESTORE_LOG_PRI 1
#endif
#include <syslog.h>
#ifndef VDEVICE_FIRMWAREUPDATE
#define VDEVICE_FIRMWAREUPDATE "VDEVICE_FIRMWAREUPDATE"
#endif
#if defined(LOG_PRI)
#undef LOG_PRI
#endif
#if defined(VCOMPONENT_LOGGER_RESTORE_LOG_PRI)
#pragma pop_macro("LOG_PRI")
#undef VCOMPONENT_LOGGER_RESTORE_LOG_PRI
#endif
#ifndef LOGF_ERROR
#define LOGF_ERROR(...) do { syslog(LOG_ERR, __VA_ARGS__); } while (0)
#endif
#ifndef LOGF_WARN
#define LOGF_WARN(...)  do { syslog(LOG_WARNING, __VA_ARGS__); } while (0)
#endif
#ifndef LOGF_INFO
#define LOGF_INFO(...)  do { syslog(LOG_INFO, __VA_ARGS__); } while (0)
#endif
#ifndef LOGF_DEBUG
#define LOGF_DEBUG(...) do { syslog(LOG_DEBUG, __VA_ARGS__); } while (0)
#endif
#ifndef LOGF_TRACE
#define LOGF_TRACE(...) do { syslog(LOG_DEBUG, __VA_ARGS__); } while (0)
#endif
/*
* Backward-compatible aliases
* ---------------------------
* Some older sources use LOGF_ERR / LOGF_NOTICE.
* The current logger API exposes LOGF_ERROR / LOGF_INFO etc.
* Keep the build working by aliasing legacy macro names to the supported ones.
*/
#ifndef LOGF_ERR
#define LOGF_ERR(...) LOGF_ERROR(__VA_ARGS__)
#endif
#ifndef LOGF_NOTICE
#define LOGF_NOTICE(...) LOGF_INFO(__VA_ARGS__)
#endif
#endif /* VCOMPONENT_COMMON_LOGGER_H */