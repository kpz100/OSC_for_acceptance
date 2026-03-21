#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "main.h"

/* ------------------- GT911 寄存器与地址定义 ------------------- */
#define GT911_ADDR           0xBA  // 7位地址 0x5D << 1
#define GT_CTRL_REG          0x8040
#define GT_CFG_REG           0x8047
#define GT_GSTID_REG         0x814E // 状态寄存器
#define GT_TP1_REG           0x814F // 第一个触摸点起始地址

/* ------------------- 触摸数据结构体 ------------------- */
typedef struct {
    uint8_t  touch_num;       // 当前触摸点数量
    uint16_t x[5];            // X 坐标
    uint16_t y[5];            // Y 坐标
    uint16_t size[5];         // 触摸点大小
} GT911_Data_t;

extern GT911_Data_t touch_data; 

/* ------------------- 引脚配置 (对应例程) ------------------- */
#define TOUCH_SCL_PORT       GPIOI
#define TOUCH_SCL_PIN        GPIO_PIN_11
#define TOUCH_SDA_PORT       GPIOI
#define TOUCH_SDA_PIN        GPIO_PIN_8
#define TOUCH_INT_PORT       GPIOG
#define TOUCH_INT_PIN        GPIO_PIN_3
#define TOUCH_RST_PORT       GPIOH
#define TOUCH_RST_PIN        GPIO_PIN_4

/* ------------------- 底层 IO 操作 ------------------- */
#define I2C_SCL(n)  HAL_GPIO_WritePin(TOUCH_SCL_PORT, TOUCH_SCL_PIN, (n ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA(n)  HAL_GPIO_WritePin(TOUCH_SDA_PORT, TOUCH_SDA_PIN, (n ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define I2C_SDA_READ() (HAL_GPIO_ReadPin(TOUCH_SDA_PORT, TOUCH_SDA_PIN) == GPIO_PIN_SET)

/* ------------------- 函数声明 ------------------- */
void Touch_I2C_GPIO_Config(void);
void Touch_I2C_Start(void);
void Touch_I2C_Stop(void);
uint8_t Touch_I2C_WriteByte(uint8_t byte);
uint8_t Touch_I2C_ReadByte(uint8_t ack);

void GT911_Reset_Sequence(void);
uint8_t GT911_Scan(void);
void GT911_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len);
void GT911_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len);

#endif /* __BSP_TOUCH_H */