#include "bsp_si5351.h"
#include "i2c.h"

extern I2C_HandleTypeDef  hi2c2;
I2C_HandleTypeDef * SI5351_I2C_HANDLE = &hi2c2;

typedef struct { 
    int32_t mult, num, denom;
} SI5351_PLL_CFG;

typedef struct {
    int32_t div, num, denom; 
    uint8_t r_div;
} SI5351_MS_CFG;

static void BSP_SI5351_Write(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(SI5351_I2C_HANDLE, SI5351_ADDR, reg, 1, &val, 1, 100);
}

static void BSP_SI5351_WriteBulk(uint8_t addr, int32_t P1, int32_t P2, int32_t P3) {
    uint8_t buf[8];
    buf[0] = (P3 >> 8) & 0xFF;
    buf[1] = P3 & 0xFF;
    buf[2] = (P1 >> 16) & 0x03;
    buf[3] = (P1 >> 8) & 0xFF;
    buf[4] = P1 & 0xFF;
    buf[5] = ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0x0F);
    buf[6] = (P2 >> 8) & 0xFF;
    buf[7] = P2 & 0xFF;
    HAL_I2C_Mem_Write(SI5351_I2C_HANDLE, SI5351_ADDR, addr, 1, buf, 8, 100);
}

static void BSP_SI5351_Calc(int32_t freq, SI5351_PLL_CFG* p, SI5351_MS_CFG* m) {
    const int32_t xtal = 25000000;
    uint8_t r_div_val = 0;
    int32_t f_search = freq;

    while (f_search < 500000 && r_div_val < 7) {
        f_search *= 2;
        r_div_val++;
    }
    m->r_div = r_div_val;

    m->div = (f_search < 8000000) ? (600000000 / f_search) : (900000000 / f_search);
    if (m->div > 2048) m->div = 2048;
    
    m->num = 0;
    m->denom = 1;

    int64_t fpll = (int64_t)f_search * m->div;
    p->mult = fpll / xtal;
    p->num = (fpll % xtal) / (xtal / 1000000);
    p->denom = 1000000;
}


static void BSP_SI5351_SetPLL(uint8_t addr, SI5351_PLL_CFG* c) {
    int32_t p1 = 128 * c->mult + (128 * c->num / c->denom) - 512;
    int32_t p2 = 128 * c->num - c->denom * (128 * c->num / c->denom);
    BSP_SI5351_WriteBulk(addr, p1, p2, c->denom);
    BSP_SI5351_Write(177, (1 << 7) | (1 << 5));
}

static void BSP_SI5351_SetMS(uint8_t addr, SI5351_MS_CFG* c, uint8_t pll_src, SI5351_Drive_MA drive_mA, uint8_t clk_reg) {
    int32_t p1 = 128 * c->div - 512;
    int32_t p2 = 0;
    int32_t p3 = c->denom;


    uint8_t buf[8];
    buf[0] = (p3 >> 8) & 0xFF;
    buf[1] = p3 & 0xFF;

    buf[2] = ((p1 >> 16) & 0x03) | (c->r_div << 4); 
    buf[3] = (p1 >> 8) & 0xFF;
    buf[4] = p1 & 0xFF;
    buf[5] = ((p3 >> 12) & 0xF0) | ((p2 >> 16) & 0x0F);
    buf[6] = (p2 >> 8) & 0xFF;
    buf[7] = p2 & 0xFF;
    
    HAL_I2C_Mem_Write(SI5351_I2C_HANDLE, SI5351_ADDR, addr, 1, buf, 8, 100);

    BSP_SI5351_Write(clk_reg, (pll_src << 5) | 0x0C | drive_mA); 
}

void BSP_SI5351_Init(void) {
    BSP_SI5351_Write(183, (2 << 6)); // Set load capacitance to 8pF
    BSP_SI5351_EnableOutputs(0); // Disable all outputs by default
}

void BSP_SI5351_SetupCLK0(int32_t freq, SI5351_Drive_MA drive_mA) {
    SI5351_PLL_CFG p_cfg; SI5351_MS_CFG m_cfg;
    BSP_SI5351_Calc(freq, &p_cfg, &m_cfg);
    BSP_SI5351_SetPLL(26, &p_cfg);                     // Configure PLLA
    BSP_SI5351_SetMS(42, &m_cfg, 0, drive_mA, 16);    // Configure MS0 using PLLA
}

void BSP_SI5351_SetupCLK2(int32_t freq, SI5351_Drive_MA drive_mA) {
    SI5351_PLL_CFG p_cfg; SI5351_MS_CFG m_cfg;
    BSP_SI5351_Calc(freq, &p_cfg, &m_cfg);
    BSP_SI5351_SetPLL(34, &p_cfg);                     // Configure PLLB
    BSP_SI5351_SetMS(58, &m_cfg, 1, drive_mA, 18);    // Configure MS2 using PLLB
}

void BSP_SI5351_EnableOutputs(uint8_t mask) {
    BSP_SI5351_Write(3, ~mask); // Register 3 uses inverted logic
}

void BSP_SI5351_EnableControl(uint8_t clk_id, uint8_t enable) {
    static uint8_t current_mask = 0; // Track current output state
    
    if (enable) {
        current_mask |= (1 << clk_id);  // Set corresponding bit to 1
    } else {
        current_mask &= ~(1 << clk_id); // Clear corresponding bit to 0
    }
    
    BSP_SI5351_Write(3, ~current_mask); // Write register with inverted logic
}

