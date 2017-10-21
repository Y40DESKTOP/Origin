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
#include "gray_bmp_rotater.h"
#include "../shared/sys_config.h"
#include "../shared/protocol.h"

struct _Server {
    SyslinkService *service;
    ResourceSync *sync;
};

static void server_working(void *server_ptr); 
static void server_destroy(Server *server);
static void server_wait_shared_buffer_ready(Server *server);
static uint8_t *server_get_shared_buffer(Server *server);
//static void server_rotate_bmp_180(Server *server, uint8_t *input, uint8_t *output);

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
    ASSERT(server);

    syslink_service_install(server->service, server_working, server);
    return syslink_service_start(server->service);
}

void server_working(void *server_ptr) {
    Server *server = (Server *)server_ptr;

    server->sync = resource_sync_new(syslink_service_host_id(server->service), 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, RESOURCE_SYNC_ID_SYNC);

    // wait sync object ready
    if (! resource_sync_pair_wait(server->sync)) {
        LOG_ERROR("fail to be synchronize with the slave resouce sync object");
        return;
    }
    LOG_TRACE("synchronised sync object");

    // get shared buffer
    server_wait_shared_buffer_ready(server);
    uint8_t *buffer = server_get_shared_buffer(server);
    if (buffer == NULL) {
        LOG_ERROR("fail to get shared buffer");
        return;
    }
    LOG_TRACE("get shared buffer");

    //data disposal start
    //please design your algorithm
    uint32_t data_size = *(uint32_t *)buffer;
    uint32_t cnt = 0;
LOG_DEBUG("DSP data size block : %d\n",data_size);
    uint8_t *deal = buffer + sizeof(uint32_t);
    while(*deal++)
    	if(*deal >= 65 && *deal <= 90)
    {
//        LOG_DEBUG("%c",*deal);
		*deal += 32;
        cnt++;
    }
LOG_DEBUG("DSP complete data disposalsize block : %d\n",cnt);
    //data disposal finish

    // post finish signal
    if (! resource_sync_post(server->sync, RESOURCE_SYNC_ID_SIGNAL_FINISH)) { 
        LOG_ERROR("fail to be synchronize with the resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_FINISH));
        return;
    }
    LOG_TRACE("synchronised resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_FINISH));

    resource_sync_destroy(server->sync);

    server_destroy(server);
}

static void server_wait_shared_buffer_ready(Server *server) {
    if (! resource_sync_wait(server->sync, RESOURCE_SYNC_ID_SIGNAL_START)) { 
        LOG_ERROR("fail to be synchronize with the resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_START));
        return;
    }
    LOG_TRACE("synchronised resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_START));
}

static uint8_t *server_get_shared_buffer(Server *server) {
    // create shared table
    NameServer_Params params;
    NameServer_Params_init(&params);
    params.maxRuntimeEntries = 10;
    NameServer_Handle handle = NameServer_create(NS_SHARED_TABLE_NAME, &params);
    if (handle == NULL) { 
        LOG_ERROR("fail to create nameserver handle"); 
        return NULL;
    }

    // get shared buffer addres in arm space 
    uint32_t buffer_addr = 0;
    uint16_t proc_id[2] = {syslink_service_host_id(server->service), MultiProc_INVALIDID};
    NameServer_getUInt32(handle, NS_KEY_NAME_BUFFER_ADDR, &buffer_addr, proc_id);
    LOG_DEBUG("buffer_addr: 0x%x", buffer_addr);

    // get shared buffer address in dsp space
    SharedRegion_SRPtr shared_buffer_addr = buffer_addr;
    uint8_t *buffer = SharedRegion_getPtr(shared_buffer_addr);
    LOG_DEBUG("buffer: 0x%x", buffer);

    // delete shared table
    NameServer_delete(&handle);

    return buffer;
}

