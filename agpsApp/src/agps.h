#include <iostream>

using std::cout;

typedef struct
{
    uint8_t  type;
    uint8_t  address;
    uint32_t data;
} tx_packet_t;

typedef struct
{
    uint8_t  reply;
    uint32_t data;
} rx_packet_t;

#define SYS_COMMAND         0x00
#define READ_REGISTER       0x01
#define WRITE_REGISTER      0x02
#define READ_PARAMETER      0x03
#define WRITE_PARAMETER     0x04

#define SYS_COMMAND_OK      0x00
#define READ_REGISTER_OK    0x01
#define WRITE_REGISTER_OK   0x02
#define READ_PARAMETER_OK   0x03
#define WRITE_PARAMETER_OK  0x04
#define INPUT_TYPE_ERROR    0x05
#define PACKET_SIZE_ERROR   0x06

typedef enum
{
    OFF = 1,
    STANDBY,
    TRIGGER_ON,
    INTERLOCK,
    RESET,
    START_LOGGER,
    STOP_LOGGER,
    READ_FLASH,
    WRITE_FLASH
} command_t;

typedef enum
{
    MACHINE_STATE = 1,
    FILE_STATE,
    HV_STATE,
    REGULATOR_STATE,
    LOGGER_STATE,
    SUPERVISOR_STATE,
    INTERLOCKS,
    ERROR,
    DRIVE_DIFFERENCE,
    DRIVE,
    ADC_CH1,
    ADC_CH2,
    ADC_CH3,
    ADC_CH4,
    ADC_CH5,
    ADC_CH6,
    ADC_CH7,
    ADC_CH8,
    ADC_CH9,
    ADC_CH10,
    ADC_CH11,
    ADC_CH12,
    I_FILAMENT,
    V_FILAMENT,
    I_HV,
    V_HV,
    I_GUN,
    I_FIL_SETPOINT,
    V_FIL_SETPOINT,
    I_HV_SETPOINT,
    V_HV_SETPOINT,
    I_HV_STANDBY,
    V_GUN_TRIGGER
} register_t