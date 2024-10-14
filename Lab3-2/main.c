#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

#define DO_TONE 956
#define SI_TONE 506

volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};

static bool active = false;
static uint32_t random_number = 0;
static uint32_t cycle = 0;
static bool wave = false;

void UpdateAllLEDs(bool toggle)
{
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

void UpdateLED(size_t idx, uint32_t toggle)
{
    if (idx > 3)
        return;
    *(leds[idx]) = !toggle;
}

void Init_GPIO(void)
{
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
}

void InitBuzzer()
{
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
}

void Buzz(bool toggle, uint32_t tone)
{
    if (!toggle)
        return;
    PB11 = 1;
    CLK_SysTickDelay(tone);
    PB11 = 0;
    CLK_SysTickDelay(tone);
}

void ShowSevenSegmentNum(size_t idx, uint32_t num)
{
    CloseSevenSegment();
    ShowSevenSegment(idx, num);
    CLK_SysTickDelay(5000);
}

// This will delay in total 20000 us (or 0.02 s)
void ShowSevenSegmentNums()
{
    for (size_t i = 0; i < 4; i++)
    {
        switch (i)
        {
        case 3:
        {
            ShowSevenSegmentNum(i, random_number % 3);
            break;
        }
        case 2:
        {
            break;
        }
        case 1:
        {
            ShowSevenSegmentNum(i, random_number / 10);
            break;
        }
        case 0:
        {
            ShowSevenSegmentNum(i, random_number % 10);
            break;
        }
        }
    }
}

void BuzzNew(size_t count, uint32_t tone)
{
    size_t cur_typ = 0;

    for (size_t i = 0; i < count; i++)
    {
        PB11 = 1;
        CLK_SysTickDelay(tone);
        PB11 = 0;
        CLK_SysTickDelay(tone);

        switch (cur_typ)
        {
        case 0:
        {
            CloseSevenSegment();
            ShowSevenSegment(3, random_number % 3);
            break;
        }
        case 1:
        {
            CloseSevenSegment();
            ShowSevenSegment(1, random_number / 10);
            break;
        }
        case 2:
        {
            CloseSevenSegment();
            ShowSevenSegment(0, random_number % 10);
            break;
        }
        }
        cur_typ = (cur_typ + 1) % 3;
    }
}

// An execute cycle is based on random number:
// if random_number % 3 == 0, then it will delay 0.02s
// if random_number % 3 == 1, then it will delay 0.020956s
// if random_number % 3 == 2, then it will delay 0.020956s when in first-half cycle
//                                             , 0.020506s when in last-half cycle
void Execute(uint32_t key)
{
    switch (key)
    {
    case 1:
    {
        for (size_t i = 0; i < random_number; i++)
        {
            PB11 = 0;
            CLK_SysTickDelay(100000);
            PB11 = 1;
            CLK_SysTickDelay(100000);
        }
        CLK_SysTickDelay(1000);
        return;
    }
    case 8:
    {
        cycle = random_number = active = false;
        UpdateAllLEDs(0);
        PB11 = 1;
        CloseSevenSegment();
        break;
    }
    case 9:
    {
#define LOCAL_RAND_MIN 00
#define LOCAL_RAND_MAX 99
        random_number = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;
        active = true;
        cycle = 0;
        UpdateAllLEDs(0);
        PB11 = 1;
        CloseSevenSegment();
        break;
    }
    }
}

void Show()
{
    if (!active)
        return;

    switch (random_number % 3)
    {
    case 0:
    {
        UpdateAllLEDs(cycle < 25);
        break;
    }
    case 1:
    {
        if (cycle < 25)
        {
            Buzz(cycle, DO_TONE);
            UpdateLED(3, 1);
        }
        else
        {
            PB11 = 1;
            UpdateLED(3, 0);
        }
        break;
    }
    case 2:
    {
        uint32_t tone = cycle < 25 ? DO_TONE : SI_TONE;
        size_t count = 500000 / (2 * tone);
        BuzzNew(count, tone);

        if (cycle < 25)
        {
            UpdateLED(3, 0);
            UpdateLED(0, 1);
        }
        else
        {
            UpdateLED(3, 1);
            UpdateLED(0, 0);
        }
        cycle = (cycle + 25) % 50;
        return;
    }
    }

    ShowSevenSegmentNums();
    cycle = (cycle + 1) % 50;
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

int main(void)
{
    uint32_t count = 0;
    SYS_Init();
    Init_GPIO();
    InitBuzzer();
    OpenKeyPad();

    while (1)
    {
        count++;
        KeyPadRisingEdge(Execute);
        Show();
        srand(count);
    }
}
