#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "LCD.h"
#include "Draw2D.h"
#include "MCU_init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "Scankey.h"

static uint32_t x = 0, y = 16; 
static bool cont = true, start = false, finished = false;
volatile uint8_t KEY_FLAG = 0;

void GPAB_IRQHandler(void)
{
    if (PA->ISRC & BIT0)
    {
        PA->ISRC |= BIT0;
        KEY_FLAG = 3;
    }
    else if (PA->ISRC & BIT1)
    {
        PA->ISRC |= BIT1;
        KEY_FLAG = 2;
    }
    else if (PA->ISRC & BIT2)
    {
        PA->ISRC |= BIT2;
        KEY_FLAG = 1;
    }
    else
    {
        PA->ISRC = PA->ISRC;
    }
}

void Init_Key(void)
{
    GPIO_SetMode(PA, (BIT3 | BIT4 | BIT5), GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, (BIT0 | BIT1 | BIT2), GPIO_MODE_QUASI);
    GPIO_EnableInt(PA, 0, GPIO_INT_FALLING);
    GPIO_EnableInt(PA, 1, GPIO_INT_FALLING);
    GPIO_EnableInt(PA, 2, GPIO_INT_FALLING);
    NVIC_EnableIRQ(GPAB_IRQn);
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_64);
    GPIO_ENABLE_DEBOUNCE(PA, (BIT0 | BIT1 | BIT2));
    PA3 = PA4 = PA5 = 0;
}

void Evaluate(void)
{
    if (!cont || !(start && !finished))
        return;

    if (x + 8 >= LCD_Xmax - 5) {
        x = 0;
        y = 16;
        start = true;
        finished = true;
        return;
    }

    x += 8;
}

void Show(void)
{
    if ((!start && !finished) || (start && finished)) {
        clear_LCD();
        return;
    }

    draw_Circle(x, y, 4, FG_COLOR, BG_COLOR);
    CLK_SysTickDelay(100000);
    draw_Circle(x, y, 4, BG_COLOR, BG_COLOR);
}

int main(void)
{
    uint32_t cnt = 0;
    SYS_Init();
    Init_Key();
    init_LCD();
    clear_LCD();

    while (1)
    {
        switch (KEY_FLAG)
        {
        case 1:
            if ((!start && !finished) || (start && finished)) {
#define LOCAL_RAND_MIN 8
#define LOCAL_RAND_MAX LCD_Xmax - 8
                x = 8;
                y = 16;
                start = true;
                finished = false;
            }
            KEY_FLAG = 0;
            break;
        case 2:
            cont = false;
            KEY_FLAG = 0;
            break;
        case 3:
            cont = true;
            KEY_FLAG = 0;
            break;
        default:
            break;
        }

        srand(cnt);
        Show();
        Evaluate();
        cnt++;
    }
}
