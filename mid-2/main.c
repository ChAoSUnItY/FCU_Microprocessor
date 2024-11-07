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

void UpdateAllLEDs(uint32_t toggle) {
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

typedef enum {
    Clear,
    ShowId,
    SegBlink,
    LedBlink,
} Status;

static uint32_t nums[4] = {0};
static Status status = Clear, prev = Clear;
static uint32_t counter = 0;
static uint32_t cycle = 0;
static bool set = true;

void Init_GPIO(void) {
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
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
        case 1: {
            prev = status;
            status = ShowId;
            nums[0] = 8;
            nums[1] = 9;
            nums[2] = 9;
            nums[3] = 9;
            break;
        }
        case 2: {
            prev = status;
            status = SegBlink;
            cycle = 3;
            break;
        }
        case 3: {
            prev = status;
            status = LedBlink;
            cycle = 6;
            break;
        }
        case 7: {
            prev = status;
            status = Clear;
            break;
        }
    }
}

void ShowSevenSegments() {
    switch (status) {
        case Clear: {
            CloseSevenSegment();
            break;
        }
        case ShowId: {
            for (int i = 0; i < 4; i++) {
                CloseSevenSegment();
                ShowSevenSegment(i, nums[i]);
                CLK_SysTickDelay(5000);
            }
            break;
        }
        case SegBlink: {
            if (cycle == 0) {
                status = prev;
                break;
            }

            for (int i = 0; i < 4; i++) {
                CloseSevenSegment();
                ShowSevenSegment(i, nums[i]);
                CLK_SysTickDelay(5000);
            }

            CloseSevenSegment();
            CLK_SysTickDelay(50000);
            cycle--;
            break;
        }
        case LedBlink: {
            if (cycle == 0) {
                UpdateAllLEDs(0);
                status = prev;
                break;
            }

            if (prev == ShowId)
                for (int i = 0; i < 4; i++) {
                    CloseSevenSegment();
                    ShowSevenSegment(i, nums[i]);
                    CLK_SysTickDelay(5000);
                }
            else
                CLK_SysTickDelay(5000 * 4);

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
