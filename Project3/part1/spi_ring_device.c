/*
 * program to show the binding of platform driver and device.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include<linux/spi/spi.h>
#include <linux/platform_device.h>

static struct spi_device *ws2812_spi_device;

static struct spi_board_info ws2812_board_info = {
		.modalias = "WS2812",
		.chip_select = 1,
		.controller_data = NULL,
		.max_speed_hz = 6400000,
		.bus_num = 1,
		.mode = SPI_MODE_3,
};		

/**
 * register the device when module is initiated
 */

static int spi_device_init(void)
{
	int ret = 0;	
	struct spi_master *master;	

	/* Register the device */
	
	master = spi_busnum_to_master( ws2812_board_info.bus_num);
	if( !master )
		return -ENODEV;

	ws2812_spi_device = spi_new_device( master, &ws2812_board_info);
	if( !ws2812_spi_device )
		return -ENODEV;

	ws2812_spi_device->bits_per_word = 24;

	ret = spi_setup( ws2812_spi_device );
	if( ret )
		spi_unregister_device( ws2812_spi_device );
	else
		printk( KERN_INFO "ws2812 registered to SPI bus %u, chipselect %u\n", 
			ws2812_board_info.bus_num, ws2812_board_info.chip_select );

	printk(KERN_ALERT "spi device is registered \n");
	
	return ret;
}

static void spi_device_exit(void)
{
	spi_unregister_device( ws2812_spi_device );

	printk(KERN_ALERT "Goodbye, unregister the device\n");
}

module_init(spi_device_init);
module_exit(spi_device_exit);
MODULE_LICENSE("GPL");