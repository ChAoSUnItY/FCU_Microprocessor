#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"
#include "Scankey.h"
#include "Seven_Segment.h"
#include "adc.h"

#define clamp(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define sign(x) ((x > 0) - (x < 0))
static int x = LCD_Xmax / 2 - 8, y = LCD_Ymax / 2 - 3;
volatile int vx = 0;
static bool mov = false;

// PA12 = 0; BLUE
// PA13 = 0; GREEN
// PA14 = 0; RED
volatile uint32_t *rgbs[3] = {&PA14, &PA13, &PA12};
volatile uint8_t u8ADF = 0;

void ADC_IRQHandler(void)
{
    uint32_t u32Flag;

    // Get ADC conversion finish interrupt flag
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if (u32Flag & ADC_ADF_INT)
        u8ADF = 1;

    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK);
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
}

void UpdateRgbLEDs(uint32_t toggle)
{
    for (uint32_t i = 0; i < 3; i++)
        *(rgbs[i]) = !toggle;
}

void ToggleRgbLed(uint8_t idx)
{
    for (uint32_t i = 0; i < 3; i++)
        *(rgbs[i]) = i == idx ? 0 : 1;
}

void Init_GPIO(void)
{
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    UpdateRgbLEDs(0);
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
    if (key == 5)
        mov = !mov;
}

void convert_adc_to_speed(uint16_t val)
{
    if (val > 2048) {
        vx = vx >= 0 ? 4 : -4;
        ToggleRgbLed(0);
    } else if (val > 1024) {
        vx = vx >= 0 ? 2 : -2;
        ToggleRgbLed(1);
    } else {
        vx = vx >= 0 ? 1: -1;
        ToggleRgbLed(2);
    }
}

void Evaluate()
{
    if (!mov)
        return;

    x += vx;

    if (x <= 0 || x + 16 >= LCD_Xmax) {
        vx = -vx;
        x = clamp(x, 0, LCD_Xmax - 16);
    }
}

void Show()
{
    uint8_t speed = abs(vx) == 4 ? 3 : abs(vx) == 2 ? 2 : 1;
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(x + x_offset, y, x + x_offset, y + 8, FG_COLOR, BG_COLOR);
    for (size_t i = 0; i < 10; i++) {
        CloseSevenSegment();
        ShowSevenSegment(0, speed);
        CLK_SysTickDelay(5000);
    }
    for (size_t x_offset = 0; x_offset < 16; x_offset++)
        draw_Line(x + x_offset, y, x + x_offset, y + 8, BG_COLOR, BG_COLOR);
}

int main(void)
{
    uint16_t adc_val;
    SYS_Init();
    Init_GPIO();
    OpenSevenSegment();
    Init_ADC();
    init_LCD();
    clear_LCD();

    while (1)
    {
        KeyPadRisingEdge(Execute);

        ADC_START_CONV(ADC);
        while (u8ADF == 0);
        adc_val = ADC_GET_CONVERSION_DATA(ADC, 7);
        convert_adc_to_speed(adc_val);
        ADC_DisableInt(ADC, ADC_ADF_INT);

        Evaluate();
        Show();
    }
}
