#include "sigar.h"

static sigar_version_t sigar_version = {
    "@@BUILD_DATE@@",
    "@@SCM_REVISION@@",
    "1.6.4",
    "@@ARCHNAME@@",
    "@@ARCHLIB@@",
    "@@BINNAME@@",
    "SIGAR-@@VERSION_STRING@@, "
    "SCM revision @@SCM_REVISION@@, "
    "built @@BUILD_DATE@@ as @@ARCHLIB@@",
    1,
    6,
    4,
    0
};

SIGAR_DECLARE(sigar_version_t *) sigar_version_get(void)
{
    return &sigar_version;
}
