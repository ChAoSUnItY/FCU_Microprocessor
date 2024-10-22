#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

#define PHASE_US 300000

volatile uint32_t *leds[4] = {&PC15, &PC14, &PC13, &PC12};
volatile uint32_t *rgbs[3] = {&PA14, &PA13, &PA12};

void UpdateAllLEDs(uint32_t toggle)
{
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

void UpdateRgbLEDs(uint32_t toggle)
{
    for (uint32_t i = 0; i < 3; i++)
        *(rgbs[i]) = !toggle;
}

void Init_GPIO(void)
{
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT14, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
    UpdateRgbLEDs(0);
}

void SetLEDs(char digit)
{
    if (digit == '0')
    {
        UpdateAllLEDs(1);
        return;
    }

    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = ~((digit >> i) & 1);
}

void InitBuzzer()
{
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
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

void PushFrontQueue(Queue *queue, uint32_t num)
{
    for (size_t i = 2; i >= 1; i--)
        queue->numbers[i] = queue->numbers[i - 1];

    queue->numbers[0] = num;

    if (queue->len != 3)
        queue->len++;
}

void PushBackQueue(Queue *queue, uint32_t num)
{
    if (queue->len == 3)
        return;

    queue->numbers[queue->len] = num;
    queue->len++;
}

uint32_t PopFrontQueue(Queue *queue)
{
    if (queue->len == 0)
        return 0;

    uint32_t front = queue->numbers[0];
    for (size_t i = 0; i < 2; i++)
        queue->numbers[i] = queue->numbers[i + 1];

    queue->len--;
    CloseSevenSegment();
    return front;
}

uint32_t PopBackQueue(Queue *queue)
{
    if (queue->len == 0)
        return 0;

    uint32_t back = queue->numbers[queue->len - 1];
    queue->len--;
    CloseSevenSegment();
    return back;
}

void ClearQueue(Queue *queue)
{
    *queue = (Queue){{0}, 0};
    CloseSevenSegment();
}

// Delays 20000 us in total
void ShowQueue(Queue *queue)
{
    for (size_t i = 0; i < queue->len; i++)
    {
        CloseSevenSegment();
        ShowSevenSegment(i, queue->numbers[i]);
        CLK_SysTickDelay(5000);
    }

    CLK_SysTickDelay(20000 - queue->len * 5000);
}

typedef enum
{
    None,
    Left,
    Right
} Direction;

static Queue queue = {{0}, 0};
static Direction dir = None;
static uint32_t cycle = 0;
static uint32_t phase = 0;

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
        PushFrontQueue(&queue, key);
        break;
    }
    case 7:
    {
        if (queue.len != 3)
            return;
        dir = Left;
        PA14 = cycle = phase = 0;
        break;
    }
    case 8:
    {
        ClearQueue(&queue);
        UpdateRgbLEDs(0);
        cycle = phase = dir = None;
        break;
    }
    case 9:
    {
        if (queue.len != 3)
            return;
        dir = Right;
        PA14 = cycle = phase = 0;
        break;
    }
    }
}

// Delays 20000 us if dir is None, 30000 us otherwise.
void Show()
{
    if (dir != None && cycle != 10) {
        ShowQueue(&queue);
        CLK_SysTickDelay(10000);
        cycle++;
        return;
    } else {
        cycle = 0;
    }

    switch (dir)
    {
        case None: {
            ShowQueue(&queue);
            break;
        }
        case Left: {
            uint32_t back = PopBackQueue(&queue);
            PushFrontQueue(&queue, back);
            break;
        }
        case Right: {
            uint32_t front = PopFrontQueue(&queue);
            PushBackQueue(&queue, front);
            break;
        }
    }

    if (dir != None) {
        switch (phase = (phase + 1) % 3) {
            case 0: {
                UpdateRgbLEDs(0);
                PA14 = 0;
                break;
            }
            case 1: {
                UpdateRgbLEDs(0);
                PA13 = 0;
                break;
            }
            case 2: {
                UpdateRgbLEDs(0);
                PA12 = 0;
                break;
            }
        }
    }
}

int main(void)
{
    SYS_Init();
    OpenKeyPad();
    OpenSevenSegment();
    Init_GPIO();
    InitBuzzer();

    while (1)
    {
        KeyPadRisingEdge(Execute);
        Show();
    }
}
