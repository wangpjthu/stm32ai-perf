/*
Copyright 2020 EEMBC and The MLPerf Authors. All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file reflects a modified version of th_lib from EEMBC. The reporting logic
in th_results is copied from the original in EEMBC.
==============================================================================*/
/// \file
/// \brief C++ implementations of submitter_implemented.h

#include "submitter_implemented.h"
#include "st_port.h"

extern uint8_t gp_buff[MAX_DB_INPUT_SIZE];
extern size_t g_buff_size;
extern size_t g_buff_pos;

int8_t aiInData_int[AI_NETWORK_IN_1_SIZE];
int8_t aiOutData[AI_NETWORK_OUT_1_SIZE];


// Implement this method to prepare for inference and preprocess inputs.
void th_load_tensor(void)
{
		size_t x;
	    size_t y;
	    for (x = y = 0; x < AI_NETWORK_IN_1_SIZE; ++x, ++y)
	    {
	        if (y == g_buff_size)
	        {
	            y = 0;
	        }
	        if (gp_buff[y] <= 127)
	        {
	        	aiInData_int[x] = ((int8_t)gp_buff[y]) - 128;
	        }
	        else
	        {
	        aiInData_int[x] = (int8_t)(gp_buff[y] - 128);
	        }
	    }
}

// Add to this method to return real inference results.
void th_results(void)
{
	int8_t outt_zero_point = -128;
	float converted=0.0;
		float outt_scale = 0.00390625;
	    const int nresults = 10;
	    char char_res[10];
	    /**
	     * The results need to be printed back in exactly this format; if easier
	     * to just modify this loop than copy to results[] above, do that.
	     */
	    th_printf("m-results-[");
	    int kCategoryCount = 10;

	    for (size_t i = 0; i < kCategoryCount; i++)
	    {
	        converted = Deq(aiOutData[i], outt_scale,outt_zero_point);
	        snprintf(char_res, 10, "%0.3f",converted);
	        th_printf(char_res);
	        if (i < (nresults - 1))
	        {
	              th_printf(",");
	        }
	    }
	    th_printf("]\r\n");
}

// Implement this method with the logic to perform one inference cycle.
void th_infer(void)
{
	AI_Run(aiInData_int, aiOutData);
}

/// \brief optional API.
void th_final_initialize(void)
{
	rxInit();
	AI_Init();
	/* Clock gating */

	__HAL_RCC_AXIRAM1_CLKGA_ENABLE();
    __HAL_RCC_AXIRAM2_CLKGA_ENABLE();
   	__HAL_RCC_AXIRAM3_CLKGA_ENABLE();
	__HAL_RCC_AHB12_CLKGA_ENABLE();
	__HAL_RCC_AHB34_CLKGA_ENABLE();
	__HAL_RCC_FLIFT_CLKGA_ENABLE();
	__HAL_RCC_ECCRAM_CLKGA_ENABLE();

	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOF_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();


#if EE_CFG_ENERGY_MODE == 1
	__HAL_RCC_GPIOD_CLK_DISABLE();
#endif


}
void th_pre() {
	#if EE_CFG_ENERGY_MODE == 1
		__HAL_RCC_USART1_CLK_DISABLE();
		__HAL_RCC_TIM5_CLK_DISABLE();
   #endif
	}

void th_post() {
	#if EE_CFG_ENERGY_MODE == 1
		__HAL_RCC_USART1_CLK_ENABLE();
		__HAL_RCC_TIM5_CLK_ENABLE();
	#endif

}

char g_cmd_buf_copy[EE_CMD_SIZE + 1];
void th_command_ready(char *p_command)
{
	if (p_command[0])
	{
		th_memcpy(g_cmd_buf_copy, p_command, EE_CMD_SIZE);
		ee_serial_command_parser_callback(g_cmd_buf_copy);
		p_command[0] = 0;
	}
}

// th_libc implementations.
int th_strncmp(const char *str1, const char *str2, size_t n) {
  return strncmp(str1, str2, n);
}

char *th_strncpy(char *dest, const char *src, size_t n) {
  return strncpy(dest, src, n);
}

size_t th_strnlen(const char *str, size_t maxlen) {
  return strnlen(str, maxlen);
}

char *th_strcat(char *dest, const char *src) { return strcat(dest, src); }

char *th_strtok(char *str1, const char *sep) { return strtok(str1, sep); }

int th_atoi(const char *str) { return atoi(str); }

void *th_memset(void *b, int c, size_t len) { return memset(b, c, len); }

void *th_memcpy(void *dst, const void *src, size_t n) {
  return memcpy(dst, src, n);
}

/* N.B.: Many embedded *printf SDKs do not support all format specifiers. */
int th_vprintf(const char *format, va_list ap) { return vprintf(format, ap); }

uint8_t buffer [128];
void th_printf(const char* fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vsnprintf ((char*)buffer, 128, fmt, ap);
	va_end(ap);
	//notify_uart(buffer, n);
#if EE_CFG_ENERGY_MODE == 0
	if(HAL_UART_Transmit(&huart3, (uint8_t*)buffer, n, HAL_MAX_DELAY) != HAL_OK)
	{
		Error_Handler();
	}
#else
	if(HAL_UART_Transmit(&huart1, (uint8_t*)buffer, n, HAL_MAX_DELAY) != HAL_OK)
	{
		Error_Handler();
	}
#endif

}

char th_getchar() { return getchar(); }

void th_serialport_initialize(void)
{
#if EE_CFG_ENERGY_MODE == 0
	/* In performance mode, we talk directly to the DUT at 115200 baud */
	huart3.Init.BaudRate = 115200;
#else
	/* In energy mode, we talk to the DUT through the IO Manager at 9600 baud */
	huart1.Init.BaudRate = 9600;
#endif
}

void th_timestamp(void)
{
#if EE_CFG_ENERGY_MODE == 1
/* USER CODE 1 BEGIN */
/* Step 1. Pull pin low */
	clear_Timestamp_Port();
/* Step 2. Hold low for at least 1us */
	US_Delay(1);
	set_Timestamp_Port();
/* Step 3. Release driver */
/* USER CODE 1 END */
#else

    /* USER CODE 2 BEGIN */
	unsigned long microSeconds = __HAL_TIM_GET_COUNTER(&htim5);
    /* USER CODE 2 END */
    /* This message must NOT be changed. */
    th_printf(EE_MSG_TIMESTAMP, microSeconds);
#endif
}

void th_timestamp_initialize(void)
{
    th_printf(EE_MSG_TIMESTAMP_MODE);
    th_timestamp();

}
