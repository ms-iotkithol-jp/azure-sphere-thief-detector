#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "mt3620.h"

#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"

#include "os_hal_uart.h"
#include "os_hal_gpt.h"

#include "CaptureTask.h"
#include "CalculateTask.h"
#include "MboxTask.h"

static constexpr UART_PORT STDOUT_PORT = OS_HAL_UART_ISU3;

void* __dso_handle = (void*)&__dso_handle;

////////////////////////////////////////////////////////////////////////////////
// Semihosting

static void SemihostingInit()
{
	mtk_os_hal_uart_ctlr_init(STDOUT_PORT);
}

void _putchar(char c)
{
	if (c == '\n') mtk_os_hal_uart_put_char(STDOUT_PORT, '\r');
	mtk_os_hal_uart_put_char(STDOUT_PORT, c);
}

////////////////////////////////////////////////////////////////////////////////
// Applicaiton Hooks

extern "C"
{
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName);
void vApplicationMallocFailedHook();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	printf("%s: %s\n", __func__, pcTaskName);
}

void vApplicationMallocFailedHook()
{
	printf("%s\n", __func__);
}

////////////////////////////////////////////////////////////////////////////////
// Main

extern "C"
{
_Noreturn void RTCoreMain();
}

_Noreturn void RTCoreMain()
{
	NVIC_SetupVectorTable();
	SemihostingInit();

	mtk_os_hal_gpt_init();

	printf("\n\n<<< sampling-rt >>>\n");

	xTaskCreate(MboxTask, "Mailbox Task", 1024 / 2, nullptr, 0, nullptr);
//	xTaskCreate(CalculateTask, "Calculate Task", 1024 / 4, nullptr, 1, nullptr);
	xTaskCreate(CaptureTask, "Capture Task", 1024 / 2, nullptr, 2, nullptr);

	vTaskStartScheduler();
	for (;;)
		__asm__("wfi");
}

////////////////////////////////////////////////////////////////////////////////
