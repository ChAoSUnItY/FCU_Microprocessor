#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

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

void SetLEDs(char digit)
{
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = ~((digit >> i) & 1);
}

void SetSevenSegments(uint32_t num)
{
    for (int i = 0; i < 4; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, num % 10);
        CLK_SysTickDelay(5000);
        num /= 10;
    }
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
        {
            last_state = read2;
        }

        if (last_state)
        {
            func(last_state);
        }
    }
}

typedef struct Queue_t
{
    uint32_t numbers[3];
    size_t len;
} Queue;

void PushQueue(Queue *queue, uint32_t num)
{
    for (size_t i = 2; i >= 1; i--)
        queue->numbers[i] = queue->numbers[i - 1];

    queue->numbers[0] = num;

    if (queue->len != 3)
        queue->len++;
}

void PopFrontQueue(Queue *queue)
{
    if (queue->len == 0)
        return;

    for (size_t i = 0; i < 2; i++)
        queue->numbers[i] = queue->numbers[i + 1];

    queue->len--;
    CloseSevenSegment();
}

void ClearQueue(Queue *queue)
{
    *queue = (Queue){{0}, 0};
    CloseSevenSegment();
}

void ShowQueue(Queue *queue)
{
    for (size_t i = 0; i < queue->len; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, queue->numbers[i]);
        CLK_SysTickDelay(5000);
    }
}

static Queue queue = {{0}, 0};

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
        PushQueue(&queue, key);
        break;
    }
    case 7:
    {
        PopFrontQueue(&queue);
        break;
    }
    case 8:
    {
        ClearQueue(&queue);
        break;
    }
    }
}

int main(void)
{
    SYS_Init();
    OpenKeyPad();
    OpenSevenSegment();
    Init_GPIO();

    while (1)
    {
        ShowQueue(&queue);
        KeyPadRisingEdge(Execute);
    }
}
