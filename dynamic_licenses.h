
#ifndef __DYNAMIC_LICENSES_H__
#define __DYNAMIC_LICENSES_H__

#include "slurm/slurm.h"
#include "src/common/list.h"

#include <inttypes.h>

extern pthread_mutex_t license_mutex;

typedef struct dynamic_licenses {
    char *      name;       /* name associated with a license */
    uint32_t    total;      /* total license configued */
    uint32_t    used;       /* used licenses */
} dynamic_licenses_t;


/* dynamic_licenses_agent - detached thread periodically attempts to update licences usage information */
extern void *dynamic_licenses_agent(void *args);
extern void stop_dynamic_licenses_agent();

#endif	/* __DYNAMIC_LICENSES_H__ */
