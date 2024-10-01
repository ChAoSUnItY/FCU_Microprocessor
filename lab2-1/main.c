#include <stdio.h>
#include <stdlib.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};

void UpdateAllLEDs(uint32_t toggle)
{
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = !toggle;
}

void UpdateLEDs(uint32_t pos)
{
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = pos == i ? 0 : 1;
}

void Init_GPIO(void)
{
	GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
	UpdateAllLEDs(0);
}

void SetLEDs(char digit)
{
	for (uint32_t i = 0; i < 4; i++)
		*(leds[i]) = ~((digit >> i) & 1);
}

void ShowSevenSegmentsByDecimal(uint32_t num)
{
	for (int i = 0; i < 4; i++)
	{
		CloseSevenSegment();
		ShowSevenSegment(i, num % 10);
		CLK_SysTickDelay(5000);

		num /= 10;
	}
}

void KeyPadRisingEdge(void (*func)(uint32_t))
{
	static uint32_t last_state = 0;
	uint32_t read1 = ScanKey();

	if (last_state != read1)
	{
		// Pressed
		CLK_SysTickDelay(25000);

		uint32_t read2 = ScanKey();

		if (read2 == read1)
		{
			last_state = read2;
		}

		if (last_state)
		{
			func(last_state);
		}
	}
}

static uint32_t random_number = 1111;

void Execute(uint32_t key)
{
	switch (key)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	{
		uint32_t digit = random_number;

		key--;
		while (key--)
			digit /= 10;

		SetLEDs(digit %= 10);
		break;
	}
	case 9:
	{
#define LOCAL_RAND_MIN 1
#define LOCAL_RAND_MAX 9
		random_number = 0;

		for (size_t i = 0; i < 4; i++)
		{
			random_number += rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;

			if (i != 3)
				random_number *= 10;
		}

		break;
	}
	}
}

int main(void)
{
	uint32_t count = 0;
	SYS_Init();
	OpenKeyPad();
	OpenSevenSegment();
	Init_GPIO();

	while (1)
	{
		count++;
		ShowSevenSegmentsByDecimal(random_number);
		KeyPadRisingEdge(Execute);
		srand(count);
	}
}
