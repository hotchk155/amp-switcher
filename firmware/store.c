#include <system.h>
#include <eeprom.h>
#include "amp-switcher.h"

#define MAGIC_COOKIE1 0x12
#define MAGIC_COOKIE2 0x37

#define EEPROM_MAGIC_COOKIE_ADDR1 0
#define EEPROM_MAGIC_COOKIE_ADDR2 1

#define EEPROM_CONFIG_BASE 2
#define EEPROM_CONFIG_SIZE sizeof(DEVICE_CONFIG)


#define EEPROM_PATCH_BASE 32
#define EEPROM_PATCH_SIZE 

////////////////////////////////////////////////////
// SAVE DATA TO EEPROM
static void storage_write(byte *data, int len, int* addr)
{
	while(len > 0) {
		eeprom_write(*addr, *data);
		++(*addr);
		++data;
		--len;
	}
}

////////////////////////////////////////////////////
// READ DATA FROM EEPROM
static void storage_read(byte *data, int len, int* addr)
{
	while(len > 0) {
		*data = eeprom_read(*addr);
		++(*addr);
		++data;
		--len;
	}
}

////////////////////////////////////////////////////
// Load a patch from EEPROM into it's appropriate
// memory slot
void storage_load_patch(byte which)
{
	int addr = EEPROM_PATCH_BASE + sizeof(DEVICE_STATUS) * which;
	storage_read((byte*)&g_patch[which], sizeof(DEVICE_STATUS), &addr);	
}

////////////////////////////////////////////////////
// Save a patch from memory into EEPROM 
void storage_save_patch(byte which)
{
	int addr = EEPROM_PATCH_BASE + sizeof(DEVICE_STATUS) * which;
	storage_write((byte*)&g_patch[which], sizeof(DEVICE_STATUS), &addr);
}

////////////////////////////////////////////////////
// Load the global device config from EEPROM
void storage_load_config()
{
	int addr = EEPROM_CONFIG_BASE;
	storage_read((byte*)&g_config, sizeof(DEVICE_CONFIG), &addr);	
}

////////////////////////////////////////////////////
// Save the global device config to EEPROM
void storage_save_config()
{
	int addr = EEPROM_CONFIG_BASE;
	storage_write((byte*)&g_config, sizeof(DEVICE_CONFIG), &addr);	
}

////////////////////////////////////////////////////
// Intialise storage module at startup
void storage_init(byte reset) 
{
	// check that data has been previously saved by checking 
	// for known values
	if(reset ||
		eeprom_read(EEPROM_MAGIC_COOKIE_ADDR1) != MAGIC_COOKIE1 || 
		eeprom_read(EEPROM_MAGIC_COOKIE_ADDR2) != MAGIC_COOKIE2) {
		
		// no usable config in memory so need to reset to default
		for(int i=0; i < NUM_PATCHES; ++i) {
			chan_init_patch(i);
			storage_save_patch(i);
		}
		init_config();
		storage_save_config();

		for(int i=0; i<5; ++i) {
			P_LED1 = 1;
			delay_ms(200);
			P_LED1 = 0;
			delay_ms(200);
		}
		
		// finally write known values to memory
		eeprom_write(EEPROM_MAGIC_COOKIE_ADDR1, MAGIC_COOKIE1);
		eeprom_write(EEPROM_MAGIC_COOKIE_ADDR2, MAGIC_COOKIE2);
		
	}
	else {
	
		// load the info from the EEPROM
		for(int i=0; i < NUM_PATCHES; ++i) {
			storage_load_patch(i);
		}
		storage_load_config();
	}
}
