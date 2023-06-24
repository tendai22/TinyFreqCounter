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
static void set_digit(unsigned int n);
static void set_point(int digit);
static void set_suppressed(int flag);

#define TEST LATA7

#define TOGGLE do { TEST ^= 1; } while(0)

static uint16_t raw_count = 0;
static int measure_finished = 0;
static uint16_t prescaler_factor = 0;

void TIM1_intr(void)
{
    static int count = 0;
    static int i = 0;
    if ((count % 32) == 0) {
        set_point(1);
        set_suppressed(1);
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
    T0CON1 = (unsigned char)(0x10 | prescaler_factor);
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
    0b11111100, //0b00111111, // 0
    0b01100000, //0b00000110, // 1
    0b11011010, //0b01011011, // 2
    0b11110010, //0b01001111, // 3
    0b01100110, //0b01100110, // 4
    0b10110110, //0b01101101, // 5
    0b10111110, //0b01111101, // 6
    0b11100000, //0b00000111, // 7
    0b11111110, //0b01111111, // 8
    0b11110110, //0b01101111, // 9
    0b11101110, //0b01110111, // A
    0b00111110, //0b01111100, // b
    0b10011100, //0b00111001, // C
    0b01111010, //0b01011110, // d
    0b10011110, //0b01111001, // E
    0b10001110, //0b01110001, // F
};

#define LED_DIGITS 3
static char led_digit[LED_DIGITS];
static int suppress_zero = 0;
static int digit_point = LED_DIGITS;

//
// set_point: set dicimal point position, topmost is 0, least digit is LED_DIGITS-1
//            if it is begger than LED_DIGITS, no points are displayed.
//

static void set_point(int digit)
{
    digit_point = digit;
}

//
// set_suppressed: If it is called with 1, zero suppress occurs.
//

static void set_suppressed(int flag)
{
    suppress_zero = flag;
}

static void set_digit(unsigned int n)
{
    int modular = 1;
    int i;
    for (i = 0; i < LED_DIGITS; ++i) {
        modular *= 10;
    }
    n %= modular;
    while (i-- >= 0) {
        led_digit[i] = n % 10;
        n /= 10;
    }
}

static void refresh_LED(void)
{
    char digit_bit;
    static int count = 0;
    int suppressed_digit = -1;
    // set digit drive bit
    digit_bit = (1 << count);
    LATB = (LATB & 0xf8) | 0;
    // zero suppress
    if (suppress_zero) {
        int i, n;
        n = digit_point > LED_DIGITS ? LED_DIGITS : digit_point;
        for (i = 0; i < n; ++i) {
            if (led_digit[i] != 0)
                break;
            // suppress
        }
        if (i == LED_DIGITS) {
            i--;    // display at least one digit
        }
        suppressed_digit = i;
    }
    // set 7-segment bit pattern
    char bit_pattern = segment_pattern[led_digit[count % 3]];
    if (count == digit_point)
        bit_pattern |= 1;   // set digit point
    if (count < suppressed_digit) {
        bit_pattern = 0;    // suppress
    }
    LATC = ~bit_pattern;        // anode common, inverted
    LATB |= digit_bit;
    static char x = 0;
    count++;
    count %= 3;
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
            // calibrating iwth 1000/997
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