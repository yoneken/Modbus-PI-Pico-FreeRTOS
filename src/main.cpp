#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "GPIO.hpp"
#include "Modbus.h"
#include <array>

//extern "C" {
//  #include "can2040.h"
//}

modbusHandler_t ModbusH;
uint16_t ModbusDATA[0x8ff];
modbusHandler_t ModbusH2;
uint16_t ModbusDATA2[0x8ff];

/*
Register address Description
Monitoring Group
0x0000 status
0x0001 speed (r / min)
0x0002 current percentage
0x0003 current (A)
0x0004 command position (p)
0x0006 motor position (p)
0x0008 position error (p)
0x000F current alarm code
0x0010 current value when alarm occurs
0x0011 speed value when alarm occurs
0x0012 input voltage value when alarm occurs
0x0020 calibration value of the 6-axis force sensor (12 bytes)
0x0030 X Force
0x0034 Y Force
0x0038 Z Force
0x0040 X Torque
0x0044 Y Torque
0x0048 Z Torque

Fn1xx Control Parameters
Number, Name, Setting range, Unit, Factory setting, Effective time, Register address
Fn100, enable the gripper, 0-1, -, 0, immediately, 0x0100
Fn101, control mode, 0-2 0：location 1：speed, -, 0, immediately 0x0101
Fn109, fault reset, 0-1, -, 0, immediately, 0x0109

Fn2xx Gain Parameters
Fn200, position loop gain, 10-20000, 0.1Hz, 200, immediately, 0x0200
Fn201, position loop feedforward, 0-1000, 0.1%, 200, immediately, 0x0201
Fn202, position loop feedforward filtering time, 0-1000, 1ms, 5, immediately, 0x0202
Fn203, speed loop gain, 10-20000, 0.1, 100, immediately, 0x0203
Fn204, speed loop integral, 10-10000, 0.1, 300, immediately, 0x0204

Fn3xx Position Parameters
Fn300, Position acceleration time, 1-2000, ms, 100, power on effective, 0x0300
Fn301, position deceleration time, 1-2000, ms, 100, power on effective, 0x0301
Fn302, position smoothing time, 1-200, ms, 10, power on effective, 0x0302
Fn303, position speed, 1-20000, r/min, 1500, immediately, 0x0303
Fn308, position error alarm value, 0x00000000-0xFFFFFFFF, Pulse, 0x20000, immediately, 0x0308
Fn310, position command alarm value, 0x00000000-0xFFFFFFFF, Pulse, 0x20000, immediately, 0x030A

Fn4xx Speed Parameters
Fn400, speed command, -20000-20000, r/min, 100, immediately, 0x0400
Fn403, speed limit, 0-20000, r/min, 5000, immediately, 0x0403

Fn5xx Torque Parameters
Fn505, sarting current limit, 5-100, -, 16, immediately, 0x0505
Fn506, hold current limit, 1-100, -, 10, immediately, 0x0506
Fn507, starting current operation time, 100-30000, ms, 1500, immediately, 0x0507

Fn6xx Communication Parameters

Fn7xx Position Command
Fn700, position command, 0x00000000-0xFFFFFFFF, pulse, 0x00000000, immediately, 0x0700
Fn702, position feedback, 0x00000000-0xFFFFFFFF, pulse, 0x00000000, read-only, 0x0702
Fn706, electronic gear ratio numerator, 1-30000, -, 100, power on effective, 0x0706
Fn707, electronic gear ratio denominator, 1-30000, -, 100, power on effective, 0x0707

Fn8xx Motor Parameters
Fn800, hardware version, -, -, 10, read-only, 0x0800
Fn801, software version, -, -, read-only, 0x0801
Fn804, motor ID, 0-999, -, 100, effective after power on, 0x0804
Fn805, rated power, 1-2000, W, 100, effective after power on, 0x0805
Fn806, rated voltage, 1-6000, 0.01V, 2400, effective after power on, 0x0806
Fn807, rated current, 1-2400, 0.01A, 800, effective after power on, 0x0807
*/

