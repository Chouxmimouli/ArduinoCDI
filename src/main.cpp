#include <Arduino.h>
#include <avr/io.h>
#include <string.h>

#define rev_limiter 10000 // Rpm value
#define ignition_cut_time 10000 // not μs. Devide your μs number by 4 μs (w/ 64 prescaler) // 1ms = 1000μs
#define trigger_coil_angle 25
#define RPM_0    10 // This curve is linear from 1000 RPM to 4000.
#define RPM_250  10
#define RPM_500  10
#define RPM_750  10
#define RPM_1000 10
#define RPM_1250 11.25
#define RPM_1500 12.5
#define RPM_1750 13.75
#define RPM_2000 15
#define RPM_2250 16.25
#define RPM_2500 17.5
#define RPM_2750 18.75
#define RPM_3000 20
#define RPM_3250 21.25
#define RPM_3500 22.5
#define RPM_3750 23.75
#define RPM_4000 25 // After this point, the curve becomes flat

////////////////////////////////////////////////////////////////////////////
////////////////////////////Serial Communcation/////////////////////////////
////////////////////////////////////////////////////////////////////////////

void uart_init() {
  UBRR0 = 103; // For 16MHz and baud rate of 115200
  UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable transmitter and receiver
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data, no parity, 1 stop bit
}

void uart_transmit_char(uint8_t data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void uart_transmit_string(const char* string) {
  for (int i = 0; string[i] != '\0'; i++)
    uart_transmit_char(string[i]);
}

void uart_transmit_uint(uint8_t data) {
  char buffer[4]; // Buffer to hold the string representation of the number
  snprintf(buffer, sizeof(buffer), "%u", data); // Convert uint8_t to string
  uart_transmit_string(buffer); // Transmit the string over UART
}

void uart_transmit_uint(uint16_t data) {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%u", data);
  uart_transmit_string(buffer);
}

void uart_transmit_uint(uint32_t data) {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "%lu", data);
  uart_transmit_string(buffer);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////Main Function///////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Global variables
boolean fresh_signal = true;

int main() {
  float ignition_map[17] = { RPM_0, RPM_250, RPM_500, RPM_750, RPM_1000, RPM_1250, RPM_1500, RPM_1750, RPM_2000, RPM_2250, RPM_2500, RPM_2750, RPM_3000, RPM_3250, RPM_3500, RPM_3750, RPM_4000 };
  float angle_difference;
  uint8_t map_index;
  uint16_t rpm = 0, pulse_interval, delay_time;

  PORTB = 0b00000000;
  DDRB = 0b00100100; // Set pin 10 and 13 as outputs and the rest are inputs
  PORTD = 0b00010000; // Enable pullup on pin 4
  DDRD = 0b00000000; // Set pin 0 to 7 as inputs
  
  TCCR1B = 0b00000011; // 64 prescaler
  TIMSK1 = 0b00000001; // Enable Timer1 overflow interrupt

  sei(); // Enable global interrupts 
  //uart_init();

  while (true) {
    if ((PIND & 0b01000000) == 0b01000000) {
      
      // Set the value of the timer to pulse_interval >> time between 2 signals 
      pulse_interval = TCNT1; 
      
      // Reset the timer
      TCNT1 = 0;

      /* When the revlimiter was active the previous rotation or when the timer overflows or on the first engine rotation, the rpm reading can be wrong
      becuase the prevous timer value is flawed or inexistant. To fix the problem I use a variable called "fresh_signal" which sets the timer when 
      the arduino recieves a signal but prevent the fuel from being ingited */

      if (fresh_signal == false) {
        
        // Convert pulse_interval into rpm
        rpm = 15000000 / pulse_interval;

        // Round the rpm value
        map_index = round(rpm / 250.0);

        // Safety measure to limit the advence to 16 degrees >map_index 9< if the rpm reading exceeds 11000rpm (in case of an rpm sensor failure)
        if (map_index > 44) {
          map_index = 9;
        }

        // Cap the value of map_index to 16 because thats the last value in the ignition table above >RPM_4000 27<
        else if (map_index > 16) {
        map_index = 16;
        }

        // Calculate the delay, not in μs, needed to ignite at the specified advence angle in ignition_map
        angle_difference = trigger_coil_angle - ignition_map[map_index];
        delay_time = pulse_interval / 360 * angle_difference;

        //////// Rev limiter and ignition ////////
        // Check if RPM exceeds the rev_limiter threshold
        if (rpm > rev_limiter) {
          // Keep ignition off for ignition_cut_time
          while (TCNT1 < ignition_cut_time);                 
          fresh_signal = true;
          //uart_transmit_string("Rev_limiter");
          //uart_transmit_string("\n \n");
        } 
        else { 
          // Wait the amount of time specified in delay_time
          //while (TCNT1 < delay_time);
          PORTB = 0b00100100; // Ignition on pin 10

          // Time during which pin 10 will be high
          // while (TCNT1 < (delay_time + 1000));
          while (TCNT1 < 1000);
          PORTB = PORTB & 0b11111011;
        }

        // Transmit the value of map_index through serial >UART<
        //uart_transmit_uint(rpm);
        //uart_transmit_string("\n");

        //turn off the led
        PORTB = PORTB & 0b11011111;
      }
    }
    else {
      fresh_signal = false;
    }
  } 
}

ISR(TIMER1_OVF_vect) {
  //uart_transmit_string("Timer Overflow");
  //uart_transmit_string("\n");
  fresh_signal = true; 
} 