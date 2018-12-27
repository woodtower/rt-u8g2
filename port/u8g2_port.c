#include "u8g2_port.h"
#include <string.h>

#define MAX_RETRY 3

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;	

static struct rt_spi_device spi_dev_ssd1306;
struct stm32_hw_spi_cs
{
    rt_uint32_t pin;
};
static struct stm32_hw_spi_cs spi_cs; 

static int rt_hw_ssd1351_config(uint8_t spi_mode, uint32_t max_hz)
{
	rt_err_t res;
	
	
	// Attach Device
	spi_cs.pin = 14;
	rt_pin_mode(spi_cs.pin, PIN_MODE_OUTPUT);
	res = rt_spi_bus_attach_device(&spi_dev_ssd1306, SPI_SSD1306_DEVICE_NAME, SPI_BUS_NAME, (void*)&spi_cs);
	if (res != RT_EOK)
	{
		return res;
	}
	
	// Set device SPI Mode
	struct rt_spi_configuration cfg;
	cfg.data_width = 8;
	switch(spi_mode)
  {
		case 0: cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB; break;
		case 1: cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_1 | RT_SPI_MSB; break;
		case 2: cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_2 | RT_SPI_MSB; break;
		case 3: cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_3 | RT_SPI_MSB; break;
  }
	cfg.max_hz = max_hz; /* 20M,SPI max 42MHz,ssd1351 4-wire spi */
	
	rt_spi_configure(&spi_dev_ssd1306, &cfg);
	
	return RT_EOK;
}

uint8_t u8x8_rt_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	switch(msg)
	{
		case U8X8_MSG_DELAY_NANO:			// delay arg_int * 1 nano second
			__asm__ volatile("nop");
			break;
		
		case U8X8_MSG_DELAY_100NANO:		// delay arg_int * 100 nano seconds
			__asm__ volatile("nop");
			break;
		
		case U8X8_MSG_DELAY_10MICRO:		// delay arg_int * 10 micro seconds
			for (uint16_t n = 0; n < 320; n++)
			{
				__asm__ volatile("nop");
			}
		break;
		
		case U8X8_MSG_DELAY_MILLI:			// delay arg_int * 1 milli second
			rt_thread_delay(arg_int);
			break;
		
		case U8X8_MSG_GPIO_AND_DELAY_INIT:  
			//Function which implements a delay, arg_int contains the amount of ms  
			//set mode 
			rt_pin_mode(u8x8->pins[U8X8_PIN_SPI_CLOCK],PIN_MODE_OUTPUT);//d0 a5 15 d1 a7 17 res b0 18 dc b1 19 cs a4 14  
			rt_pin_mode(u8x8->pins[U8X8_PIN_SPI_DATA],PIN_MODE_OUTPUT);
			rt_pin_mode(u8x8->pins[U8X8_PIN_RESET],PIN_MODE_OUTPUT);
			rt_pin_mode(u8x8->pins[U8X8_PIN_DC],PIN_MODE_OUTPUT);
			rt_pin_mode(u8x8->pins[U8X8_PIN_CS],PIN_MODE_OUTPUT);
		
			rt_pin_mode(u8x8->pins[U8X8_PIN_I2C_DATA],PIN_MODE_OUTPUT);
			rt_pin_mode(u8x8->pins[U8X8_PIN_I2C_CLOCK],PIN_MODE_OUTPUT);
		
			//set value
			rt_pin_write(u8x8->pins[U8X8_PIN_SPI_CLOCK],1);
			rt_pin_write(u8x8->pins[U8X8_PIN_SPI_DATA],1);
			rt_pin_write(u8x8->pins[U8X8_PIN_RESET],1);
			rt_pin_write(u8x8->pins[U8X8_PIN_DC],1);
			rt_pin_write(u8x8->pins[U8X8_PIN_CS],1);
			break;
		
		case U8X8_MSG_DELAY_I2C:				// arg_int is the I2C speed in 100KHz, e.g. 4 = 400 KHz
			for (uint16_t n = 0; n < (arg_int<=2?160:40); n++)
			{
				__asm__ volatile("nop");
			}
      break;							// arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
		
    case U8X8_MSG_GPIO_I2C_CLOCK:		// arg_int=0: Output low at I2C clock pin
			if (arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_I2C_CLOCK],1);  
			else rt_pin_write(u8x8->pins[U8X8_PIN_I2C_CLOCK],0);  
      break;							// arg_int=1: Input dir with pullup high for I2C clock pin
		
    case U8X8_MSG_GPIO_I2C_DATA:			// arg_int=0: Output low at I2C data pin
			if (arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_I2C_DATA],1);  
			else rt_pin_write(u8x8->pins[U8X8_PIN_I2C_DATA],0);  			
      break;							// arg_int=1: Input dir with pullup high for I2C data pin
		
		case U8X8_MSG_GPIO_SPI_CLOCK:  
			//Function to define the logic level of the clockline  
			if (arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_SPI_CLOCK],1);  
			else rt_pin_write(u8x8->pins[U8X8_PIN_SPI_CLOCK],0);
			break;
							
		case U8X8_MSG_GPIO_SPI_DATA:
			//Function to define the logic level of the data line to the display  
			if (arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_SPI_DATA],1);  
			else rt_pin_write(u8x8->pins[U8X8_PIN_SPI_DATA],0);  
			break;

		case U8X8_MSG_GPIO_CS:
			// Function to define the logic level of the CS line  
			if(arg_int)		rt_pin_write(u8x8->pins[U8X8_PIN_CS],1);
			else		rt_pin_write(u8x8->pins[U8X8_PIN_CS],0);
			break;

		case U8X8_MSG_GPIO_DC:
			//Function to define the logic level of the Data/ Command line  
			if(arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_DC],1);
			else rt_pin_write(u8x8->pins[U8X8_PIN_DC],0);
			break;  

		case U8X8_MSG_GPIO_RESET:
			//Function to define the logic level of the RESET line
			if (arg_int) rt_pin_write(u8x8->pins[U8X8_PIN_RESET],1);
			else rt_pin_write(u8x8->pins[U8X8_PIN_RESET],0);
			break;

		default:  
			//A message was received which is not implemented, return 0 to indicate an error 
			return 0; 
	}
	return 1;
}

