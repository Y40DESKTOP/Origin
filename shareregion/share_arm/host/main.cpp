#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
//#include "log.h"
#include "sys_config.h"
#include "protocol.h"
#include "syslink.h"
#include "resource_sync.h"
#include <ti/syslink/Std.h>     
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>

#define BUFFER_SIZE_4M ( 2 * 1024 * 1024 * sizeof(uint8_t))
//memory block size for share region

#define BUFSIZE_1M ( 2 * 1024 * 1024 * sizeof(uint8_t))
//memory block size for primary data change to double
#define ARM  0

int main(int argc, char *argv[]) {

    Syslink *syslink = syslink_new("DSP");
    syslink_start(syslink);
    // init sync object
    ResourceSync *sync = resource_sync_new(syslink_slave_id(syslink), 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, RESOURCE_SYNC_ID_SYNC);
    resource_sync_pair_wait(sync);

    // calloc 4m shared buffer form shared space

    IHeap_Handle heap = (IHeap_Handle) SharedRegion_getHeap(SYS_CFG_SHARED_REGION_1);
    uint32_t *buffer = (uint32_t *)Memory_calloc(heap, BUFFER_SIZE_4M, 0, NULL);
    //define the read/write files

    int fp_r;
    ssize_t ret,rbytes = 1, wbytes;
    ssize_t rbytes_arm = 0;

//    fp_r = open("/dev/davinci_mcbsp",O_RDONLY);
        fp_r = open("test.pdf",O_RDONLY);
    if(fp_r < 0){
        perror("failed to open davinci_mcbsp\n");
        return -1;
    }

    int fp_w;
    fp_w = open("result.txt",O_RDWR|O_CREAT|O_TRUNC,0666);
    if(fp_w < 0){
        perror("failed to open result.txt\n");
        return -1;
    }
    //handle SharedRegion
    NameServer_Params params;
    NameServer_Params_init(&params);
    NameServer_Handle handle = NameServer_create(NS_SHARED_TABLE_NAME, &params);

    // write input file data and file size to shared buffer,
    // buffer[0]: input/output file size
    // buffer[1]: input file data start address
    while( rbytes ){
#if ARM
        (char *)buf_arm = malloc(BUFSIZE_1M);
        rbytes_arm = read(fp_r,&buf_arm,BUFSIZE_1M);
#endif
        buffer[0] = BUFSIZE_1M;
        rbytes = read(fp_r,&buffer[1],buffer[0]);

        // get shared buffer address in shared space
        SharedRegion_SRPtr shared_buffer_addr = SharedRegion_getSRPtr(buffer, SYS_CFG_SHARED_REGION_1);

        NameServer_addUInt32(handle, NS_KEY_NAME_BUFFER_ADDR, shared_buffer_addr);
        // post start signal
        resource_sync_post(sync, RESOURCE_SYNC_ID_SIGNAL_START); 
        // wait finish signal
        resource_sync_wait(sync, RESOURCE_SYNC_ID_SIGNAL_FINISH);


        buffer[0] = (uint32_t)rbytes;
        wbytes = write(fp_w,&buffer[1],buffer[0]);
        printf("write data size : %d\n",wbytes);
    }
    // realease 

    Memory_free(heap, buffer, BUFFER_SIZE_4M);
    NameServer_remove(handle, NS_KEY_NAME_BUFFER_ADDR);
    NameServer_delete(&handle);

    resource_sync_destroy(sync);
    syslink_stop(syslink);
    syslink_destroy(syslink);
    return 0;
}
