#include "bsp_touch.h"
#include "bsp_dwt.h"

// I2C clock frequency: ~250kHz (clock period ~4us, delay 2us each)
// Precise delay to avoid busy-loop and improve timing stability
static void I2C_Delay(void)
{
    BSP_DWT_Delay_us(2);
}

// Initialize I2C bus: release SCL and SDA lines
void Touch_I2C_Init(void)
{
    I2C_SCL(1);
    I2C_SDA(1);
}

// Wait for slave ACK with deadlock protection (timeout: 500us)
// Returns I2C_ACK (1) if acknowledged, I2C_NACK (0) if timeout
static uint8_t Touch_I2C_WaitACK(void)
{
    uint32_t timeout = 0;
    I2C_SDA(1);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    
    // Wait for SDA low (slave pulls it). If SDA stays high after timeout, no ACK received
    while(I2C_SDA_READ())
    {
        timeout++;
        if(timeout > 250)  // 250 * 2us = 500us timeout
        {
            I2C_SCL(0);
            return I2C_NACK;
        }
        I2C_Delay();
    }
    I2C_SCL(0);
    I2C_Delay();
    return I2C_ACK;
}

// I2C START condition: SDA falls while SCL is high
void Touch_I2C_Start(void)
{
    I2C_SDA(1);
    I2C_SCL(1);
    I2C_Delay();
    I2C_SDA(0);
    I2C_Delay();
    I2C_SCL(0);
    I2C_Delay();
}

// I2C STOP condition: SDA rises while SCL is high
void Touch_I2C_Stop(void)
{
    I2C_SDA(0);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SDA(1);
    I2C_Delay();
}

// Send 8-bit data (MSB first), wait for ACK from slave
// Returns I2C_ACK (1) if acknowledged, I2C_NACK (0) if not
uint8_t Touch_I2C_WriteByte(uint8_t byte)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        I2C_SDA((byte & 0x80) >> 7);
        I2C_Delay();
        I2C_SCL(1);
        I2C_Delay();
        I2C_SCL(0);
        I2C_Delay();
        byte <<= 1;
    }
    return Touch_I2C_WaitACK();
}

// Receive 8-bit data (MSB first), send ACK/NACK to slave
// Parameters: ack - 1 for ACK (continue), 0 for NACK (last byte)
// Returns: received byte value
uint8_t Touch_I2C_ReadByte(uint8_t ack)
{
    uint8_t byte = 0;
    I2C_SDA(1);
    I2C_Delay();
    for(uint8_t i = 0; i < 8; i++)
    {
        byte <<= 1;
        I2C_SCL(1);
        I2C_Delay();
        if(I2C_SDA_READ()) byte |= 0x01;
        I2C_SCL(0);
        I2C_Delay();
    }
    
    I2C_SDA(ack ? 0 : 1);  // Send ACK (0) or NACK (1)
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SCL(0);
    I2C_Delay();
    I2C_SDA(1);             // Release bus
    return byte;
}

// GT911 reset sequence to set I2C address
// RST=0, INT=0 -> RST=1, INT releases (soft address select: 0x5D)
// Wait 50ms for firmware initialization
void GT911_Reset_Sequence(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure INT pin as output for reset sequence
    GPIO_InitStruct.Pin = TOUCH_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);

    // Pull RST and INT low to select I2C address based on INT state
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOUCH_INT_PORT, TOUCH_INT_PIN, GPIO_PIN_RESET); 
    HAL_Delay(10);
    
    // Release RST to lock I2C address (INT was low, so address = 0x5D)
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    
    // Restore INT as input (floating, driven by GT911)
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);
    
    // Allow GT911 firmware time to initialize (50ms)
    HAL_Delay(50); 
}
