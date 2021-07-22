/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"    
#include "string.h"
#include "internalflash.h"
#include "mbcrc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t uart1Revflag = 0;
uint8_t uart2Revflag = 0;
uint8_t uart1Data[1074];
uint8_t uart2Data[1074];
uint16_t uart1len = 0;
uint16_t uart2len = 0;
uint8_t uart1RxDatatmp;
uint8_t uart2RxDatatmp;

/* 跳转计时定义 */
uint16_t jumptim = 0;

/* 保持boot状态标志位 */
uint8_t timflag = 0;

/* 单字节超时计数 */
uint16_t Time6Flag = 0;
uint16_t Time7Flag = 0;

/* 单包数据需要接收的长度 */
uint16_t uart1packlen = 0;
uint16_t uart2packlen = 0;

funtioncode_f Funtioncode;

const uint8_t appfinishflag[8] = {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A}; 
uint8_t readfinishflag[8];

uint64_t JumpTimMAX = 0;

uint8_t StartDevFlag[7] = {0xEE,0x0F,0x00,0x03,0x01,0x33,0x0E};

DEVINFO DevInfoData = {
    /* 设备头 */
    0x0C0C0C0C0C0C0C0C,
    /* 设备ID */
    0x0000000000000000,
    /* 读取的蓝牙设备MAC */
    0x0000000000000000,
    /* 读取的LORA设备ID */
    0x0000000000000000, 

};

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 单字节接收超时计时 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* 单字节超时计时器 */
  if(htim->Instance == htim6.Instance)  
  {
    ++Time6Flag;
  }
  else if(htim->Instance == htim7.Instance)  
  {
    ++Time7Flag;
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* 主串口回调函数 */
  if(huart->Instance == USART1)
  {
    uart1Data[uart1len++] = uart1RxDatatmp;
    Time6Flag = 0;  
    if(uart1len < 2)
      HAL_TIM_Base_Start_IT(&htim6);  
    if((uart1Revflag == 0) && (uart1len > 3))
    {
      /* 表示正在接收单包数据 */
      uart1Revflag = 1;
      /* 获取接下来的数据长度 */
      uart1packlen = (uint16_t)(uart1Data[2]<<8) | uart1Data[3];
    }
    else if(uart1Data[0] != 0xFF)
    {
      uart1len = 0;
    }
    if(uart1Revflag == 1)
    {
      if(uart1packlen == uart1len-4)
      {
        /* 表述数据接收完毕 */
        uart1Revflag = 6;
        /* 可以关闭超时计时中断 */
        Time6Flag = 0;
        HAL_TIM_Base_Stop(&htim7);
      }
    }
    HAL_UART_Receive_IT(&huart1,&uart1RxDatatmp, 1); 
  }else if(huart->Instance == USART2)
  {
    uart2Data[uart2len++] = uart2RxDatatmp;
    Time7Flag = 0;  
    if(uart2len < 2)
      HAL_TIM_Base_Start_IT(&htim7);  
    if((uart2Revflag == 0) && (uart2len > 3))
    {
      /* 表示正在接收单包数据 */
      uart2Revflag = 1;
      /* 获取接下来的数据长度 */
      uart2packlen = (uint16_t)(uart2Data[2]<<8) | uart2Data[3];
    }
    else if(uart2Data[0] != 0xFF)
    {
      uart2len = 0;
    }    
    if(uart2Revflag == 1)
    {
      if(uart2packlen == uart2len-4)
      {
        /* 表述数据接收完毕 */
        uart2Revflag = 6;
        /* 可以关闭超时计时中断 */
        Time7Flag = 0;
        HAL_TIM_Base_Stop(&htim7);
      }
    }
    HAL_UART_Receive_IT(&huart2,&uart2RxDatatmp, 1);   
  
  }
}

