/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
  
/* 配置相关功能码定??? */
  typedef enum {
    /*主串口写FLASH指令*/
    UART1DOWN = 0x01,
    /*蓝牙串口写FLASH指令*/
    UART2DOWN = 0x02,    
    /*擦除指定flash*/
    ERASFLASH = 0x0A,
    /*复位重启*/
    RESETDEV = 0x0B,    
  }funtioncode_f; 
 
  
/* 定义设备信息 */  
  typedef struct {
    /* 设备头 */
    uint64_t DevPackFlag;
    /* 设备ID */
    uint64_t DevID;
    /* 读取的蓝牙设备MAC */
    uint64_t BleMACID;
    /* 读取的LORA设备ID */
    uint64_t LoRadevEui;
  }DEVINFO;



/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BLEWAKEUP_Pin GPIO_PIN_0
#define BLEWAKEUP_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* 定义BOOTLOADER跳转时间读取位置 */
#define JUMPTIMADD           FLASH_ADDR_APPLICATION - (1024 * 3)
/* 是否存有主程序判断地址 */
#define DEVAPPSTAADD         FLASH_ADDR_APPLICATION - (1024 * 2)
/* 设备固定信息存储地址 */
#define DEVINFOADD           FLASH_ADDR_APPLICATION - (1024 * 1)


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
