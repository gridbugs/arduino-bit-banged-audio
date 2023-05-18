#include <stdio.h>
#include <avr/io.h>

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

void ADC_init(void) {
    PRR &= ~(1 << PRADC); // disable power reduction ADC bit
    ADMUX = 0; // use AREF pin for reference voltage, right adjust the result, select ADC0 channel
    ADCSRA = (1 << ADEN);// | (1 << ADATE); // enable the ADC and auto-trigger
    //ADCSRB &= ~7; // select free running mode
    DIDR0 = 0xFF; // disable all the digital inputs sharing pins with the ADC
}

uint16_t ADC_read(void) {
    ADCSRA |= (1 << ADSC); // start the first conversion to initiate free running mode
    while ((ADCSRA & (1 << ADSC)) != 0); // wait for the start bit to clear
    uint16_t lo = (uint16_t)ADCL;
    uint16_t hi = (uint16_t)ADCH << 8;;
    return hi | lo;
}


#define DELAY_BASE 1000
int main(void) {
    USART0_init();
    ADC_init();
    printf("Hello, World!\r\n");
    DDRB = 0xFF;
    uint8_t v = 0;
    while (1) {
        PORTB = v;
        v += 1;
        uint16_t adc = ADC_read();
        uint16_t delay = adc;
        if (v == 0) {
           // printf("adc: %u\r\n", adc);
        }
        while (delay > 0) {
            delay -= 1;
        }
    }
    return 0;
}
