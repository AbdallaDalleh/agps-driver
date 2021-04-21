#include <iostream>

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>
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
    int status = pasynOctetSyncIO->connect(name, 1, &asyn_user, NULL);
    if(status != asynSuccess)
    {
        // TODO: Use asynPrint
        fprintf(stderr, "Could not connect to port %s\n", name);
        return;
    }
}

int AGPSController::write_read(uint8_t command, uint8_t address, uint32_t* value)
{
    size_t bytes;
    int status;
    int reason;
    char rx_array[RX_PACKET_SIZE];
    char tx_array[TX_PACKET_SIZE];

    *(tx_array) = command;
    *(tx_array + 1) = address;
    *(tx_array + 2) = *value;
    status = pasynOctetSyncIO->write(this->asyn_user, tx_array, TX_PACKET_SIZE, 1, &bytes);
    if(status != asynSuccess || bytes != TX_PACKET_SIZE)
        return status;
    
    status = pasynOctetSyncIO->read(this->asyn_user, rx_array, RX_PACKET_SIZE, 1, &bytes, &reason);
    if(status != asynSuccess)
        return status;
    
    int reply = *(rx_array);
    if(reply == INPUT_TYPE_ERROR || reply == PACKET_SIZE_ERROR || bytes != RX_PACKET_SIZE /* || reason != ASYN_EOM_CNT */)
        return asynError;
    
    *value = *(rx_array + 2);
    return asynSuccess;
}