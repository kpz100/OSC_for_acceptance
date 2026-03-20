#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "main.h"

// GT911 Touch Panel I2C GPIO Configuration
#define TOUCH_SCL_PORT       GPIOI      // I2C SCL port
#define TOUCH_SCL_PIN        GPIO_PIN_11 // I2C SCL pin
#define TOUCH_SDA_PORT       GPIOI      // I2C SDA port
#define TOUCH_SDA_PIN        GPIO_PIN_8  // I2C SDA pin
#define TOUCH_INT_PORT       GPIOG      // Interrupt/Address select port
#define TOUCH_INT_PIN        GPIO_PIN_3  // Interrupt/Address select pin
#define TOUCH_RST_PORT       GPIOH      // Reset port
#define TOUCH_RST_PIN        GPIO_PIN_4  // Reset pin

// I2C Signal Control Macros (Open-drain, active low when pulled)
#define I2C_SCL(n)  HAL_GPIO_WritePin(TOUCH_SCL_PORT, TOUCH_SCL_PIN, (n ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA(n)  HAL_GPIO_WritePin(TOUCH_SDA_PORT, TOUCH_SDA_PIN, (n ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA_READ() (HAL_GPIO_ReadPin(TOUCH_SDA_PORT, TOUCH_SDA_PIN) == GPIO_PIN_SET)

// I2C ACK/NACK Definitions
#define I2C_ACK     1   // Acknowledge (SDA pulled low)
#define I2C_NACK    0   // Not acknowledge (SDA released high)

// Function Prototypes
// Initialize I2C bus with softwarecontrol (open-drain mode)
void Touch_I2C_Init(void);

// Generate I2C START condition
void Touch_I2C_Start(void);

// Generate I2C STOP condition
void Touch_I2C_Stop(void);

// Transmit 8-bit data on I2C bus (MSB first)
uint8_t Touch_I2C_WriteByte(uint8_t byte);

// Receive 8-bit data from I2C bus (MSB first)
uint8_t Touch_I2C_ReadByte(uint8_t ack);

// GT911 power-on reset sequence to configure I2C address
void GT911_Reset_Sequence(void);

#endif /* __BSP_TOUCH_H */
