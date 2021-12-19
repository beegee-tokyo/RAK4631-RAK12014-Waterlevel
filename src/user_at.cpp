/**
 * @file user_at.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Custom AT command extension for WisBlock API
 * @version 0.1
 * @date 2021-12-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "app.h"
#include <at_cmd.h>

/*********************************************************************/
// Example AT command to change the value of the variable new_val:
// Query the value AT+SETVAL=?
// Set the value AT+SETVAL=120000
/*********************************************************************/
int32_t new_val = 3000;

/**
 * @brief Called by AT command handler if a custom AT command was received
 * 
 * @param user_cmd The received AT command (Without the "AT")
 * @param cmd_size The length of the received AT command
 * @return true If custom AT command could be handled
 * @return false If it was an unknown AT command
 */
bool user_at_handler(char *user_cmd, uint8_t cmd_size)
{
	MYLOG("APP", "Received User AT commmand >>%s<< len %d", user_cmd, cmd_size);

	// Get the command itself
	char *param;

	param = strtok(user_cmd, "=");
	MYLOG("APP", "Commmand >>%s<<", param);

	// Check if the command is supported
	if (strcmp(param, (const char *)"+OVFL") == 0)
	{
		// check if it is query or set
		param = strtok(NULL, ":");
		MYLOG("APP", "Param string >>%s<<", param);

		if (strcmp(param, (const char *)"?") == 0)
		{
			// It is a query, use AT_PRINTF to respond
			AT_PRINTF("OVFL: %d", overflow_treshold);
		}
		else
		{
			new_val = strtol(param, NULL, 0);
			if ((new_val > 20) && (new_val < 1000))
			{
				overflow_treshold = new_val;
				MYLOG("APP", "OVFL:%d", overflow_treshold);
				AT_PRINTF("OVFL: %d", overflow_treshold);
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	// Check if the command is supported
	if (strcmp(param, (const char *)"+LOLV") == 0)
	{
		// check if it is query or set
		param = strtok(NULL, ":");
		MYLOG("APP", "Param string >>%s<<", param);

		if (strcmp(param, (const char *)"?") == 0)
		{
			// It is a query, use AT_PRINTF to respond
			AT_PRINTF("LOLV: %d", lowlevel_treshold);
		}
		else
		{
			new_val = strtol(param, NULL, 0);
			if ((new_val > 20) && (new_val < 1000))
			{
				lowlevel_treshold = new_val;
				MYLOG("APP", "OVFL:%d", lowlevel_treshold);
				AT_PRINTF("LOLV: %d", lowlevel_treshold);
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}
