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


void ADC_init(void) {
    PRR &= ~(1 << PRADC); // disable power reduction ADC bit
    ADMUX = 0; // use AREF pin for reference voltage, right adjust the result, select ADC0 channel
    ADCSRA = (1 << ADEN); // enable the ADC
    DIDR0 = 0xFF; // disable all the digital inputs sharing pins with the ADC
}

void ADC_start_read(uint8_t channel) {
    // This also clears the control bits in ADMUX which should be 0 anyway.
    // Doing this in two stages (clearing the channel bits and then or-ing in
    // the new channel bits seems to blend the value at ADC0 with the intended
    // ADC channel so we do it in one stage by writing the channel directly to
    // the ADC.
    ADMUX = (channel & 0xF);
    ADCSRA |= (1 << ADSC); // start the conversion
}

uint16_t ADC_complete_read(void) {
    while ((ADCSRA & (1 << ADSC)) != 0); // wait for the start bit to clear
    uint16_t lo = (uint16_t)ADCL;
    uint16_t hi = (uint16_t)ADCH << 8;;
    return hi | lo;
}

uint16_t ADC_read(uint8_t channel) {
    ADC_start_read(channel);
    return ADC_complete_read();
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


typedef struct {
    uint16_t period_adc;
    uint16_t pulse_width_adc;
} voice_data_raw_t;

typedef struct {
    uint16_t period;
    uint16_t pulse_width;
} voice_data_t;

inline voice_data_t voice_data_from_raw(voice_data_raw_t raw, voice_data_raw_t all_raw) {
    int period_index = (raw.period_adc + all_raw.period_adc) / 4;
    uint16_t period = periods[period_index];
    uint16_t scale = 8;
    uint16_t divisor = 15 + ((1 + (raw.pulse_width_adc / 16)) * ((1 + (all_raw.pulse_width_adc / 16))));
    return (voice_data_t) {
        .period = period,
        .pulse_width = (period * scale) / divisor,
    };
}

typedef struct {
    voice_data_t data;
    uint16_t countdown;
} voice_t;

voice_t make_voice(uint16_t period) {
    return (voice_t) {
        .data = (voice_data_t) {
            .period = period,
            .pulse_width = period / 2,
        },
        .countdown = period,
    };
}

uint8_t tick_voice(voice_t* voice) {
    voice->countdown -= 1;
    if (voice->countdown == 0) {
        voice->countdown = voice->data.period;
    }
    return voice->countdown < voice->data.pulse_width;
}

#define NUM_ADC_CHANNELS 8

int main(void) {
    ADC_init();
    USART0_init();
    printf("\r\nHello, World!\r\n");

    DDRB = 0xFF;

    timer_init();

    voice_t voices[3] = {
        make_voice(periods[100]),
        make_voice(periods[148]),
        make_voice(periods[196]),
    };

    uint8_t current_adc_index = 0;
    uint16_t adc_buffer[NUM_ADC_CHANNELS] = { 0 };

    uint16_t count = 0;

    while (1) {
        ADC_start_read(current_adc_index);
        while (!timer_match_check_and_clear());
        uint8_t v0 = tick_voice(&voices[0]);
        uint8_t v1 = tick_voice(&voices[1]);
        uint8_t v2 = tick_voice(&voices[2]);
        PORTB = v0 | (v1 << 1) | (v2 << 2);

        voice_data_raw_t all_raw = (voice_data_raw_t) {
            .period_adc = adc_buffer[6],
            .pulse_width_adc = adc_buffer[7],
        };

        if ((count & 0xF) == 4) {
            voice_data_raw_t voice0_raw = (voice_data_raw_t) {
                .period_adc = adc_buffer[0],
                .pulse_width_adc = adc_buffer[1],
            };
            voices[0].data = voice_data_from_raw(voice0_raw, all_raw);
        }

        if ((count & 0xF) == 8) {
            voice_data_raw_t voice1_raw = (voice_data_raw_t) {
                .period_adc = adc_buffer[2],
                .pulse_width_adc = adc_buffer[3],
            };
            voices[1].data = voice_data_from_raw(voice1_raw, all_raw);
        }

        if ((count & 0xF) == 12) {
            voice_data_raw_t voice2_raw = (voice_data_raw_t) {
                .period_adc = adc_buffer[4],
                .pulse_width_adc = adc_buffer[5],
            };
            voices[2].data = voice_data_from_raw(voice2_raw, all_raw);
        }

        if ((count & 0xF) == 0) {
            // Ignore the least significant bits to reduce noise
            adc_buffer[current_adc_index] = ADC_complete_read();
            current_adc_index = (current_adc_index + 1) % NUM_ADC_CHANNELS;
        }
        count += 1;
    }

    return 0;
}
