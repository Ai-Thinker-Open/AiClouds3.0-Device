/*
 * @Author: your name
 * @Date: 2019-11-13 10:36:41
 * @LastEditTime: 2019-11-13 10:43:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ESP8266_RTOS_SDK\mine\ESP8266ForMqttPWM\components\rgb\include\rgb.h
 */

#ifndef RGB_H_
#define RGB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

/* Define your drive pin */
#define GPIO_RGB_SCL 15
#define GPIO_RGB_SDA 4

/* Set GPIO Direction */
#define SCL_LOW gpio_set_level(GPIO_RGB_SCL, 0)
#define SCL_HIGH gpio_set_level(GPIO_RGB_SCL, 1)

#define SDA_LOW gpio_set_level(GPIO_RGB_SDA, 0)
#define SDA_HIGH gpio_set_level(GPIO_RGB_SDA, 1)

#define R_MAX 255
#define G_MAX 255
#define B_MAX 255

/* Function declaration */
void rgbControl(uint8_t R, uint8_t B, uint8_t G);
void rgbLedInit(void);
void rgbGpioInit(void);
void rgbSensorTest(uint8_t rgbcou);

#endif