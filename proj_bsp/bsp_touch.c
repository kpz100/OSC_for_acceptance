#include "bsp_touch.h"
#include "bsp_dwt.h"

GT911_Data_t touch_data;

// 模拟 I2C 延时，根据例程约为 ?
static void I2C_Delay(void)
{
    BSP_DWT_Delay_us(2); // 稍微增加延时以保证稳定性
}

/**
 * @brief 初始化触摸屏控制引脚
 */
void Touch_I2C_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // SCL, SDA 配置为开漏输出
    GPIO_InitStruct.Pin = TOUCH_SCL_PIN | TOUCH_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    // RST 配置为推挽输出
    GPIO_InitStruct.Pin = TOUCH_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(TOUCH_RST_PORT, &GPIO_InitStruct);

    I2C_SCL(1);
    I2C_SDA(1);
}

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

void Touch_I2C_Stop(void)
{
    I2C_SDA(0);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SDA(1);
    I2C_Delay();
}

/**
 * @brief 等待应答信号
 */
static uint8_t Touch_I2C_WaitACK(void)
{
    uint8_t timeout = 0;
    I2C_SDA(1);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    while (I2C_SDA_READ())
    {
        timeout++;
        if (timeout > 200)
        {
            Touch_I2C_Stop();
            return 1; // 错误
        }
    }
    I2C_SCL(0);
    I2C_Delay();
    return 0; // 成功
}

uint8_t Touch_I2C_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
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

uint8_t Touch_I2C_ReadByte(uint8_t ack)
{
    uint8_t byte = 0;
    I2C_SDA(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        byte <<= 1;
        I2C_SCL(1);
        I2C_Delay();
        if (I2C_SDA_READ()) byte |= 0x01;
        I2C_SCL(0);
        I2C_Delay();
    }
    I2C_SDA(ack ? 0 : 1);
    I2C_Delay();
    I2C_SCL(1);
    I2C_Delay();
    I2C_SCL(0);
    I2C_SDA(1);
    return byte;
}

/**
 * @brief GT911 复位序列并设置 I2C 地址为 0x5D (0xBA)
 */
void GT911_Reset_Sequence(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 设置 INT 为输出以进行地址选择
    GPIO_InitStruct.Pin = TOUCH_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TOUCH_INT_PORT, TOUCH_INT_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    // 恢复 INT 为输入模式
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(TOUCH_INT_PORT, &GPIO_InitStruct);
    HAL_Delay(50);
}

void GT911_WR_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    Touch_I2C_Start();
    Touch_I2C_WriteByte(GT911_ADDR);
    Touch_I2C_WriteByte((uint8_t)(reg >> 8));
    Touch_I2C_WriteByte((uint8_t)(reg & 0xFF));
    for (uint8_t i = 0; i < len; i++)
    {
        Touch_I2C_WriteByte(buf[i]);
    }
    Touch_I2C_Stop();
}

void GT911_RD_Reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    Touch_I2C_Start();
    Touch_I2C_WriteByte(GT911_ADDR);
    Touch_I2C_WriteByte((uint8_t)(reg >> 8));
    Touch_I2C_WriteByte((uint8_t)(reg & 0xFF));
    Touch_I2C_Stop(); // 某些 GT911 版本需要停止再开始

    Touch_I2C_Start();
    Touch_I2C_WriteByte(GT911_ADDR | 0x01);
    for (uint8_t i = 0; i < len; i++)
    {
        buf[i] = Touch_I2C_ReadByte(i == (len - 1) ? 0 : 1);
    }
    Touch_I2C_Stop();
}

/**
 * @brief 扫描触摸屏并将结果存入 touch_data
 */
uint8_t GT911_Scan(void)
{
    uint8_t status;
    uint8_t buf[40];
    uint8_t res = 0;

    GT911_RD_Reg(GT_GSTID_REG, &status, 1);

    // 检查 Buffer Status 位 (Bit 7)
    if ((status & 0x80) == 0) return 0;

    uint8_t touch_cnt = status & 0x0F;
    if (touch_cnt > 0 && touch_cnt <= 5)
    {
        GT911_RD_Reg(GT_TP1_REG, buf, touch_cnt * 8);
        
        touch_data.touch_num = touch_cnt;
        for (uint8_t i = 0; i < touch_cnt; i++)
        {
            // 解析坐标 (小端模式)
            touch_data.x[i] = ((uint16_t)buf[i * 8 + 2] << 8) | buf[i * 8 + 1];
            touch_data.y[i] = ((uint16_t)buf[i * 8 + 4] << 8) | buf[i * 8 + 3];
            touch_data.size[i] = ((uint16_t)buf[i * 8 + 6] << 8) | buf[i * 8 + 5];
        }
        res = 1;
    }
    else
    {
        touch_data.touch_num = 0;
    }

    // 必须清零状态寄存器，否则不会触发下次数据更新
    uint8_t clear = 0;
    GT911_WR_Reg(GT_GSTID_REG, &clear, 1);

    return res;
}