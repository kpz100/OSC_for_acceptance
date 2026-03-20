#include "bsp_sdram.h"

extern SDRAM_HandleTypeDef hsdram1; // 声明由CubeMX生成的句柄


static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
    FMC_SDRAM_CommandTypeDef Command;
    uint32_t tmpmrd = 0;

    /* 1. 时钟使能 */
    Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;
    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);
    HAL_Delay(1);

    /* 2. 预充电 */
    Command.CommandMode            = FMC_SDRAM_CMD_PALL;
    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

    /* 3. 自动刷新 */
    Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.AutoRefreshNumber      = 8;
    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

    /* 4. 加载模式寄存器 */
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2          |
                       SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL    |
                       SDRAM_MODEREG_CAS_LATENCY_3            |
                       SDRAM_MODEREG_OPERATING_MODE_STANDARD  |
                       SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;  

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.ModeRegisterDefinition = tmpmrd;
    HAL_SDRAM_SendCommand(hsdram, &Command, SDRAM_TIMEOUT);

    /* 5. 设置刷新计数器 (120MHz时钟对应918) */
    HAL_SDRAM_ProgramRefreshRate(hsdram, 918);
}

/**
  * @brief  BSP层SDRAM初始化接口
  */
void BSP_SDRAM_Init(void)
{
    SDRAM_Initialization_Sequence(&hsdram1);
}

/**
  * @brief  SDRAM 完整性自检测试
  * @return 0: 成功
  * 其他: 发生错误的物理地址
  */
uint32_t BSP_SDRAM_SelfTest(void)
{
    uint32_t i;
    // 【优化】加上 volatile 防止编译器过度优化循环读写操作
    volatile uint32_t *pOut = (volatile uint32_t *)SDRAM_BANK_ADDR;
    const uint32_t test_pattern = 0x55AA55AA;

    /* 1. 遍历写入测试数据 */
    for (i = 0; i < SDRAM_SIZE / 4; i++) {
        pOut[i] = i ^ test_pattern;
    }

    /* 2. 遍历读回并校验 */
    for (i = 0; i < SDRAM_SIZE / 4; i++) {
        if (pOut[i] != (i ^ test_pattern)) {
            return (uint32_t)&pOut[i]; // 返回具体的错误地址以便定位硬件虚焊
        }
    }
    
    return 0; // 测试通过
}