#pragma once

#include "log.hpp"

#define GNC_UNUSED(x) (void)(x)

#define GNC_TODO(x) Log::writeFormatted(Log::Warning, "TODO: %s", x);
