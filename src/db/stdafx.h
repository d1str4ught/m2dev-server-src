#pragma once

#include "libthecore/stdafx.h"

#ifndef OS_WINDOWS
#include <semaphore.h>
#else
#define isdigit iswdigit
#define isspace iswspace
#endif

#include "common/length.h"
#include "common/tables.h"
#include "common/singleton.h"
#include "common/utils.h"
#include "common/stl.h"
#include "common/service.h"

#include <memory>