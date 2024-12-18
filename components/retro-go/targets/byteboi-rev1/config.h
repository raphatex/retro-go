// Target definition
#define RG_TARGET_NAME             "BYTEBOI-REV1"

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDSPI_HOST       SPI2_HOST
#define RG_STORAGE_SDSPI_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
// #define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_DEFAULT
// #define RG_STORAGE_FLASH_PARTITION  "vfs"

// Audio
#define RG_AUDIO_USE_INT_DAC        1   // 0 = Disable, 1 = GPIO25, 2 = GPIO26, 3 = Both
#define RG_AUDIO_USE_EXT_DAC        0   // 0 = Disable, 1 = Enable

// Video
#define RG_SCREEN_DRIVER            0   // 0 = ILI9341
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         0
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_MARGIN_TOP        0
#define RG_SCREEN_MARGIN_BOTTOM     0
#define RG_SCREEN_MARGIN_LEFT       0
#define RG_SCREEN_MARGIN_RIGHT      0
#define RG_SCREEN_INIT()                                                                                         \
    ILI9341_CMD(0xEF, 0x03, 0x80, 0x02);                                                                         \
    ILI9341_CMD(0xCF, 0x00, 0xc1, 0x30);                                                                         \
    ILI9341_CMD(0xED, 0x64, 0x03, 0x12, 0x81);                                                                   \
    ILI9341_CMD(0xE8, 0x85, 0x00, 0x78);                                                                         \
    ILI9341_CMD(0xCB, 0x39, 0x2c, 0x00, 0x34, 0x02);                                                             \
    ILI9341_CMD(0xF7, 0x20);                                                                                     \
    ILI9341_CMD(0xEA, 0x00, 0x00);                                                                               \
    ILI9341_CMD(0xC0, 0x23);                 /* Power control   //VRH[5:0] */                                    \
    ILI9341_CMD(0xC1, 0x10);                 /* Power control   //SAP[2:0];BT[3:0] */                            \
    ILI9341_CMD(0xC5, 0x3e,0x28);           /* VCM control */                                                    \
    ILI9341_CMD(0xC7, 0x86);                 /* VCM control2 */                                                  \
    ILI9341_CMD(0x36, (0x20 | 0x80 | 0x08)); /* Memory Access Control */                                         \
    ILI9341_CMD(0xB1, 0x00, 0x10);           /* Frame Rate Control (1B=70, 1F=61, 10=119) */                     \
    ILI9341_CMD(0xB6, 0x08, 0xC2, 0x27);           /* Display Function Control */                                \
    ILI9341_CMD(0xF6, 0x01, 0x30);                                                                               \
    ILI9341_CMD(0xF2, 0x00); /* 3Gamma Function Disable */                                                       \
    ILI9341_CMD(0x26, 0x01); /* Gamma curve selected */                                                          \
    ILI9341_CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); \
    ILI9341_CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);



// Input
// Refer to rg_input.h to see all available RG_KEY_* and RG_GAMEPAD_*_MAP types
#define RG_GAMEPAD_I2C_MAP {\
    {RG_KEY_UP,     (1<<0)},\
    {RG_KEY_RIGHT,  (1<<2)},\
    {RG_KEY_DOWN,   (1<<3)},\
    {RG_KEY_LEFT,   (1<<1)},\
    {RG_KEY_SELECT, (1<<4)},\
    {RG_KEY_A,      (1<<6)},\
    {RG_KEY_B,      (1<<5)},\
}
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_START,  RG_KEY_A | RG_KEY_SELECT},\
    {RG_KEY_MENU,   RG_KEY_B | RG_KEY_SELECT},\
    {RG_KEY_OPTION, RG_KEY_UP| RG_KEY_SELECT},\
}

// Battery
#define RG_BATTERY_DRIVER           1
#define RG_BATTERY_ADC_UNIT         ADC_UNIT_1
#define RG_BATTERY_ADC_CHANNEL      ADC_CHANNEL_0
#define RG_BATTERY_CALC_PERCENT(raw) (((raw) * 2.f - 3500.f) / (4200.f - 3500.f) * 100.f)
#define RG_BATTERY_CALC_VOLTAGE(raw) ((raw) * 2.f * 0.001f)

// Status LED
//#define RG_GPIO_LED                 GPIO_NUM_14

// I2C BUS
#define RG_I2C_DRIVER               1   // 0 = AW9523, 1 = PCF9539
#define RG_GPIO_I2C_SDA             GPIO_NUM_23
#define RG_GPIO_I2C_SCL             GPIO_NUM_22

// SPI Display
#define RG_GPIO_LCD_MISO            GPIO_NUM_5
#define RG_GPIO_LCD_MOSI            GPIO_NUM_32
#define RG_GPIO_LCD_CLK             GPIO_NUM_26
#define RG_GPIO_LCD_CS              GPIO_NUM_33
#define RG_GPIO_LCD_DC              GPIO_NUM_21
#define RG_GPIO_LCD_BCKL            GPIO_NUM_12
#define RG_GPIO_LCD_RST             GPIO_NUM_27

// SPI SD Card
#define RG_GPIO_SDSPI_MISO          GPIO_NUM_5
#define RG_GPIO_SDSPI_MOSI          GPIO_NUM_32
#define RG_GPIO_SDSPI_CLK           GPIO_NUM_26
#define RG_GPIO_SDSPI_CS            GPIO_NUM_2

// External I2S DAC
// #define RG_GPIO_SND_I2S_BCK         GPIO_NUM_NC
// #define RG_GPIO_SND_I2S_WS          GPIO_NUM_NC
// #define RG_GPIO_SND_I2S_DATA        GPIO_NUM_NC
#define RG_GPIO_SND_AMP_ENABLE      GPIO_NUM_13