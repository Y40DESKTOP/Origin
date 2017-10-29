#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "server.h"
#include "syslink_service.h"
#include "resource_sync.h"

#include "../shared/sys_config.h"
#include "../shared/protocol.h"

struct _Server {
    SyslinkService *service;
    ResourceSync *sync;
};

static void server_working(void *server_ptr); 
static void server_destroy(Server *server);
static void server_wait_shared_buffer_ready(Server *server);


Server *server_new() {
    Server *server = (Server *)calloc(1, sizeof(Server));

    server->service = syslink_service_new("HOST");

    return server;
}

void server_destroy(Server *server) {
    if (! server)
        return;

    syslink_service_destroy(server->service);
    free(server);
}

bool server_start(Server *server) {
    
    syslink_service_install(server->service, server_working, server);
    return syslink_service_start(server->service);
}

void server_working(void *server_ptr) {

    Server *server = (Server *)server_ptr;

    server->sync = resource_sync_new(syslink_service_host_id(server->service), 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, RESOURCE_SYNC_ID_SYNC);
        // wait sync object ready
        if (! resource_sync_pair_wait(server->sync)) {
            return;
        }


        // get shared buffer
    NameServer_Params params;
    NameServer_Params_init(&params);
    params.maxRuntimeEntries = 10;
    NameServer_Handle handle = NameServer_create(NS_SHARED_TABLE_NAME, &params);
    if (handle == NULL) { 
        return;
    }

    uint8_t *deal,*buffer;
    uint32_t data_size = 0,cnt = 0;

    uint32_t buffer_addr = 0;
    uint16_t proc_id[2] = {0, 0};

while( 1 ){
    server_wait_shared_buffer_ready(server);

    proc_id[0] = syslink_service_host_id(server->service);
    proc_id[1] = MultiProc_INVALIDID;

    NameServer_getUInt32(handle, NS_KEY_NAME_BUFFER_ADDR, &buffer_addr, proc_id);
    SharedRegion_SRPtr shared_buffer_addr = buffer_addr;
    buffer = SharedRegion_getPtr(shared_buffer_addr);

    if (buffer == NULL) {
        return;
    }

    //data disposal start
    //please design your algorithm
    data_size = *(uint32_t *)buffer;
    deal = buffer + sizeof(uint32_t);

    while(*deal)
    if(*deal >= 65 && *deal <= 90)
    {
        *deal += 32;
        cnt++;
	deal++;
    }
    //data disposal finish

    // post finish signal
    if (! resource_sync_post(server->sync, RESOURCE_SYNC_ID_SIGNAL_FINISH)) { 
        return;
    }
}
    NameServer_delete(&handle);
    resource_sync_destroy(server->sync);
    server_destroy(server);
}

static void server_wait_shared_buffer_ready(Server *server) {
    if (! resource_sync_wait(server->sync, RESOURCE_SYNC_ID_SIGNAL_START)) { 
        return;
    }
}

