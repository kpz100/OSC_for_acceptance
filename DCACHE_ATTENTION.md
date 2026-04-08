# DCACHE 注意事项

每次修改dcache和对应代码段时，要在此进行对应注释。

## 链接脚本配置

```c
LR_IROM1 0x08000000 0x00200000  {    ; load region size_region
  ER_IROM1 0x08000000 0x00200000  {  ; load address = execution address
   *.o (RESET, +First)
   *(InRoot$$Sections)
   .ANY (+RO)
   .ANY (+XO)
  }
  RW_IRAM1 0x20000000 0x00020000  {  ; RW data
   .ANY (+RW +ZI)
  }
  RW_IRAM2_PRE 0x24000000 0x00004000  {
   .ANY (+RW +ZI)
  }
  RW_IRAM2_POST 0x24010000 0x00078000  {
   .ANY (+RW +ZI)
  }
  
  
  RW_MPU_REGION_1 0x24004000 UNINIT 0x00010000  {   ; 64kb
    *(.bss.MPU_REGION_1)
  }
  RW_MPU_REGION_2 0x30000000 UNINIT 0x00004000  {   ; 16kb
    *(.bss.MPU_REGION_2)  
  }
  RW_LCD_FRAME_BUFFER 0xC0000000 UNINIT 0x00177000  {  ; 32mb
    *(.bss.LCD_FRAME_BUFFER)
  }
  RW_LCD_OTHER_BUFFER 0xC0177000 UNINIT 0x01E89000  {
    *(.bss.LCD_UI_RAMPOOL)
  }
}
```

## 对应的变量

### adc_control

```c
ADC_CHANNEL_NUM 2u
ADC_LENGTH 4096u
MAG_LENGTH 2048u

static uint8_t adc_buffer[ADC_CHANNEL_NUM][ADC_LENGTH]
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

static uint8_t show_buffer[ADC_CHANNEL_NUM][SHOW_LENGTH]
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);

static float fft_buffer[ADC_CHANNEL_NUM][FFT_LENGTH]
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);

static float mag_buffer[ADC_CHANNEL_NUM][MAG_LENGTH]
    __attribute__((section(".bss.MPU_REGION_1"))) __ALIGNED(32);
```

### dac_control

```c
static DAC_Struct ch_dac_config[DAC_CHANNEL_NUM]
    __attribute__((section(".bss.MPU_REGION_2"))) __ALIGNED(32);
```

### bsp_lcd_single

```c
static uint32_t lcd_frame_buffer[LCD_WIDTH * LCD_HEIGHT] 
    __attribute__((used, section(".bss.LCD_FRAME_BUFFER"))) __ALIGNED(32);
```

### ui_rampool

```c
LCD_UI_Pool_Struct g_ui_pool
    __attribute__((section(".bss.LCD_UI_RAMPOOL"))) __ALIGNED(32);
``` 