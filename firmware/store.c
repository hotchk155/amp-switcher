#include <system.h>
#include <eeprom.h>
#include "amp-switcher.h"

#define MAGIC_COOKIE1 0x12
#define MAGIC_COOKIE2 0x35

#define EEPROM_MAGIC_COOKIE_ADDR1 0
#define EEPROM_MAGIC_COOKIE_ADDR2 1

#define EEPROM_CONFIG_BASE 2
#define EEPROM_CONFIG_SIZE sizeof(DEVICE_CONFIG)


#define EEPROM_PATCH_BASE 32
#define EEPROM_PATCH_SIZE sizeof(DEVICE_STATUS)

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
void storage_load_patch(byte which)
{
	int addr = EEPROM_PATCH_BASE + EEPROM_PATCH_SIZE * which;
	storage_read((byte*)&g_status, EEPROM_PATCH_SIZE, &addr);	
}

////////////////////////////////////////////////////
void storage_save_patch(byte which)
{
	int addr = EEPROM_PATCH_BASE + EEPROM_PATCH_SIZE * which;
	storage_write((byte*)&g_status, EEPROM_PATCH_SIZE, &addr);
}

////////////////////////////////////////////////////
void storage_load_config()
{
	int addr = EEPROM_CONFIG_BASE;
	storage_read((byte*)&g_status, EEPROM_CONFIG_SIZE, &addr);	
}

////////////////////////////////////////////////////
void storage_save_config()
{
	int addr = EEPROM_CONFIG_BASE;
	storage_write((byte*)&g_status, EEPROM_CONFIG_SIZE, &addr);	
}

////////////////////////////////////////////////////
void storage_init() 
{
	if(eeprom_read(EEPROM_MAGIC_COOKIE_ADDR1) != MAGIC_COOKIE1 || 
		eeprom_read(EEPROM_MAGIC_COOKIE_ADDR2) != MAGIC_COOKIE2) {
		for(int i=0; i < NUM_PATCHES; ++i) {
			init_patch(&g_status, i);
			storage_save_patch(i);
		}
		init_config();
		storage_save_config();
		eeprom_write(EEPROM_MAGIC_COOKIE_ADDR1, MAGIC_COOKIE1);
		eeprom_write(EEPROM_MAGIC_COOKIE_ADDR2, MAGIC_COOKIE2);
	}
	else {
		storage_load_config();
		storage_load_patch(0);
	}
}
