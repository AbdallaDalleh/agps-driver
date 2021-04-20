#include <iostream>

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynGenericPointerSyncIO.h>
#include <iocsh.h>

#include "agps.h"

class AGPSController : public asynPortDriver
{
public:
    AGPSController(const char* port_name, const char* asyn_name);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

private:
    asynUser* asyn_user;
    int write_read(uint8_t command, uint8_t address, uint32_t* value);
};

AGPSController::AGPSController(const char* port_name, const char* name)
    : asynPortDriver(port_name,
                    1,
                    asynInt32Mask | asynFloat64Mask,
                    0,
                    ASYN_CANBLOCK,
                    1,
                    0, 0)
{
    int status = pasynGenericPointerSyncIO->connect(name, 1, &asyn_user, NULL);
    if(status != asynSuccess)
    {
        // TODO: Use asynPrint
        fprintf(stderr, "Could not connect to port %s\n", name);
        return;
    }
}

int AGPSController::write_read(uint8_t command, uint8_t address, uint32_t* value)
{
    rx_packet_t rx;
    tx_packet_t tx = {
        .type    = command,
        .address = address,
        .data    = *value
    };

    int status = pasynGenericPointerSyncIO->write(this->asyn_user, &tx, 1);
    if(status != asynSuccess)
        return status;
    
    status = pasynGenericPointerSyncIO->read(this->asyn_user, &rx, 1);
    if(status != asynSuccess)
        return status;
    
    if(rx.reply == INPUT_TYPE_ERROR || rx.reply == PACKET_SIZE_ERROR)
        return asynError;
    
    *value = rx.data;
    return asynSuccess;
}