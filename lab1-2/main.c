#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"

volatile uint32_t* leds[4] = {&PC15, &PC14, &PC13, &PC12};

void UpdateAllLEDs(uint32_t toggle) {
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = !toggle;
}

void UpdateLEDs(uint32_t pos) {
    for (uint32_t i = 0; i < 4; i++)
        *(leds[i]) = pos == i ? 0 : 1;
}

void Init_GPIO(void) {
    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT15, GPIO_MODE_OUTPUT);
    UpdateAllLEDs(0);
}

typedef enum {
    S_Uninitialized,
    S_L2R,
    S_Flash,
    S_R2L
} State;

int main(void)
{
    State state = S_Uninitialized;
    uint32_t i, pos;
  SYS_Init();
    OpenKeyPad();
    Init_GPIO();
    
    while (1) {
        i = ScanKey();
        
        if (i != state) {
            switch (i) {
                case 1: {
                    state = S_L2R;
                    UpdateLEDs(pos = 0);
                    CLK_SysTickDelay(100000);
                    break;
                }
                case 2: {
                    state = S_Flash;
                    UpdateAllLEDs(pos = 1);
                    CLK_SysTickDelay(100000);
                    break;
                }
                case 3: {
                    state = S_R2L;
                    UpdateLEDs(pos = 3);
                    CLK_SysTickDelay(100000);
                    break;
                }
                default: {
                    state = S_Uninitialized;
                    UpdateAllLEDs(0);
                    break;
                }
            }
        }
        
        switch (i) {
            case 1: {
                UpdateLEDs(pos = (pos + 1) % 4);
                CLK_SysTickDelay(100000);
                break;
            }
            case 2: {
                UpdateAllLEDs(pos ^= 1);
                CLK_SysTickDelay(100000);
                break;
            }
            case 3: {
                UpdateLEDs(pos = (pos ? pos - 1 : 3));
                CLK_SysTickDelay(100000);
                break;
            }
            default: break;
        }
    }
}
