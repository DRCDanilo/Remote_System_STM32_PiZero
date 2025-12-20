/*
 * shell.c
 *
 *  Created on: Dec 1, 2025
 *      Author: david
 */

#include "shell.h"
#include <stdlib.h>
#include <stdint.h>
#include "BMP280.h"


h_shell_t hshell1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

static char shell_uart2_received_char;


/**
 * @brief Transmit function for shell driver using UART2
 */



/**
 * @brief Checks if a character is valid for a shell command.
 *
 * This function checks if a character is valid for a shell command. Valid characters include alphanumeric characters and spaces.
 *
 * @param c The character to check.
 * @return 1 if the character is valid, 0 otherwise.
 */
static int is_character_valid(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == ' ') || (c == '=');
}

static int is_string_valid(char* str)
{
	int reading_head = 0;
	while(str[reading_head] != '\0'){
		//		char c = str[reading_head];
		if(!is_character_valid(str[reading_head])){
			if(reading_head == 0){
				return 0;
			}
			else{
				str[reading_head] = '\0';
				return 1;
			}
		}
		reading_head++;
	}
	return 1;
}

/**
 * @brief Help command implementation.
 *
 * This function is the implementation of the help command. It displays the list of available shell commands and their descriptions.
 *
 * @param h_shell The pointer to the shell instance.
 * @param argc The number of command arguments.
 * @param argv The array of command arguments.
 * @return 0 on success.
 */
static int sh_help(h_shell_t* h_shell, int argc, char** argv)
{
	int i, size;
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Code \t | Description \r\n");
	h_shell->drv.transmit(h_shell->print_buffer, size);
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "----------------------\r\n");
	h_shell->drv.transmit(h_shell->print_buffer, size);

	for (i = 0; i < h_shell->func_list_size; i++){
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%s \t | %s\r\n", h_shell->func_list[i].string_func_code, h_shell->func_list[i].description);
		h_shell->drv.transmit(h_shell->print_buffer, size);
	}
	return 0;
}

static int sh_test_list(h_shell_t* h_shell, int argc, char** argv)
{
	int size;
	for(int arg=0; arg<argc; arg++){
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Arg %d \t %s\r\n", arg, argv[arg]);
		h_shell->drv.transmit(h_shell->print_buffer, size);
	}
	return 0;
}


uint8_t shell_uart2_transmit(const char *pData, uint16_t size)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)pData, size, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t *)pData, size, HAL_MAX_DELAY);
	return size;
}

uint8_t shell_uart2_receive(char *pData, uint16_t size)
{
	*pData = shell_uart2_received_char;
	return 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1 || huart->Instance == USART2) {
		printf("pitieeeeeeeeeeee\r\n");
		//		HAL_UART_Transmit(&huart2, (uint8_t *)&shell_uart2_received_char, 1, HAL_MAX_DELAY);
		HAL_UART_Receive_IT(huart, (uint8_t *)&shell_uart2_received_char, 1);
		shell_run(&hshell1);
	}
}

void loop(){


}

void init_device(void){
// Initialisation user interface
	// SHELL
	hshell1.drv.transmit = shell_uart2_transmit;
	hshell1.drv.receive = shell_uart2_receive;
	shell_init(&hshell1);
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&shell_uart2_received_char, 1);
	HAL_UART_Receive_IT(&huart1, (uint8_t *)&shell_uart2_received_char, 1);

}




static int sh_get_temp(h_shell_t* h_shell, int argc, char** argv)
{
	uint32_t comp;
    uint8_t data[3];
    BMP280_TempMes(data);

    comp = (((uint32_t)data[0] << 16) | (uint32_t)data[1] << 8 | (uint32_t)data[2]);

    // Respond with a confirmation
    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Temperature value: %luH\r\n", comp);
    h_shell->drv.transmit(h_shell->print_buffer, size);
}


static int sh_get_pres(h_shell_t* h_shell, int argc, char** argv)
{
	uint32_t comp;
    uint8_t data[3];
    BMP280_PresMes(data);

    comp = (((uint32_t)data[0] << 16) | (uint32_t)data[1] << 8 | (uint32_t)data[2]);

    // Respond with a confirmation
    int size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "Temperature value: %luH\r\n", comp);
    h_shell->drv.transmit(h_shell->print_buffer, size);
}



/**
 * @brief Initializes the shell instance.
 *
 * This function initializes the shell instance by setting up the internal data structures and registering the help command.
 *
 * @param h_shell The pointer to the shell instance.
 */
