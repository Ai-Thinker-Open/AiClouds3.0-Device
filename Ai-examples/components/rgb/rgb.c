#include "rgb.h"

static void rgbDelay(unsigned int us)
{
    /* Define your delay function */
    os_delay_us(us);
}

/************ generation clock *********************/
static void clkProduce(void)
{
    SCL_LOW; // SCL=0
    rgbDelay(40);
    SCL_HIGH; // SCL=1
    rgbDelay(40);
}

/**********  send 32 zero ********************/
static void send_32zero(void)
{
    uint8_t i;

    SDA_LOW; // SDA=0
    for (i = 0; i < 32; i++)
    {
        clkProduce();
    }
}

/********* invert the grey value of the first two bits ***************/
static uint8_t takeAntiCode(uint8_t dat)
{
    uint8_t tmp = 0;

    tmp = ((~dat) & 0xC0) >> 6;
    return tmp;
}

/****** send gray data *********/
static void dataSend(uint32_t dx)
{
    uint8_t i;

    for (i = 0; i < 32; i++)
    {
        if ((dx & 0x80000000) != 0)
        {

            SDA_HIGH; //  SDA=1;
        }
        else
        {
            SDA_LOW; //  SDA=0;
        }

        dx <<= 1;
        clkProduce();
    }
}

/******* data processing  ********************/
static void dataDealwithSend(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t dx = 0;

    dx |= (uint32_t)0x03 << 30; // The front of the two bits 1 is flag bits
    dx |= (uint32_t)takeAntiCode(b) << 28;
    dx |= (uint32_t)takeAntiCode(g) << 26;
    dx |= (uint32_t)takeAntiCode(r) << 24;

    dx |= (uint32_t)b << 16;
    dx |= (uint32_t)g << 8;
    dx |= r;

    dataSend(dx);
}

void rgbControl(uint8_t R, uint8_t G, uint8_t B)
{
    //contron power

    send_32zero();
    dataDealwithSend(R, G, B); // display red
    dataDealwithSend(R, G, B); // display red
}

void rgbLedInit(void)
{
    //contron power

    send_32zero();
    dataDealwithSend(0, 0, 0); // display red
    dataDealwithSend(0, 0, 0);
}

void rgbGpioInit(void)
{
    /* Migrate your driver code */

    // SCL/SDA
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = ((1ULL << GPIO_RGB_SCL) | (1ULL << GPIO_RGB_SDA));
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    SCL_LOW;
    SDA_LOW;

    printf("rgbGpioInit \r\n");
}

void rgbSensorTest(uint8_t rgbcou)
{
    /* Test LOG model */

    if (0 == rgbcou)
    {
        rgbControl(0, 0, 250);
        //      GAgent_Printf(GAGENT_CRITICAL, "RGB : B");
    }
    else if (1 == rgbcou)
    {
        rgbControl(0, 250, 0);
        //      GAgent_Printf(GAGENT_CRITICAL, "RGB : G");
    }
    else if (2 == rgbcou)
    {
        rgbControl(250, 0, 0);
        //      GAgent_Printf(GAGENT_CRITICAL, "RGB : R");
    }
}