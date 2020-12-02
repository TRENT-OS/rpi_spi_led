/* Copyright (C) 2020, HENSOLDT Cyber GmbH */
/**
 * @file
 * @brief   SPI LED driver.
 */
#include "OS_Error.h"
#include "LibDebug/Debug.h"
#include "TimeServer.h"
#include "bcm2837_spi.h"
#include "max7219.h"
#include "font.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <camkes.h>

/**
 * @brief organizational data for spi led driver.
 */
static struct
{
    bool           init_ok;
    spiled_t       spi_led_ctx;
} ctx =
{
    .init_ok       = false,
};

static const if_OS_Timer_t timer =
    IF_OS_TIMER_ASSIGN(
        timeServer_rpc,
        timeServer_notify);

static
__attribute__((__nonnull__))
int
impl_spiled_spi_writenb(
    spiled_t* spi,
    const uint8_t* tx_data,
    uint32_t tx_len
)
{
    bcm2837_spi_writenb(
        (void*)tx_data,
        tx_len);

    return SPILED_OK;
}


static
__attribute__((__nonnull__))
void
impl_spiled_spi_cs(
    spiled_t* spi,
    uint8_t cs
)
{
    bcm2837_spi_chipSelect(cs ? BCM2837_SPI_CS0 : BCM2837_SPI_CS2);
    return;
}


static
__attribute__((__nonnull__))
void
impl_spiled_wait(
    spiled_t* spi,
    uint32_t us
)
{
    // TimeServer.h provides this helper function, it contains the hard-coded
    // assumption that the RPC interface is "timeServer_rpc"
    TimeServer_sleep(&timer, TimeServer_PRECISION_USEC, us);
}

void post_init(void)
{
    Debug_LOG_INFO("BCM2837_SPI_LED init");

    // initialize BCM2837 SPI library
    if (!bcm2837_spi_begin(regBase))
    {
        Debug_LOG_ERROR("bcm2837_spi_begin() failed");
        return;
    }

    bcm2837_spi_setBitOrder(BCM2837_SPI_BIT_ORDER_MSBFIRST);
    bcm2837_spi_setDataMode(BCM2837_SPI_MODE0);
    // divider 8 gives 50 MHz assuming the RasPi3 is running with the default
    // 400 MHz, but for some reason we force it to run at just 250 MHz with
    // "core_freq=250" in config.txt and thus end up at 31.25 MHz SPI speed.
    bcm2837_spi_setClockDivider(BCM2837_SPI_CLOCK_DIVIDER_128);
    bcm2837_spi_chipSelect(BCM2837_SPI_CS0);
    bcm2837_spi_setChipSelectPolarity(BCM2837_SPI_CS0, 0);

    // initialize MAX7219 SPI library
    static const spiled_config_t spiled_config =
    {
        .num_devices = 4,
        .decode_mode = 0,
        .intensity = MAX7219_INTENSITY_8,
        .scan_limit = MAX7219_SCAN_8 
    };

    static const spiled_hal_t hal =
    {
        ._spiled_spi_writenb  = impl_spiled_spi_writenb,
        ._spiled_spi_cs    = impl_spiled_spi_cs,
        ._spiled_wait      = impl_spiled_wait,
    };

    Max7219_Init(&ctx.spi_led_ctx, &spiled_config, &hal);

    if ( (NULL == ctx.spi_led_ctx.cfg) || (NULL == ctx.spi_led_ctx.hal) )
    {
        Debug_LOG_ERROR("Max7219_Init() failed");
        return;
    }

    ctx.init_ok = true;

    Debug_LOG_INFO("BCM2837_SPI_LED done");
}


OS_Error_t 
__attribute__((__nonnull__))
led_rpc_display_char_on_device(
    unsigned char character,
    uint8_t device
) 
{
    Debug_LOG_DEBUG("SPI display_char_on_device()");

    if (!ctx.init_ok)
    {
        Debug_LOG_ERROR("initialization failed, fail call %s()", __func__);
        return OS_ERROR_INVALID_STATE;
    }
    
    int ret = 0;
    const uint8_t * bytes = font8x8[(uint8_t)character];
    for (uint8_t j = 0; j < MAX7219_DIGIT_7; j++)
    {
        ret = write_digit(&(ctx.spi_led_ctx),device,(j+1),bytes[j]);
        if(ret != MAX7219_OK){
            Debug_LOG_ERROR("SPILED_display_char_on_device() failed with %d!",ret);
            return OS_ERROR_INVALID_STATE;
        }
    }
    //ctx.spi_led_ctx.hal->_spiled_wait(&(ctx.spi_led_ctx),1000000);

    return OS_SUCCESS;
}