/* 进行数据解析判断接收到的单帧数据属于的类型指令 */
uint8_t Data_Analy(uint8_t *dat, uint16_t dlen)
{
  uint8_t ackdata[30];
  uint32_t inputaddr = 0;
  uint32_t inputdatalen = 0;
  uint16_t crcdata = 0;
  /*首先校验CRC判断是否数据正确*/
  crcdata = usMBCRC16( dat, dlen-2 );
  /* 打印计算得出的CRC数据 */
//  printf("CRC : %016x\n",crcdata);
  if(crcdata != ((uint16_t)(dat[dlen-2]<<8)|(dat[dlen-1]))) 
  {
    inputaddr = (uint16_t)((dat[4]<<8)|dat[5])*8;     
    ackdata[0] = 0xEE;
    ackdata[1] = UART1DOWN;
    ackdata[2] = 0x00;
    ackdata[3] = 0x05;
    inputaddr = inputaddr*8;
    ackdata[4] = (uint8_t)(inputaddr>>8);
    ackdata[5] = (uint8_t)inputaddr;
    ackdata[6] = 0x01; 
    crcdata = usMBCRC16( ackdata, 7 );
    ackdata[7] = (uint8_t)(crcdata>>8);
    ackdata[8] = (uint8_t)crcdata;      
    /*发送应答数据*/
    HAL_UART_Transmit(&huart1 , (uint8_t *)ackdata, 9, 0xFFFF);   
    HAL_UART_Transmit(&huart2 , (uint8_t *)ackdata, 9, 0xFFFF); 
    
    printf("CRC check FAIL !!!\n");
    return 1;
  }
  /*其次判断功能码*/
  Funtioncode = (funtioncode_f)dat[1];
  /* 等待获取数据使设备进入相关模式*/
  switch(Funtioncode)
  {
   /* 主串口下载程序到flash */  
   case UART1DOWN:
      /*获取实际有效字节长度*/
      inputdatalen = (uint32_t)((dat[2]<<8)|dat[3]);
      if((inputdatalen-4)%8 != 0) 
      {
        printf("inputdatalen error \r\n");
        return 1;
      }
      /*获取地址*/
      inputaddr = (uint16_t)((dat[4]<<8)|dat[5])*8; 
      if(inputaddr%8 != 0) 
      {
        printf("inputaddr error \r\n");
        return 1;
      }
//      /*擦除对应flash*/
//      Erase_ST_Flash(inputaddr,1);
      ackdata[0] = 0xEE;
      ackdata[1] = UART1DOWN;
      ackdata[2] = 0x00;
      ackdata[3] = 0x05;
      inputaddr = inputaddr*8;
      ackdata[4] = (uint8_t)(inputaddr>>8);
      ackdata[5] = (uint8_t)inputaddr;
      /*判断正确可以进行flash写入*/
      if(Write_Data_Flash(inputaddr, &dat[6], inputdatalen-4) == 0)
      {
        ackdata[6] = 0x00;
        printf("Flash write success addr %d",inputaddr);
      }  
      else
      {
        ackdata[6] = 0x01; 
        printf("Flash write fail addr %d",inputaddr);
      }
      crcdata = usMBCRC16( ackdata, 7 );
      ackdata[7] = (uint8_t)(crcdata>>8);
      ackdata[8] = (uint8_t)crcdata;      
      /*发送应答数据*/
      HAL_UART_Transmit(&huart1 , (uint8_t *)ackdata, 9, 0xFFFF); 

//      Read_Data_Flash(0, ttem, 40);
      break;
   /* 蓝牙串口下载程序到flash */  
   case UART2DOWN:
      /*获取实际有效字节长度*/
      inputdatalen = (uint32_t)((dat[2]<<8)|dat[3]);
      if((inputdatalen-4)%8 != 0) 
      {
        printf("inputdatalen error \r\n");
        return 1;
      }
      /*获取地址*/
      inputaddr = (uint16_t)((dat[4]<<8)|dat[5]); 
      if(inputaddr%8 != 0)
      {
        printf("inputaddr error \r\n");
        return 1;
      }        
//      /*擦除对应flash*/
//      Erase_ST_Flash(inputaddr,1);
      ackdata[0] = 0xEE;
      ackdata[1] = UART2DOWN;
      ackdata[2] = 0x00;
      ackdata[3] = 0x05;
      inputaddr = inputaddr*8;      
      ackdata[4] = (uint8_t)(inputaddr>>8);
      ackdata[5] = (uint8_t)inputaddr;
      /*判断正确可以进行flash写入*/
      if(Write_Data_Flash(inputaddr, &dat[6], inputdatalen-4) == 0)
      {
        ackdata[6] = 0x00;
        printf("Flash write success addr %d",inputaddr);        
      }  
      else
      {
        ackdata[6] = 0x01; 
        printf("Flash write fail addr %d",inputaddr);        
      }
      crcdata = usMBCRC16( ackdata, 7 );
      ackdata[7] = (uint8_t)(crcdata>>8);
      ackdata[8] = (uint8_t)crcdata;
      HAL_Delay(5);
      /* 发送应答数据 */
      HAL_UART_Transmit(&huart2 , (uint8_t *)ackdata, 9, 0xFFFF); 
      HAL_UART_Transmit(&huart1 , (uint8_t *)ackdata, 9, 0xFFFF); 
//      Read_Data_Flash(0, ttem, 40);
      break;      
  case ERASFLASH:
//      if((inputdatalen-4)%8 != 0) return 1;
      /*获取地址*/
      inputaddr = (uint16_t)((dat[4]<<8)|dat[5]);
      /*获取擦除数据的页*/
      inputdatalen = (uint32_t)(dat[6]<<16) | (dat[7]<<8) | dat[8];
      inputdatalen = inputdatalen/1024;
      if(inputdatalen >= 128) inputdatalen = 64;
      /*打印要擦除的页数*/
      printf("Erasure FLASH page %d \r\n",inputdatalen);      
//      if(inputaddr%8 != 0) return 1;    
      ackdata[0] = 0xEE;
      ackdata[1] = ERASFLASH;
      ackdata[2] = 0x00;
      ackdata[3] = 0x05;
      ackdata[4] = (uint8_t)(inputaddr>>8);
      ackdata[5] = (uint8_t)inputaddr;   
      /*擦除对应位置上的数据*/
      if(Erase_ST_Flash(inputaddr,inputdatalen) == 0)
      {
        /* 主程序已经删除标志 */
        Erase_ST_Flash(DEVAPPSTAADD,1);
        ackdata[6] = 0x00;
      }  
      else
      {
        ackdata[6] = 0x01;      
      }   
      crcdata = usMBCRC16( ackdata, 7 );
      ackdata[7] = (uint8_t)(crcdata>>8);
      ackdata[8] = (uint8_t)crcdata;        
      /*发送应答数据*/
      HAL_UART_Transmit(&huart2 , (uint8_t *)ackdata, 9, 0xFFFF);
             
      break;
  case RESETDEV:   
      Write_Data_Flash(FLASH_ADDR_APPLICATION - 4096,(uint8_t *)appfinishflag,1);    
    
      ackdata[0] = 0xEE;
      ackdata[1] = RESETDEV;
      ackdata[2] = 0x00;
      ackdata[3] = 0x05;
      ackdata[4] = 0x00;
      ackdata[5] = 0x00;   
      ackdata[6] = 0x00;
      crcdata = usMBCRC16( ackdata, 7 );
      ackdata[7] = (uint8_t)(crcdata>>8);
      ackdata[8] = (uint8_t)crcdata;        
      /*发鿁应答数捿*/
      HAL_UART_Transmit(&huart2 , (uint8_t *)ackdata, 9, 0xFFFF); 
      HAL_UART_Transmit(&huart1 , (uint8_t *)ackdata, 9, 0xFFFF);
      
      printf("Device Reset!!!\n");
      __set_FAULTMASK(1);
      NVIC_SystemReset();      
      break;
    default:
      printf("Device Data error\n");
      break;
  }
  
  return 0;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &uart1RxDatatmp, 1);
  HAL_UART_Receive_IT(&huart2, &uart2RxDatatmp, 1);
  
  /* 判断是否是第一次启动并且读取设备信息 */
  memcpy(readfinishflag,(uint8_t *)DEVAPPSTAADD,8);
  /* 读取本次需要延时的时间 */
  memcpy(&JumpTimMAX,(uint8_t *)JUMPTIMADD,8);  
  if(JumpTimMAX > 1000) JumpTimMAX = 3;
 
  
  /*调试与提示打印*/
  printf("bootloader start ...\n");
  printf("Jump Application Time %d\n",(uint32_t)JumpTimMAX);
  printf("\n");
  printf("\n");
  printf("\n");
  
  HAL_UART_Transmit(&huart2 , (uint8_t *)StartDevFlag, 7, 0xFFFF); 
  HAL_UART_Transmit(&huart1 , (uint8_t *)StartDevFlag, 7, 0xFFFF);   
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* 获得接收串口标志，判断是否已经接收完成一帧数�? */
    if((uart1Revflag == 1) && (Time6Flag > 100))
    {
        /* 表述数据接收完毕 */
        uart1Revflag = 6;        
        /* 可以关闭超时计时中断 */
        Time6Flag = 0;
        HAL_TIM_Base_Stop(&htim6);    
    }
    else if((uart2Revflag == 1) && (Time7Flag > 200))
    {
        /* 表述数据接收完毕 */
        uart2Revflag = 6;        
        /* 可以关闭超时计时中断 */
        Time7Flag = 0;
        HAL_TIM_Base_Stop(&htim7);     
    
    }
    if(uart1Revflag == 6)
    {
      timflag = 1;
      /* 获得接收数据完成后进入相关数据解�? */ 
      uart1Revflag = 0;
      /* 解析主串口接收数�? */
      Data_Analy(uart1Data, uart1len);
      /* 解析完成清空接收 */
      memset(uart1Data,0,1074);
      uart1len = 0;
    }
    else if(uart2Revflag == 6)
    {
      timflag = 1;
      /* 获得接收数据完成后进入相关数据解�? */ 
      uart2Revflag = 0;
      /* 解析主串口接收数�? */
      Data_Analy(uart2Data, uart2len);
      /* 解析完成清空接收 */
      memset(uart2Data,0,1074);
      uart2len = 0;
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_Delay(100);
    /* 计时并判断是否需要跳�? */
    if(timflag == 0)
    /* 如果开始蓝牙连接状态就跳入升级固件，不然就倒计时5秒跳转 */
//    if(BLEWakeUp == 1)
    {
      jumptim++;
      if(jumptim%10 == 0)
        printf("Time %d\n",jumptim/10);      
    }
    /* 计时之后跳转 */
    if((jumptim > JumpTimMAX*10) && (BLEWakeUp == 1))
    {
      jumptim = 0;
      /* 判断是否有主程序 */
      memcpy(readfinishflag,(uint8_t *)DEVAPPSTAADD,8);
//      if(strstr(appfinishflag,readfinishflag) != NULL)
      {
        /* 进行跳转 */
        printf("jump to application...\n");
        jump_application();
      }
    }
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