uint8_t u8x8_byte_rt_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	/* u8g2/u8x8 will never send more than 32 bytes between START_TRANSFER and END_TRANSFER */
	struct rt_i2c_msg msgs;
	static uint8_t buffer[32];
	static uint8_t buf_idx;
	uint8_t *data;
	
	rt_uint8_t t = 0;
	switch(msg)
	{
		case U8X8_MSG_BYTE_SEND:
			data = (uint8_t *)arg_ptr;
			while( arg_int > 0 )
			{
				buffer[buf_idx++] = *data;
				data++;
				arg_int--;
			}
			break;
		case U8X8_MSG_BYTE_INIT:
			i2c_bus = rt_i2c_bus_device_find(I2C_DEVICE_NAME);
			if (i2c_bus == RT_NULL)
			{
				// rt_kprintf("Failed to find bus\n");
				return 0;
			}
			break;
		case U8X8_MSG_BYTE_SET_DC:
			break;
		case U8X8_MSG_BYTE_START_TRANSFER:
			buf_idx = 0;
			break;
		case U8X8_MSG_BYTE_END_TRANSFER:
			if (i2c_bus == RT_NULL)
			{
				rt_kprintf("Failed to find bus\n");
				return 0;
			}
			// I2C Data Transfer
			msgs.addr  = u8x8_GetI2CAddress(u8x8)>>1;
			msgs.flags = RT_I2C_WR;
			msgs.buf   = buffer;
			msgs.len   = buf_idx;
			while(rt_i2c_transfer(i2c_bus, &msgs, 1) != 1 && t < MAX_RETRY)
			{
				t++;
			};
			if(t >= MAX_RETRY)
			{
				return 0;
			}
			break;
		default:
			return 0;
	}
	return 1;
}

uint8_t u8x8_byte_rt_4wire_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    
    uint8_t i;
    uint8_t *data;

    uint8_t tx[256];
    uint8_t rx[256];

    static uint8_t buf_idx;
    static uint8_t buffer_tx[256];

    switch(msg) 
    {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
				
            // printf("Buffering Data %d \n", arg_int);
            while( arg_int > 0) 
            {
                // printf("%.2X ", (uint8_t)*data);
                buffer_tx[buf_idx++] = (uint8_t)*data;
							  rt_spi_send(&spi_dev_ssd1306, (uint8_t*)data, 1);
                data++;
                arg_int--;
            }  
            // printf("\n");
            break;

        case U8X8_MSG_BYTE_INIT:
            /* SPI mode has to be mapped to the mode of the current controller, at least Uno, Due, 101 have different SPI_MODEx values */
            /*   0: clock active high, data out on falling edge, clock default value is zero, takover on rising edge */
            /*   1: clock active high, data out on rising edge, clock default value is zero, takover on falling edge */
            /*   2: clock active low, data out on rising edge */
            /*   3: clock active low, data out on falling edge */
				    u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);		
			    	rt_hw_ssd1351_config(u8x8->display_info->spi_mode, u8x8->display_info->sck_clock_hz);
				    
            // printf("SPI Device Mode Set\n");
            break;

        case U8X8_MSG_BYTE_SET_DC:
            u8x8_gpio_SetDC(u8x8, arg_int);
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
					  u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:      
            memset( tx, 0, ARRAY_SIZE(tx)*sizeof(uint8_t) );
            memset( rx, 0, ARRAY_SIZE(rx)*sizeof(uint8_t) );
  
            // printf("SPI Data Sending %d\n", buf_idx);            
            for (i = 0; i < buf_idx; ++i)
            {
                // printf("%.2X ", buffer_tx[i]);
                tx[i] = buffer_tx[i];
            }
            // printf("\n");
						
						u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
						buf_idx = 0;
           
            break;
        
        default:
            return 0;
    }  
    return 1;
}