OS_Error_t 
__attribute__((__nonnull__))
led_rpc_display_char(
    unsigned char character
)
{
    Debug_LOG_DEBUG("SPI display_char()");

    if (!ctx.init_ok)
    {
        Debug_LOG_ERROR("initialization failed, fail call %s()", __func__);
        return OS_ERROR_INVALID_STATE;
    }

    int ret = 0;
    const uint8_t * bytes = font8x8[(uint8_t)character];
    for (size_t device = 0; device < ctx.spi_led_ctx.cfg->num_devices; device++)
    {
        for (uint8_t j = 0; j < MAX7219_DIGIT_7; j++)
        {
            ret = write_digit(&(ctx.spi_led_ctx),(device + 1),(j+1),bytes[j]);
            if(ret != MAX7219_OK){
                Debug_LOG_ERROR("SPILED_display_char() failed with %d!",ret);
                return OS_ERROR_INVALID_STATE;
            }
        }
    }
    //ctx.spi_led_ctx.hal->_spiled_wait(&(ctx.spi_led_ctx),1000000);

    return OS_SUCCESS;
}


OS_Error_t
__attribute__((__nonnull__))
led_rpc_clear_display(void)
{
    Debug_LOG_DEBUG("SPI clear_display()");

    if (!ctx.init_ok)
    {
        Debug_LOG_ERROR("initialization failed, fail call %s()", __func__);
        return OS_ERROR_INVALID_STATE;
    }

    display_all_off(&(ctx.spi_led_ctx));
    //ctx.spi_led_ctx.hal->_spiled_wait(&(ctx.spi_led_ctx),1000000);

    return OS_SUCCESS;
}

OS_Error_t
__attribute__((__nonnull__))
led_rpc_scroll_text(
    const char * text
)
{
    Debug_LOG_DEBUG("SPI scroll_text()");

    if (!ctx.init_ok)
    {
        Debug_LOG_ERROR("initialization failed, fail call %s()", __func__);
        return OS_ERROR_INVALID_STATE;
    }

    int ret = 0;

    /*
     * buf: Data structure that stores the current bit pattern of the different LEDs on the 8x8 LED matrix.
     *      Data is shifted for each digit to the left.
     * 
     * Structure:
     *      digit0 - device4 | digit0 - device3 | digit0 - device2 | digit0 - device1 
     *      digit1 - device4 | ...
     */
    uint8_t num_devices = ctx.spi_led_ctx.cfg->num_devices; 
    uint8_t buf[num_devices * (MAX7219_DIGIT_7 + 1)];
    memset(buf,0,num_devices * (MAX7219_DIGIT_7 + 1) * sizeof(uint8_t));

    for (size_t letter = 0; letter < strlen(text); letter++) //go through all the letters
    {
        const uint8_t * bytes = font8x8[(uint8_t)text[letter]];
        for (int8_t shift = 7; shift >= 0; shift--) //shift each letter into the buf
        {
            for (size_t digit = 0; digit < MAX7219_DIGIT_7; digit++) //make this for every digit
            {
                for (size_t dev_loc = 0; dev_loc < (num_devices - 1); dev_loc++) //and for every device
                {
                    buf[(digit * num_devices) + dev_loc] = (buf[(digit * num_devices) + dev_loc] << 1) //shift data one-bit to the left...
                                                          | (((buf[(digit * num_devices) + dev_loc + 1]) & (1 << 7)) > 0); //... and fill with msb bit of previous device
                }
                buf[(digit * num_devices) + num_devices - 1] = (buf[(digit * num_devices) + num_devices - 1] << 1) //shift data one-bit to the left ...
                                                               | (((bytes[digit]) & (1 << shift)) > 0); //... and fill with according bit of current character
                //print current content of buf
                for (size_t device = 1; device <= num_devices; device++)
                {
                    ret = write_digit(&(ctx.spi_led_ctx),device,(digit + 1),buf[(digit * num_devices) + (num_devices - device)]);
                    if(ret != MAX7219_OK){
                        Debug_LOG_ERROR("SPILED_scroll_text() failed with %d!",ret);
                        return OS_ERROR_INVALID_STATE;
                    }
                }
            }
            ctx.spi_led_ctx.hal->_spiled_wait(&(ctx.spi_led_ctx),100000);
        }
    }

    //push all data out of display
    for (size_t i = 0; i < num_devices; i++)
    {
        for (int8_t shift = 7; shift >= 0; shift--) //shift each letter into the buf
        {
            for (size_t digit = 0; digit < MAX7219_DIGIT_7; digit++) //make this for every digit
            {
                for (size_t dev_loc = 0; dev_loc < (num_devices - 1); dev_loc++) //and for every device
                {
                    buf[(digit * num_devices) + dev_loc] = (buf[(digit * num_devices) + dev_loc] << 1)
                                                          | (((buf[(digit * num_devices) + dev_loc + 1]) & (1 << 7)) > 0);
                }
                buf[(digit * num_devices) + num_devices - 1] = (buf[(digit * num_devices) + num_devices - 1] << 1); //let all the data just be pushed out
                //print current content of buf
                for (size_t device = 1; device <= num_devices; device++)
                {
                    ret = write_digit(&(ctx.spi_led_ctx),device,(digit + 1),buf[(digit * num_devices) + (num_devices - device)]);
                    if(ret != MAX7219_OK){
                        Debug_LOG_ERROR("SPILED_scroll_text() failed with %d!",ret);
                        return OS_ERROR_INVALID_STATE;
                    }
                }
            }
            ctx.spi_led_ctx.hal->_spiled_wait(&(ctx.spi_led_ctx),100000);
        }
    }
    
    return OS_SUCCESS;
}
