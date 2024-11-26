#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"
#include "Scankey.h"
#include "adc.h"

#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define sign(x) ((x > 0) - (x < 0))
/* Small rect */
volatile int sx = LCD_Xmax / 2 - 4, sy = LCD_Ymax / 2 - 4, svx = 0, svy = 0;
/* Bottom rect */
volatile int bx = LCD_Xmax / 2 - 8, by = LCD_Ymax - 8;
volatile int bvx = 0;
volatile int16_t uval = 0;
/* Upper rect */
int ux = 2, uy = 2;
volatile bool started = false, gameover = false;

volatile uint16_t vx, vy;

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

void ADC_IRQHandler(void)
{
    uint32_t u32Flag;
    int16_t u16_val;

    // Get ADC conversion finish interrupt flag
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if (u32Flag & ADC_ADF_INT)
    {
        u16_val = ADC_GET_CONVERSION_DATA(ADC, 0) + 2048;
        uval = u16_val;
    }

    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK);
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
    ADC_START_CONV(ADC);
}

void ResetState()
{
    sx = LCD_Xmax / 2 - 4;
    sy = LCD_Ymax / 2 - 4;
#define LOCAL_RAND_MIN -1
#define LOCAL_RAND_MAX 1
    while (svx == 0)
        svx = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;
    while (svy == 0)
        svy = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;
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

void Execute(uint8_t key)
{
    switch (key)
    {
    case 1:
    case 4:
    case 7:
    {
        if (started)
            return;
        /* if (!gameover)
            return; */

        started = true;
        gameover = false;
        ResetState();
        break;
    }
    }
}

void handleCollision(int x1, int y1, int width1, int height1,
                     int x2, int y2, int width2, int height2)
{
    int dx = (x1 + width1 / 2) - (x2 + width2 / 2);
    int dy = (y1 + height1 / 2) - (y2 + height2 / 2);
    int width = (width1 + width2) / 2;
    int height = (height1 + height2) / 2;
    int crossWidth = width * dy;
    int crossHeight = height * dx;

    if (abs(dx) <= width && abs(dy) <= height)
    {
        if (crossWidth > crossHeight)
        {
            if (crossWidth > -crossHeight)
            {
                Buzz(1);
                svx = -svx;
            }
            else
            {
                Buzz(1);
                svy = -svy;
            }
        }
        else
        {
            if (crossWidth > -crossHeight)
            {
                Buzz(1);
                svx = -svx;
            }
            else
            {
                Buzz(1);
                svy = -svy;
            }
        }
    }
}

void Evaluate()
{
    if (!started || gameover)
        return;

    if (uval > 4000)
        bvx = 4;
    else if (uval == 2048)
        bvx = -4;
    else
        bvx = 0;

    sx += svx;
    sy += svy;
    bx += bvx;

    /* Check bottom box */

    if (bx <= 0 || bx + 16 >= LCD_Xmax)
    {
        bx = clamp(bx, 0, LCD_Xmax - 16);
    }

    /* Check small box basic */

    if (sx <= 0 || sx + 8 >= LCD_Xmax)
    {
        Buzz(1);
        svx = -svx;
        sx = clamp(sx, 0, LCD_Xmax - 8);
    }

    if (sy <= 0)
    {
        Buzz(1);
        svy = -svy;
        sy = 0;
    }

    if (sy + 8 >= LCD_Ymax)
    {
        /* Bottom rect didn't catch it */
        gameover = true;
        return;
    }

    /* Check small box advanced, including buttom box and upper box */

    handleCollision(sx, sy, 8, 8, bx, by, 16, 8);
    handleCollision(ux, uy, 16, 8, sx, sy, 8, 8);
}

void Show()
{
    /* Small box */
    if (started)
        for (size_t x_offset = 0; x_offset < 8; x_offset++)
            draw_Line(sx + x_offset, sy, sx + x_offset, sy + 8, FG_COLOR, BG_COLOR);
    /* Bottom box */
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(bx + x_offset, by, bx + x_offset, by + 8, FG_COLOR, BG_COLOR);
    /* Upper box */
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(ux + x_offset, uy, ux + x_offset, uy + 8, FG_COLOR, BG_COLOR);
    if (gameover)
        printS(LCD_Xmax / 2 - 32, LCD_Ymax / 2 - 4, "GAMEOVER");
    CLK_SysTickDelay(100000);
    /* Small box */
    if (started)
        for (size_t x_offset = 0; x_offset < 8; x_offset++)
            draw_Line(sx + x_offset, sy, sx + x_offset, sy + 8, BG_COLOR, BG_COLOR);
    /* Bottom box */
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(bx + x_offset, by, bx + x_offset, by + 8, BG_COLOR, BG_COLOR);
    /* Upper box */
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(ux + x_offset, uy, ux + x_offset, uy + 8, BG_COLOR, BG_COLOR);
}

int main(void)
{
    uint32_t count = 0;
    InitBuzzer();
    SYS_Init();
    Init_ADC();
    init_LCD();
    clear_LCD();

    PD14 = 0;

    while (1)
    {
        count++;
        KeyPadRisingEdge(Execute);
        Evaluate();
        Show();
        srand(count);
    }
}
