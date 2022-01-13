/**
 * @file app.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Application specific functions. Mandatory to have init_app(), 
 *        app_event_handler(), ble_data_handler(), lora_data_handler()
 *        and lora_tx_finished()
 * @version 0.1
 * @date 2021-04-23
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "app.h"

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "MHC-WL";

/** Required for give semaphore from ISR */
BaseType_t g_higher_priority_task_woken = pdTRUE;

/** Send Fail counter **/
uint8_t send_fail = 0;

/** LoRaWAN payload */
sensor_payload_s g_sensor_payload;

/** Battery level uinion */
analog_s batt_level;

/**
 * @brief Application specific setup functions
 * 
 */
void setup_app(void)
{
	// Enable BLE
	g_enable_ble = true;
#if API_DEBUG == 0
	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		}
		else
		{
			break;
		}
	}
#endif
}

/**
 * @brief Application specific initializations
 * 
 * @return true Initialization success
 * @return false Initialization failure
 */
bool init_app(void)
{
	bool result = true;
	MYLOG("APP", "init_app");

	// COnfigure supply control IO
	pinMode(WB_IO2, OUTPUT);

	// Start I2C
	Wire.begin();

	result = init_tof();

	// Initialize ACC sensor
	result |= init_acc();

	return result;
}

/**
 * @brief Application specific event handler
 *        Requires as minimum the handling of STATUS event
 *        Here you handle as well your application specific events
 */
void app_event_handler(void)
{
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");

		// Get water level
		get_water_level();

		// Get Battery status
		batt_level.analog16 = read_batt() / 10;
		MYLOG("APP", "Battery level %d\n", batt_level.analog16 * 10);

		g_sensor_payload.batt_1 = batt_level.analog8[1];
		g_sensor_payload.batt_2 = batt_level.analog8[0];

#if MY_DEBUG > 0
		char payload_log[64] = {0};
		sprintf(payload_log, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
				g_sensor_payload.data_flag0, g_sensor_payload.data_flag1, g_sensor_payload.level_1, g_sensor_payload.level_2,
				g_sensor_payload.data_flag2, g_sensor_payload.data_flag3, g_sensor_payload.batt_1, g_sensor_payload.batt_2,
				g_sensor_payload.data_flag4, g_sensor_payload.data_flag5, g_sensor_payload.alarm_of,
				g_sensor_payload.data_flag6, g_sensor_payload.data_flag7, g_sensor_payload.alarm_ll,
				g_sensor_payload.data_flag8, g_sensor_payload.data_flag9, g_sensor_payload.valid);
		MYLOG("APP", "%s", payload_log);
#endif
		// Send button status
		lmh_error_status result = send_lora_packet((uint8_t *)&g_sensor_payload, PAYLOAD_LENGTH);
		switch (result)
		{
		case LMH_SUCCESS:
			MYLOG("APP", "Packet enqueued");
			break;
		case LMH_BUSY:
			MYLOG("APP", "LoRa transceiver is busy");
			break;
		case LMH_ERROR:
			MYLOG("APP", "Packet error, too big to send with current DR");
			break;
		}
	}

	// ACC trigger event
	if ((g_task_event_type & ACC_TRIGGER) == ACC_TRIGGER)
	{
		g_task_event_type &= N_ACC_TRIGGER;
		MYLOG("APP", "ACC triggered");

		read_acc();
		clear_acc_int();
		restart_advertising(15);
	}
}

/**
 * @brief Handle BLE UART data
 * 
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		// BLE UART data handling
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			/** BLE UART data arrived */
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}

/**
 * @brief Handle received LoRa Data
 * 
 */
void lora_data_handler(void)
{

	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");
		}
		else
		{
			MYLOG("APP", "Join network failed");
			/// \todo here join could be restarted.
			lmh_join();

			// If BLE is enabled, restart Advertising
			if (g_enable_ble)
			{
				restart_advertising(15);
			}
		}
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		MYLOG("APP", "LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");

		if (!g_rx_fin_result)
		{
			// Increase fail send counter
			send_fail++;

			if (send_fail == 10)
			{
				// Too many failed sendings, reset node and try to rejoin
				delay(100);
				sd_nvic_SystemReset();
			}
		}
	}

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo LoRa data arrived
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		char log_buff[g_rx_data_len * 3] = {0};
		uint8_t log_idx = 0;
		for (int idx = 0; idx < g_rx_data_len; idx++)
		{
			sprintf(&log_buff[log_idx], "%02X ", g_rx_lora_data[idx]);
			log_idx += 3;
		}
		MYLOG("APP", "%s", log_buff);
	}
}
