#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"

const char number[7] = "1109998";
volatile uint32_t* leds[4] = {&PC15, &PC14, &PC13, &PC12};

void UpdateAllLEDs(uint32_t toggle) {
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = !toggle;
}

void UpdateLEDs(uint32_t pos) {
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = pos == i ? 0 : 1;
}

void Init_GPIO(void) {
	GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
	UpdateAllLEDs(0);
}

void SetLEDs(char digit) {
	if (digit == '0') {
		UpdateAllLEDs(1);
		return;
	}
	
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = ~((digit >> i) & 1);
}

int main(void)
{
	uint32_t i;
  SYS_Init();
	OpenKeyPad();
	Init_GPIO();
		
	while (1) {
		i = ScanKey();
		if (i >= 1 && i <= 7) {
			SetLEDs(number[i - 1]);
		} else {
			UpdateAllLEDs(0);
		}
	}
}
