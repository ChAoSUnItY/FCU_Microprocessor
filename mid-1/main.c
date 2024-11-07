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

static uint32_t nums[4] = {0};
static bool show_result = false;
static uint32_t result = 0;

void KeyPadRisingEdge(void (*func)(uint32_t)) {
    static uint32_t last_state = 0;
    uint32_t read1 = ScanKey();

    if (last_state != read1) {
        // Pressed
        CLK_SysTickDelay(25000);

        uint32_t read2 = ScanKey();

        if (read2 == read1)
            last_state = read2;

        if (last_state)
            func(last_state);
    }
}

void Execute(uint32_t key) {
    show_result = key >= 1 && key <= 3;

    switch (key) {
        case 1: {
            result = 0;

            for (size_t i = 0; i < 4; i++)
                result += nums[i];
            break;
        }
        case 2: {
            result = nums[0];

            for (size_t i = 1; i < 4; i++)
                result = MIN(result, nums[i]);
            break;
        }
        case 3: {
            result = nums[0];

            for (size_t i = 1; i < 4; i++)
                result = MAX(result, nums[i]);
            break;
        }
        case 9: {
#define LOCAL_RAND_MIN 1
#define LOCAL_RAND_MAX 9
            for (size_t i = 0; i < 4; i++)
                nums[i] = rand() % (LOCAL_RAND_MAX + 1 - LOCAL_RAND_MIN) + LOCAL_RAND_MIN;
            break;
        }
    }
}

void ShowSevenSegments() {
    if (show_result) {
        uint32_t r = result;

        for (size_t i = 0; i < 4; i++) {
            CloseSevenSegment();
            ShowSevenSegment(i, r % 10);
            CLK_SysTickDelay(5000);
            r /= 10;
        }
        return;
    }

    for (int i = 0; i < 4; i++) {
        CloseSevenSegment();
        ShowSevenSegment(i, nums[i]);
        CLK_SysTickDelay(5000);
    }
}

void Show() {
    ShowSevenSegments();
}

int main(void) {
    SYS_Init();
    OpenKeyPad();
    OpenSevenSegment();

    while (1) {
        KeyPadRisingEdge(Execute);
        Show();
    }
}
