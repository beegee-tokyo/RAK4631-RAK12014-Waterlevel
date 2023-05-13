/**
 * @file ir_tof.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialize and read information from ITR20001T sensor
 * @version 0.1
 * @date 2021-09-22
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "app.h"

/** Instance of sensor class */
VL53L0X tof_sensor;

/** Structure to parse uint16_t into a uin8_t array */
analog_s analog_val;

/** Overflow threshold for alarm */
uint16_t overflow_treshold = 20;

/** Low level treshold for alarm */
uint16_t lowlevel_treshold = 1000;

/** Counter for overflow detection, to smoothen the signal, overflow has to be triggered 5 times in a row before the alarm is set */
uint8_t overflow_counter = 0;

/** Counter for low level detection, to smoothen the signal, low level has to be triggered 10 times in a row before the alarm is set */
uint8_t lowlevel_counter = 0;

/** Switch between usage of on/off pin 0 and power on/off 1 */
#define POWER_OFF 1

// Power pin for RAK12014
uint8_t xshut_pin = WB_IO3;

/**
 * @brief INitialize the VL53L01 sensor
 *
 * @return true If sensor was found
 * @return false If sensor is not connected
 */
bool init_tof(void)
{
	// Enable module power
	digitalWrite(WB_IO2, HIGH);

	// On/Off control pin
	pinMode(xshut_pin, OUTPUT);

	// Sensor on
	digitalWrite(xshut_pin, HIGH);

	// Wait for sensor wake-up
	delay(150);

	tof_sensor.setTimeout(500);
	if (!tof_sensor.init())
	{
		MYLOG("ToF", "Failed to detect and initialize sensor!");
		return false;
	}

#if defined LONG_RANGE
	// lower the return signal rate limit (default is 0.25 MCPS)
	tof_sensor.setSignalRateLimit(0.1);
	// increase laser pulse periods (defaults are 14 and 10 PCLKs)
	tof_sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
	tof_sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
#endif

#if defined HIGH_SPEED
	// reduce timing budget to 20 ms (default is about 33 ms)
	tof_sensor.setMeasurementTimingBudget(20000);
#elif defined HIGH_ACCURACY
	// increase timing budget to 200 ms
	tof_sensor.setMeasurementTimingBudget(200000);
#endif

	// Sensor off
	digitalWrite(xshut_pin, LOW);
	return true;
}

/**
 * @brief Get the measured distance
 * 		It is not the real water level, it is the distance between the sensor
 * 		and the water surface. The water level is calculated in the data
 * 		integration of the LPWAN server.
 * 		Measured data is written directly into the LPWAN packet.
 *
 */
void get_water_level(void)
{
#if POWER_OFF == 1
	init_tof();
#endif

	// Sensor on
	digitalWrite(xshut_pin, HIGH);
	delay(300);

	uint64_t collected = 0;
	bool got_valid_data = false;
	// uint64_t single_reading = (uint64_t)tof_sensor.readRangeContinuousMillimeters();
	uint64_t single_reading = (uint64_t)tof_sensor.readRangeSingleMillimeters();
	if (tof_sensor.timeoutOccurred() || (single_reading == 65535))
	{
		collected = analog_val.analog16;
		MYLOG("ToF", "Timeout");
	}
	else
	{
		// We are measuring against water surface, sometimes waves or reflections can give too high values
		// The tank is only ~1100mm deep, so any value above should be discarded
		if (single_reading < 1100)
		{
			collected = single_reading;
			// We got at least one measurement
			got_valid_data = true;
		}
		else
		{
			collected = analog_val.analog16;
			// We got at least one measurement
			got_valid_data = true;
			MYLOG("ToF", "Measured > 1100mm");
		}
	}
	delay(100);

	for (int reading = 0; reading < 10; reading++)
	{
		single_reading = (uint64_t)tof_sensor.readRangeSingleMillimeters();
		if (tof_sensor.timeoutOccurred() || (single_reading == 65535))
		{
			MYLOG("ToF", "Timeout");
		}
		else
		{
			// We are measuring against water surface, sometimes waves or reflections can give too high values
			// The tank is only ~1100mm deep, so any value above should be discarded
			if (single_reading < 1100)
			{
				collected += single_reading;
				collected = collected / 2;
				// We got at least one measurement
				got_valid_data = true;
			}
		}
		delay(100);
	}

	// If we failed to get a valid reading, we set it to the last measured value
	g_solution_data.addPresence(LPP_CHANNEL_WL_VALID, got_valid_data);

	MYLOG("ToF", "Measured distance %d mm", (uint16_t)collected);

	MYLOG("ToF", "Water level %d mm", 1100 - (uint16_t)collected);

	// For now we use fixed 1100mm as tank depth, should be made flexible!
	uint16_t new_level = 1100 - collected;

	// Add level to the payload in cm
	g_solution_data.addAnalogInput(LPP_CHANNEL_WLEVEL, (float)(new_level / 10));

	// Just in case next readings go wrong
	analog_val.analog16 = collected;

	// Check for overflow
	if (collected < overflow_treshold)
	{
		overflow_counter++;
		if (overflow_counter == 5)
		{
			g_solution_data.addPresence(LPP_CHANNEL_WL_HIGH, true);
			overflow_counter = 0;
		}
		else
		{
			g_solution_data.addPresence(LPP_CHANNEL_WL_HIGH, false);
		}
	}
	else
	{
		g_solution_data.addPresence(LPP_CHANNEL_WL_HIGH, false);
		overflow_counter = 0;
	}

	// Check for water outage
	if (collected > lowlevel_treshold)
	{
		lowlevel_counter++;
		if (lowlevel_counter == 10)
		{
			g_solution_data.addPresence(LPP_CHANNEL_WL_LOW, true);
			lowlevel_counter = 0;
		}
		else
		{
			g_solution_data.addPresence(LPP_CHANNEL_WL_LOW, false);
		}
	}
	else
	{
		g_solution_data.addPresence(LPP_CHANNEL_WL_LOW, false);
		lowlevel_counter = 0;
	}

	// Sensor off
	digitalWrite(xshut_pin, LOW);

#if POWER_OFF == 1
	// Disable power for IO slot
	digitalWrite(WB_IO2, LOW);
#endif
}
