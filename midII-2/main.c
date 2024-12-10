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

typedef struct {
    char name[7];
    int price;
} Item;

Item items[4] = {
    {
        "cola",
        10
    },
    {
        "milk",
        20
    },
    {
        "coffee",
        35
    },
    {
        "juice",
        55
    }
};
static size_t idx = 0, lb = 0, ub = 2;

static int buying_seq[5] = {-1, -1, -1, -1, -1};
static int buying_cnt = 0;
static bool show_on_7seg = false;
static size_t total = 0;

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
    case 2:
    {
        if (idx == 0)
            return;

        if (idx == 1 && ub == 3) {
            lb = 0;
            ub = 2;
        }

        idx--;
        break;
    }
    case 5:
    {
        if (idx == 3)
            return;

        if (idx == 2 && ub == 2) {
            lb = 1;
            ub = 3;
        }

        idx++;
        break;
    }
    case 7:
    {
        // Show on 7seg
        show_on_7seg = true;
        break;
    }
    case 8:
    {
        if (buying_cnt == 5)
            return;

        Item *item = &items[idx];
        buying_seq[buying_cnt++] = idx;
        total += item->price;
        break;
    }
    case 9:
    {
        if (buying_cnt == 0)
            return;

        int prev_idx = buying_seq[--buying_cnt];
        Item *item = &items[prev_idx];
        total -= item->price;
        break;
    }
    }
}

void ShowSevenSegments()
{
    size_t t = total;
    for (int i = 0; i < 3; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, t % 10);
        CLK_SysTickDelay(5000);
        t /= 10;
    }
}

void Show()
{
    if (show_on_7seg)
        ShowSevenSegments();

    char buf[17] = {0};

    for (size_t i = lb; i <= ub; i++) {
        Item *item = &items[i];
        if (idx == i)
            sprintf(buf, ">");
        else
            sprintf(buf, " ");
        sprintf(buf + 1, "%d. %6s   $%d", i + 1, item->name, item->price);
        print_Line(i - lb, buf);
    }


    char buf1[17] = "select:         ";
    
    for (size_t i = 0; i < buying_cnt; i++) {
        buf1[9 + i] = buying_seq[i] + '1';
    }

    print_Line(3, buf1);
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
