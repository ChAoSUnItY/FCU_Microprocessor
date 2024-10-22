#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"
#include "LCD.h"

static uint32_t pass[4] = {0};
static bool pass_init = false;
static uint32_t input[4] = {0};
static size_t len = 0;
static uint32_t try_cnt = 0;
// LED
static uint32_t cycle = 0;
static char pat[13][5] = {
    "1000",
    "0100",
    "0010",
    "0001",
    "0010",
    "0100",
    "1000",
    "0100",
    "0010",
    "0001",
    "0010",
    "0100",
    "1000"};
static bool enable = false;

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

void Execute(uint32_t key)
{
    switch (key)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    {
        if (try_cnt == 4)
            return;

        if (len == 4)
            return;

        input[len++] = key;

        char buf[17] = "                ";
        for (size_t i = 0; i < len; i++)
            sprintf(buf + i, "%d", input[i]);
        print_Line(try_cnt, buf);
        break;
    }
    case 7:
    {
#define LOCAL_RAND_MIN 1
#define LOCAL_RAND_MAX 6
        pass_init = true;

        for (size_t i = 0; i < 4; i++)
            pass[i] = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;

        break;
    }
    case 8:
    {
        clear_LCD();
        try_cnt = 0;
        memset(input, 0, 4);
        len = 0;
        break;
    }
    case 9:
    {
        if (try_cnt == 4)
            return;

        char buf[17] = "                ";
        for (size_t i = 0; i < len; i++)
            sprintf(buf + i, "%d", input[i]);

        bool fail = false;

        for (size_t i = 0; i < 4; i++)
        {
            if (input[i] != pass[i])
            {
                fail = true;
                break;
            }
        }

        if (len < 4)
        {
            sprintf(buf + 4, "       %-5s", "NULL");
            print_Line(try_cnt++, buf);
            CLK_SysTickDelay(5000);
        }
        else if (fail)
        {
            sprintf(buf + 4, "       %-5s", "ERROR");
            print_Line(try_cnt++, buf);
            CLK_SysTickDelay(5000);
            Buzz(1);
        }
        else
        {
            sprintf(buf + 4, "       %-5s", "PASS");
            print_Line(try_cnt++, buf);
            CLK_SysTickDelay(5000);
            enable = true;
        }

        memset(input, 0, 4);
        len = 0;
        break;
    }
    }
}

void ShowSevenSegments()
{
    if (!pass_init)
        return;

    for (int i = 0; i < 4; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, pass[3 - i]);
        CLK_SysTickDelay(5000);
    }
}

void Show()
{
    if (pass_init)
        ShowSevenSegments();

    if (!enable)
        return;

    if (cycle == 13)
    {
        cycle = 0;
        enable = false;
        UpdateAllLEDs(0);
        return;
    }

    for (size_t i = 0; i < 4; i++)
        *(leds[i]) = !(pat[cycle][i] - '0');
    cycle++;
    CLK_SysTickDelay(50000);
}

int main(void)
{
    uint32_t count = 0;
    SYS_Init();
    OpenKeyPad();
    OpenSevenSegment();
    init_LCD();
    clear_LCD();

    while (1)
    {
        KeyPadRisingEdge(Execute);
        Show();
        count++;
    }
}
