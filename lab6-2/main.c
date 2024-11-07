#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "LCD.h"
#include "MCU_init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "Scankey.h"

static uint32_t availables[4] = {0}, sum = 0, selected[4] = {0};
static size_t start_idx = 0, idx = 0, pressed_cnt = 0;
volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};

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

void KeyPadRisingEdge(void (*func)(uint8_t)) {
    static uint8_t last_state = 0;
    uint8_t read1 = ScanKey();

    if (last_state != read1) {
        // Pressed
        CLK_SysTickDelay(25000);

        uint8_t read2 = ScanKey();

        if (read2 == read1)
            last_state = read2;

        if (last_state)
            func(last_state);
    }
}

void Execute(uint8_t key) {
    switch (key) {
        case 4: {
            if (idx == 0)
                return;

            if (idx == 1 && start_idx == 1)
                start_idx = 0;

            idx--;
            break;
        }
        case 5: {
            if (pressed_cnt == 4)
                return;

            sum += selected[pressed_cnt] = availables[idx];
            Buzz(1);
            ToggleLED(pressed_cnt++);
            break;
        }
        case 6: {
            if (idx == 3)
                return;

            if (idx == 2 && start_idx == 0)
                start_idx = 1;

            idx++;
            break;
        }
        case 7: {
            if (pressed_cnt != 0)
                return;

#define LOCAL_RAND_MIN 10
#define LOCAL_RAND_MAX 99
            for (size_t i = 0; i < 4; i++)
                availables[i] = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;

            start_idx = idx = 0;
            break;
        }
        case 8: {
            if (pressed_cnt == 0)
                return;

            sum -= selected[--pressed_cnt];
            ToggleLED(pressed_cnt);
            break;
        }
        case 9: {
            start_idx = idx = sum = pressed_cnt = 0;
            memset(selected, 0, 4);
            UpdateAllLEDs(0);
            break;
        }
        default:
            break;
    }
}

void Show(void) {
    uint8_t sz;
    char buffer[17] = { 0 };

    sz = sprintf(buffer, "SUM = %d", sum);
    for (size_t i = sz; i < 16; i++)
        buffer[i] = ' ';
    print_Line(0, buffer);
    CLK_SysTickDelay(1000);
    
    for (size_t i = start_idx, line = 1; i < start_idx + 3; i++) {
        sz = sprintf(buffer, "%c %d", i == idx ? '>' : ' ', availables[i]);
        buffer[sz] = '\0';
        print_Line(line++, buffer);
        CLK_SysTickDelay(1000);
    }
}

int main(void) {
    uint32_t cnt = 0;
    SYS_Init();
    init_LCD();
    clear_LCD();

    while (1) {
        srand(cnt);
        Show();
        KeyPadRisingEdge(Execute);
        cnt++;
    }
}
