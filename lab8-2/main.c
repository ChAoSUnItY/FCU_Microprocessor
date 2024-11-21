#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "LCD.h"
#include "MCU_init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "Scankey.h"

#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
static uint8_t sorted_ranks[4] = {8, 6, 4, 2};
volatile uint8_t nums[4] = {0}, x_poses[4] = {0}, ranks[4] = {0};
volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};
volatile bool start = false, done = false;

void UpdateAllLEDs(uint32_t toggle)
{
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

void ToggleLED(uint32_t pos)
{
    *(leds[pos]) ^= true;
}

void Init_GPIO(void)
{
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
}

void reset_state(void);

void EINT1_IRQHandler(void)
{
    GPIO_CLR_INT_FLAG(PB, BIT15);
    start = true;
}

void init_extint(void)
{
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_EnableEINT1(PB, 15, GPIO_INT_RISING);
    NVIC_EnableIRQ(EINT1_IRQn);
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCLKSRC_LIRC, GPIO_DBCLKSEL_64);
    GPIO_ENABLE_DEBOUNCE(PB, BIT15);
}

volatile uint8_t KEY_FLAG = 0;

void GPAB_IRQHandler(void)
{
    if (PA->ISRC & BIT0)
    {
        PA->ISRC |= BIT0;
        reset_state();
    }
    else if (PA->ISRC & BIT1)
    {
        PA->ISRC |= BIT1;
        reset_state();
    }
    else if (PA->ISRC & BIT2)
    {
        PA->ISRC |= BIT2;
        reset_state();
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

void reset_state(void)
{
    UpdateAllLEDs(0);
    uint8_t generated;
#define LOCAL_RAND_MIN 10
#define LOCAL_RAND_MAX 99
    start = done = false;

    for (size_t i = 0; i < 4; i++)
    {
        x_poses[i] = 0;
    regen:
        generated = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;

        for (size_t j = 0; j < i; j++)
            if (generated == nums[j])
                goto regen;
        nums[i] = generated;
    }

    for (size_t i = 0; i < 4; i++)
    {
        int rank_index = 0;
        for (int j = 0; j < 4; j++)
        {
            if (nums[i] < nums[j])
            {
                rank_index++;
            }
        }
        ranks[i] = sorted_ranks[rank_index];
    }
}

void evaluate(void)
{
#define RIGHT_BOUNDARY (LCD_Xmax - 16)
    uint8_t done_count = 0;

    if (start && !done)
    {
        for (size_t i = 0; i < 4; i++)
        {
            if (x_poses[i] == RIGHT_BOUNDARY)
            {
                /* Reached end, do nothing */
                if (*leds[i])
                    ToggleLED(i);

                done_count++;
                continue;
            }

            x_poses[i] = clamp(x_poses[i] + ranks[i], 0, RIGHT_BOUNDARY);
        }

        if (done_count == 4)
            done = true;
    }
}

void show(void)
{
    static char LINE_CLEAR[17] = "                 ";
    static uint8_t y_poses[4] = {0, 16, 32, 48};
    char buf[3] = {0};
    for (size_t i = 0; i < 4; i++)
    {
        print_Line(i, LINE_CLEAR);
        sprintf(buf, "%d", nums[i]);
        printS(x_poses[i], y_poses[i], buf);
    }
}

int main(void)
{
    SYS_Init();
    init_extint();
    Init_Key();
    Init_GPIO();
    init_LCD();
    clear_LCD();

    reset_state();

    while (1)
    {
        show();
        evaluate();
    }
}
