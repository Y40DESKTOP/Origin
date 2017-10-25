#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/knl/Task.h>

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "server.h"
#include "syslink_service.h"
#include "resource_sync.h"
#include "../shared/protocol.h"   
#include "../shared/sys_config.h"
#include "../shared/umsg/Umsg.h"

struct _Server {
    SyslinkService *service;
};

static void server_working(void *server_ptr); 
static void server_destroy(Server *server);

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

    // create sync object
    ResourceSync *sync = resource_sync_new(syslink_service_host_id(server->service), 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, REC_SYNC_ID_SYNC);
    resource_sync_pair_wait(sync);

    // umsg module init // TODO:realease 
    Umsg_setup();

    // open umsg object
    resource_sync_wait(sync, REC_SYNC_ID_SIG_UMSG_CREATED);   // wait for umsg object had created 
    Umsg_Handle umsg_ping;  
    Umsg_open(SYS_CFG_UMSG_PING, &umsg_ping);
    Umsg_Handle umsg_pong;  
    Umsg_open(SYS_CFG_UMSG_PONG, &umsg_pong);
    resource_sync_post(sync, REC_SYNC_ID_SIG_UMSG_OPENED);    



    int ret = 1,bytes0 = 0, bytes1 = 0;
    while ( ret ) {       // payload up to max - 1
        // ping
        MessagePing *ping = Umsg_get(umsg_ping);
        bytes0 = ping->bytes;
        LOG_INFO("bytes0:%u,ping->buffer:%s\n",bytes0,ping->buffer);
        Umsg_free(umsg_ping, ping);

        // pong
        MessagePing *pong = Umsg_get(umsg_pong);
        bytes1 = pong->bytes;
        LOG_INFO("bytes1:%u,ping->buffer:%s\n",bytes1,pong->buffer);
        Umsg_free(umsg_pong, pong);
                 
        ret = ( bytes0 || bytes1 );
#if 0        
        MessagePong *pong = Umsg_alloc(umsg_pong);
        Umsg_put(umsg_pong, pong); 

        // payload
        LOG_INFO("payload: %d,%c", payload,testb);
#endif
        Task_sleep(2000);
    }

    

    // close umsg object
    Umsg_close(&umsg_ping);
    Umsg_close(&umsg_pong);
    resource_sync_post(sync, REC_SYNC_ID_SIG_UMSG_CLOSED);  

    // release umsg module
    Umsg_destroy();

    // release sync object
    resource_sync_destroy(sync);

    // release syslink 
    server_destroy(server);
}
