#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Scankey.h"
#include "Seven_Segment.h"

static uint32_t nums[2] = {0};
static size_t len = 0;

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
    case 7:
    case 8:
    case 9:
    {
        if (len == 2)
            len = 0;

        nums[len++] = key;
        break;
    }
    }
}

void ShowSevenSegments()
{
    for (int i = 0; i < len; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, nums[i]);
        CLK_SysTickDelay(5000);
    }
}

void Show()
{
    ShowSevenSegments();

    if (len != 2)
    {
        clear_LCD();
        return;
    }

    char buf[16] = {0};

    sprintf(buf, "%d + %d = %d", nums[0], nums[1], nums[0] + nums[1]);
    print_Line(0, buf);
    CLK_SysTickDelay(5000);
    memset(buf, 0, 16);
    sprintf(buf, "%d - %d = %d", nums[0], nums[1], nums[0] - nums[1]);
    print_Line(1, buf);
    CLK_SysTickDelay(5000);
    memset(buf, 0, 16);
    sprintf(buf, "%d * %d = %d", nums[0], nums[1], nums[0] * nums[1]);
    print_Line(2, buf);
    CLK_SysTickDelay(5000);
    memset(buf, 0, 16);
    sprintf(buf, "%d / %d = %d", nums[0], nums[1], nums[0] / nums[1]);
    print_Line(3, buf);
    CLK_SysTickDelay(5000);
    memset(buf, 0, 16);
}

int main(void)
{
    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();
    OpenSevenSegment();

    while (1)
    {
        KeyPadRisingEdge(Execute);
        Show();
    }
}
