#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include "syslink.h"
#include "resource_sync.h"
#include "log.h"
#include "../shared/protocol.h"     
#include "../shared/sys_config.h"   
#include <ti/syslink/Std.h>     
#include "../shared/umsg/Umsg.h"

int main(int argc, char *argv[]) {

    
    int fp_r;
    int ret = 1,bytes0 = 0, bytes1 = 0;

    fp_r = open("/dev/davinci_mcbsp",O_RDWR);
    if(fp_r < 0){
        perror("Failed to open davinci_mcbsp\n");
        return -1;
    }


    // syslink init
    Syslink * syslink = syslink_new("DSP");
    if (! syslink_connect(syslink)) {
        LOG_ERROR("fail to connect to slave");
        return -1;
    }
    LOG_DEBUG("connected to slave");

    uint32_t proc_id = syslink_proc_id(syslink);

    // create sync object //TODO: release
    ResourceSync *sync = resource_sync_new(proc_id, 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, REC_SYNC_ID_SYNC);
    resource_sync_pair_wait(sync);

    // umsg module init
    Umsg_setup();

    // create umsg object
    Umsg_Params params;
    Umsg_Params_init(&params);
    params.msgSize = sizeof(MessagePing); // Notice: ASSERT(msgSize % sizeof(uint32_t)) == 0)
    params.poolCount = 4;                 // Notice: ASSERT(poolCount > 3)
    params.inboxCount = 3;
    params.regionId = 0;

    Umsg_Handle umsg_ping = Umsg_create(SYS_CFG_UMSG_PING, TRUE /* writer */, proc_id, &params); 

    params.msgSize = sizeof(MessagePong); // Notice: ASSERT(msgSize % sizeof(uint32_t)) == 0)
    Umsg_Handle umsg_pong = Umsg_create(SYS_CFG_UMSG_PONG, TRUE /* reader */, proc_id, &params); 

    // ensure umsg object is ready
    resource_sync_post(sync, REC_SYNC_ID_SIG_UMSG_CREATED);  
    resource_sync_wait(sync, REC_SYNC_ID_SIG_UMSG_OPENED);  



	while ( ret ) {        // payload up to max
		// ping
		MessagePing *ping = (MessagePing *)Umsg_alloc(umsg_ping);
		bytes0 = read(fp_r,ping->buffer,1024 * 96);
        ping->bytes = bytes0;
		Umsg_put(umsg_ping, ping);  
		
		// pong
		MessagePong *pong = (MessagePong *)Umsg_alloc(umsg_pong);
		bytes1 = read(fp_r,pong->buffer,1024 * 96);
        pong->bytes = bytes1;
		Umsg_put(umsg_pong, pong);  

        ret = ( bytes0 || bytes1 );
#if 0                
		MessagePong *pong = (MessagePong *)Umsg_get(umsg_pong);  
		payload = pong->payload;
		testb = pong->testb;
		Umsg_free(umsg_pong, pong);
#endif		
	}
	
	close(fp_r);
    // release umsg object
    resource_sync_wait(sync, REC_SYNC_ID_SIG_UMSG_CLOSED);    // wait for writer umsg object closed
    Umsg_delete(&umsg_ping);
    Umsg_delete(&umsg_pong);

    // release umsg module
    Umsg_destroy();

    // release sync object
    resource_sync_destroy(sync);

    // release syslink
    syslink_disconnect(syslink);
    syslink_destroy(syslink);

    LOG_DEBUG("quit ...");

    return 0;
}
