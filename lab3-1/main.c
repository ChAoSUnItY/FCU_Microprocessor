#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"

#define DO_TONE 956

volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};

void UpdateAllLEDs(uint32_t toggle)
{
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
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
    if (digit == '0')
    {
        UpdateAllLEDs(1);
        return;
    }

    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = ~((digit >> i) & 1);
}

void InitBuzzer()
{
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
}

void Buzz(size_t times)
{
    for (size_t i = 0; i < times; i++)
    {
        PB11 = 0;
        CLK_SysTickDelay(100000);
        PB11 = 1;
        CLK_SysTickDelay(100000);
    }
    CLK_SysTickDelay(1000);
}

void Execute(uint32_t key)
{
    if (key >= 1 && key <= 6)
    {
        SetLEDs(key);
        Buzz(key);
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

int main(void)
{
    SYS_Init();
    Init_GPIO();
    InitBuzzer();
    OpenKeyPad();

    while (1)
    {
        KeyPadRisingEdge(Execute);
    }
}
