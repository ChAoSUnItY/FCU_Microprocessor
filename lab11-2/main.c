#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "Scankey.h"
#include "Note_Freq.h"
#include "LCD.h"
#include "adc.h"

#define P125ms 125000
#define P250ms 250000
#define P500ms 500000
#define P1S 1000000

static uint16_t FUR_ELISE_MUSIC[72] = {
	E6, D6u, E6, D6u, E6, B5, D6, C6, A5, A5, 0, 0,
	C5, E5, A5, B5, B5, 0, C5, A5, B5, C6, C6, 0,
	E6, D6u, E6, D6u, E6, B5, D6, C6, A5, A5, 0, 0,
	C5, E5, A5, B5, B5, 0, E5, C6, B5, A5, A5, 0,
	B5, C6, D6, E6, E6, 0, G5, F6, E6, D6, D6, 0,
	F5, E6, D6, C6, C6, 0, E5, D6, C6, B5, B5, 0};
static uint32_t FUR_ELISE_PITCH[72] = {
	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,
	P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms, P250ms,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms};
static uint16_t LITTLE_STAR_MUSIC[48] = {
	C4, C4, G4, G4, A4, A4, G4, 0,
    F4, F4, E4, E4, D4, D4, C4, 0,
    G4, G4, F4, F4, E4, E4, D4, 0,
    G4, G4, F4, F4, E4, E4, D4, 0,
    C4, C4, G4, G4, A4, A4, G4, 0,
    F4, F4, E4, E4, D4, D4, C4, 0};
static uint32_t LITTLE_STAR_PITCH[48] = {
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S,
	P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P500ms, P1S};
static uint16_t *music[2] = { LITTLE_STAR_MUSIC, FUR_ELISE_MUSIC };
static uint32_t *pitch[2] = { LITTLE_STAR_PITCH, FUR_ELISE_PITCH };
static size_t cnt[2] = { 48, 72 }, select = 2;

volatile uint32_t volumn = 0;

void ADC_IRQHandler(void)
{
    uint32_t u32Flag;
    int16_t u16_val;

    // Get ADC conversion finish interrupt flag
    u32Flag = ADC_GET_INT_FLAG(ADC, ADC_ADF_INT);

    if (u32Flag & ADC_ADF_INT)
    {
        u16_val = ADC_GET_CONVERSION_DATA(ADC, 7);
		volumn = u16_val * 100 / 4096;
    }

    ADC_CLR_INT_FLAG(ADC, u32Flag);
}

void Init_ADC(void)
{
    ADC_Open(ADC, ADC_INPUT_MODE, ADC_OPERATION_MODE, ADC_CHANNEL_MASK);
    ADC_POWER_ON(ADC);
    ADC_EnableInt(ADC, ADC_ADF_INT);
    NVIC_EnableIRQ(ADC_IRQn);
    ADC_START_CONV(ADC);
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
    switch (key)
    {
        case 5: {
            select = 0;
			break;
        }
        case 6: {
            select = 1;
            break;
        }
        default: {
            select = 2;
            break;
        }
    }
}

int32_t main(void)
{
	uint8_t i;
	SYS_Init();
	OpenKeyPad();
	Init_ADC();

	PWM_EnableOutput(PWM0, PWM_CH_0_MASK);
	PWM_Start(PWM0, PWM_CH_0_MASK);

	while (1) {
		KeyPadRisingEdge(Execute);

		if (select == 2) {
			i = 0;
			PWM_DisableOutput(PWM0, PWM_CH_0_MASK);
		} else {
			PWM_EnableOutput(PWM0, PWM_CH_0_MASK);
			if (i == cnt[select])
				i = 0;
			else i++;

			PWM_ConfigOutputChannel(PWM0, PWM_CH0, music[select][i], volumn); // 0=Buzzer ON
			if (music[select][i] != 0)
				PWM_EnableOutput(PWM0, PWM_CH_0_MASK);
			else
				PWM_DisableOutput(PWM0, PWM_CH_0_MASK);
			CLK_SysTickDelay(pitch[select][i]);
		}
	}
}