static pico_cpp::GPIO_Pin ledPin(25,pico_cpp::PinType::Output);
void vTaskMaster( void * pvParameters )
{
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. 
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );
    */
    modbus_t telegram[2];

    uint32_t u32NotificationValue;

    telegram[0].u8id = 8; // slave address
    telegram[0].u8fct = MB_FC_WRITE_MULTIPLE_REGISTERS; // function code (this one is registers read)
    telegram[0].u16RegAdd = 0x030; // start address in slave
    telegram[0].u16CoilsNo = 1; // number of elements (coils or registers) to read
    telegram[0].u16reg = ModbusDATA; // pointer to a memory array in the Arduino


    telegram[1].u8id = 8; // slave address
    telegram[1].u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
    telegram[1].u16RegAdd = 0x030; // start address in slave
    telegram[1].u16CoilsNo = 8; // number of elements (coils or registers) to read
    telegram[1].u16reg = ModbusDATA; // pointer to a memory array in the Arduino
    
    for(;;)
    {
            
	        ModbusQuery(&ModbusH, telegram[1]); // make a query
	        u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
	        if(u32NotificationValue)
	        {
	  	    //handle error
		    //  while(1);
	        }
            vTaskDelay(100);

            if(xSemaphoreTake(ModbusH.ModBusSphrHandle , portMAX_DELAY) == pdTRUE){
                printf("Master %d\n", ModbusDATA[0x0]);
                ModbusDATA[0x0]++;
                xSemaphoreGive(ModbusH.ModBusSphrHandle);
            }
        
	        ModbusQuery(&ModbusH, telegram[0]); // make a query
	        u32NotificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // block until query finishes
	        if(u32NotificationValue)
	        {
	  	    //handle error
		    //  while(1);
	        }
            vTaskDelay(100);
                                  
    }
}

void vTaskSlave( void * pvParameters )
{
    for(;;)
    {
        if(xSemaphoreTake(ModbusH2.ModBusSphrHandle , portMAX_DELAY) == pdTRUE){
            gpio_put(PICO_DEFAULT_LED_PIN, ModbusDATA2[0x030] & 0x01 );
            printf("Slave %d\n", ModbusDATA2[0x030]);
            xSemaphoreGive(ModbusH2.ModBusSphrHandle);
        }
        vTaskDelay(100);
    }
}

#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define UART1_TX_PIN 4
#define UART1_RX_PIN 5


void initSerial()
{
    uart_init(uart0, 115200);
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart0, false);


    uart_init(uart1, 115200);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart1, false);
}


void initLED()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}


int main() {
    stdio_init_all();

    BaseType_t xReturnedMaster, xReturnedSlave;
    TaskHandle_t xHandleMaster = NULL, xHandleSlave = NULL;
    UBaseType_t uxCoreAffinityMask;

    initSerial();
    initLED();

    /* Create the task, storing the handle. */
    xReturnedMaster = xTaskCreate(
                    vTaskMaster,       /* Function that implements the task. */
                    "Master task",   /* Text name for the task. */
                    512,             /* Stack size in words, not bytes. */
                    ( void * ) 1,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandleMaster );

    xReturnedSlave = xTaskCreate(
                    vTaskSlave,       /* Function that implements the task. */
                    "Slave task",   /* Text name for the task. */
                    512,             /* Stack size in words, not bytes. */
                    ( void * ) 1,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandleSlave );

    // force to run on core1
    uxCoreAffinityMask = ( ( 1 << 1 ));
    vTaskCoreAffinitySet( xHandleSlave, uxCoreAffinityMask );

    ModbusH.uModbusType = MB_MASTER;
    ModbusH.port = uart1;
    ModbusH.u8id = 0; // For master it must be 0
    ModbusH.u16timeOut = 1000;
    // ModbusH.EN_Port = NULL;
    ModbusH.EN_Port = (uint16_t *) 1; //enables the RS485 ChipSelect
    ModbusH.EN_Pin = 3; //Pi controlling RS485 ChipSelect
    ModbusH.u16regs = ModbusDATA;
    ModbusH.u16regsize= sizeof(ModbusDATA)/sizeof(ModbusDATA[0]);
    ModbusH.xTypeHW = USART_HW;
    //Initialize Modbus library
    ModbusInit(&ModbusH);
    //Start capturing traffic on serial Port
    ModbusStart(&ModbusH);


    ModbusH2.uModbusType = MB_SLAVE;
    ModbusH2.port = uart0;
    ModbusH2.u8id = 8; // For master it must be 0
    ModbusH2.u16timeOut = 1000;
    // ModbusH.EN_Port = NULL;
    ModbusH2.EN_Port = NULL; //enables the RS485 ChipSelect
    ModbusH2.EN_Pin = 2; //Pi controlling RS485 ChipSelect
    ModbusH2.u16regs = ModbusDATA2;
    ModbusH2.u16regsize= sizeof(ModbusDATA2)/sizeof(ModbusDATA2[0]);
    ModbusH2.xTypeHW = USART_HW;
    //Initialize Modbus library
    ModbusInit(&ModbusH2);
    //Start capturing traffic on serial Port
    ModbusStart(&ModbusH2);

    vTaskStartScheduler();

    while(1)
    {
        configASSERT(0);    /* We should never get here */
    }

}