#include <stdio.h>
#include <avr/io.h>
#include "periods.h"

// The arduino clock is 16Mhz and the USART0 divides this clock rate by 16
#define USART0_CLOCK_HZ 1000000
#define BAUD_RATE_HZ 9600
#define UBRR_VALUE (USART0_CLOCK_HZ / BAUD_RATE_HZ)

// Send a character over USART0.
int USART0_tx(char data, struct __file* _f) {
    while (!(UCSR0A & (1 << UDRE0))); // wait for the data buffer to be empty
    UDR0 = data; // write the character to the data buffer
    return 0;
}

// Create a stream associated with transmitting data over USART0 (this will be
// used for stdout so we can print to a terminal with printf).
static FILE uartout = FDEV_SETUP_STREAM(USART0_tx, NULL, _FDEV_SETUP_WRITE);

void USART0_init(void) {
    UBRR0H = (UBRR_VALUE >> 8) & 0xF; // set the high byte of the baud rate
    UBRR0L = UBRR_VALUE & 0xFF; // set the low byte of the baud rate
    UCSR0B = 1 << TXEN0; // enable the USART0 transmitter
    UCSR0C = 3 << UCSZ00; // use 8-bit characters
    stdout = &uartout;
}

void led_on(void) {
    PORTB |= 1;
}

void led_off(void) {
    PORTB &= ~1;
}

void timer_init(void) {
    // Put timer in CTC mode where the counter resets when it equals OCR1A
    TCCR1A &= ~((1 << WGM11) | (1 << WGM10));
    TCCR1B |= (1 << WGM12);
    TCCR1B &= ~(1 << WGM13);
    TCCR1B |= (1 << CS10);
    TCCR1B &= ~(1 << CS11);
    TCCR1B &= ~(1 << CS12);
    OCR1A = 160; // resets at 100khz
    OCR1B = 0xFFFF;
}

uint16_t read_timer_counter(void) {
    return TCNT1;
}

int timer_match_check_and_clear(void) {
    if (TIFR1 & (1 << OCF1A)) {
        TIFR1 |= (1 << OCF1A);
        return 1;
    } else {
        return 0;
    }
}

uint32_t rand() {
    static uint32_t x = 70;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

uint16_t rand_u16() {
    return (uint16_t)rand();
}

typedef struct {
    uint16_t period;
    uint16_t pulse_width;
    uint16_t countdown;
} voice_t;

voice_t make_voice(uint16_t period) {
    return (voice_t) {
        .period = period,
        .pulse_width = period / 2,
        .countdown = period,
    };
}

uint8_t tick_voice(voice_t* voice) {
    voice->countdown -= 1;
    if (voice->countdown == 0) {
        voice->countdown = voice->period;
    }
    return voice->countdown < voice->pulse_width;
}

int main(void) {
    USART0_init();
    printf("\r\nHello, World!\r\n");

    DDRB = 0xFF;

    timer_init();

    voice_t voices[3] = {
        make_voice(227),
        make_voice(303),
        make_voice(454),
    };

    while (1) {
        while (!timer_match_check_and_clear());
        uint8_t v0 = tick_voice(&voices[0]);
        uint8_t v1 = tick_voice(&voices[1]);
        uint8_t v2 = tick_voice(&voices[2]);
        PORTB = v0 | (v1 << 1) | (v2 << 2);
    }

    return 0;
}
