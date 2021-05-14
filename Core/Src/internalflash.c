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



/**
* 说明 : 往ST的FLASH写入指定长度的数据（由于是单页写入，写入的字节不能超过4K字节）
* 参数 : addr, 写入的起始地址（保证双字对齐）
*        ptr, 数据存放地址（注意是uint64_t类型，即一次写入8字节）
*        ndword, 写入的Dword数（注意双字数）
* 返回 : 成功返回0，失败返回1
*/ 
uint8_t Write_ST_Flash(uint32_t addr, uint64_t* ptr, uint16_t ndword)
{
  uint8_t page;
  uint32_t PAGEError=0;
  FLASH_EraseInitTypeDef EraseInitStruct;
  /**不能超4K */
  if(ndword > 512)
  {
    return 1;
  }
  /** 解锁FLASH寄存器 */
  HAL_FLASH_Unlock();
  /** 清除所有错误标志（如果不清除会导致写失败） */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS); 
  /** 计算用户编程地址在FLASH中哪个页(每页4K字节) */
  /** 如果地址在Bank1则用下面这个公式计算页 */
  page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  /** 如果地址在Bank2则用下面这个公式计算页 */
  //  page = (addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE; 
  
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES; /* 擦除方式：页擦除 */
  EraseInitStruct.Banks       = FLASH_BANK_1;          /* 擦除页所在的区域：BANK1 */
  EraseInitStruct.Page        = page;                  /* 擦除页的编号 */
  EraseInitStruct.NbPages     = 1;                     /* 擦除页的数量：1 */
  
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /* error */
    return 1;
  }
  for(uint16_t i=0; i<ndword; i++)
  {
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, ptr[i]) == HAL_OK)
    {
      addr = addr + 8;
    }
    else
    {
      /* error */
      return 1;      
    }
  }
  HAL_FLASH_Lock();
  return 0;
}



/**
* 说明 : 从ST的flash中连续读取指定长度的数据并储存到指定地址（4字节读取）
* 参数 : addr, 读取的起始地址
*        ptr, 存放地址
*        nword, 读取的word数
* 返回 : 成功返回0，失败返回1
*/
void Read_ST_Flash(uint32_t addr, uint32_t* ptr, uint16_t nword)
{
  for(uint16_t i=0; i<nword; i++)
  {
    ptr[i] = *(__IO uint32_t *)addr;
    addr = addr + 4;
  }
}


















