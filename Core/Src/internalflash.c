#include "internalflash.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


/*主程序写入地址用于更改主程序指定位置，不能轻易变动*/
#define APPSTATADD     FLASH_ADDR_APPLICATION       

/*单页已用字节个数*/
static uint16_t onepageval = 0;

/*当前写入地址*/
static uint32_t presentadd = APPSTATADD;

/*写入数据到FLASH需要以4k为动态缓冲，当最后数据接收结束并未达到4K时进行FF填充*/
/*需要进行uint8_t 到 uint64_t 指针指向转换*/

/**
*说明 : 擦除4k字节长度的FLASH，用于擦除一页数据
*参数 : addr 指定擦除位置
*       npage 指定擦除页数
*返回 : 成功返回0，失败返回1
*
*/
uint8_t Erase_ST_Flash(uint32_t addr, uint16_t npage)
{
  uint8_t page;
  uint32_t PAGEError=0;  
  FLASH_EraseInitTypeDef EraseInitStruct;

  
  addr = addr + APPSTATADD;
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
  EraseInitStruct.NbPages     = npage;                     /* 擦除页的数量：1 */
  
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /* error */
      return 1;
  }
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
//  uint8_t page;
//  uint32_t PAGEError=0;
//  FLASH_EraseInitTypeDef EraseInitStruct;
//  /** 记录是否存在不够4字节 */
//  uint8_t modword = 0;
  
  /**不能超4K */
  if(ndword > 512)
  {
    return 1;
  }

  /** 解锁FLASH寄存器 */
  HAL_FLASH_Unlock();
  /** 清除所有错误标志（如果不清除会导致写失败） */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS); 
//  /** 计算用户编程地址在FLASH中哪个页(每页4K字节) */
//  /** 如果地址在Bank1则用下面这个公式计算页 */
//  page = (addr - FLASH_BASE) / FLASH_PAGE_SIZE;
//  /** 如果地址在Bank2则用下面这个公式计算页 */
//  //  page = (addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE; 
//  
//  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES; /* 擦除方式：页擦除 */
//  EraseInitStruct.Banks       = FLASH_BANK_1;          /* 擦除页所在的区域：BANK1 */
//  EraseInitStruct.Page        = page;                  /* 擦除页的编号 */
//  EraseInitStruct.NbPages     = 1;                     /* 擦除页的数量：1 */
//  
//  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
//  {
//    /* error */
//    return 1;
//  }
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
*说明：将4字节写入FLASH 转换成单字节写入FLASH
*参数： addrd表示字节的起始位置
*       *ptrd写入字符串指针
*       ndchar写入单字节字符串的字节长度
*返回： 0表示写入完成  1表示写入错误
***/
uint8_t Write_Data_Flash(uint32_t addrd, uint8_t* ptrd, uint16_t ndchar)
{
  uint8_t ret = 0;
  uint8_t *cptr;
  uint16_t ndtchar;
    
  cptr = malloc(ndchar+8);
  /** 计算写入的字节数据是否属于4字节，不属于4字节需要在后面添加0xFF */  
  if((ndchar%8) != 0)
  {
    /*如果8的余数不为0则认为数据错误跳出*/
    return 1;
  }
  else
  {
    /** 获得uint64_t的字节长度 */
    ndtchar = (ndchar/8);  
  }
  /** 将申请的空间填充为0xff */
  memset(cptr,0xff,ndchar+8);
  /** 判断是否在之后添加0xFF */
  for(int j=0;j<ndchar;j++)
    cptr[j] = ptrd[j];
  /* 将处理完成的数据写入flash */
  ret = Write_ST_Flash(addrd+APPSTATADD, (uint64_t *)cptr, ndtchar);
  
  free(cptr); 
  
  return ret;
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

/**
* 说明 : 从ST的flash中连续读取指定长度的数据并储存到指定地址（1字节读取）
* 参数 : addr, 读取的起始地址
*        ptr, 存放地址
*        nword, 读取的word数
* 返回 : 成功返回0，失败返回1
*/
void Read_Data_Flash(uint32_t addrd, uint8_t* ptrd, uint16_t nchar)
{
  for(uint16_t i=0; i<nchar; i++)
  {
    ptrd[i] = *(__IO uint32_t *)(addrd+APPSTATADD);
    addrd = addrd + 1;
  }
}


/****
 *进行字节转换与缓存写入FLASH中
 *saddr ： 第几包的数据位置
 *ndword ：当次写入单字节个数
 *ackn ： 是否已经写完 0为未写完  1为已经写完
*****/ 
void Buffer_Flash(uint8_t* ptr, uint16_t ndword, uint8_t ackn, uint8_t* pageptr)
{
//  uint16_t ackval;
//  
//  /*赋值此次进入的单字节长度*/
//  ackval = ndword;
//  char *Ptr = NULL;
//  Ptr = (char *)malloc(4096 * sizeof(char));
  if(ackn == 0)
  {
    /*输入字节小于叠加余数*/
    if((ndword <= (4096-onepageval)) && (4096-onepageval > 0))
    {
      /*将对应字节放入申请的4k堆栈中*/
      onepageval += ndword;
      /*将对应字符串放入缓冲队列*/
      memcpy(&pageptr[onepageval],ptr,ndword);
    }
    else
    {
      /*将前半部分写入堆栈中，后半部分等待写完flash之后再写入堆栈中*/
      memcpy(&pageptr[onepageval],ptr,4096-onepageval);
      /*之后再整体写道FLASH中*/
      Write_ST_Flash(presentadd, (uint64_t*) pageptr, 4096);
      /*更改当前写入地址*/  
      presentadd += APPSTATADD;
      /*标记之后占用的下一次写入的字节数*/
      onepageval = ndword - (4096-onepageval);
    }
  }
  else
  {
    /*将剩余字节填充FF进行flash写入*/
    /*判断flash字节剩余个数加入循环填充*/
    for(int i=4096-onepageval;i<=4096;i++)
    {
      pageptr[i] = 0xFF;
    }
    Write_ST_Flash(presentadd, (uint64_t*) pageptr, 4096);
  }
  
//  /*释放申请堆栈*/
//  free(Ptr);
}


/* 跳转主程序函数 */
void jump_application(void)
{
  typedef void (*funcPtr)(void);
  uint32_t jumpAddr = *(uint32_t*)(FLASH_ADDR_APPLICATION + 0x04); /* reset ptr in vector table */
  funcPtr usrMain = (funcPtr)jumpAddr;
  
  HAL_RCC_DeInit();
  HAL_DeInit();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;
  for (int i = 0; i < 8; i++)
  {
    NVIC->ICER[i]=0xFFFFFFFF;
    NVIC->ICPR[i]=0xFFFFFFFF;
  }
  /* Set stack pointer as in application's vector table */
  __set_MSP(*(uint32_t*)FLASH_ADDR_APPLICATION);
  usrMain();
}















