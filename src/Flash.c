#include <pfdl.h>

int Flash_Read(int block, int bytes, unsigned char *buf)
{
    pfdl_descriptor_t desc;
    pfdl_request_t req;
    pfdl_status_t stat;

    desc.wide_voltage_mode_u08 = 0; // Full speed mode
    desc.fx_MHz_u08 = 32; //32MHz

    stat = PFDL_Open(&desc);

    req.index_u16 = block * 1024;
    req.data_pu08 = buf;
    req.bytecount_u16 = bytes;
    req.command_enu = PFDL_CMD_READ_BYTES;

    stat = PFDL_Execute(&req);

    // ToDo: エラー処理

    while(stat == PFDL_BUSY) {
        stat = PFDL_Handler();
    }

    PFDL_Close();
    
    return(0);
}

int Flash_Write(int block, int bytes, unsigned char *buf)
{
    pfdl_descriptor_t desc;
    pfdl_request_t req;
    pfdl_status_t stat;

    desc.wide_voltage_mode_u08 = 0; // Full speed mode
    desc.fx_MHz_u08 = 32; //32MHz

    stat = PFDL_Open(&desc);

    /* イレース */
    req.index_u16 = block;
    req.command_enu = PFDL_CMD_ERASE_BLOCK;

    stat = PFDL_Execute(&req);

    // ToDo: エラー処理

    while(stat == PFDL_BUSY) {
        stat = PFDL_Handler();
    }

    /* 書き込み */
    req.index_u16 = block * 1024;
    req.data_pu08 = buf;
    req.bytecount_u16 = bytes;
    req.command_enu = PFDL_CMD_WRITE_BYTES;

    stat = PFDL_Execute(&req);

    // ToDo: エラー処理

    while(stat == PFDL_BUSY) {
        stat = PFDL_Handler();
    }
    
    PFDL_Close();
    
    return(0);
}
