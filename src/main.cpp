#include <Arduino.h>
#include <avr/io.h>

#define rev_limiter 10500 // rpm value
#define ignition_cut_time 20000 // >microseconds< // 1ms = 1000μs
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

void uart_transmit_uint16(uint16_t data) {
  uart_transmit_char((data >> 8) & 0xFF); // Transmit the high byte
  uart_transmit_char(data & 0xFF); // Transmit the low byte
}

void uart_transmit_string(const char* string) {
  for (int i = 0; string[i] != '\0'; i++)
    uart_transmit_char(string[i]);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////Main Function///////////////////////////////
////////////////////////////////////////////////////////////////////////////

int main() {
  uint8_t ignition_map[23] = { E_0, E_500, E_1000, E_1500, E_2000, E_2500, E_3000, E_3500, E_4000, E_4500, E_5000, E_5500, E_6000, E_6500, E_7000, E_7500, E_8000, E_8500, E_9000, E_9500, E_10000, E_10500, E_11000 };
  uint8_t led = 0, angle_difference, map_index;
  uint32_t pulse_interval, delay_time;
  float rpm;

  PORTB = 0b00000000;
  DDRB = 0b00100010;
  DDRD = 0b01000000;
  TCCR1B = 0b00000011;

  uart_init();

  while (true) {
    // While pin 8 is LOW
    while ((PINB & 0b00000001) == 0b00000000);
    // If pin 8 is HIGH
    if ((PINB & 0b00000001) == 0b00000001){
      pulse_interval = TCNT1;
      TCNT1 = 0;
     
      // Convert pulse_interval >in microseconds< into rpm
      rpm = 1 / pulse_interval / 60000000;

      // Rounds the rpm value
      map_index = round(rpm / 500);
     
      // Calculate the delay >in microseconds< needed to ignite at the specified advence angle in ignition_map
      angle_difference = trigger_coil_angle - ignition_map[map_index];
      delay_time = pulse_interval / 360 * angle_difference;

      // Lighting the led up on pin 13 every x revolutions
      led++;
      if (led == 1) {
        PORTB = 0b00100000;
        led = 0;
      }

      // Rev limiter and ignition
      if (map_index < 23) {
        //Waits the amount of time specified in delay_time
        while (TCNT1 < delay_time){
          // Check if RPM exceeds the rev_limiter threshold
          if (rpm >= rev_limiter) {
            // Keep ignition off for ignition_cut_time
            while (TCNT1 < ignition_cut_time) {
              PORTB = 0b00000000;  // Turn off ignition
            }
          } 
          else {
            PORTB = 0b00000010; // regular ignition
          }
        }
      }
      // Time during which pin 9 will be high >microseconds<
      while (TCNT1 < delay_time + 25)
      PORTB = 0b00000000;
    }
      // Transmit the value of map_index via serial
      uart_transmit_char(map_index);
      uart_transmit_string("\n");
  }
}

// To do : transmit rpm over UART >> convert float to uint16_t or int