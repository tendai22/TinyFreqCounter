/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC18F27Q43
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/uart1.h"

extern char getch(void);
extern void putch(char c);
extern int kbhit(void);
static void refresh_LED(void);
static void set_digit(int n);

#define TEST LATA7

#define TOGGLE do { TEST ^= 1; } while(0)

static uint16_t raw_count = 0;
static int measure_finished = 0;
static uint16_t prescaler_factor = 0;

void TIM1_intr(void)
{
    static int count = 0;
    static int i = 0;
    if ((count % 100) == 0) {
        set_digit(i++);
    }
    if ((count++ & 0x7) == 0) {
        // Stop Measurement
        TMR1_StopTimer();
        TMR0_StopTimer();
        raw_count = TMR0_ReadTimer();
        if (raw_count != 0)
            measure_finished = 1;
        TOGGLE;
        TOGGLE;
        TMR1_WriteTimer(0);
        TMR0_WriteTimer(0);
        TMR0_StartTimer();
        TMR1_StartTimer();
    }
    refresh_LED();
}

void TIM2_intr(void)
{
    return;
    // Start Measurement
    TMR1_WriteTimer(0);
    TMR0_WriteTimer(0);
    T0CON1 = (0x10 | prescaler_factor);
    TMR0_StartTimer();
    TMR1_StartTimer();
    TMR0IF = 0;  // clear flag
    TEST = 1;
    TOGGLE;
    TOGGLE;
}

int TIM0_rollover(void)
{
    return TMR0IF;      // TIM0 roll over flag
}

/*
 * LED dynamic driver
 */

static char segment_pattern[16] = {
    0b00111111, //0b11111100, // 0
    0b00000110, //0b01100000, // 1
    0b01011011, //0b11011010, // 2
    0b01001111, //0b11110010, // 3
    0b01100110, //0b01100110, // 4
    0b01101101, //0b10110110, // 5
    0b01111101, //0b10111110, // 6
    0b00000111, //0b11100000, // 7
    0b01111111, //0b11111110, // 8
    0b01101111, //0b11110110, // 9
    0b01110111, //0b11101110, // A
    0b01111100, //0b00111110, // b
    0b00111001, //0b10011100, // C
    0b01011110, //0b01111010, // d
    0b01111001, //0b10011110, // E
    0b01110001, //0b10001110, // F
};

static char digit[3];

static void set_digit(int n)
{
    digit[2] = n % 10;
    n /= 10;
    digit[1] = n % 10;
    n /= 10;
    digit[0] = n % 10;
}

static void refresh_LED(void)
{
    static char digit_bit = 1;
    static int count = 1;
    digit_bit <<= 1;
    if (digit_bit & 0x8)
        digit_bit = 1;
    LATB = (LATB & 0xf8) | digit_bit;
    LATC = ~segment_pattern[digit[(count++) % 3]];
}

/*
                         Main application
 */
void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global Interrupts
    // Use the following macros to:

    // Enable high priority global interrupts
    INTERRUPT_GlobalInterruptHighEnable();

    // Enable low priority global interrupts.
    INTERRUPT_GlobalInterruptLowEnable();

    // Disable high priority global interrupts
    //INTERRUPT_GlobalInterruptHighDisable();

    // Disable low priority global interrupts.
    //INTERRUPT_GlobalInterruptLowDisable();
    
    // set interrupt handler
    TMR1_SetInterruptHandler(TIM1_intr);
    TMR2_SetInterruptHandler(TIM2_intr);
    
    // RC2 as TEST pin
    ANSELC2 = 0;
    LATC2 = 0;
    TRISC2 = 0; // output

    
    LATC = 0;
    
    printf("start\n");
    //TIM2_intr();
    //TMR2_StartTimer();
    char c;
    uint32_t freq;
    while (1)
    {
        if (measure_finished) {
            if (prescaler_factor >= 16)
                break;
            printf("%2d %5u\n", prescaler_factor, raw_count);
            freq = (((uint32_t)raw_count * 10 * 1000) / 997);
            //freq = ((uint32_t)raw_count * 10);
            if (prescaler_factor > 4 && (1024 <= raw_count && raw_count < 2048)) {
                freq >>= (14 - prescaler_factor);
                if (freq & 1)
                    freq++;
                freq >>= 1;
                printf("freq = %lu.%lu MHz %s\n", freq / 10, freq % 10);
            } else if (TIM0_rollover() == 0 && prescaler_factor <= 4) {
                freq *= 100;
                freq >>= (15 - prescaler_factor);
                printf("freq = %ld kHz\n", freq);
            }
            prescaler_factor++;
            measure_finished = 0;
        }
        __delay_ms(10);
    }
    printf("end\n");
    while (1);
}
/**
 End of File
*/