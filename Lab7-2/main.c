#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"
#include "Scankey.h"

#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define CIRCLE_BOX_SIZE 6
#define BOX_SIZE 5
#define DEFAULT_X 64
#define DEFAULT_Y 60
static int x = DEFAULT_X, y = DEFAULT_Y, vx = 0, vy = 0, box_x, box_y;
static bool show_box;

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
    }
}

void KeyPadRisingEdge(void (*func)(uint8_t))
{
    static uint8_t last_state = 0;
    uint8_t read1 = ScanKey();

    if (last_state != read1)
    {
        // Pressed
        CLK_SysTickDelay(25000);

        uint8_t read2 = ScanKey();

        if (read2 == read1)
            last_state = read2;

        if (last_state)
            func(last_state);
    }
}

void GenerateBox();

void Execute(uint8_t key)
{
    if ((key != 5 && key != 8) && (vx != 0 || vy != 0))
        return;

    switch (key)
    {
    case 1:
    {
        vx = -1;
        vy = -1;
        break;
    }
    case 3:
    {
        vx = 1;
        vy = -1;
        break;
    }
    case 4:
    {
        vx = -1;
        vy = 0;
        break;
    }
    case 5:
    {
        vx = vy = 0;
        break;
    }
    case 6:
    {
        vx = 1;
        vy = 0;
        break;
    }
    case 7:
    {
        vx = -1;
        vy = 1;
        break;
    }
    case 8:
    {
        GenerateBox();
        break;
    }
    case 9:
    {
        vx = 1;
        vy = 1;
        break;
    }
    }
}

void GenerateBox()
{
#define RAND(MIN, MAX) rand() % (MAX + 1 - MIN) + MIN
    box_x = RAND(0, LCD_Xmax / 2 - 5);
    box_y = RAND(0, LCD_Ymax / 2 - 5);
    show_box = true;
}

void ResetState()
{
    x = DEFAULT_X;
    y = DEFAULT_Y;
    show_box = vx = vy = 0;
}

bool CheckBoxCollision()
{
    return !(x + 4 <= box_x || box_x + BOX_SIZE <= x - 4 || y + 4 <= box_y || box_y + BOX_SIZE <= y - 4);
}

void Evaluate()
{
    x += vx;
    y += vy;

    if (x <= 3 || x + 3 >= LCD_Xmax)
    {
        vx = -vx;
        x = clamp(x, 0, LCD_Xmax - 3);
        Buzz(1);
    }

    if (y <= 3 || y + 3 >= LCD_Ymax)
    {
        vy = -vy;
        y = clamp(y, 0, LCD_Ymax - 3);
        Buzz(1);
    }

    if (CheckBoxCollision())
    {
        ResetState();
    }
}

void Show()
{
    draw_Circle(x, y, 3, FG_COLOR, BG_COLOR);
    if (show_box)
        draw_Rectangle(box_x, box_y, box_x + 5, box_y + 5, FG_COLOR, BG_COLOR);
    CLK_SysTickDelay(100000);
    draw_Circle(x, y, 3, BG_COLOR, BG_COLOR);
    if (show_box)
        draw_Rectangle(box_x, box_y, box_x + 5, box_y + 5, BG_COLOR, BG_COLOR);
}

int main(void)
{
    uint32_t i = 0;
    SYS_Init();
    init_LCD();
    clear_LCD();
    GenerateBox();

    while (1)
    {
        KeyPadRisingEdge(Execute);
        Evaluate();
        Show();
    }
}
