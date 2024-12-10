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

typedef enum {
    Up,
    None,
    Down
} Status;

static Status status = None; 
static size_t num = 0;

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
    case 4:
    {
        status = None;
        break;
    }
    case 7:
    {
        if (status == None)
            status = Down;
        break;
    }
    case 8:
    {
        if (status != None)
            break;
#define LOCAL_RAND_MIN 00
#define LOCAL_RAND_MAX 99
        num = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;
        break;
    }
    case 9:
    {
        if (status == None)
            status = Up;
        break;
    }
    }
}

void ShowSevenSegments()
{
    int n = num;
    for (int i = 0; i < 2; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, n % 10);
        CLK_SysTickDelay(5000);
        n /= 10;
    }
}

void Show()
{
    ShowSevenSegments();

    char buf[17] = {0};

    snprintf(buf, 17, "%d                 ", num);
    print_Line(0, buf);
}

void Eval()
{
    switch (status)
    {
        case Up:
        {
            if (num != 99)
                num++;
            else
                num = 0;
            for (size_t i = 0; i < 5; i++)
                Show();
            break;
        }
        case Down:
        {
            if (num != 0)
                num--;
            else
                num = 99;
            for (size_t i = 0; i < 5; i++)
                Show();
            break;
        }
    }
}

int main(void)
{
    uint32_t cnt = 0;
    SYS_Init();
    init_LCD();
    clear_LCD();
    OpenKeyPad();
    OpenSevenSegment();

    while (1)
    {
        srand(cnt);
        KeyPadRisingEdge(Execute);
        Show();
        Eval();
        cnt++;
    }
}
