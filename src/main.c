#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <zephyr/storage/flash_map.h>
#include <ff.h>

LOG_MODULE_REGISTER( main );

#define FLASH_STORAGE_PARTITION		flash_storage_partition
#define FLASH_STORAGE_PARTITION_ID	FIXED_PARTITION_ID( FLASH_STORAGE_PARTITION )
#define DISK_MOUNT_PT 				"/NAND:"
#define RECORDS_DIRECTORY_PATH 		"/NAND:/Records/"

typedef enum
{
    SYSTEM_NO_ERROR = 0,
    SYSTEM_ERROR = -99,
    SYSTEM_ERROR_INIT_GPIO,
    SYSTEM_ERROR_INIT_STATUS_LED,
    SYSTEM_ERROR_INIT_BLE_CONNECTION,
    SYSTEM_ERROR_INIT_ICM20948,
    SYSTEM_ERROR_BLE_DEVICE_NACK_RECEIVED,
    SYSTEM_ERROR_BLE_PACKET_RECEIVED_WRONG_SIZE,
    SYSTEM_ERROR_BLE_PACKET_RECEIVED_WRONG_DATA,

} system_error_t;

static struct fs_mount_t fsMount;
static const struct gpio_dt_spec ledRed = GPIO_DT_SPEC_GET( DT_NODELABEL( led_red ), gpios );

static system_error_t Memory_Flash_Setup_Disk( void )
{
	struct fs_mount_t *mp = &fsMount;
	static FATFS fatFs;
	unsigned int storagePartitionId;
	const struct flash_area *pfa;

	mp->storage_dev = ( void * )FLASH_STORAGE_PARTITION_ID;
	storagePartitionId = FLASH_STORAGE_PARTITION_ID;
	
	if ( flash_area_open( storagePartitionId, &pfa ) < 0 )
	{
		flash_area_close( pfa );
		LOG_ERR( "Failed to setup flash area" );
		return SYSTEM_ERROR;
	}

	mp->type = FS_FATFS;
	mp->fs_data = &fatFs;
	mp->mnt_point = DISK_MOUNT_PT;	
	
	if ( fs_mount( mp ) < 0 )
	{
		LOG_ERR( "Failed to mount filesystem" );
		return SYSTEM_ERROR;
	}

	k_msleep( 50 );
	return SYSTEM_NO_ERROR;
}

void main(void)
{
	struct fs_file_t file;

	if ( !device_is_ready( ledRed.port) )
	{
		LOG_WRN( "Initialize led devices failed" );
		return;
	}

	if ( gpio_pin_configure_dt( &ledRed, GPIO_OUTPUT_INACTIVE ) < 0 )
	{
		LOG_WRN( "Configure led pins failed" );
		return;
	}

	Memory_Flash_Setup_Disk();
	
	if ( usb_enable( NULL ) < 0 )
	{
		LOG_ERR( "Failed to enable USB" );
		return;
	}

	LOG_INF( "The device is put in USB mass storage mode.\n" );

	while (1)
	{
		gpio_pin_toggle_dt( &ledRed );
		k_msleep( 1000 );
	}
}
