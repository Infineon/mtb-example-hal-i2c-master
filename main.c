/*******************************************************************************
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
* Copyright 2019-2024, Cypress Semiconductor Corporation (an Infineon company) or
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Delay of 1000ms between commands */
#define CMD_TO_CMD_DELAY        (1000UL)

/* Packet positions */
#define PACKET_SOP_POS          (0UL)
#define PACKET_CMD_POS          (1UL)
#define PACKET_EOP_POS          (2UL)

/* Start and end of packet markers */
#define PACKET_SOP              (0x01UL)
#define PACKET_EOP              (0x17UL)

/* I2C slave address to communicate with */
#define I2C_SLAVE_ADDR          (0x24UL)

/* I2C bus frequency */
#define I2C_FREQ                (400000UL)

/* Command valid status */
#define STATUS_CMD_DONE         (0x00UL)

/* Packet size */
#define PACKET_SIZE             (3UL)

#ifdef XMC7200D_E272K8384
#define KIT_XMC72
#endif

/*******************************************************************************
* Global Variables
*******************************************************************************/


/*******************************************************************************
* Function Prototypes
*******************************************************************************/


/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  uint32_t status - status indicates success or failure
*
* Return:
*  void
*
*******************************************************************************/
void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}


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
    cyhal_i2c_t mI2C;
    cyhal_i2c_cfg_t mI2C_cfg;
    uint8_t cmd = CYBSP_LED_STATE_ON;
    uint8_t buffer[PACKET_SIZE];

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    /* Board init failed. Stop program execution */
    handle_error(result);

#if defined(KIT_XMC72)
    /*Configure clock settings for KIT_XMC72_EVK */
    cyhal_clock_t clock_fll, clock_hf, clock_peri;
    result = cyhal_clock_reserve(&clock_hf, &CYHAL_CLOCK_HF[0]);
    result = cyhal_clock_reserve(&clock_fll, &CYHAL_CLOCK_FLL);
    if(result == CY_RSLT_SUCCESS){
    result = cyhal_clock_set_source(&clock_hf, &clock_fll);
    }
    /* Set divider to 1 for Peripheral Clock */
    result = cyhal_clock_reserve(&clock_peri, CYHAL_CLOCK_PERI);
    if(result == CY_RSLT_SUCCESS){
    result = cyhal_clock_set_divider(&clock_peri,1);
    }
#endif

    /* Initialize the retarget-io */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                  CY_RETARGET_IO_BAUDRATE);
    /* Retarget-io init failed. Stop program execution */
    handle_error(result);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "HAL: I2C Master "
           "****************** \r\n\n");

    /* I2C Master configuration settings */
    printf(">> Configuring I2C Master..... ");
    mI2C_cfg.is_slave = false;
    mI2C_cfg.address = 0;
    mI2C_cfg.frequencyhal_hz = I2C_FREQ;

    /* Init I2C master */
    result = cyhal_i2c_init(&mI2C, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    /* I2C master init failed. Stop program execution */
    handle_error(result);

    /* Configure I2C Master */
    result = cyhal_i2c_configure(&mI2C, &mI2C_cfg);
    /* I2C master configuration failed. Stop program execution */
    handle_error(result);

    printf("Done\r\n\n");

    /* Enable interrupts */
    __enable_irq();

    for (;;)
    {
        /* Create packet to be sent to slave */
        buffer[PACKET_SOP_POS] = PACKET_SOP;
        buffer[PACKET_EOP_POS] = PACKET_EOP;
        buffer[PACKET_CMD_POS] = cmd;

        /* Send packet with command to the slave */
        if (CY_RSLT_SUCCESS == cyhal_i2c_master_write(&mI2C, I2C_SLAVE_ADDR,
                                                  buffer, PACKET_SIZE, 0, true))
        {
            /* Read response packet from the slave */
            if (CY_RSLT_SUCCESS == cyhal_i2c_master_read(&mI2C, I2C_SLAVE_ADDR,
                                                 buffer, PACKET_SIZE, 0, true))
            {
                /* Check packet structure and status */
                if ((PACKET_SOP == buffer[PACKET_SOP_POS]) &&
                   (PACKET_EOP == buffer[PACKET_EOP_POS]) &&
                   (STATUS_CMD_DONE == buffer[PACKET_CMD_POS]))
                {
                    /* Next command to be written */
                    cmd = (cmd == CYBSP_LED_STATE_ON) ?
                           CYBSP_LED_STATE_OFF : CYBSP_LED_STATE_ON;
                }
                else
                {
                    handle_error(1);
                }
            }
            /* Give delay between commands */
            cyhal_system_delay_ms(CMD_TO_CMD_DELAY);
        }
    }
}