void shell_init(h_shell_t* h_shell)
{
	int size = 0;

	h_shell->func_list_size = 0;

	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n=> Monsieur Shell v0.2.2 without FreeRTOS <=\r\n");
	h_shell->drv.transmit(h_shell->print_buffer, size);
	h_shell->drv.transmit(PROMPT, sizeof(PROMPT));

	shell_add(h_shell, "help", sh_help, "Help");
	shell_add(h_shell, "test", sh_test_list, "Test list");
	shell_add(h_shell, "GETT", sh_get_temp, "Get temperature");
	shell_add(h_shell, "GETP", sh_get_pres, "Get pressure");

}

/**
 * @brief Adds a shell command to the instance.
 *
 * This function adds a shell command to the shell instance.
 *
 * @param h_shell The pointer to the shell instance.
 * @param c The character trigger for the command.
 * @param pfunc Pointer to the function implementing the command.
 * @param description The description of the command.
 * @return 0 on success, or a negative error code on failure.
 */
int shell_add(h_shell_t* h_shell, char* string_func_code, shell_func_pointer_t pfunc, char* description)
{
	if(is_string_valid(string_func_code))
	{
		if (h_shell->func_list_size < SHELL_FUNC_LIST_MAX_SIZE)
		{
			h_shell->func_list[h_shell->func_list_size].string_func_code = string_func_code;
			h_shell->func_list[h_shell->func_list_size].func = pfunc;
			h_shell->func_list[h_shell->func_list_size].description = description;
			h_shell->func_list_size++;
			return 0;
		}
	}
	return -1;
}

/**
 * @brief Executes a shell command.
 *
 * This function executes a shell command based on the input buffer.
 *
 * @param h_shell The pointer to the shell instance.
 * @param buf The input buffer containing the command.
 * @return 0 on success, or a negative error code on failure.
 */
static int shell_exec(h_shell_t* h_shell, char* buf)
{
	int i, argc;
	char* argv[SHELL_ARGC_MAX];
	char* p;

	// Create argc, argv**
	argc = 1;
	argv[0] = buf;
	for (p = buf; *p != '\0' && argc < SHELL_ARGC_MAX; p++)
	{
		if (*p == ' ')
		{
			*p = '\0';
			argv[argc++] = p + 1;
		}
	}

	for (i = 0; i < h_shell->func_list_size; i++)
	{
		if(strcmp(h_shell->func_list[i].string_func_code, argv[0])==0)
		{

			return h_shell->func_list[i].func(h_shell, argc, argv);
		}
	}

	int size;
	size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "%s : no such command\r\n", argv[0]);
	h_shell->drv.transmit(h_shell->print_buffer, size);
	return -1;
}

/**
 * @brief Runs the shell.
 *
 * This function runs the shell, processing user commands.
 *
 * @param h_shell The pointer to the shell instance.
 * @return Never returns, it's an infinite loop.
 */
int shell_run(h_shell_t* h_shell)
{
	static int cmd_buffer_index;
	char c;
	int size;

	h_shell->drv.receive(&c, 1);

	switch(c)
	{
	case '\r': // Process RETURN key
	case '\n':
		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, "\r\n");
		h_shell->drv.transmit(h_shell->print_buffer, size);
		h_shell->cmd_buffer[cmd_buffer_index++] = 0; // Add '\0' char at the end of the string
		//		size = snprintf(h_shell->print_buffer, SHELL_PRINT_BUFFER_SIZE, ":%s\r\n", h_shell->cmd_buffer);
		//		h_shell->drv.transmit(h_shell->print_buffer, size);
		cmd_buffer_index = 0; // Reset buffer
		shell_exec(h_shell, h_shell->cmd_buffer);
		h_shell->drv.transmit(PROMPT, sizeof(PROMPT));
		break;

	case '\b': // Backspace
		if (cmd_buffer_index > 0) // Is there a character to delete?
		{
			h_shell->cmd_buffer[cmd_buffer_index] = '\0'; // Removes character from the buffer, '\0' character is required for shell_exec to work
			cmd_buffer_index--;
			h_shell->drv.transmit("\b \b", 3); // "Deletes" the character on the terminal
		}
		break;

	default: // Other characters
		// Only store characters if the buffer has space
		if (cmd_buffer_index < SHELL_CMD_BUFFER_SIZE)
		{
			if (is_character_valid(c))
			{
				h_shell->drv.transmit(&c, 1); // echo
				h_shell->cmd_buffer[cmd_buffer_index++] = c; // Store
			}
		}
	}
	return 0;
}
