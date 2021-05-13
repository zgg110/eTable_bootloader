#include "internalflash.h"
#include <stdint.h>
#include <string.h>



int internal_flash_write(uint32_t addr, uint8_t *data, uint64_t length){
  HAL_FLASH_Unlock();
  for(int i = 0; i < length; i++){
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, data[i]);
  }
  HAL_FLASH_Lock();
  return 0;
}


int internal_flash_erase_app_sectors(){
  HAL_FLASH_Unlock();
  FLASH_EraseInitTypeDef eraseInit = {0};
  eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  eraseInit.Sector = FLASH_SECTOR_5;
  eraseInit.NbSectors = 2;
  eraseInit.Banks = FLASH_BANK_1;
  uint32_t sectorError = 0;
  HAL_FLASHEx_Erase(&eraseInit, &sectorError);
  HAL_FLASH_Lock();
  return sectorError != 0xFFFFFFFF;
}



















