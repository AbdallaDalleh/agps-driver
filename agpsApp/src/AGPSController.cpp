#include <iostream>

using std::cout;
using std::endl;

#include <epicsExport.h>
#include <asynPortDriver.h>
#include <asynOctetSyncIO.h>
#include <iocsh.h>

#include "agps.h"

#define I_PARAMETER "i_parameter"
#define F_PARAMETER "f_parameter"
#define I_REGISTER  "i_register"
#define F_REGISTER  "f_register"

class AGPSController : public asynPortDriver
{
public:
    AGPSController(const char* port_name, const char* asyn_name);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    // virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    // virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    // virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

protected:
    int index_i_parameter;
    int index_f_parameter;
    int index_i_register;
    int index_f_register;

private:
    asynUser* asyn_user;
    int write_read(uint8_t command, uint8_t address, uint32_t* value);
};

AGPSController::AGPSController(const char* port_name, const char* name)
    : asynPortDriver(port_name,
                    255,
                    asynInt32Mask | asynFloat64Mask,
                    0,
                    ASYN_CANBLOCK | ASYN_MULTIDEVICE,
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

    createParam(F_PARAMETER, asynParamFloat64, &index_f_parameter);
    createParam(I_PARAMETER, asynParamInt32,   &index_i_parameter);
    createParam(F_REGISTER,  asynParamFloat64, &index_f_register);
    createParam(I_REGISTER,  asynParamInt32,   &index_i_register);
}

asynStatus AGPSController::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int address;
    int function = pasynUser->reason;
    int status;
    uint32_t data;

    getAddress(pasynUser, &address);
    if(function == index_i_register)
    {
        // These are addresses for integer registers in the register map.
        if((address >= 0 && address <= 7) || address == 48 || address == 50)
        {
            status = write_read(COMMAND_READ_REGISTER, address, &data);
            if(status != asynSuccess)
            {
                cout << "Read integer register failed" << endl;
                return (asynStatus) status;
            }

            // setIntegerParam(function, data);
            *value = data;
            return asynSuccess;
        }
        else
        {
            cout << "Invalid integer register address" << endl;
            return asynError;
        }
    }
    else if(function == index_i_parameter)
    {
        if(address == 34 || (address >= 36 && address <= 59))
        {
            status = write_read(COMMAND_READ_PARAMETER, address, &data);
            if(status != asynSuccess)
            {
                cout << "Read integer parameter failed" << endl;
                return (asynStatus) status;
            }

            // setIntegerParam(function, value);
            *value = data;
            return asynSuccess;
        }
        else
        {
            cout << "Invalid integer parameter address" << endl;
            return asynError;
        }
    }
    else
    {
        cout << "Unknown readInt32 function: " << function << endl;
        return asynError;
    }
}

int AGPSController::write_read(uint8_t command, uint8_t address, uint32_t* value)
{
    size_t bytes;
    int  status;
    int  reason;
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

extern "C" {

asynStatus AGPSControllerConfigure(const char* port_name, const char* name)
{
    new AGPSController(port_name, name);
    return asynSuccess;
}

static const iocshArg arg0 = {"Port name", iocshArgString};
static const iocshArg arg1 = {"Asyn Port name", iocshArgString};
static const iocshArg* args[2] = { &arg0, &arg1 };
static const iocshFuncDef AGPSControllerFuncDef = {"AGPSControllerConfigure", 2, args};
static void  AGPSControllerConfigureCallFunc(const iocshArgBuf* args)
{
    AGPSControllerConfigure(args[0].sval, args[1].sval);
}

static void AGPSControllerRegister()
{
    iocshRegister(&AGPSControllerFuncDef, AGPSControllerConfigureCallFunc);
}

epicsExportRegistrar(AGPSControllerRegister);

}