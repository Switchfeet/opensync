/*
Copyright (c) 2015, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ev.h>
#include <getopt.h>
#include <time.h>
#include <string.h>

#include "memutil.h"
#include "log.h"
#include "json_util.h"
#include "os.h"
#include "ovsdb.h"
#include "target.h"
#include "network_metadata.h"
#include "cellm_mgr.h"

/* Default log severity */
static log_severity_t  log_severity = LOG_SEVERITY_INFO;

/* Log entries from this file will contain "MAIN" */
#define MODULE_ID LOG_MODULE_ID_MAIN

/**
 * @brief cellm manager
 *
 */
static cellm_mgr_t cellm_mgr;

/**
 * @brief cellm manager accessor
 */
cellm_mgr_t *
cellm_get_mgr(void)
{
    return &cellm_mgr;
}

static void
cellm_setup_handlers(cellm_mgr_t *mgr)
{
    cellm_handlers_t *handlers;

    handlers = &mgr->handlers;
    handlers->cellm_mgr_init = cellm_init_mgr;
    handlers->system_call = system;
}

bool
cellm_init_mgr(struct ev_loop *loop)
{
    cellm_mgr_t *mgr = cellm_get_mgr();

    mgr->loop = loop;

    MEMZERO(mgr->modem_info);
    mgr->modem_info = osn_get_cell_modem_info();

    mgr->mqtt_interval = CELLM_MODEM_INFO_INTERVAL;

    return true;
}

/**
 * Main
 *
 * Note: Command line arguments allow overriding the log severity
 */
int main(int argc, char **argv)
{
    struct ev_loop *loop = EV_DEFAULT;
    cellm_mgr_t *mgr;
    cellm_handlers_t *handlers;

    // Parse command-line arguments
    if (os_get_opt(argc, argv, &log_severity))
    {
        return -1;
    }

    // Initialize logging library
    target_log_open("CELLM", 0);  // 0 = syslog and TTY (if present)
    LOGN("Starting CELLM (CELL Manager)");
    log_severity_set(log_severity);

    // Enable runtime severity updates
    log_register_dynamic_severity(loop);

    // Install crash handlers that dump the stack to the log file
    backtrace_init();

    // Initialize target structure
    if (!target_init(TARGET_INIT_MGR_CELLM, loop))
    {
        LOGE("Initializing CELLM "
             "(Failed to initialize target library)");
        return -1;
    }


    mgr = cellm_get_mgr();
    memset(mgr, 0, sizeof(cellm_mgr_t));

    handlers = &mgr->handlers;
    cellm_setup_handlers(mgr);
    // Initialize the manager
    LOGI("cellm_mgr_init");
    if (!handlers->cellm_mgr_init(loop))
    {
        LOGE("Initializing CELLM "
              "(Failed to initialize manager)");
        return -1;
    }

    if (!ovsdb_init_loop(loop, "CELLM"))
    {
        LOGE("Initializing CELLM "
             "(Failed to initialize OVSDB)");
        return -1;
    }

    // Register to relevant OVSDB tables events
    if (cellm_ovsdb_init())
    {
        LOGE("Initializing CELLM "
             "(Failed to initialize CELLM tables)");
        return -1;
    }

    LOGI("cellm_event_init");
    cellm_event_init();


    // Start the event loop
    ev_run(loop, 0);

    target_close(TARGET_INIT_MGR_CELLM, loop);

    if (!ovsdb_stop_loop(loop))
    {
        LOGE("Stopping CELLM "
             "(Failed to stop OVSDB)");
    }

    ev_loop_destroy(loop);

    LOGN("Exiting CELLM");

    return 0;
}
