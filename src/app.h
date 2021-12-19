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
#include <WisBlock-API.h>

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
#define INT1_PIN WB_IO3
bool init_acc(void);
void clear_acc_int(void);
void read_acc(void);

/** ToF stuff */
#include <VL53L0X.h>
bool init_tof(void);
void get_water_level(void);
extern uint16_t overflow_treshold;
extern uint16_t lowlevel_treshold;

/**
 * @brief LoRa packet structure
 * 		Using Cayenne LPP format
 * 
 */
struct sensor_payload_s
{
	uint8_t data_flag0 = 0x01; // 1 Channel # 1
	uint8_t data_flag1 = 0x02; // 2 Analog Input
	uint8_t level_1 = 0;	   // 3 Water level
	uint8_t level_2 = 0;	   // 4 Water level
	uint8_t data_flag2 = 0x02; // 5 Channel # 2
	uint8_t data_flag3 = 0x02; // 6 Analog Input
	uint8_t batt_1 = 0;		   // 7 Battery level
	uint8_t batt_2 = 0;		   // 8 Battery level
	uint8_t data_flag4 = 0x03; // 9 Channel # 3
	uint8_t data_flag5 = 0x66; // 10 Presence sensor (Alarm)
	uint8_t alarm_of = 0;	   // 11 Alarm flag overflow
	uint8_t data_flag6 = 0x04; // 12 Channel # 4
	uint8_t data_flag7 = 0x66; // 13 Presence sensor (Alarm)
	uint8_t alarm_ll = 0;	   // 14 Alarm flag low level
};
extern sensor_payload_s g_sensor_payload;
#define PAYLOAD_LENGTH sizeof(sensor_payload_s)

/** Analog value union */
union analog_s
{
	uint16_t analog16 = 0;
	uint8_t analog8[2];
};

#endif