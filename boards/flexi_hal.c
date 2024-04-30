/*
  flexi_hal.c - driver code for STM32F4xx ARM processors on Flexi-HAL board

  Part of grblHAL

  Copyright (c) 2022 Expatria Technologies

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "driver.h"

#if defined(BOARD_FLEXI_HAL)

#include <math.h>
#include <string.h>

#include "main.h"
#include "i2c.h"
#include "grbl/protocol.h"
#include "grbl/settings.h"

DMA_HandleTypeDef hdma_spi1_tx;
typedef void (*count_callback_ptr)(void);

#if 0
static uint8_t keycode = 0;
static count_callback_ptr keypad_callback = NULL;
static bool pendant_tx_active = 0;

void I2C_PendantRead (uint32_t i2cAddr, uint16_t memaddress, uint16_t size, uint8_t * data, count_callback_ptr callback)
{

    uint32_t ms = hal.get_elapsed_ticks();  //50 ms timeout
    uint32_t timeout_ms = ms + 50;

    uint8_t addr[2];

    addr[0] = (uint8_t)(memaddress & 0xFF);
    addr[1] = (uint8_t)((memaddress >> 8) & 0xFF);

    if(keypad_callback != NULL || pendant_tx_active) //we are in the middle of a read
        return;

    keypad_callback = callback;

    while((i2c_send (i2cAddr, addr, 2, 1)) != HAL_OK){
        if (ms > timeout_ms){
            pendant_tx_active = 0;
            i2c_init();
            return;
        }
        hal.delay_ms(1, NULL);
        ms = hal.get_elapsed_ticks();
    }        

    while((i2c_receive (i2cAddr, data, size, 0)) != HAL_OK){
        if (ms > timeout_ms){
            keypad_callback = NULL;
            keycode = 0;
            i2c_init();
            return;
        }
        hal.delay_ms(1, NULL);
        ms = hal.get_elapsed_ticks();
    }
}

void I2C_PendantWrite (uint32_t i2cAddr, uint8_t *buf, uint16_t bytes)
{

    uint32_t ms = hal.get_elapsed_ticks();  //50 ms timeout
    uint32_t timeout_ms = ms + 50;

    if(keypad_callback != NULL || pendant_tx_active) //we are in the middle of a read
        return;
    pendant_tx_active = 1;

    
    while((i2c_send (i2cAddr, buf, bytes, 0)) != HAL_OK){
        if (ms > timeout_ms){
            pendant_tx_active = 0;
            i2c_init();
            return;
        }
        hal.delay_ms(1, NULL);
        ms = hal.get_elapsed_ticks();
    }
    
}

void HAL_FMPI2C_MemRxCpltCallback(FMPI2C_HandleTypeDef *hi2c)
{
    if(keypad_callback) {
        keypad_callback();
        keypad_callback = NULL;
    }
}

void HAL_FMPI2C_MasterTxCpltCallback(FMPI2C_HandleTypeDef *hi2c)
{
    pendant_tx_active = 0;
}

#endif

// called from stream drivers while tx is blocking, returns false to terminate

#if 0
bool flexi_stream_tx_blocking (void)
{
    // TODO: Restructure st_prep_buffer() calls to be executed here during a long print.

    grbl.on_execute_realtime(state_get());

    return !(sys.rt_exec_state & EXEC_RESET);
}

#endif
#endif


/**
  * @brief This function handles DMA2 stream3 global interrupt.
  */

static void MX_DMA_Init (void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();

  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

void board_init (void)
{
    #if defined(BOARD_FLEXI_HAL) && KEYPAD_ENABLE
    //i2c_port = I2C_GetPort();
    #endif

    MX_DMA_Init();
}
#endif

