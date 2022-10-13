/******************************************************************************
* File Name:   main.c
*
* Description: This example project demonstrates the basic operation of the
* I2C resource as Master using HAL APIs. The I2C master sends the
* command packets to the I2C slave to control an user LED.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2019-2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "resource_map.h"

/***************************************
*            Constants
****************************************/
#define CMD_TO_CMD_DELAY        (1000UL)
#define PACKET_SOP_POS          (0UL)
#define PACKET_CMD_POS          (1UL)
#define PACKET_EOP_POS          (2UL)
#define PACKET_STS_POS          (1UL)

/* Start and end of packet markers */
#define PACKET_SOP              (0x01UL)
#define PACKET_EOP              (0x17UL)

/* I2C slave address to communicate with */
#define I2C_SLAVE_ADDR          (0x24UL)

/* I2C bus frequency */
#define I2C_FREQ                (400000UL)

/* I2C slave interrupt priority */
#define I2C_SLAVE_IRQ_PRIORITY  (7u)

/* Command valid status */
#define STS_CMD_DONE            (0x00UL)
#define STS_CMD_FAIL            (0xFFUL)

/* Buffer and packet size */
#define PACKET_SIZE             (3UL)


/***************************************
*          Global Variables
****************************************/
#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_SLAVE))
cyhal_i2c_t sI2C;
uint8_t i2c_write_buffer[PACKET_SIZE];
uint8_t i2c_read_buffer[PACKET_SIZE] = {PACKET_SOP, STS_CMD_FAIL, PACKET_EOP};
#endif

/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void handle_error(void)
{
     /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);
}

