#include <iostream>
#include <string>
#include <cstring>

using std::cout;
using std::endl;
using std::string;

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
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int index_i_parameter;
    int index_f_parameter;
    int index_i_register;
    int index_f_register;

private:
    asynUser* asyn_user;
    asynStatus performIO(uint8_t command, uint8_t address, uint32_t* value);
};

AGPSController::AGPSController(const char* port_name, const char* name)
    : asynPortDriver(port_name,
                    255,
                    asynInt32Mask | asynFloat64Mask | asynDrvUserMask,
                    asynInt32Mask | asynFloat64Mask,
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
    createParam(I_PARAMETER, asynParamFloat64, &index_i_parameter);
    createParam(F_REGISTER,  asynParamFloat64, &index_f_register);
    createParam(I_REGISTER,  asynParamInt32,   &index_i_register);

    uint32_t data = 0;
    status = performIO(COMMAND_CONTROL, 0, &data);
    if(status != asynSuccess)
        cout << "Warning: Could not take control of the power supply." << endl;
}

asynStatus AGPSController::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int address;
    int function = pasynUser->reason;
    asynStatus status;
    uint32_t data;
    float float_value;

    if(function < index_f_parameter || function > index_i_register)
    {
        cout << "Invalid asyn function: " << function << endl;
        return asynError;
    }

    getAddress(pasynUser, &address);
    if(function == index_f_register || function == index_i_register)
    {
        status = performIO(COMMAND_READ_REGISTER, address, &data);
        if(status != asynSuccess)
        {
            cout << "Read register with address " << address << " failed" << endl;
            return status;
        }
    }
    else
    {
        status = performIO(COMMAND_READ_PARAMETER, address, &data);
        if(status != asynSuccess)
        {
            cout << "Read parameter with address " << address << " failed" << endl;
            return status;
        }
    }

    if(function == index_f_parameter || function == index_f_register)
    {
        memcpy(&float_value, &data, sizeof(data));
        *value = float_value;
    }
    else
        *value = data;

    return asynSuccess;
}

asynStatus AGPSController::writeFloat64(asynUser* pasynUser, epicsFloat64 value)
{
    int address;
    int function = pasynUser->reason;
    asynStatus status;
    uint32_t data;

    getAddress(pasynUser, &address);
    if(function < index_f_parameter || function > index_i_register)
    {
        cout << "Invalid asyn function: " << function << endl;
        return asynError;
    }

    if(function == index_f_parameter || function == index_f_register)
    {
        float v = (float) value;
        memcpy(&data, &v, sizeof(data));
    }
    else
        data = (uint32_t) value;

    if(function == index_f_register || function == index_i_register)
    {
        status = performIO(COMMAND_WRITE_REGISTER, address, &data);
        if(status != asynSuccess)
        {
            cout << "Write register with address " << address << " failed" << endl;
            return status;
        }
    }
    else
    {
        status = performIO(COMMAND_WRITE_PARAMETER, address, &data);
        if(status != asynSuccess)
        {
            cout << "Write parameter with address " << address << " failed" << endl;
            return status;
        }
    }

    return asynSuccess;
}

asynStatus AGPSController::readInt32(asynUser* pasynUser, epicsInt32* value)
{
    string message;
    uint8_t command;
    uint32_t data;
    asynStatus status;
    int address;
    int function = pasynUser->reason;
    getAddress(pasynUser, &address);

    if(function != index_i_register && function != index_i_parameter)
    {
        cout << "Unknown function: " << function << endl;
        return asynError;
    }

    if(function == index_i_register)
    {
        command = COMMAND_READ_REGISTER;
        message = "Read register";
    }
    else
    {
        command = COMMAND_READ_PARAMETER;
        message = "Read parameter";
    }

    status = performIO(command, address, &data);
    if(status != asynSuccess)
    {
        cout << message << " with address " << address << " failed" << endl;
        return status;
    }

    *value = data;
    return asynSuccess;
}

asynStatus AGPSController::performIO(uint8_t command, uint8_t address, uint32_t* value)
{
    size_t  tx_bytes;
    size_t  rx_bytes;
    uint8_t reply;
    asynStatus status;
    int  reason;
    char rx_array[RX_PACKET_SIZE];
    char tx_array[TX_PACKET_SIZE];

    memcpy(tx_array, &command, sizeof(command));
    memcpy(tx_array + 1, &address, sizeof(address));
    memcpy(tx_array + 2, value, sizeof(*value));
    status = pasynOctetSyncIO->writeRead(this->asyn_user, tx_array, TX_PACKET_SIZE, rx_array, RX_PACKET_SIZE, 1, &tx_bytes, &rx_bytes, &reason);
    if(status != asynSuccess || tx_bytes != TX_PACKET_SIZE || rx_bytes != RX_PACKET_SIZE || reason != ASYN_EOM_CNT)
        return status;

    memcpy(&reply, rx_array, sizeof(reply));
    if(reply >= REPLY_SYS_COMMAND_ERROR)
        return asynError;
    
    if(command == COMMAND_READ_PARAMETER || command == COMMAND_READ_REGISTER)
        memcpy(value, rx_array + 1, sizeof(uint32_t));
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
