/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"


/* Task Structure definition */
typedef struct task {
    int state; //Task's current state
    unsigned long period; //Task period in ms
    unsigned long elapsedTime; //Time elapsed since last task tick in ms
    int (*TickFct)(int); //Task tick function
} task;


/* enumerations for state machine states */
enum BUTTON_STATES {TEMP_INC, TEMP_DEC, BUTTON_INIT} BUTTON_STATE;
enum TEMP_SENSOR_STATE {READ_TEMP, TEMP_INIT};
enum HEAT_STATE {HEAT_ON, HEAT_OFF, HEAT_INIT};

/* global vars for thermostat */
int seconds = 0; //var counting seconds since start to pass to server
int16_t ambientTemp = 0; //init temp var to keep track of temperature read by sensor
int16_t setPoint = 20; //init setPoint to keep track of the set point of the thermostat

#define DISPLAY(x) UART2_write(uart, &output, x, NULL);

// I2C Global Variables
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[3] = {
    { 0x48, 0x0000, "11X" },
    { 0x49, 0x0000, "116" },
    { 0x41, 0x0001, "006" }
};
uint8_t             txBuffer[1];
uint8_t             rxBuffer[2];
I2C_Transaction     i2cTransaction;

// UART Global Variables
char                output[64];
int                 bytesToSend;

// Driver Handles - Global variables
I2C_Handle      i2c;
UART2_Handle     uart;
Timer_Handle    timer0;

volatile unsigned char TimerFlag = 0;
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
}

//callback to increase temp on button 0 press
void gpioTempIncreaseCallback(uint_least8_t index)
{
    BUTTON_STATE = TEMP_INC;
}

//callback to decrease temp on button 1 press
void gpioTempDecreaseCallback(uint_least8_t index)
{
    BUTTON_STATE = TEMP_DEC;
}

void initUART(void)
{
    UART2_Params uartParams;
    size_t bytesRead;
    size_t bytesWritten = 0;
    uint32_t status     = UART2_STATUS_SUCCESS;

    /* Create a UART where the default read and write mode is BLOCKING */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 115200;

    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }
}

// Make sure you call initUART() before calling this function.
void initI2C(void)
{
    int8_t              i, found;
    I2C_Params          i2cParams;

    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Init the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;

        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY(snprintf(output, 64, "Found\n\r"))
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.targetAddress))
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}


void initTimer(void)
{
    Timer_Params    params;

    // Init the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000; //100ms timer period
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }

    DISPLAY( snprintf(output, 64, "Timer Configured\n\r"))
}

int16_t readTemp(void)
{
    int     j;
    int16_t temperature = 0;

    i2cTransaction.readCount  = 2;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;

        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }

    return temperature;
 }

//reads button state every 200ms and changes set temp if button is pressed
int buttonTick(int state){
    switch(state){
        case TEMP_INC:
            if(setPoint < 99){
                setPoint++; //only keeps raising set temperature if within threshold
            }
            BUTTON_STATE = BUTTON_INIT; //stops increasing after press
            break;
        case TEMP_DEC:
            if(setPoint > 0){
                setPoint--;
            }
            BUTTON_STATE = BUTTON_INIT; //stops decreasing after press
            break;
        default:
            break;
    }

    //reset state after press
    state = BUTTON_STATE;
    return state;
}


//initializes and reads new temperature every 500ms
int tempTick(int state){
    switch(state){
        case TEMP_INIT:
            state = READ_TEMP;
            break;
        case READ_TEMP:
            ambientTemp = readTemp(); //set ambient temp to that which the sensor reads
            break;
        default:
            break;

    }
    return state;
}

//checks set point versus read temp value and then updates server
int reportTick(int state){
    if(ambientTemp < setPoint){
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
        state = HEAT_ON;
    }
    else{
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
        state = HEAT_OFF;
    }

    DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", ambientTemp, setPoint, state, seconds));

    seconds++;
    return state;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    int         j;
    int16_t     temperature = 0;
    int16_t     setpoint = 0;
    uint16_t    timer = 0;
    uint8_t     heat = 0;
    uint32_t    seconds = 0;
  //  size_t bytesWritten = 0;

    /* Call driver init functions */
    GPIO_init();

#ifdef CONFIG_GPIO_TMP_EN
    GPIO_setConfig(CONFIG_GPIO_TMP_EN, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);
    /* Allow the sensor to power on */
    sleep(1);
#endif

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioTempIncreaseCallback);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);

    /*
     *  If more than one input pin is available for your device, interrupts
     *  will be enabled on CONFIG_GPIO_BUTTON1.
     */
    if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
        /* Configure BUTTON1 pin */
        GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

        /* Install Button callback */
        GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioTempDecreaseCallback);
        GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
    }


    initUART(); // The UART must be initialized before calling initI2C()
    DISPLAY( snprintf(output, 64, "UART + GPIO + Timer +I2C + Interrupts by Eric Gregori\n\r"))
    DISPLAY( snprintf(output, 64, "GPIO + Interrupts configured\n\r"))
    initI2C();
    initTimer();

    //list of tasks initialized
            task taskList[3] = {
                //Button state check every 200 ms
                {
                     .state = BUTTON_INIT,
                     .period = 200,
                     .elapsedTime = 200,
                     .TickFct = &buttonTick
                },
                //Temp state check every 500ms
                {
                     .state = TEMP_INIT,
                     .period = 500,
                     .elapsedTime = 500,
                     .TickFct = &tempTick
                },
                //Server report every second
                {
                     .state = HEAT_INIT,
                     .period = 1000,
                     .elapsedTime = 1000,
                     .TickFct = &reportTick
                }

            };

    // Loop Forever
    // The student should add flags (similiar to the timer flag) to the button handlers.
    // Timer interrupt set to 100ms
    DISPLAY( snprintf(output, 64, "Starting Task Scheduler\n\r"))
    while (1)
    {
        unsigned int i = 0;

        //cycles every task each 100ms and sets state accordingly
        for(i = 0; i < 3; ++i)
        {
            //if time elapsed matches or exceeds period
             if(taskList[i].elapsedTime >= taskList[i].period)
             {
                 //calls tickfunction and resets counter
                 taskList[i].state = taskList[i].TickFct(taskList[i].state);
                 taskList[i].elapsedTime = 0;
             }
             //ticks each task every 100ms and updates count
             taskList[i].elapsedTime += 100;
        }


        //Every 100ms flag is raised
        while (!TimerFlag){}   // Wait for timer period
        TimerFlag = 0;         // Lower flag raised by timer
    }

    return (NULL);
}
