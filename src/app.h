/**
 * @file app.h
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief For application specific includes and definitions
 * @version 0.1
 * @date 2021-04-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef APP_H
#define APP_H

#include <Arduino.h>
/** Add you required includes after Arduino.h */

#include <Wire.h>
/** Include the WisBlock-API */
#include <WisBlock-API-V2.h>

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                     \
	do                                      \
	{                                       \
		if (tag)                            \
			PRINTF("[%s] ", tag);           \
		PRINTF(__VA_ARGS__);                \
		PRINTF("\n");                       \
		if (g_ble_uart_is_connected)        \
		{                                   \
			g_ble_uart.printf(__VA_ARGS__); \
		}                                   \
	} while (0)
#else
#define MYLOG(...)
#endif

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

/** Application events */
#define ACC_TRIGGER 0b1000000000000000
#define N_ACC_TRIGGER 0b0111111111111111

/** Application stuff */
extern BaseType_t g_higher_priority_task_woken;

/** Accelerometer stuff */
#include <SparkFunLIS3DH.h>
#define INT1_PIN WB_IO5
bool init_acc(void);
void clear_acc_int(void);
void read_acc(void);

/** ToF stuff */
#include <VL53L0X.h>
bool init_tof(void);
void get_water_level(void);
extern uint16_t overflow_treshold;
extern uint16_t lowlevel_treshold;

// LoRaWAN stuff
#include <wisblock_cayenne.h>
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1	   // Base Board
#define LPP_CHANNEL_WLEVEL 61  // RAK12014
#define LPP_CHANNEL_WL_LOW 62  // RAK12014
#define LPP_CHANNEL_WL_HIGH 63 // RAK12014
#define LPP_CHANNEL_WL_VALID 64 // RAK12014

extern WisCayenne g_solution_data;

/** Analog value union */
union analog_s
{
	uint16_t analog16 = 0;
	uint8_t analog8[2];
};

#endif