#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_SLAVE))
/*******************************************************************************
* Function Name: handle_slave_event
********************************************************************************
* Summary:
* This is a callback function for I2C slave events. If a write event occurs, 
* the command packet is verified and executed.
*
* Parameters:
*  callback_arg : extra argument that can be passed to callback
*  event        : I2C event
*
* Return:
*  void
*
*******************************************************************************/
void handle_slave_event(void *callback_arg, cyhal_i2c_event_t event)
{
    if (0UL == (CYHAL_I2C_SLAVE_ERR_EVENT & event))
    {
        if (0UL != (CYHAL_I2C_SLAVE_WR_CMPLT_EVENT & event))
        {   
            /* Check start and end of packet markers. */
            if ((i2c_write_buffer[PACKET_SOP_POS] == PACKET_SOP) &&
                (i2c_write_buffer[PACKET_EOP_POS] == PACKET_EOP))
                {
                    /* Execute command */
                    cyhal_gpio_write( CYBSP_USER_LED, i2c_write_buffer[PACKET_CMD_POS]);
                    
                    /* Update status of received command. */
                    i2c_read_buffer[PACKET_STS_POS] = STS_CMD_DONE;
                }
            
            /* Configure read buffer for the next write */
            i2c_write_buffer[PACKET_SOP_POS] = 0;
            i2c_write_buffer[PACKET_EOP_POS] = 0;
            cyhal_i2c_slave_config_write_buffer(&sI2C, i2c_write_buffer, PACKET_SIZE);
        }
            
        if (0UL != (CYHAL_I2C_SLAVE_RD_CMPLT_EVENT & event))
        {
            /* Configure write buffer for the next read */
            i2c_read_buffer[PACKET_STS_POS] = STS_CMD_FAIL;
            cyhal_i2c_slave_config_read_buffer(&sI2C, i2c_read_buffer, PACKET_SIZE);
        }
    }
}
#endif

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function.
*   1. I2C Master sends command packet to the slave
*   2. I2C Master reads the response packet to generate the next command
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    /* Set up the device based on configurator selections */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    result = cy_retarget_io_init( CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, 
                                  CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("**************************\r\n");
    printf("HAL: I2C Master\r\n");
    printf("**************************\r\n\n");

#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_SLAVE))
    /* Configure user LED */
    printf(">> Configuring user LED..... ");
    result = cyhal_gpio_init( CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, 
                              CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    printf("Done\r\n");

    cyhal_i2c_cfg_t sI2C_cfg;

    /* Configure I2C Slave */
    printf(">> Configuring I2C Slave..... ");
    sI2C_cfg.is_slave = true;
    sI2C_cfg.address = I2C_SLAVE_ADDR;
    sI2C_cfg.frequencyhal_hz = I2C_FREQ;
    result = cyhal_i2c_init( &sI2C, sI2C_SDA, sI2C_SCL, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    result = cyhal_i2c_configure(&sI2C, &sI2C_cfg);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    printf("Done\r\n");

    cyhal_i2c_slave_config_read_buffer( &sI2C, i2c_read_buffer, PACKET_SIZE);
    cyhal_i2c_slave_config_write_buffer( &sI2C, i2c_write_buffer, PACKET_SIZE);
    cyhal_i2c_register_callback( &sI2C, handle_slave_event, NULL);
    cyhal_i2c_enable_event( &sI2C,
        (cyhal_i2c_event_t)(CYHAL_I2C_SLAVE_WR_CMPLT_EVENT 
                           | CYHAL_I2C_SLAVE_RD_CMPLT_EVENT 
                           | CYHAL_I2C_SLAVE_ERR_EVENT),    
                           I2C_SLAVE_IRQ_PRIORITY, true);
#endif

#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_MASTER))
    cyhal_i2c_t mI2C;
    cyhal_i2c_cfg_t mI2C_cfg;
    uint8_t cmd = CYBSP_LED_STATE_ON;
    uint8_t buffer[PACKET_SIZE];

    /* Configure I2C Master */
    printf(">> Configuring I2C Master..... ");
    mI2C_cfg.is_slave = false;
    mI2C_cfg.address = 0;
    mI2C_cfg.frequencyhal_hz = I2C_FREQ;
    result = cyhal_i2c_init( &mI2C, mI2C_SDA, mI2C_SCL, NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    result = cyhal_i2c_configure( &mI2C, &mI2C_cfg);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    printf("Done\r\n\n");
#endif

    /* Enable interrupts */
    __enable_irq();
 
#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_SLAVE))
    printf("User LED should start blinking \r\n");
#endif

    for (;;)
    {
#if ((I2C_MODE == I2C_MODE_BOTH) || (I2C_MODE == I2C_MODE_MASTER))
        /* create packet to be sent to slave.  */
        buffer[PACKET_SOP_POS] = PACKET_SOP;
        buffer[PACKET_EOP_POS] = PACKET_EOP;
        buffer[PACKET_CMD_POS] = cmd;

        /* Send packet with command to the slave. */
        if (CY_RSLT_SUCCESS == cyhal_i2c_master_write( &mI2C, I2C_SLAVE_ADDR,
                                                  buffer, PACKET_SIZE, 0, true))
        {
            /* Read response packet from the slave. */
            if (CY_RSLT_SUCCESS == cyhal_i2c_master_read( &mI2C, I2C_SLAVE_ADDR,
                                                 buffer, PACKET_SIZE , 0, true))
            {
                /* Check packet structure and status */
                if ((PACKET_SOP   == buffer[PACKET_SOP_POS]) &&
                   (PACKET_EOP   == buffer[PACKET_EOP_POS]) &&
                   (STS_CMD_DONE == buffer[PACKET_CMD_POS]))
                    {
                        /* Next command to be written. */
                        cmd = (cmd == CYBSP_LED_STATE_ON) ?
                               CYBSP_LED_STATE_OFF : CYBSP_LED_STATE_ON;
                    }
                else
                    {
                        handle_error();
                    }
            }

            /* Give delay between commands. */
            cyhal_system_delay_ms(CMD_TO_CMD_DELAY);
        }
#endif
    }
}
