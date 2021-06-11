#include "stm32l4xx.h"




/*单字节写入函数*/
uint8_t Write_Data_Flash(uint32_t addrd, uint8_t* ptrd, uint16_t ndchar);
/*单字节读取数据函数*/
void Read_Data_Flash(uint32_t addrd, uint8_t* ptrd, uint16_t nchar);
/*擦除页*/
uint8_t Erase_ST_Flash(uint32_t addr, uint16_t npage);



