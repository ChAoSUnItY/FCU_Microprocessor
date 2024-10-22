#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

#define PHASE_US 300000

static bool rand_active = false;
static bool calc_active = false;
static uint32_t num = 0;
static uint32_t result = 0;

void InitBuzzer()
{
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
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
            last_state = read2;

        if (last_state)
            func(last_state);
    }
}

void ShowSevenSegments(uint32_t num);

void Execute(uint32_t key)
{
    switch (key)
    {
    case 1:
    {
        if (!rand_active)
            return;

        calc_active = true;
        uint32_t a = num / 100, b = num % 100;
        result = a + b;
        break;
    }
    case 2:
    {
        if (!rand_active)
            return;

        calc_active = true;
        uint32_t a = num / 100, b = num % 100;
        result = a > b ? a - b : b - a;
        
        if (b > a) {
            ShowSevenSegments(result);

            for (size_t i = 0; i < 2; i++) {
                PB11 = 0;
                CLK_SysTickDelay(100000);
                PB11 = 1;
                CLK_SysTickDelay(100000);
            }
        }
        break;
    }
    case 8:
    {
        result = num = calc_active = rand_active = 0;
        CloseSevenSegment();
        break;
    }
    case 9:
    {
#define LOCAL_RAND_MIN 0
#define LOCAL_RAND_MAX 9
        calc_active = false;
        num = 0;

        for (size_t i = 0; i < 4; i++)
        {
            num += rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;

            if (i != 3)
                num *= 10;
        }

        rand_active = true;
        break;
    }
    }
}

void ShowSevenSegments(uint32_t num)
{
    for (int i = 0; i < 4; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, num % 10);
        CLK_SysTickDelay(5000);
        num /= 10;
    }
}

void Show()
{
    if (calc_active) {
        ShowSevenSegments(result);
        return;
    }

    if (rand_active) {
        ShowSevenSegments(num);
        return;
    }
}

int main(void)
{
    uint32_t count = 0;
    SYS_Init();
    OpenKeyPad();
    OpenSevenSegment();
    InitBuzzer();

    while (1)
    {
        count++;
        KeyPadRisingEdge(Execute);
        Show();
        srand(count);
    }
}
