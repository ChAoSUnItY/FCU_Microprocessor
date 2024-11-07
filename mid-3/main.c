#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MCU_init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Seven_Segment.h"

#define MIN(a, b) ((a > b) ? b : a)
#define MAX(a, b) ((a > b) ? a : b)

volatile uint32_t* leds[4] = {&PC15, &PC14, &PC13, &PC12};

void ShowSevenSegments();
void UpdateAllLEDs(uint32_t toggle) {
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

typedef enum {
    Initial,
    Inserted,
    Checkout
} Status;

static uint32_t inserted = 0;
static uint32_t counter = 0;
static uint32_t cycle = 0;
static bool set = true;
static Status status = Initial;

void Init_GPIO(void) {
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
}

void InitBuzzer()
{
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
}

void Buzz(size_t times)
{
    
    for (size_t i = 0; i < times; i++)
    {
        PB11 = 0;
        for (size_t j = 0; j < 5; j++) ShowSevenSegments();
        PB11 = 1;
        for (size_t j = 0; j < 5; j++) ShowSevenSegments();
    }
    CLK_SysTickDelay(1000);
}

void KeyPadRisingEdge(void (*func)(uint32_t)) {
    static uint32_t last_state = 0;
    uint32_t read1 = ScanKey();

    if (last_state != read1) {
        // Pressed
        CLK_SysTickDelay(1000);

        uint32_t read2 = ScanKey();

        if (read2 == read1)
            last_state = read2;

        if (last_state)
            func(last_state);
    }
}

void Execute(uint32_t key) {
    switch (key) {
        case 1:
        case 2:
        case 3: {
            if (status == Checkout || status != Inserted)
                break;

            if (inserted < 5 * (key + 1)) {
                Buzz(2);
                break;
            } 

            inserted -= 5 * (key + 1);
            break;
        }
        case 4:
        case 5:
        case 6: {
            if (status == Checkout || status == Inserted)
                break;

            status = Inserted;
            inserted = 10 * key;
            break;
        }
        case 7: {
            if (status == Checkout || status != Inserted)
                break;
            
            status = Checkout;
            cycle = 6;
            break;
        }
        case 8: {
            inserted = 0;
            status = Initial;
            break;
        }
    }
}

void ShowSevenSegments() {
    switch (status) {
        case Initial: {
            CloseSevenSegment();
            break;
        }
        case Inserted: {
            uint32_t id = inserted;
            for (int i = 0; i < 2; i++) {
                CloseSevenSegment();
                ShowSevenSegment(i, id % 10);
                CLK_SysTickDelay(5000);
                id /= 10;
            }
            break;
        }
        case Checkout: {
            CloseSevenSegment();
            ShowSevenSegment(3, 0x0C);
            CLK_SysTickDelay(5000);

            uint32_t id = inserted;
            for (int i = 0; i < 2; i++) {
                CloseSevenSegment();
                ShowSevenSegment(i, id % 10);
                CLK_SysTickDelay(5000);
                id /= 10;
            }

            if (cycle == 0) {
                UpdateAllLEDs(0);
                break;
            }

            if (counter == 10) {
                UpdateAllLEDs(set);
                set ^= true;
                counter = 0;
                cycle--;
            } else {
                counter++;
            }
            break;
        }
    }
}

void Show() {
    ShowSevenSegments();
}

int main(void) {
    SYS_Init();
    Init_GPIO();
    OpenKeyPad();
    OpenSevenSegment();

    while (1) {
        KeyPadRisingEdge(Execute);
        Show();
    }
}
