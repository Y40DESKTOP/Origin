#include <QApplication>
#include <QFile>
#include <QPixmap>
#include <QLabel>

#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

#include "log.h"
#include "sys_config.h"
#include "protocol.h"
#include "syslink.h"
#include "resource_sync.h"

#include <ti/syslink/Std.h>     
#include <ti/syslink/utils/Memory.h>
#include <ti/ipc/NameServer.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>



#define BUFFER_SIZE_4M (1024 * 1024 * sizeof(uint32_t))
#define BUFSIZE_1M ( 1024 * 1024 * sizeof(uint8_t))

int main(int argc, char *argv[]) {
    // init qt
    QApplication a(argc, argv);

    // init syslink
    Syslink *syslink = syslink_new("DSP");
    syslink_start(syslink);

    // init sync object
    ResourceSync *sync = resource_sync_new(syslink_slave_id(syslink), 
            SYS_CFG_LINE_ID, SYS_CFG_EVT_ID_RESOURCE_SYNC, RESOURCE_SYNC_ID_SYNC);
    resource_sync_pair_wait(sync);

    // calloc 4m shared buffer form shared space
    IHeap_Handle heap = (IHeap_Handle) SharedRegion_getHeap(SYS_CFG_SHARED_REGION_1);
    uint32_t *buffer = (uint32_t *)Memory_calloc(heap, BUFFER_SIZE_4M, 0, NULL);

    // write input file data and file size to shared buffer,
    // buffer[0]: input/output file size
    // buffer[1]: input file data start address
#if 0    
    QFile input_file;
    input_file.setFileName("input.bmp");
    input_file.open(QIODevice::ReadOnly);
    buffer[0] = (uint32_t)input_file.size();
    input_file.read((char *)&buffer[1], buffer[0]); 
#endif
    int fp_r;
    ssize_t ret,bytes0, bytes1;

    //fp_r = open("/dev/davinci_mcbsp",O_RDONLY);
    fp_r = open("test.txt",O_RDONLY);
    if(fp_r < 0){
	perror("failed to open davinci_mcbsp\n");
	return -1;
    }
    buffer[0] = BUFSIZE_1M;
    bytes0 = read(fp_r,&buffer[1],buffer[0]);
    LOG_INFO("read data size : %d\n",buffer[0]);

//    LOG_INFO("input image size: %d", buffer[0]);

    // get shared buffer address in shared space
    SharedRegion_SRPtr shared_buffer_addr = SharedRegion_getSRPtr(buffer, SYS_CFG_SHARED_REGION_1);
    LOG_INFO("shared_buffer_addr: 0x%x", shared_buffer_addr);

    // shared the shared buffer address in shared space
    NameServer_Params params;
    NameServer_Params_init(&params);
    NameServer_Handle handle = NameServer_create(NS_SHARED_TABLE_NAME, &params);
    NameServer_addUInt32(handle, NS_KEY_NAME_BUFFER_ADDR, shared_buffer_addr);

    // post start signal
    resource_sync_post(sync, RESOURCE_SYNC_ID_SIGNAL_START); 
    LOG_TRACE("synchronised resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_START));

    // wait finish signal
    resource_sync_wait(sync, RESOURCE_SYNC_ID_SIGNAL_FINISH);
    LOG_TRACE("synchronised resource %s", resoruce_id2str(RESOURCE_SYNC_ID_SIGNAL_FINISH));

    int fp_w;
    fp_w = open("result.txt",O_RDWR|O_CREAT|O_TRUNC,0666);
    if(fp_w < 0){
	perror("failed to open result.txt\n");
	return -1;
    }
    buffer[0] = (uint32_t)bytes0;
    bytes1 = write(fp_w,&buffer[1],buffer[0]);
    printf("write data size : %d\n",bytes1);
#if 0
    // show input bmp image
    QLabel label_input;
    QPixmap pixmap_input;
    pixmap_input.loadFromData((const uchar *)&buffer[1], (uint)buffer[0]);
    label_input.setPixmap(pixmap_input);
    label_input.setGeometry(0, 33, 800, 207);
    label_input.show();

    // show output bmp image
    QLabel label_output;
    QPixmap pixmap_output;
    pixmap_output.loadFromData((const uchar *)&buffer[1] + buffer[0], (uint)buffer[0]);
    label_output.setPixmap(pixmap_output);
    label_output.setGeometry(0, 273, 800, 207);
    label_output.show();

    // run qt
    a.exec();
#endif
    // realease 
    Memory_free(heap, buffer, BUFFER_SIZE_4M);
    NameServer_remove(handle, NS_KEY_NAME_BUFFER_ADDR);
    NameServer_delete(&handle);
    resource_sync_destroy(sync);
    syslink_stop(syslink);
    syslink_destroy(syslink);

    return 0;
}
