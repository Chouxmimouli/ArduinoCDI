#include <Arduino.h>
#include <avr/io.h>
#include <util/delay.h>

#define rev_limiter 21
#define ignition_cut_time 100
#define trigger_coil_angle 27
#define E_0     10
#define E_500   10
#define E_1000  10
#define E_1500  13
#define E_2000  16
#define E_2500  19
#define E_3000  22
#define E_3500  25
#define E_4000  27
#define E_4500  27
#define E_5000  27
#define E_5500  27
#define E_6000  27
#define E_6500  27
#define E_7000  27
#define E_7500  27
#define E_8000  27
#define E_8500  27
#define E_9000  27
#define E_9500  27
#define E_10000 27
#define E_10500 27
#define E_11000 27

////////////////////////////////////////////////////////////////////////////
////////////////////////////Serial Communcation/////////////////////////////
////////////////////////////////////////////////////////////////////////////

void uart_init() {
  UBRR0 = 103; // For 16MHz and baud rate of 9600
  UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable transmitter and receiver
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data format
}

void uart_transmit_char(uint8_t data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void uart_transmit_string(const char* string) {
  for (int i = 0; string[i] != '\0'; i++)
    uart_transmit_char(string[i]);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////Main Function///////////////////////////////
////////////////////////////////////////////////////////////////////////////

int main() {
  uint8_t map1[23] = { E_0, E_500, E_1000, E_1500, E_2000, E_2500, E_3000, E_3500, E_4000, E_4500, E_5000, E_5500, E_6000, E_6500, E_7000, E_7500, E_8000, E_8500, E_9000, E_9500, E_10000, E_10500, E_11000 };
  uint32_t pulse_delay, curve_select;
  uint8_t led = 0;
  PORTB = 0b00000000;
  DDRB = 0b00100010;
  DDRD = 0b01000000;
  TCCR1B = 0b00000011;

  uart_init();

  while (true) {
    while ((PINB & 0b00000001) == 0b00000000);
    while ((PINB & 0b00000001) == 0b00000001);
    pulse_delay = TCNT1;
    TCNT1 = 0;
    curve_select = 30000 / pulse_delay;
    pulse_delay *= trigger_coil_angle - map1[curve_select];
    pulse_delay /= 360;
    led++;
    if (led == 1) {
      PORTB = 0b00100000;
      led = 0;
    }
    if (curve_select < 23) {
      while (TCNT1 < pulse_delay){
        // Check if RPM exceeds the rev_limiter threshold
        if (curve_select >= rev_limiter) {
          // Ignition cut logic
          PORTB = 0b00000000;  // Turn off ignition
          _delay_ms(ignition_cut_time);  // Keep ignition off for ignition_cut_time
        } else {
            if (PIND & 0b10111111) {
            PORTB = 0b00000010; // regular ignition
            }
        }
      }
      while (TCNT1 < pulse_delay + 25){
        PORTB = 0b00000000;
      }
    }

    // Transmit the value of curve_select via serial
    uart_transmit_string("curve_select: ");
    uart_transmit_char(curve_select + '0');
    uart_transmit_string("\n");
  }
}